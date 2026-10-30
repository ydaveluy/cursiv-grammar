#pragma once
#include <string_view>
#include <unordered_set>
#include <variant>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "trac/core/ast.hpp"
namespace trac_native_followup {
template<class Language> bool run(const Language& language) {
  namespace a = trac::ast;
  const auto r = language.language.parser->parse("#(ps,(#(rs)),#(rs),two words)");
  const auto* root = r.template value_as<a::Program>();
  if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !root || root->macros.size() != 1) return false;
  const auto* active = cursiv::ast_cast<a::Active>(root->macros[0]);
  if (!active || active->name != "ps" || active->args.size() != 3 || !active->args[0]->quoted || active->args[1]->quoted || active->args[2]->quoted) return false;
  for (int i = 0; i < 2; ++i) {
    const auto* p = std::get_if<a::Macro*>(&active->args[i]->value);
    const auto* nested = p ? cursiv::ast_cast<a::Active>(*p) : nullptr;
    if (!nested || nested->name != "rs" || nested->parent != active->args[i]) return false;
  }
  const auto* text = std::get_if<std::string_view>(&active->args[2]->value);
  if (!text || *text != "two words") return false;
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && r.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  walk(walk, *root);
  return owned && seen.size() == r.nodes.size();
}
} // namespace trac_native_followup
