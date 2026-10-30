// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "trac/core/CoreServices.hpp"
#include "trac/core/ast.hpp"
#include "native_followup_cases.hpp"

namespace {

void report(const std::filesystem::path& path, const char* what) {
  std::fprintf(stderr, "trac: %s: %s\n", path.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      trac::TracCoreServices, trac::TracSharedCoreServices,
      trac::install_core_module>();
  if (!trac_native_followup::run(language)) {
    std::fprintf(stderr, "trac: native argument protection or ownership mismatch\n");
    return 1;
  }

  int count = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLES_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".trac")
        continue;
      ++count;
      const std::filesystem::path path = entry.path();

      std::ifstream file(path);
      if (!file) {
        report(path, "not readable");
        ++failures;
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        report(path, "does not parse clean");
        ++failures;
        continue;
      }

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<trac::ast::Program>) {
        report(path, "built no Program root");
        ++failures;
      }
    }

  if (count == 0) {
    report(CURSIV_EXAMPLES_DIR, "no examples found");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("trac: %d example(s) parse\n", count);
  return 0;
}
