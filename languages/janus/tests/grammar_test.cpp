// The janus grammar end to end: every example parses clean, and the shapes
// easy to get wrong are pinned -- the right-associative operator chain, and
// a unary operator recursing on the whole expression rather than on a
// single operand.

#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "janus/core/ast.hpp"
#include "janus/generated/Janus/Module.hpp"

namespace {

namespace ast = janus::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      janus::generated::Janus::create_parser();
  return *p;
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && n->type == &cursiv::ast_type_tag<T>)
             ? static_cast<T*>(n)
             : nullptr;
}

const std::string& kept(std::string text) {
  static std::deque<std::string> documents;
  documents.push_back(std::move(text));
  return documents.back();
}

cursiv::ParseResult parse(std::string_view text) { return engine().parse(text); }

void every_example_parses_clean() {
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".janus") continue;
    const std::string text = cursiv::test::read_file(entry.path().string());
    CHECK(!text.empty());
    const cursiv::ParseResult r = parse(kept(text));
    CHECK(r.accepted);
    CHECK(!r.cst().recovered());
    ast::Program* const root = as<ast::Program>(r.value);
    CHECK(root != nullptr);
  }
}

// The operator chain is right-recursive: every level is the same
// precedence, grouped right to left.
void chain_is_right_associative() {
  const cursiv::ParseResult r =
      parse(kept("PROCEDURE p\n  z += a + b + c\n"));
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const program = as<ast::Program>(r.value);
  CHECK(program != nullptr && program->procedures.size() == 1);
  if (program == nullptr || program->procedures.size() != 1) return;
  CHECK(program->procedures[0]->statements.size() == 1);
  ast::ModStmt* const mod = as<ast::ModStmt>(program->procedures[0]->statements[0]);
  CHECK(mod != nullptr && mod->op == ast::ModOp::plus_eq);
  if (mod == nullptr) return;
  ast::BinaryExpr* const outer = as<ast::BinaryExpr>(mod->value);
  CHECK(outer != nullptr && outer->op == ast::BinOp::plus);
  if (outer == nullptr) return;
  CHECK(as<ast::Lvalue>(outer->left) != nullptr);
  ast::BinaryExpr* const inner = as<ast::BinaryExpr>(outer->right);
  CHECK(inner != nullptr && inner->op == ast::BinOp::plus);
  if (inner == nullptr) return;
  CHECK(as<ast::Lvalue>(inner->left) != nullptr);
  CHECK(as<ast::Lvalue>(inner->right) != nullptr);
}

// A unary `-`/`~` recurses on the whole expression, not on a single
// operand: `-a + b` is `-(a + b)`, the unary wraps the entire chain that
// follows it.
void unary_wraps_whole_expression() {
  const cursiv::ParseResult r = parse(kept("PROCEDURE p\n  z += -a + b\n"));
  CHECK(r.accepted && !r.cst().recovered());
  ast::Program* const program = as<ast::Program>(r.value);
  CHECK(program != nullptr && program->procedures.size() == 1);
  if (program == nullptr || program->procedures.size() != 1) return;
  ast::ModStmt* const mod = as<ast::ModStmt>(program->procedures[0]->statements[0]);
  CHECK(mod != nullptr);
  if (mod == nullptr) return;
  ast::UnaryExpr* const un = as<ast::UnaryExpr>(mod->value);
  CHECK(un != nullptr && un->op == ast::UnaryOp::negate);
  if (un == nullptr) return;
  ast::BinaryExpr* const sum = as<ast::BinaryExpr>(un->operand);
  CHECK(sum != nullptr && sum->op == ast::BinOp::plus);
}

// An lvalue statement's two tails are distinct constructs: a modifying
// `<=>` expression against a swap's `:` lvalue.
void exchange_and_swap_are_distinct() {
  const cursiv::ParseResult r1 = parse(kept("PROCEDURE p\n  x <=> y\n"));
  CHECK(r1.accepted && !r1.cst().recovered());
  ast::Program* const p1 = as<ast::Program>(r1.value);
  CHECK(p1 != nullptr && p1->procedures.size() == 1 &&
        p1->procedures[0]->statements.size() == 1);
  if (p1 != nullptr && p1->procedures.size() == 1 &&
      p1->procedures[0]->statements.size() == 1) {
    ast::ModStmt* const mod =
        as<ast::ModStmt>(p1->procedures[0]->statements[0]);
    CHECK(mod != nullptr && mod->op == ast::ModOp::exchange);
  }

  const cursiv::ParseResult r2 = parse(kept("PROCEDURE p\n  x : y\n"));
  CHECK(r2.accepted && !r2.cst().recovered());
  ast::Program* const p2 = as<ast::Program>(r2.value);
  CHECK(p2 != nullptr && p2->procedures.size() == 1 &&
        p2->procedures[0]->statements.size() == 1);
  if (p2 != nullptr && p2->procedures.size() == 1 &&
      p2->procedures[0]->statements.size() == 1) {
    ast::SwapStmt* const swap =
        as<ast::SwapStmt>(p2->procedures[0]->statements[0]);
    CHECK(swap != nullptr);
  }
}

}  // namespace

int main() {
  every_example_parses_clean();
  chain_is_right_associative();
  unary_wraps_whole_expression();
  exchange_and_swap_are_distinct();
  return cursiv::test::report("janus_grammar_test");
}
