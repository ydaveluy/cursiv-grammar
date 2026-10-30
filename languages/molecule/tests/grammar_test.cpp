// The smoke test of the language: every document under example/ parses
// clean -- no repair, no diagnostic -- and builds a Molecule root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
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

  {
    const auto parse = language.language.parser->parse("CuSO4 · 5H2O");
    const auto* formula = parse.value_as<molecule::ast::Molecule>();
    if (!parse.accepted || parse.cst().recovered() ||
        !parse.diagnostics.empty() || !formula || formula->parts.size() != 2)
      return fail("hydrate", "no complete hydrate AST");
    const auto* water = formula->parts[1];
    if (!water || water->parent != formula || water->multiplier != 5 ||
        water->structures.size() != 2 || water->structures[0]->count != 2 ||
        water->structures[1]->count || water->structures[0]->parent != water)
      return fail("hydrate", "coefficient does not multiply all of H2O");
    const auto* hydrogen = cursiv::ast_cast<molecule::ast::Element>(
        water->structures[0]->symbol);
    const auto* oxygen = cursiv::ast_cast<molecule::ast::Element>(
        water->structures[1]->symbol);
    if (!hydrogen || hydrogen->name != "H" || !oxygen || oxygen->name != "O")
      return fail("hydrate", "wrong element projections");
  }
  for (const auto* source : {"K4[Fe(CN)6]", "8H2S·46H2O", "Og", "C60"}) {
    const auto parse = language.language.parser->parse(source);
    if (!parse.accepted || parse.cst().recovered() || !parse.diagnostics.empty())
      return fail(source, "native formula did not parse cleanly");
  }
  for (const auto* source : {"H0", "0H2O", "CuSO4·0H2O", "(OH)0"}) {
    auto document = language.shared->documentFactory->fromString(
        source, "file:///native.mol");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty() || diagnostics.size() != 1)
      return fail(source, "missing positive-count diagnostic");
  }

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
