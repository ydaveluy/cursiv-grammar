#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "modelica/core/ast.hpp"

namespace modelica_inheritance {
inline bool run(const auto& language) {
  namespace a = modelica::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "modelica inheritance: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"MO(model M
extends A(break x, p=break, break connect(a[1].b,c.d[2]), q(start=1), r=2);
Real y=break;
end M;)MO";
  const auto result = language.parser->parse(source);
  const bool clean = result.accepted && !result.cst().recovered() &&
                     result.diagnostics.empty() && result.value;
  check(clean, "inheritance and modification breaks parse cleanly");
  if (!clean) {
    std::fprintf(stderr, "modelica inheritance: accepted=%u recovered=%u\n",
        unsigned(result.accepted), unsigned(result.cst().recovered()));
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "modelica inheritance: kind=%u bytes=%u..%u element=%u\n",
          unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    return false;
  }
  bool ordered = false, named = false, connected = false;
  unsigned breaks = 0;
  for (const auto* node : result.nodes) {
    if (const auto* ext = cursiv::ast_cast<a::ExtendsClause>(node)) {
      if (ext->mod && ext->mod->args.size() == 5) {
        const auto* first = cursiv::ast_cast<a::InheritanceBreak>(ext->mod->args[0]);
        const auto* second = cursiv::ast_cast<a::ElementMod>(ext->mod->args[1]);
        const auto* third = cursiv::ast_cast<a::InheritanceBreak>(ext->mod->args[2]);
        ordered = first && second && third && second->mod &&
                  cursiv::ast_cast<a::BreakExpr>(second->mod->value) &&
                  cursiv::ast_cast<a::ElementMod>(ext->mod->args[3]) &&
                  cursiv::ast_cast<a::ElementMod>(ext->mod->args[4]);
      }
    }
    if (const auto* b = cursiv::ast_cast<a::InheritanceBreak>(node)) {
      named |= b->name == "x" && !b->connection;
      const auto* c = b->connection;
      connected |= b->name.empty() && c && c->parent == b && c->a && c->b &&
          c->a->parts.size() == 2 && c->a->parts[0]->name == "a" &&
          c->a->parts[0]->subscripts.size() == 1 && c->a->parts[1]->name == "b" &&
          c->b->parts.size() == 2 && c->b->parts[0]->name == "c" &&
          c->b->parts[1]->name == "d" && c->b->parts[1]->subscripts.size() == 1 &&
          c->docStrings.empty() && !c->annotation;
    }
    if (cursiv::ast_cast<a::BreakExpr>(node)) ++breaks;
  }
  check(ordered && named && connected && breaks == 2,
        "ordered arguments, broken name/connection and break values retain typed captures");
  auto owned = [&](const cursiv::ParseResult& parsed) {
    if (!parsed.value) return false;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool valid = true;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node && parsed.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *parsed.value);
    return valid && seen.size() == parsed.nodes.size();
  };
  check(owned(result), "all inheritance captures have unique parents and CST ownership");
  for (const auto source : {
          "model M extends A(); end M;",
          "model M extends A(break 'x y',break connect(a,b)); end M;",
          "model M Real x(start=break); end M;",
          "model M extends A(p()=break); end M;",
          "type T=Real(start=break);",
          "model M algorithm for i in 1:2 loop break; end for; end M;"}) {
    const auto r = language.parser->parse(source);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && owned(r);
    if (!clean) std::fprintf(stderr, "modelica inheritance: positive failed: %s\n", source);
    check(clean, "native scoped break or prior context rejected");
  }
  for (const auto source : {
          "model M extends A(break); end M;",
          "model M extends A(break x.y); end M;",
          "model M extends A(break connect(a)); end M;",
          "model M extends A(break connect(a,b) \"doc\"); end M;",
          "model M extends A(break connect(a,b) annotation(x=1)); end M;",
          "model M Real x(break y); end M;",
          "model M annotation(break x); end M;",
          "model extends M(break x) end M;",
          "model M equation x=break; end M;",
          "model M algorithm x:=break; end M;",
          "model M Real x=break+1; end M;",
          "model M extends A(p=f(break)); end M;"}) {
    const auto r = language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "modelica inheritance: unexpectedly clean: %s\n", source);
    check(rejected, "break must stay in its native context and shape");
  }
  return ok;
}
} // namespace modelica_inheritance
