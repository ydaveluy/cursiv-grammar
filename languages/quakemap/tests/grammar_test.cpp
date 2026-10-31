// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "quakemap/core/CoreServices.hpp"
#include "quakemap/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "quakemap: %s: %s\n", file.string().c_str(), what);
}

int failed_expectation(const char* expectation, const cursiv::ParseResult& r) {
  const auto* map=cursiv::ast_cast<quakemap::ast::Map>(r.value);
  std::fprintf(stderr,"quakemap: %s: accepted=%d repaired=%d diagnostics=%zu ast-error=%u entities=%zu\n",
      expectation,static_cast<int>(r.accepted),static_cast<int>(r.cst().recovered()),
      r.diagnostics.size(),static_cast<unsigned>(r.error),map?map->entities.size():0);
  for(const auto& d:r.diagnostics)
    std::fprintf(stderr,"  diagnostic kind=%u bytes=%zu:%zu\n",
        static_cast<unsigned>(d.kind),static_cast<std::size_t>(d.begin),static_cast<std::size_t>(d.end));
  if(map) for(std::size_t e=0;e<map->entities.size();++e) {
    const auto* entity=map->entities[e];
    std::fprintf(stderr,"  entity[%zu] keys=%zu brushes=%zu\n",e,entity->keyvalues.size(),entity->brushes.size());
    for(const auto* kv:entity->keyvalues)
      std::fprintf(stderr,"    key=[%.*s] value=[%.*s]\n",static_cast<int>(kv->key.size()),kv->key.data(),static_cast<int>(kv->value.size()),kv->value.data());
    for(const auto* brush:entity->brushes) {
      std::fprintf(stderr,"    brush lines=%zu\n",brush->lines.size());
      for(const auto* line:brush->lines) {
        std::fprintf(stderr,"      texture=[%.*s] coords=%zu values=",static_cast<int>(line->texture.size()),line->texture.data(),line->coords.size());
        for(const auto value:line->values) std::fprintf(stderr," %.17g",value);
        std::fputc('\n',stderr);
      }
    }
  }
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      quakemap::QuakemapCoreServices, quakemap::QuakemapSharedCoreServices,
      quakemap::install_core_module>();

  {
    const auto result = language.language.parser->parse(R"map(
// Classic Quake1 MAP syntax; properties may follow brushes.
{
"classname" "worldspawn"
{
( 0 0 0 ) ( 0 1 0 ) ( 1 0 0 ) *water 0 0 0 .5 1.
( 0 0 0 )
( 0 1 0 )
( 1 0 0 ) +0light 0 0 0 1e0 1
( 0 0 0 ) ( 0 1 0 ) ( 1 0 0 ) a 0 0 0 1 1
( 0 0 0 ) ( 0 1 0 ) ( 1 0 0 ) textures/stone_1 0 0 0 1 1
( "0" 0 0 ) ( 0 1 0 ) ( 1 0 0 ) 0 0 0 0 "1" 1
}
"path" "C:\maps\a.map"
}
)map");
    const auto *map = result.value_as<quakemap::ast::Map>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !map || map->entities.size() != 1)
      return failed_expectation("native rich map syntax/root",result);
    const auto *entity = map->entities[0];
    if (entity->keyvalues.size() != 2 || entity->brushes.size() != 1 ||
        entity->keyvalues[1]->value != R"(C:\maps\a.map)")
      return failed_expectation("native property/brush order and raw backslashes",result);
    const auto &lines = entity->brushes[0]->lines;
    if (lines.size() != 5 || lines[0]->texture != "*water" ||
        lines[1]->texture != "+0light" || lines[2]->texture != "a" ||
        lines[3]->texture != "textures/stone_1" || lines[4]->texture != "0" ||
        lines[0]->values.size() != 5 || lines[0]->values[3] != .5 ||
        lines[0]->values[4] != 1 || lines[1]->values[3] != 1)
      return failed_expectation("native texture and numeric captures",result);
  }
  {
    const auto result = language.language.parser->parse(
        "{\n\"message\" \"the \"\"hidden\"\" room\"\n}\n");
    const auto *map = result.value_as<quakemap::ast::Map>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !map || map->entities.size() != 1 ||
        map->entities[0]->keyvalues.size() != 2 ||
        map->entities[0]->keyvalues[0]->value != "the " ||
        map->entities[0]->keyvalues[1]->key != "hidden" ||
        map->entities[0]->keyvalues[1]->value != " room")
      return failed_expectation("native adjacent quoted tokens remain two pairs",result);
  }
  for (const auto *source : {
           "{\n{\n(0 0) (0 1 0) (1 0 0) a 0 0 0 1 1\n}\n}",
           "{\n{\n(0 0 0) (0 1 0) a 0 0 0 1 1\n}\n}",
           "{\n{\n(0 0 0) (0 1 0) (1 0 0) a 0 0 0 1\n}\n}",
           "{\n{\n(0 0 0) (0 1 0) (1 0 0) a 0 0 0 1\n1\n}\n}",
           "{\n{\n(0\n0 0) (0 1 0) (1 0 0) a 0 0 0 1 1\n}\n}"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty()) {
      std::fprintf(stderr, "quakemap: native-invalid plane accepted: %s\n", source);
      return 1;
    }
  }

  {
    constexpr char key[] = "{\n\"before\0after\" \"value\"\n}\n";
    constexpr char value[] = "{\n\"key\" \"before\0after\"\n}\n";
    for (const auto source : {std::string_view(key, sizeof(key) - 1),
                              std::string_view(value, sizeof(value) - 1)}) {
      const auto result = language.language.parser->parse(source);
      if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
        return failed_expectation("native quoted token stops at NUL", result);
    }
  }

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".map")
        continue;
      ++total;
      const std::filesystem::path path = entry.path();
      std::ifstream file(path);
      if (!file) {
        fail(path, "not readable");
        ++failures;
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      // `recovered()` is set when the parser had to repair the input to keep
      // going: a clean document leaves it false.
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (!result.accepted || result.cst().recovered() ||
          !result.diagnostics.empty()) {
        fail(path, "the parse was recovered");
        ++failures;
        continue;
      }

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<quakemap::ast::Map>) {
        fail(path, "the parse built no Map");
        ++failures;
      }
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "quakemap: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("quakemap: %d example(s) parse\n", total);
  return 0;
}
