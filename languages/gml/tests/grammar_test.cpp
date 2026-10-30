// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "gml/core/CoreServices.hpp"
#include "gml/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "gml: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      gml::GmlCoreServices, gml::GmlSharedCoreServices,
      gml::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("graph []\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      gml::GmlCoreServices, gml::GmlSharedCoreServices,
      gml::install_core_module>();

  {
    const std::string source = "graph [\rtext \"alpha\r beta\r gamma\"\r]\r";
    const auto result = language.language.parser->parse(source);
    const auto* root = result.value_as<gml::ast::List>();
    const auto* nested = root && root->entries.size() == 1 ?
        std::get_if<gml::ast::List*>(&root->entries[0]->value) : nullptr;
    const auto* text = nested && (*nested)->entries.size() == 1 ?
        std::get_if<std::string_view>(&(*nested)->entries[0]->value) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !text || *text != "alpha beta gamma") return 1;
    for (const auto source : {"graph [ value -NAN ]", "graph [ value +NAN ]"}) {
      const auto invalid = language.language.parser->parse(source);
      if (invalid.accepted && !invalid.cst().recovered() && invalid.diagnostics.empty())
        return 1;
    }
  }

  const std::string source = R"GML(# graph data
graph [
  sample_value 1.E+12
  big 1234567890123456789012345678901234567890
  text "A &quot;x&quot; &amp; &#x1f680;"
  multiline "alpha
    beta
   gamma"
  infinity -INF
  invalidReference "&bogus;&#99999999999999999999999999;"
]
)GML";
  const auto parsed = language.language.parser->parse(source);
  const auto* root = cursiv::ast_cast<gml::ast::List>(parsed.value);
  if (!root || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      root->entries.size() != 1)
    return 1;
  const auto* const list = std::get<gml::ast::List*>(root->entries[0]->value);
  if (!list || list->entries.size() != 6 ||
      list->entries[0]->key != "sample_value" ||
      std::get<gml::ast::Number*>(list->entries[0]->value)->literal != "1.E+12" ||
      std::get<gml::ast::Number*>(list->entries[1]->value)->literal !=
          "1234567890123456789012345678901234567890" ||
      std::get<std::string_view>(list->entries[2]->value) != "A \"x\" & 🚀" ||
      !list->entries[2]->quoted ||
      std::get<std::string_view>(list->entries[3]->value) != "alpha beta gamma" ||
      std::get<gml::ast::Number*>(list->entries[4]->value)->literal != "-INF" ||
      std::get<std::string_view>(list->entries[5]->value) !=
          "&bogus;&#99999999999999999999999999;") {
    std::fprintf(stderr, "gml: source number or decoded string was lost\n");
    return 1;
  }
  const std::string invalid = "graph [ 2bad 1 text bare ]";
  const auto permissive = language.language.parser->parse(invalid);
  if (!permissive.accepted || permissive.cst().recovered() ||
      !permissive.diagnostics.empty()) return 1;
  const auto validation = cursiv::test::validationHelper(language.language, invalid);
  bool key = false, value = false;
  for (const auto& diagnostic : validation.diagnostics) {
    if (!diagnostic.code || !diagnostic.code->isString()) continue;
    key |= diagnostic.code->string() == "gml.key" &&
           diagnostic.range.begin == 8 && diagnostic.range.end == 12;
    value |= diagnostic.code->string() == "gml.value" &&
             diagnostic.range.begin == 20 && diagnostic.range.end == 24;
  }
  if (!key || !value) return 1;

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".gml")
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
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<gml::ast::List>) {
      fail(path, "the parse built no List");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "gml: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("gml: %d example(s) parse\n", total);
  return 0;
}
