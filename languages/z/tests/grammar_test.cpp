#include "audit_regressions.hpp"
// The smoke test of the language: every file of `example/` parses clean
// (no recovery, no diagnostics), and one document's fields hold what it
// declares.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "z/core/CoreServices.hpp"
#include "z/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "z: %s\n", what.c_str());
  return 1;
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      z::ZCoreServices, z::ZSharedCoreServices, z::install_core_module>();

  std::vector<std::filesystem::path> examples;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".zed")
      examples.push_back(entry.path());
  if (examples.empty()) return fail("no example found");

  for (const std::filesystem::path& path : examples) {
    const std::string text = readFile(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");
    if (!result.diagnostics.empty())
      return fail(path.filename().string() + " carries diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<z::ast::Specification>)
      return fail(path.filename().string() + " built no specification");
  }

  // Constructs no example in `example/` reaches: `lambda`/`mu`/`theta`,
  // unique existence, binding extension, a generic schema/axiomatic/
  // horizontal/conjecture paragraph, an operator template paragraph.
  const std::filesystem::path coveragePath =
      std::filesystem::path(CURSIV_TEST_DIR) / "coverage.zed";
  const std::string coverageText = readFile(coveragePath);
  const cursiv::ParseResult coverageResult =
      language.language.parser->parse(coverageText);
  if (coverageResult.cst().recovered())
    return fail("coverage.zed does not parse clean");
  if (!coverageResult.diagnostics.empty())
    return fail("coverage.zed carries diagnostics");
  if (coverageResult.value == nullptr ||
      coverageResult.value->type != &cursiv::ast_type_tag<z::ast::Specification>)
    return fail("coverage.zed built no specification");

  // ss.zed: one section, `ss`, with `ds` as a parent, and eight paragraphs
  // in its body -- a value a converter could silently get wrong is not
  // caught by the structural checks above.
  const std::string ssText = readFile(std::filesystem::path(CURSIV_EXAMPLE_DIR) / "ss.zed");
  const cursiv::ParseResult ssResult = language.language.parser->parse(ssText);
  const auto& spec =
      *static_cast<const z::ast::Specification*>(ssResult.value);
  if (spec.items.size() != 1) return fail("ss.zed: not one top-level item");
  if (spec.items[0]->type != &cursiv::ast_type_tag<z::ast::Section>)
    return fail("ss.zed: the top-level item is not a section");
  const auto& section = *static_cast<const z::ast::Section*>(spec.items[0]);
  if (section.name != "ss") return fail("ss.zed: the section is not named ss");
  if (section.parents.size() != 1 || section.parents[0] != "ds")
    return fail("ss.zed: the section does not declare ds as its parent");
  if (section.body.size() != 8)
    return fail("ss.zed: the section does not hold its eight paragraphs");
  if (section.body[0]->type != &cursiv::ast_type_tag<z::ast::HorizontalDef>)
    return fail("ss.zed: FID is not a horizontal definition");
  const auto& fid = *static_cast<const z::ast::HorizontalDef*>(section.body[0]);
  if (fid.name != "FID") return fail("ss.zed: the first paragraph is not FID");

  // Propositional connective precedence (Spivey, "The Z Notation: A
  // Reference Manual", 3.7 Predicates): decreasing binding power
  // ¬ > ∧ > ∨ > ⇒ > ⇔, and "⇒ associates to the right, [while] the other
  // binary ones associate to the left" -- so `a ⇒ b ⇒ c` is `a ⇒ (b ⇒ c)`.
  const std::string precText =
      "─ section prec └\n\n─ ⊢? a ⇒ b ⇒ c └\n";
  const cursiv::ParseResult precResult =
      language.language.parser->parse(precText);
  if (precResult.cst().recovered() || !precResult.diagnostics.empty())
    return fail("the precedence probe does not parse clean");
  const auto& precSpec =
      *static_cast<const z::ast::Specification*>(precResult.value);
  const auto& precSection =
      *static_cast<const z::ast::Section*>(precSpec.items[0]);
  if (precSection.body.size() != 1 ||
      precSection.body[0]->type != &cursiv::ast_type_tag<z::ast::Conjecture>)
    return fail("the precedence probe built no Conjecture");
  const auto& conjecture =
      *static_cast<const z::ast::Conjecture*>(precSection.body[0]);
  if (conjecture.value->type != &cursiv::ast_type_tag<z::ast::Infix>)
    return fail("a ⇒ b ⇒ c: expected an Infix at the top");
  const auto& outer = *static_cast<const z::ast::Infix*>(conjecture.value);
  if (outer.op != "⇒" ||
      outer.right->type != &cursiv::ast_type_tag<z::ast::Infix>)
    return fail("a ⇒ b ⇒ c: expected a ⇒ (b ⇒ c), right-associative");
  if (static_cast<const z::ast::Infix*>(outer.right)->op != "⇒")
    return fail("a ⇒ b ⇒ c: the nested node is not another ⇒");

  std::puts("z: every example parses");
  return 0;
}
