// The muparser grammar end to end: every example parses clean, and the two
// shapes easy to get wrong are pinned -- `^` against unary minus, and the
// ternary's right-associative chaining.

#include <memory>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "muparser/core/ast.hpp"
#include "muparser/generated/Muparser/Module.hpp"


#include "audit_regressions.hpp"

namespace {

namespace ast = muparser::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      muparser::generated::Muparser::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

cursiv::ParseResult parse(std::string_view text) { return engine().parse(text); }

const char* const kExamples[] = {
    "example1.muparser",
};

void every_example_parses_clean() {
  for (const char* const name : kExamples) {
    const std::string text = cursiv::test::read_file(
        std::string(CURSIV_EXAMPLE_DIR) + "/" + name);
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(text);
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    ast::Program* const root = as<ast::Program>(r.value);
    CHECK(root != nullptr && !root->exprs.empty());
  }
}

// The corpus example never exercises assignment, equality, `&&`/`||`,
// `<=`/`>=`/`<`, or a bare variable reference: constructs.muparser does.
void own_constructs_parse_clean() {
  const std::string text = cursiv::test::read_file(
      std::string(CURSIV_TEST_DIR) + "/constructs.muparser");
  CHECK(!text.empty());
  const cursiv::ParseResult r = parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  ast::Program* const root = as<ast::Program>(r.value);
  CHECK(root != nullptr && !root->exprs.empty());
}

// `^` binds tighter than unary minus, which itself binds tighter than
// `*`/`/`: `-2^2` is `-(2^2)`, not `(-2)^2`.
void pow_against_unary() {
  const cursiv::ParseResult r = parse("-2^2\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const root = as<ast::Program>(r.value);
  CHECK(root != nullptr && root->exprs.size() == 1);
  if (root == nullptr || root->exprs.empty()) return;
  ast::UnaryExpr* const neg = as<ast::UnaryExpr>(root->exprs[0]);
  CHECK(neg != nullptr);
  if (neg == nullptr) return;
  ast::BinaryExpr* const pow = as<ast::BinaryExpr>(neg->operand);
  CHECK(pow != nullptr && pow->op == ast::Operator::pow);
}

// The `? :` in the else branch chains right: `a?b:c?d:e` is `a?b:(c?d:e)`.
void ternary_chains_right() {
  const cursiv::ParseResult r = parse("1?2:0?3:4\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const root = as<ast::Program>(r.value);
  CHECK(root != nullptr && root->exprs.size() == 1);
  if (root == nullptr || root->exprs.empty()) return;
  ast::CondExpr* const outer = as<ast::CondExpr>(root->exprs[0]);
  CHECK(outer != nullptr);
  if (outer == nullptr) return;
  CHECK(as<ast::NumberLiteral>(outer->thenValue) != nullptr);
  ast::CondExpr* const inner = as<ast::CondExpr>(outer->elseValue);
  CHECK(inner != nullptr);
}

// FUNCTION takes exactly one argument, FUNCTIONMULTI one or more; both build
// the same node.
void function_calls_both_arities() {
  const cursiv::ParseResult r = parse("sin(1)\n");
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const root = as<ast::Program>(r.value);
  CHECK(root != nullptr && root->exprs.size() == 1);
  if (root != nullptr && !root->exprs.empty()) {
    ast::FunctionCall* const call = as<ast::FunctionCall>(root->exprs[0]);
    CHECK(call != nullptr && call->name == "sin" && call->args.size() == 1);
  }

  const cursiv::ParseResult r2 = parse("max(1,2,3)\n");
  CHECK(r2.accepted && !r2.cst().recovered());
  ast::Program* const root2 = as<ast::Program>(r2.value);
  CHECK(root2 != nullptr && root2->exprs.size() == 1);
  if (root2 != nullptr && !root2->exprs.empty()) {
    ast::FunctionCall* const call = as<ast::FunctionCall>(root2->exprs[0]);
    CHECK(call != nullptr && call->name == "max" && call->args.size() == 3);
  }
}

// An assignment reads as a primary, reachable anywhere an operand is, not
// only at the top of an expression.
void assignment_inside_a_call() {
  const cursiv::ParseResult r = parse("sin(x=1)\n");
  CHECK(r.accepted && !r.cst().recovered());
}

}  // namespace

int main() {
  muparser_regressions();
  every_example_parses_clean();
  own_constructs_parse_clean();
  pow_against_unary();
  ternary_chains_right();
  function_calls_both_arities();
  assignment_inside_a_call();
  return cursiv::test::report("muparser_grammar_test");
}
