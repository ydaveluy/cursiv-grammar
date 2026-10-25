// The smoke test of the language: every document under example/ parses clean
// and builds the tree it declares.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "bdf/core/CoreServices.hpp"
#include "bdf/core/ast.hpp"

namespace {

int fail(const char* path, const char* what) {
  std::fprintf(stderr, "bdf: %s: %s\n", path, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      bdf::BdfCoreServices,
      bdf::BdfSharedCoreServices,
      bdf::install_core_module>();

  int failures = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::string path = entry.path().string();

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(path.c_str(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(path.c_str(), "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bdf::ast::Font>) {
      failures += fail(path.c_str(), "the parse built no font");
      continue;
    }
  }

  if (failures != 0) return 1;
  std::puts("bdf: every example parses");
  return 0;
}
