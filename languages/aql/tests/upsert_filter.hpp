#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "aql/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace aql_upsert {
namespace ast = aql::ast;
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "aql UPSERT: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const cursiv::AstReflection& reflection,
                    const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool valid = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { valid = false; return; }
    reflection.visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          valid &= child.parent == &node &&
                   parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return valid && seen.size() == parsed.nodes.size();
}
inline bool run(const cursiv::Parser& parser, const cursiv::AstReflection& reflection) {
  const std::string source =
      "UPSERT FILTER CURRENT.name == @name INSERT {name:@name} "
      "UPDATE {hits:OLD.hits+1} IN users OPTIONS {keepNull:false} RETURN NEW";
  const auto parsed = parser.parse(source);
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      !parents(reflection, parsed)) return fail("native FILTER UPSERT or ownership", source);
  const auto* query = cursiv::ast_cast<ast::DataModificationQuery>(parsed.value);
  const auto* upsert = query ? query->upsert_ : nullptr;
  const auto* comparison = upsert ? cursiv::ast_cast<ast::BinaryExpr>(upsert->search) : nullptr;
  const auto* field = comparison ? cursiv::ast_cast<ast::DotExpr>(comparison->left) : nullptr;
  const auto* parameter = comparison ? cursiv::ast_cast<ast::BindParamRef>(comparison->right) : nullptr;
  const auto* insert = upsert ? cursiv::ast_cast<ast::ObjectLit>(upsert->insert_) : nullptr;
  const auto* modify = upsert ? cursiv::ast_cast<ast::ObjectLit>(upsert->modify) : nullptr;
  const auto* sum = modify && modify->pairs.size() == 1
      ? cursiv::ast_cast<ast::BinaryExpr>(modify->pairs[0]->value) : nullptr;
  if (!query || !upsert || !upsert->isFilter || !upsert->isUpdate ||
      !comparison || comparison->op != ast::BinOp::eq || !field || field->field != "name" ||
      !parameter || parameter->name != "@name" || !insert || insert->pairs.size() != 1 ||
      insert->pairs[0]->key != "name" || !sum || sum->op != ast::BinOp::add ||
      !upsert->target || upsert->target->name != "users" || !upsert->options ||
      !upsert->options->value || upsert->options->value->pairs.size() != 1 || !query->return_)
    return fail("search mode/operator, shared tails or AST values lost", source);

  struct Positive {
    const char* source;
    bool filter;
    bool update;
    bool bind;
    bool collection = false;
  };
  for (const auto& test : {
           Positive{"UPSERT {name:@name} INSERT @doc UPDATE @patch IN users", false, true, false},
           Positive{"UPSERT @search INSERT {} REPLACE {} INTO @@collection RETURN OLD", false, false, true},
           Positive{"UPSERT @@search INSERT {} UPDATE {} IN users", false, true, false, true},
           Positive{"UPSERT FILTER CURRENT.age >= 18 INSERT {} REPLACE {} IN users", true, false, false},
           Positive{"upsert filter @predicate insert {} update {} in users", true, true, true}}) {
    const auto result = parser.parse(test.source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !parents(reflection, result)) return fail("native search variant", test.source);
    const auto* q = cursiv::ast_cast<ast::DataModificationQuery>(result.value);
    const auto* op = q ? q->upsert_ : nullptr;
    if (!op || op->isFilter != test.filter || op->isUpdate != test.update ||
        (cursiv::ast_cast<ast::BindParamRef>(op->search) != nullptr) != test.bind ||
        (cursiv::ast_cast<ast::CollectionRef>(op->search) != nullptr) != test.collection ||
        (!test.filter && !test.bind && !test.collection &&
         !cursiv::ast_cast<ast::ObjectLit>(op->search)))
      return fail("search/update mode retained", test.source);
  }
  for (const char* text : {
           "UPSERT 1 INSERT {} UPDATE {} IN users",
           "UPSERT true INSERT {} UPDATE {} IN users",
           "UPSERT null INSERT {} UPDATE {} IN users",
           "UPSERT [] INSERT {} UPDATE {} IN users",
           "UPSERT F() INSERT {} UPDATE {} IN users",
           "UPSERT @search + 1 INSERT {} UPDATE {} IN users",
           "UPSERT FILTER INSERT {} UPDATE {} IN users",
           "UPSERT FILTER CURRENT.x == 1 UPDATE {} IN users"}) {
    const auto result = parser.parse(text);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return fail("invalid native UPSERT accepted clean", text);
  }
  return true;
}
}  // namespace aql_upsert
