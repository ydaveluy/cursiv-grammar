#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cql3/core/ast.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace cql3_json {
namespace ast = cql3::ast;
inline bool fail(std::string_view reason, std::string_view source) {
  std::fprintf(stderr, "CQL JSON INSERT: %.*s\n%.*s\n",
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
  const std::string source = R"CQL(INSERT INTO ks.t JSON '{"k":1}';
INSERT INTO t JSON ? DEFAULT NULL IF NOT EXISTS USING TTL 90 AND TIMESTAMP :ts;
INSERT INTO t JSON :payload DEFAULT UNSET;
INSERT INTO t (k,v) VALUES (1,'x') IF NOT EXISTS USING TIMESTAMP 4;
)CQL";
  const auto parsed = language.parser->parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(language, parsed)) return fail("native syntax or complete ownership", source);
  const auto* root = cursiv::ast_cast<ast::Root>(parsed.value);
  if (!root || root->statements.size() != 4) return fail("four INSERTs", source);
  const auto* first = cursiv::ast_cast<ast::Insert>(root->statements[0]);
  const auto* second = cursiv::ast_cast<ast::Insert>(root->statements[1]);
  const auto* third = cursiv::ast_cast<ast::Insert>(root->statements[2]);
  const auto* ordinary = cursiv::ast_cast<ast::Insert>(root->statements[3]);
  const auto* json = first ? cursiv::ast_cast<ast::StringLit>(first->jsonValue) : nullptr;
  const auto* positional = second ? cursiv::ast_cast<ast::BindMarker>(second->jsonValue) : nullptr;
  const auto* named = third ? cursiv::ast_cast<ast::BindMarker>(third->jsonValue) : nullptr;
  const auto* timestamp = second && second->using_
      ? cursiv::ast_cast<ast::BindMarker>(second->using_->timestamp) : nullptr;
  const auto* ttl = second && second->using_
      ? cursiv::ast_cast<ast::IntLit>(second->using_->ttl) : nullptr;
  if (!first || first->keyspace != "ks" || first->table != "t" || !json ||
      json->value != "{\"k\":1}" || first->jsonDefault != ast::JsonDefault::none_ ||
      !first->columns.empty() || !first->values.empty() || !second || !positional ||
      !positional->name.empty() || second->jsonDefault != ast::JsonDefault::null_ ||
      !second->ifNotExists || !timestamp || timestamp->name != "ts" ||
      !ttl || ttl->value != "90" || !third || !named || named->name != "payload" ||
      third->jsonDefault != ast::JsonDefault::unset_ || !ordinary || ordinary->jsonValue ||
      ordinary->jsonDefault != ast::JsonDefault::none_ || ordinary->columns.size() != 2 ||
      ordinary->values.size() != 2 || !ordinary->ifNotExists || !ordinary->using_)
    return fail("JSON payload/default states or shared INSERT tails lost", source);

  for (const char* text : {
           "insert into t json :p default null;", "INSERT INTO t JSON ? DEFAULT UNSET;"}) {
    const auto result = language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !parents(language, result)) return fail("case-insensitive native default/marker", text);
  }
  for (const char* text : {
           "INSERT INTO t VALUES (1);", "INSERT INTO t (k) JSON '{}';",
           "INSERT INTO t JSON 1;", "INSERT INTO t JSON NULL;", "INSERT INTO t JSON TRUE;",
           "INSERT INTO t JSON 0x01;", "INSERT INTO t JSON uuid();",
           "INSERT INTO t JSON '{}' DEFAULT;", "INSERT INTO t JSON '{}' DEFAULT true;",
           "INSERT INTO t JSON '{}' DEFAULT NULL DEFAULT UNSET;",
           "INSERT INTO t JSON '{}' IF NOT EXISTS DEFAULT NULL;",
           "INSERT INTO t (k) VALUES (1) DEFAULT NULL;"}) {
    const auto result = language.parser->parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("native-disallowed INSERT/JSON form accepted", text);
  }
  return true;
}
}  // namespace cql3_json
