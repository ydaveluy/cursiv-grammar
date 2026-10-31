#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "atl/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace atl_drop_reverse {
namespace ast = atl::ast;
inline bool fail(std::string_view reason, std::string_view source) {
  std::fprintf(stderr, "ATL drop/reverse: %.*s\n%.*s\n",
               static_cast<int>(reason.size()), reason.data(),
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
  const std::string source = R"ATL(module M;
create OUT:M refining IN:M;
rule Remove { from s:IN!A to drop }
rule Replace {
  from s:IN!A
  to drop
    t:OUT!B in OUT mapsTo s -> (and <- s.a, drop <:= s.b) (normal <- s.c),
    u:OUT!B -> () (),
    v:distinct OUT!B foreach (i in Sequence{1,2}) mapsTo s (normal <- i)
}
rule Empty { from s:IN!A to }
rule Absent { from s:IN!A }
rule Called() { to drop }
)ATL";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) return fail("native TCS syntax/ownership", source);
  const auto* module = cursiv::ast_cast<ast::Module>(parsed.value);
  if (!module || module->elements.size() != 5) return fail("five rules", source);
  const auto* remove = cursiv::ast_cast<ast::MatchedRule>(module->elements[0]);
  const auto* replace = cursiv::ast_cast<ast::MatchedRule>(module->elements[1]);
  const auto* empty = cursiv::ast_cast<ast::MatchedRule>(module->elements[2]);
  const auto* absent = cursiv::ast_cast<ast::MatchedRule>(module->elements[3]);
  const auto* called = cursiv::ast_cast<ast::CalledRule>(module->elements[4]);
  if (!remove || !remove->outPattern || !remove->outPattern->hasDrop ||
      !remove->outPattern->elements.empty() || !replace || !replace->outPattern ||
      !replace->outPattern->hasDrop || replace->outPattern->elements.size() != 3 ||
      !empty || !empty->outPattern || empty->outPattern->hasDrop ||
      !empty->outPattern->elements.empty() || !absent || absent->outPattern ||
      !called || !called->outPattern || !called->outPattern->hasDrop)
    return fail("drop/empty/absent patterns retain distinct AST states", source);
  const auto* t = replace->outPattern->elements[0];
  const auto* u = replace->outPattern->elements[1];
  const auto* v = replace->outPattern->elements[2];
  if (t->name != "t" || t->isForEach || t->inModel != "OUT" || t->mapsTo != "s" ||
      !t->hasReverseBindings || t->reverseBindings.size() != 2 || t->bindings.size() != 1 ||
      u->name != "u" || !u->hasReverseBindings || !u->reverseBindings.empty() ||
      !u->bindings.empty() || v->name != "v" || !v->isForEach ||
      v->hasReverseBindings || !v->reverseBindings.empty() || v->mapsTo != "s" ||
      v->iterVar != "i" || v->bindings.size() != 1)
    return fail("reverse presence/order, foreach restriction or shared mapsTo", source);
  for (unsigned i = 0; i < 2; ++i) {
    const auto* binding = t->reverseBindings[i];
    const auto* member = cursiv::ast_cast<ast::MemberAccess>(binding->value);
    const auto* object = member ? cursiv::ast_cast<ast::NameRef>(member->object) : nullptr;
    if (binding->name != (i ? "drop" : "and") || binding->isAssignment != (i == 1) ||
        !member || member->name != (i ? "b" : "a") || !object || object->name != "s")
      return fail("reverse Binding reuses operators/keyword property names and values", source);
  }
  if (t->bindings[0]->name != "normal" || t->bindings[0]->isAssignment ||
      v->bindings[0]->name != "normal") return fail("ordinary bindings retained", source);

  const std::string prefix = "module M; create OUT:M refining IN:M; rule R { from s:IN!A to ";
  for (const char* tail : {
           "drop, t:OUT!B", "drop drop", "t:OUT!B -> x",
           "t:OUT!B -> (a s.a)", "t:OUT!B -> (a <- s.a,)",
           "t:OUT!B (a <- s.a) -> (b <- s.b)",
           "t:distinct OUT!B foreach(i in Sequence{1}) -> (a <- i)"}) {
    const std::string text = prefix + tail + " }";
    const auto invalid = language.parser->parse(text);
    if (invalid.accepted && !invalid.cst().recovered() && invalid.diagnostics.empty())
      return fail("malformed/native-disallowed reverse/drop placement accepted", text);
  }
  return true;
}
}  // namespace atl_drop_reverse
