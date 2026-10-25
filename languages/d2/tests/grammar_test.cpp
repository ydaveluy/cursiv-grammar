// The grammar test of this language: every document under example/ parses
// clean, and the tree's root is a Diagram.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "d2/core/CoreServices.hpp"
#include "d2/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "d2: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      d2::D2CoreServices, d2::D2SharedCoreServices, d2::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".d2")
        continue;
      ++count;
      std::ifstream file(entry.path());
      if (!file) {
        failures += fail(entry.path(), "the document is not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(entry.path(), "did not parse clean");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<d2::ast::Diagram>) {
        failures += fail(entry.path(), "did not build a Diagram root");
      }
    }
  }
  if (count == 0) return fail(fs::path(CURSIV_EXAMPLE_DIR), "no example found");
  if (failures != 0) return 1;

  std::printf("d2: %d example(s) parse clean\n", count);
  return 0;
}
