#pragma once
#include "peoplecode/core/ast.hpp"

namespace peoplecode_followup {
inline void run(const cursiv::Parser& parser) {
  namespace ast = peoplecode::ast;
  const auto result = parser.parse(
      "iF &a != &b tHeN &x = 'don''t'; eLsE &x = \"say \"\"yes\"\"\"; eNd-If;\n"
      "&n = 2 ** 3 ** 2 + .01;\n"
      "fOr &i = 00 To 3 StEp 1; &x = &i; EnD-FoR;\n"
      "EvAlUaTe &x WhEn <= 2; &x = 0; WhEn Not>= 5; &x = 1; EnD-EvAlUaTe;\n"
      "Local array of string &a; Local array of array of string &b;\n");
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) {
    const auto source = result.cst().text();
    std::fprintf(stderr, "peoplecode follow-up: %.*s\n", int(source.size()), source.data());
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "kind=%u begin=%zu end=%zu element=%u\n",
                   unsigned(d.kind), std::size_t(d.begin), std::size_t(d.end), unsigned(d.element));
  }
  CHECK(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
  const auto* root = result.value_as<ast::Program>();
  CHECK(root && root->statements.size() == 6);
  if (!root || root->statements.size() != 6) return;
  const auto* conditional = cursiv::ast_cast<ast::IfStmt>(root->statements[0]);
  const auto* first = conditional && conditional->thenBranch.size() == 1
      ? cursiv::ast_cast<ast::AssignStmt>(conditional->thenBranch[0]) : nullptr;
  const auto* second = conditional && conditional->elseBranch.size() == 1
      ? cursiv::ast_cast<ast::AssignStmt>(conditional->elseBranch[0]) : nullptr;
  const auto* left = first ? cursiv::ast_cast<ast::StringLit>(first->value) : nullptr;
  const auto* right = second ? cursiv::ast_cast<ast::StringLit>(second->value) : nullptr;
  const auto* comparison = conditional ? cursiv::ast_cast<ast::BinaryExpr>(conditional->cond) : nullptr;
  CHECK(first && second && left && left->value == "don't" && right &&
        right->value == "say \"yes\"" && first->parent == conditional &&
        left->parent == first && comparison && comparison->op == ast::BinOp::not_equal);
  const auto* assignment = cursiv::ast_cast<ast::AssignStmt>(root->statements[1]);
  const auto* add = assignment ? cursiv::ast_cast<ast::BinaryExpr>(assignment->value) : nullptr;
  const auto* power = add ? cursiv::ast_cast<ast::BinaryExpr>(add->left) : nullptr;
  const auto* nested = power ? cursiv::ast_cast<ast::BinaryExpr>(power->left) : nullptr;
  const auto* decimal = add ? cursiv::ast_cast<ast::DecimalLit>(add->right) : nullptr;
  CHECK(add && add->op == ast::BinOp::add && power && power->op == ast::BinOp::power &&
        nested && nested->op == ast::BinOp::power && decimal && decimal->value == .01);
  const auto* loop = cursiv::ast_cast<ast::ForStmt>(root->statements[2]);
  CHECK(loop && loop->step && loop->body.size() == 1);
  const auto* evaluate = cursiv::ast_cast<ast::EvaluateStmt>(root->statements[3]);
  CHECK(evaluate && evaluate->branches.size() == 2);
  const auto* array = cursiv::ast_cast<ast::VarDeclStmt>(root->statements[4]);
  const auto* nestedArray = cursiv::ast_cast<ast::VarDeclStmt>(root->statements[5]);
  CHECK(array && array->varType == "array of string" && nestedArray &&
        nestedArray->varType == "array of array of string");
  for (const auto* badSource : {"&x = 'line\nnext';", "&x = \"line\nnext\";"}) {
    const auto bad = parser.parse(badSource);
    CHECK(!bad.accepted || bad.cst().recovered() || !bad.diagnostics.empty());
  }
}
} // namespace peoplecode_followup
