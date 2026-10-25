// The smoke test of the language: every document under example/ parses
// clean -- no repair, no diagnostic -- and builds a Molecule root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "molecule/core/CoreServices.hpp"
#include "molecule/core/ast.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "molecule: %s: %s\n", path.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      molecule::MoleculeCoreServices, molecule::MoleculeSharedCoreServices,
      molecule::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (entry.path().extension() != ".mol") continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path().string(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path().string(), "did not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path().string(), "left a diagnostic");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<molecule::ast::Molecule>) {
      failures += fail(entry.path().string(), "built no Molecule");
    }
  }

  if (count == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");
  if (failures > 0) return 1;

  std::printf("molecule: %d examples parse\n", count);
  return 0;
}
