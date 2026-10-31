#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "bicep/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace bicep_postfix {
namespace ast = bicep::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "bicep postfix: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& language) {
  const std::string source =
      "var safe = a.?b\nvar index = a[?1]\nvar last = a[^2]\n"
      "var both = a[?^3]\nvar resource = parent::child\nvar asserted = a!\n"
      "var chain = a!.?b[?^1]!.c\nvar cond = flag ? a : b\n"
      "var ordinary = a.b[0].f()\nvar spaced = a. /*marker*/ ?b\n";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) return fail("native member forms/ownership", source);
  const auto* root = cursiv::ast_cast<ast::Program>(parsed.value);
  if (!root || root->body.size() != 10) return fail("ten variables", source);
  const auto value = [&](std::size_t index) -> const ast::Expr* {
    const auto* variable = cursiv::ast_cast<ast::VariableDecl>(root->body[index]);
    return variable ? variable->value : nullptr;
  };
  const auto* safe = cursiv::ast_cast<ast::PropertyExpr>(value(0));
  const auto* index = cursiv::ast_cast<ast::IndexExpr>(value(1));
  const auto* last = cursiv::ast_cast<ast::IndexExpr>(value(2));
  const auto* both = cursiv::ast_cast<ast::IndexExpr>(value(3));
  const auto* resource = cursiv::ast_cast<ast::NamedAccessExpr>(value(4));
  const auto* asserted = cursiv::ast_cast<ast::NonNullExpr>(value(5));
  const auto* chain = cursiv::ast_cast<ast::PropertyExpr>(value(6));
  const auto* nonNull = chain ? cursiv::ast_cast<ast::NonNullExpr>(chain->object) : nullptr;
  const auto* innerIndex = nonNull ? cursiv::ast_cast<ast::IndexExpr>(nonNull->object) : nullptr;
  const auto* innerSafe = innerIndex ? cursiv::ast_cast<ast::PropertyExpr>(innerIndex->object) : nullptr;
  const auto* firstAssertion = innerSafe ? cursiv::ast_cast<ast::NonNullExpr>(innerSafe->object) : nullptr;
  const auto* cond = cursiv::ast_cast<ast::CondExpr>(value(7));
  const auto* ordinary = cursiv::ast_cast<ast::MethodCallExpr>(value(8));
  const auto* ordinaryIndex = ordinary ? cursiv::ast_cast<ast::IndexExpr>(ordinary->object) : nullptr;
  const auto* ordinaryProperty = ordinaryIndex ? cursiv::ast_cast<ast::PropertyExpr>(ordinaryIndex->object) : nullptr;
  const auto* spaced = cursiv::ast_cast<ast::PropertyExpr>(value(9));
  if (!safe || !safe->safe || safe->property != "b" ||
      !index || !index->safe || index->fromEnd ||
      !last || last->safe || !last->fromEnd ||
      !both || !both->safe || !both->fromEnd ||
      !resource || resource->name != "child" || !asserted ||
      !chain || chain->safe || chain->property != "c" ||
      !innerIndex || !innerIndex->safe || !innerIndex->fromEnd ||
      !innerSafe || !innerSafe->safe || innerSafe->property != "b" || !firstAssertion ||
      !cond || !cursiv::ast_cast<ast::Ident>(cond->thenValue) ||
      !cursiv::ast_cast<ast::Ident>(cond->elseValue) ||
      !ordinaryProperty || ordinaryProperty->safe || ordinaryIndex->safe ||
      ordinaryIndex->fromEnd || !ordinary->call || ordinary->call->name != "f" ||
      !spaced || !spaced->safe)
    return fail("markers, chain order, resource/ternary boundary or ordinary members", source);
  for (const char* text : {"var x = a:b", "var x = a.?f()", "var x = a[?]",
                           "var x = a[^]", "var x = a[^?1]", "var x = a.?",
                           "var x = a::", "var x = a?[1]"}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid native postfix accepted clean", text);
  }
  return true;
}
}  // namespace bicep_postfix
