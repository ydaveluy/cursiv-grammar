// The smoke test of the language: every corpus document parses clean, a
// grammar of our own exercises what the corpus never does, and res_proof is
// read back field by field to catch a converter that silently mismaps its
// value.

#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "spass/core/CoreServices.hpp"
#include "spass/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "spass: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "pelleter_57.spass",
    "res_proof.spass",
    "sort_decls.spass",
};

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      spass::SpassCoreServices, spass::SpassSharedCoreServices,
      spass::install_core_module>();

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<spass::ast::Problem>)
      return fail((std::string(name) + ": built no Problem").c_str());
  }

  // Fresh SPASS 3.9 controls: underscores, digit-leading/numeric symbols,
  // native line comments, and the two reserved quantifiers.
  const auto source = [](std::string_view name, std::string_view term) {
    return "% comment\n"
           "begin_problem(test_1).\nlist_of_descriptions.\n"
           "name({*Test*}).\nauthor({*Audit*}).\nstatus(unknown).\n"
           "description({*Syntax*}).\nend_of_list.\nlist_of_symbols.\n"
           "functions[(" + std::string(name) + ",0)].\n"
           "predicates[(p,1),(somequant,2)].\nend_of_list.\n"
           "list_of_formulae(axioms).\nformula(" + std::string(term) +
           ").\nend_of_list.\nend_problem. % EOF comment";
  };
  for (const auto name : {"_a", "a_b", "1_a", "123"}) {
    const auto text = source(name, "p(" + std::string(name) + ")");
    const auto parsed = language.language.parser->parse(text);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
      return fail("native symbol/comment control failed");
    const auto* root = static_cast<const spass::ast::Problem*>(parsed.value);
    if (!root || (**root->logicalPart->symbols).functions[0]->name != name)
      return fail("native symbol spelling lost");
  }
  for (const auto term : {"nor(p(a),p(a))", "nand(p(a),p(a))", "xor(p(a),p(a))",
                          "true", "not(p(a))", "implies(p(a),p(a))", "equal(a,a)"}) {
    const auto text = source("a", term);
    const auto parsed = language.language.parser->parse(text);
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty())
      return fail("native connective failed");
  }
  for (const auto text : {source("a-b", "p(a-b)"),
                          source("a", "somequant([X],p(X))"),
                          source("a", ""), source("forall", "p(forall)"),
                          source("a", "not"), source("a", "not(p(a),p(a))"),
                          source("a", "implies(p(a))"),
                          source("a", "implies(p(a),p(a),p(a))"),
                          source("a", "equal(a,a,a)")}) {
    const auto parsed = language.language.parser->parse(text);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("invalid native construct accepted cleanly");
  }

  // features.spass covers pred_decl, gen_decl, settings, dnf clauses, the
  // arity sentinel and assoc lists -- none of which the corpus exercises.
  {
    const std::string text = read_file(CURSIV_FEATURES_SAMPLE);
    if (text.empty()) return fail("features.spass: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("features.spass: does not parse clean");
    if (!result.diagnostics.empty())
      return fail("features.spass: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<spass::ast::Problem>)
      return fail("features.spass: built no Problem");

    const auto& problem = *static_cast<const spass::ast::Problem*>(root);
    const spass::ast::LogicalPart& lp = *problem.logicalPart;
    if (!lp.declarations.has_value() ||
        (*lp.declarations)->declarations.size() != 4)
      return fail("features.spass: expected 4 declarations");
    if (lp.clauseLists.size() != 1 || lp.clauseLists[0]->clauses.size() != 3)
      return fail("features.spass: expected 3 clauses");
    if (problem.settings.size() != 2)
      return fail("features.spass: expected 2 settings sections");
  }

  // res_proof.spass, read back field by field.
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/res_proof.spass");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& problem = *static_cast<const spass::ast::Problem*>(result.value);

  if (problem.name != "ProofDemo") return fail("res_proof: wrong name");

  const spass::ast::Description& desc = *problem.description;
  if (desc.status != spass::ast::LogState::unsatisfiable)
    return fail("res_proof: wrong status");
  if (!desc.name.has_value() || *desc.name != "test.dfg")
    return fail("res_proof: wrong description name (FREE_TEXT mismapped)");
  if (!desc.author.has_value() || *desc.author != "SPASS")
    return fail("res_proof: wrong author");

  const spass::ast::LogicalPart& lp = *problem.logicalPart;
  if (!lp.symbols.has_value()) return fail("res_proof: no symbol list");
  const spass::ast::SymbolList& symbols = **lp.symbols;
  if (symbols.functions.size() != 1 || symbols.functions[0]->name != "skf1" ||
      !symbols.functions[0]->arity.has_value() ||
      *symbols.functions[0]->arity != 1)
    return fail("res_proof: wrong function symbol");
  if (symbols.predicates.size() != 1 || symbols.predicates[0]->name != "P" ||
      *symbols.predicates[0]->arity != 2)
    return fail("res_proof: wrong predicate symbol");

  if (lp.proofLists.size() != 1) return fail("res_proof: no proof list");
  const spass::ast::ProofList& proof = *lp.proofLists[0];
  if (!proof.proofType.has_value() || *proof.proofType != "SPASS")
    return fail("res_proof: wrong proof type");
  if (proof.steps.size() != 10) return fail("res_proof: wrong step count");

  const spass::ast::Step& step0 = *proof.steps[0];
  const auto* const ref = std::get_if<std::int64_t>(&step0.reference);
  if (ref == nullptr || *ref != 10) return fail("res_proof: wrong reference");
  const auto* const rule = std::get_if<std::string_view>(&step0.ruleAppl);
  if (rule == nullptr || *rule != "SpR")
    return fail("res_proof: wrong rule application");
  if (step0.parents == nullptr || step0.parents->items.size() != 2)
    return fail("res_proof: wrong parent count");
  const auto* const p0 = std::get_if<std::int64_t>(&step0.parents->items[0]);
  const auto* const p1 = std::get_if<std::int64_t>(&step0.parents->items[1]);
  if (p0 == nullptr || *p0 != 3 || p1 == nullptr || *p1 != 1)
    return fail("res_proof: wrong parent values");

  std::puts("spass: the corpus parses");
  return 0;
}
