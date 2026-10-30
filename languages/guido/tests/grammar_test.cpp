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
#include "guido/core/CoreServices.hpp"
#include "guido/core/ast.hpp"
#include "native_cases.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "guido: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      guido::GuidoCoreServices, guido::GuidoSharedCoreServices,
      guido::install_core_module>();

  {
    const std::string source = R"GMN([s1:3\:5:*1/4.. {s2:7:,s6:0:} \title<"a\:b">])GMN";
    const auto parsed = language.language.parser->parse(source);
    const auto* root = parsed.value_as<guido::ast::Prog>();
    const auto* sequence = root && root->sequences.size() == 1 ? root->sequences[0] : nullptr;
    const auto* tab = sequence && sequence->items.size() == 3 ?
        cursiv::ast_cast<guido::ast::Tablature>(sequence->items[0]) : nullptr;
    const auto* tag = sequence && sequence->items.size() == 3 ?
        cursiv::ast_cast<guido::ast::Tag>(sequence->items[2]) : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !tab || tab->string != "s1" || tab->frets != "3:5" || tab->dots != ".." ||
        !tab->duration || !tab->duration->numerator ||
        tab->duration->numerator->value != "1" || !tag ||
        tag->parameters.size() != 1 || tag->parameters[0]->text != "a:b" ||
        !cursiv::test::validationHelper(language.language, source).diagnostics.empty()) {
      fail(source, "native tablature, duration and colon escapes must retain their values");
      return 1;
    }
  }
  if (cursiv::test::validationHelper(language.language, "[s1:0:....]").diagnostics.empty()) {
    fail("[s1:0:....]", "tablature permits at most three duration dots");
    return 1;
  }

  for (const auto source : {R"GMN([\title<"a\"b">])GMN",
                            R"GMN([\title<'a\'b'>])GMN"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty()) {
      fail(source, "escaped quote must stay inside its string");
      return 1;
    }
  }
  for (const auto source : {R"GMN([\title<"unfinished\">])GMN",
                            R"GMN([\title<'unfinished\'>])GMN"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty()) {
      fail(source, "an escaped delimiter cannot close a string");
      return 1;
    }
  }

  int total = 0;
  int failures = native_regressions::run(language);
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".gmn")
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
      // going: a clean document leaves it false. A diagnostic can still fire
      // on an otherwise-accepted parse (an out-of-range number, say), which
      // recovered() alone would not catch.
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        fail(path, "the parse was recovered");
        ++failures;
        continue;
      }
      if (!result.diagnostics.empty()) {
        fail(path, "the parse carries diagnostics");
        ++failures;
        continue;
      }

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<guido::ast::Prog>) {
        fail(path, "the parse built no Prog");
        ++failures;
      }
    }

  if (total == 0) {
    std::fprintf(stderr, "guido: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("guido: %d example(s) parse\n", total);
  return 0;
}
