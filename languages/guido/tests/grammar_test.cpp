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
