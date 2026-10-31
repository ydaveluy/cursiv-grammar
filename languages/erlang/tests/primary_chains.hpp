#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "erlang/core/ast.hpp"

namespace erlang_chains {
namespace ast = erlang::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "erlang primary chains: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const auto& language, const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) {
    std::fputs("erlang ownership: no root or root has a parent\n", stderr);
    return false;
  }
  bool okay = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto describe = [&](const char* role, const cursiv::AstNode& node) {
    const auto name = language.astReflection->getTypeMetaData(node.type).name;
    std::fprintf(stderr, "erlang ownership: %s type=%.*s node=%p parent=%p cst=%u",
        role, int(name.size()), name.data(), static_cast<const void*>(&node),
        static_cast<const void*>(node.parent), unsigned(node.cstNodeId));
    if (node.cstNodeId < parsed.cst().size()) {
      const auto& cst = parsed.cst()[node.cstNodeId];
      const auto text = parsed.cst().text(node.cstNodeId);
      std::fprintf(stderr, " bytes=%u..%u text=%.*s", cst.begin, cst.end,
          int(text.size() > 160 ? 160 : text.size()), text.data());
    }
    std::fputc('\n', stderr);
  };
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) {
      describe("duplicate child", node);
      okay = false;
      return;
    }
    language.astReflection->visitMembers(node,
        [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
          const bool parent = child.parent == &node;
          const bool contains = node.cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() &&
              parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          if (!parent || !contains) {
            std::fprintf(stderr, "erlang ownership: member=%.*s index=%u parent_ok=%u cst_contains=%u\n",
                int(member.size()), member.data(), index, unsigned(parent), unsigned(contains));
            describe("owner", node);
            describe("child", child);
          }
          okay &= parent && contains;
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  for (const auto* node : parsed.nodes)
    if (!seen.contains(node)) describe("unreachable arena node", *node);
  if (!okay || seen.size() != parsed.nodes.size())
    std::fprintf(stderr, "erlang ownership: visited=%zu arena=%zu\n", seen.size(), parsed.nodes.size());
  return okay && seen.size() == parsed.nodes.size();
}
inline bool run(const auto& language) {
  const std::string source =
      "f(M,R,F) -> M#{a=>F}#{a:=F}(1), R#r.f(2), #r{f=F}#r.f(3), "
      "(F())#{a=>1}, (M#{a=>F})#r.f, (#r{f=F})#{a=>1}, F()(1), "
      "M#{a=>F}(), #{}().";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) {
    std::fprintf(stderr, "accepted=%d recovered=%d diagnostics=%zu\n",
                 parsed.accepted, parsed.cst().recovered(), parsed.diagnostics.size());
    for (const auto& d : parsed.diagnostics)
      std::fprintf(stderr, "kind=%u begin=%u end=%u\n", unsigned(d.kind), d.begin, d.end);
    return fail("native primary/call syntax or complete ownership", source);
  }
  const ast::FunctionClause* clause = nullptr;
  for (const auto* node : parsed.nodes)
    if (const auto* c = cursiv::ast_cast<ast::FunctionClause>(node)) clause = c;
  if (!clause || clause->body.size() != 9) return fail("nine expressions", source);
  const auto call = [&](std::size_t i) { return cursiv::ast_cast<ast::Call>(clause->body[i]); };
  const auto* first = call(0);
  const auto* map = first ? cursiv::ast_cast<ast::MapExpr>(first->target) : nullptr;
  const auto* innerMap = map ? cursiv::ast_cast<ast::MapExpr>(map->base) : nullptr;
  const auto* second = call(1);
  const auto* record = second ? cursiv::ast_cast<ast::RecordExpr>(second->target) : nullptr;
  const auto* third = call(2);
  const auto* access = third ? cursiv::ast_cast<ast::RecordExpr>(third->target) : nullptr;
  const auto* created = access ? cursiv::ast_cast<ast::RecordExpr>(access->base) : nullptr;
  const auto* afterCall = cursiv::ast_cast<ast::MapExpr>(clause->body[3]);
  const auto* afterMap = cursiv::ast_cast<ast::RecordExpr>(clause->body[4]);
  const auto* afterRecord = cursiv::ast_cast<ast::MapExpr>(clause->body[5]);
  const auto* repeatedCall = call(6);
  const auto* emptyCall = call(7);
  const auto* emptyMapCall = call(8);
  const auto* emptyMap = emptyMapCall ? cursiv::ast_cast<ast::MapExpr>(emptyMapCall->target) : nullptr;
  if (!first || first->args.size() != 1 || !map || map->fields.size() != 1 ||
      !map->fields[0]->exact || !innerMap || innerMap->fields.size() != 1 ||
      innerMap->fields[0]->exact || !second || !record || record->name != "r" ||
      record->accessField != "f" || !record->base || !third || !access ||
      access->accessField != "f" || !created || created->base || created->name != "r" ||
      created->fields.size() != 1 || created->fields[0]->name != "f" ||
      !afterCall || !cursiv::ast_cast<ast::Call>(afterCall->base) ||
      !afterMap || !cursiv::ast_cast<ast::MapExpr>(afterMap->base) ||
      !afterRecord || !cursiv::ast_cast<ast::RecordExpr>(afterRecord->base) ||
      !repeatedCall || !cursiv::ast_cast<ast::Call>(repeatedCall->target) ||
      !emptyCall || !emptyCall->args.empty() || !cursiv::ast_cast<ast::MapExpr>(emptyCall->target) ||
      !emptyMap || emptyMap->base || !emptyMap->fields.empty() || !emptyMapCall->args.empty())
    return fail("call/update/record access nesting or field modes lost", source);

  for (const char* text : {"f(F)->#{}#{a=>F}(1).", "f(F)->#r{}#r{f=F}(1).",
                           "f()->#{}#{}#{}."}) {
    const auto r = language.parser->parse(text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !parents(language, r))
      return fail("fresh literal/update chain keeps proper source boundaries and ownership", text);
  }

  for (const char* text : {"f(F)->F()#{}.", "f(R)->R#r{}#{}.",
                           "f(M)->M#{}#r{}.", "f()->#r{}#{}.",
                           "f()->#{}#r{}.", "f()->#r.f#{}."}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native expr_max/family boundary bypassed without parentheses", text);
  }
  return true;
}
}  // namespace erlang_chains
