// Orwell end to end: every document in example/ parses clean, and the AST
// holds what each one declares -- application patterns, guarded equations,
// the operator text a binary term carries.

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "orwell/core/ast.hpp"
#include "orwell/generated/Orwell/Module.hpp"

namespace {

namespace ast = orwell::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      orwell::generated::Orwell::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      orwell::generated::Orwell::create_ast_reflection();
  return *r;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr &&
          reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  return r;
}

// Every document of the corpus, clean, with its AST checked against the
// field values the source spells.
void corpus() {
  const std::string dir = std::string(CURSIV_EXAMPLE_DIR) + "/example/";

  // example1: `power x n =1`
  {
    const cursiv::ParseResult r = build(cursiv::test::read_file(dir + "example1.orwell"));
    CHECK(!r.cst().recovered() && r.diagnostics.empty());
    ast::Program* const p = r.value_as<ast::Program>();
    CHECK(p != nullptr && p->decls.size() == 1);
    ast::Def* const def = p != nullptr ? as<ast::Def>(p->decls[0]) : nullptr;
    CHECK(def != nullptr && def->clauses.size() == 1);
    ast::DefClause* const clause = def != nullptr ? def->clauses[0] : nullptr;
    CHECK(clause != nullptr && !clause->hasElse);
    ast::PatApply* const pat =
        clause != nullptr ? as<ast::PatApply>(clause->pat) : nullptr;
    CHECK(pat != nullptr && pat->args.size() == 2);
    ast::PatVar* const head = pat != nullptr ? as<ast::PatVar>(pat->ctor) : nullptr;
    CHECK(head != nullptr && head->name == "power");
    ast::Rhs* const rhs = clause != nullptr ? clause->rhs : nullptr;
    CHECK(rhs != nullptr && rhs->simple != nullptr && rhs->guards.empty());
    ast::IntLit* const value = rhs != nullptr ? as<ast::IntLit>(rhs->simple) : nullptr;
    CHECK(value != nullptr && value->value == 1);
  }

  // example2: `power x n =1, if n=0`
  {
    const cursiv::ParseResult r = build(cursiv::test::read_file(dir + "example2.orwell"));
    CHECK(!r.cst().recovered() && r.diagnostics.empty());
    ast::Program* const p = r.value_as<ast::Program>();
    ast::Def* const def = p != nullptr && !p->decls.empty() ? as<ast::Def>(p->decls[0]) : nullptr;
    ast::DefClause* const clause =
        def != nullptr && !def->clauses.empty() ? def->clauses[0] : nullptr;
    ast::Rhs* const rhs = clause != nullptr ? clause->rhs : nullptr;
    CHECK(rhs != nullptr && rhs->simple == nullptr && rhs->guards.size() == 1);
    ast::GuardedResult* const guard =
        rhs != nullptr && !rhs->guards.empty() ? rhs->guards[0] : nullptr;
    ast::IntLit* const result =
        guard != nullptr ? as<ast::IntLit>(guard->result) : nullptr;
    CHECK(result != nullptr && result->value == 1);
    // `n=0`, spelled with no space, is ONE ID token, not a comparison.
    ast::VarRef* const cond =
        guard != nullptr ? as<ast::VarRef>(guard->condition) : nullptr;
    CHECK(cond != nullptr && cond->name == "n=0");
  }

  // example3: `roots a b c = [(-b+r)/(2*a),(-b-r)/(2*a)], if d>0`
  {
    const cursiv::ParseResult r = build(cursiv::test::read_file(dir + "example3.orwell"));
    CHECK(!r.cst().recovered() && r.diagnostics.empty());
    ast::Program* const p = r.value_as<ast::Program>();
    ast::Def* const def = p != nullptr && !p->decls.empty() ? as<ast::Def>(p->decls[0]) : nullptr;
    ast::DefClause* const clause =
        def != nullptr && !def->clauses.empty() ? def->clauses[0] : nullptr;
    ast::PatApply* const pat =
        clause != nullptr ? as<ast::PatApply>(clause->pat) : nullptr;
    CHECK(pat != nullptr && pat->args.size() == 3);
    ast::Rhs* const rhs = clause != nullptr ? clause->rhs : nullptr;
    CHECK(rhs != nullptr && rhs->guards.size() == 1);
    ast::ListTerm* const list =
        rhs != nullptr && !rhs->guards.empty()
            ? as<ast::ListTerm>(rhs->guards[0]->result)
            : nullptr;
    CHECK(list != nullptr && list->items.size() == 2);
    // `d>0`, tight, is likewise one ID token: see the note in example2 above.
    ast::VarRef* const cond = rhs != nullptr && !rhs->guards.empty()
                                   ? as<ast::VarRef>(rhs->guards[0]->condition)
                                   : nullptr;
    CHECK(cond != nullptr && cond->name == "d>0");
    ast::BinaryTerm* const elem0 =
        list != nullptr && !list->items.empty()
            ? as<ast::BinaryTerm>(list->items[0])
            : nullptr;
    CHECK(elem0 != nullptr && elem0->op == "/");
  }
}

// Documents of my own, one per construct the corpus never exercises: syndecl
// (all three `tylhs` shapes), condecl, typedecl, opdecl, tuple/list/
// constructor patterns, a `where` clause, a guarded multi-branch equation,
// a `%else` multi-clause equation, a list comprehension, a range, operator
// sections, and float/string literals, one small document each.
void own_coverage() {
  const std::string dir = std::string(CURSIV_TEST_DIR) + "/";
  for (const char* name : {"types.orwell", "fixity.orwell", "patterns.orwell",
                           "guards.orwell", "multiclause.orwell",
                           "comprehensions.orwell", "sections.orwell",
                           "literals.orwell"}) {
    const cursiv::ParseResult r = build(cursiv::test::read_file(dir + name));
    if (r.cst().recovered() || !r.diagnostics.empty()) {
      std::printf("FAIL %s: did not parse clean\n", name);
      ++failures;
    }
    ast::Program* const p = r.value_as<ast::Program>();
    if (p == nullptr || p->decls.empty()) {
      std::printf("FAIL %s: built no declarations\n", name);
      ++failures;
    }
  }

  // The `%else` clause read back: the marker sets `hasElse`, the first
  // clause never does.
  const cursiv::ParseResult r =
      build(cursiv::test::read_file(dir + "multiclause.orwell"));
  ast::Program* const p = r.value_as<ast::Program>();
  ast::Def* const def = p != nullptr && !p->decls.empty() ? as<ast::Def>(p->decls[0]) : nullptr;
  CHECK(def != nullptr && def->clauses.size() == 2);
  CHECK(def != nullptr && def->clauses.size() == 2 && !def->clauses[0]->hasElse &&
        def->clauses[1]->hasElse);
}

}  // namespace

int main() {
  corpus();
  own_coverage();
  return cursiv::test::report("orwell_grammar_test");
}
