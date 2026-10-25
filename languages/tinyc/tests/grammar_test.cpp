#include "audit_regressions.hpp"
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
#include "tinyc/core/CoreServices.hpp"
#include "tinyc/core/ast.hpp"

namespace {

void fail(const std::filesystem::path &file, const char *what) {
  std::fprintf(stderr, "tinyc: %s: %s\n", file.string().c_str(), what);
}

} // namespace

int main() {
  audit_regressions::run();
  const auto language = cursiv::create_language<tinyc::TinycCoreServices,
                                                tinyc::TinycSharedCoreServices,
                                                tinyc::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".tinyc")
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

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<tinyc::ast::Program>) {
        fail(path, "the parse built no Program");
        ++failures;
      }
    }

  if (total == 0) {
    std::fprintf(stderr, "tinyc: no example files found\n");
    return 1;
  }
  if (failures > 0)
    return 1;
  std::printf("tinyc: %d example(s) parse\n", total);
  return cursiv::test::report("tinyc_grammar_test");
}
