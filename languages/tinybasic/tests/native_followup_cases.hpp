#pragma once
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "tinybasic/core/ast.hpp"
namespace tinybasic_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = tinybasic::ast;
  const auto result = language.language.parser->parse("1 0 A=-2*3+4-5\n20GOTO1 2\n");
  const auto* root = result.template value_as<a::Program>();
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !root || root->lines.size() != 2) return false;
  const auto* let = cursiv::ast_cast<a::LetStmt>(root->lines[0]->statement);
  const auto* sub = let ? cursiv::ast_cast<a::BinaryExpr>(let->value) : nullptr;
  const auto* add = sub ? cursiv::ast_cast<a::BinaryExpr>(sub->left) : nullptr;
  const auto* neg = add ? cursiv::ast_cast<a::Negate>(add->left) : nullptr;
  const auto* mul = neg ? cursiv::ast_cast<a::BinaryExpr>(neg->operand) : nullptr;
  const auto* jump = cursiv::ast_cast<a::GotoStmt>(root->lines[1]->statement);
  if (root->lines[0]->number != 10 || !jump || jump->target != 12 || !sub || sub->op != a::Operator::subtract ||
      !add || add->op != a::Operator::add || !mul || mul->op != a::Operator::multiply) return false;
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  walk(walk, *root);
  if (!owned || seen.size() != result.nodes.size()) return false;
  for (const auto bad : {"10A=2*-3\n", "10A=2+-3\n", "10A=2++3\n", "10GOTO2147483648\n"}) {
    const auto r = language.language.parser->parse(bad);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) return false;
  }
  return true;
}
} // namespace tinybasic_native_followup
