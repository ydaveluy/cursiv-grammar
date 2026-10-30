#pragma once
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "sqlite/core/CoreServices.hpp"
#include "sqlite/core/ast.hpp"
namespace native_followup {
inline bool run() {
  const auto language = cursiv::create_language<sqlite::SqliteCoreServices,
      sqlite::SqliteSharedCoreServices, sqlite::install_core_module>();
  const auto& parser = *language.language.parser;
  for (const auto source : {
      "SELECT 1abc;", "SELECT 1e;", "SELECT 0x_FF;", "SELECT 1_;",
      "SELECT 1__2;", "SELECT 1é;", "SELECT 1$;", "SELECT 1e2x;",
      "CREATE TABLE select(x);", "CREATE TABLE returning(x);", "SELECT select;",
      "SELECT 1 SELECT 2;", "CREATE TABLE x(a) CREATE TABLE y(b);"}) {
    const auto r = parser.parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) {
      std::fprintf(stderr, "sqlite native: invalid source accepted: %s\n", source);
      return false;
    }
  }
  const auto r = parser.parse(
      ";;CREATE TABLE window(filter,over,left,indexed,abort);;"
      "CREATE TABLE \"select\"(\"from\"); SELECT 1_2,0xF_F,1.2e+3 alias,1'joined';;");
  if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) {
    std::fprintf(stderr, "sqlite native: contextual names, numeric and statement boundaries\n");
    for (const auto& d : r.diagnostics)
      std::fprintf(stderr,"  kind=%d begin=%zu end=%zu element=%u\n",
          int(d.kind),std::size_t(d.begin),std::size_t(d.end),unsigned(d.element));
    return false;
  }
  const auto* root = r.value_as<sqlite::ast::Script>();
  const auto* sel = root && root->statements.size()==3
      ? cursiv::ast_cast<sqlite::ast::Select>(root->statements[2]->statement) : nullptr;
  const auto* core = sel && !sel->parts.empty() ? sel->parts[0]->core : nullptr;
  if (!core || core->columns.size()!=4 || core->columns[2]->alias!="alias" ||
      core->columns[3]->alias!="joined" ||
      !cursiv::ast_cast<sqlite::ast::NumberLit>(core->columns[0]->value) ||
      cursiv::ast_cast<sqlite::ast::NumberLit>(core->columns[0]->value)->value!="1_2") {
    std::fprintf(stderr,"sqlite native: token spellings and explicit alias boundaries retained\n");
    return false;
  }
  const auto joined = parser.parse("SELECT j.a FROM (t1 JOIN t2 USING(a)) AS j;");
  const auto* joinedRoot = joined.value_as<sqlite::ast::Script>();
  const auto* joinedSelect = joinedRoot && joinedRoot->statements.size()==1
      ? cursiv::ast_cast<sqlite::ast::Select>(joinedRoot->statements[0]->statement) : nullptr;
  const auto* first = joinedSelect && !joinedSelect->parts.empty() && joinedSelect->parts[0]->core->from
      ? joinedSelect->parts[0]->core->from->first : nullptr;
  if (!joined.accepted || joined.cst().recovered() || !joined.diagnostics.empty() ||
      !first || first->alias!="j" || !first->join || first->join->steps.size()!=1) {
    std::fprintf(stderr,"sqlite native: parenthesized join alias and join subtree retained\n");
    return false;
  }
  const auto pragmas = parser.parse("PRAGMA foreign_keys=ON; PRAGMA journal_mode=DELETE; PRAGMA x=DEFAULT;");
  if (!pragmas.accepted || pragmas.cst().recovered() || !pragmas.diagnostics.empty()) {
    std::fprintf(stderr,"sqlite native: PRAGMA nmnum keyword values retained\n");return false;
  }
  const auto attachment = parser.parse("ATTACH ':memory:' AS ('aux'||'1') KEY 'ignored'; DETACH ('aux'||'1');");
  const auto* attachRoot = attachment.value_as<sqlite::ast::Script>();
  const auto* attach = attachRoot && attachRoot->statements.size()==2
      ? cursiv::ast_cast<sqlite::ast::AttachStmt>(attachRoot->statements[0]->statement) : nullptr;
  const auto* detach = attachRoot && attachRoot->statements.size()==2
      ? cursiv::ast_cast<sqlite::ast::DetachStmt>(attachRoot->statements[1]->statement) : nullptr;
  if (!attachment.accepted || attachment.cst().recovered() || !attachment.diagnostics.empty() ||
      !attach || !cursiv::ast_cast<sqlite::ast::BinaryExpr>(attach->schemaExpr) || !attach->key ||
      !detach || !cursiv::ast_cast<sqlite::ast::BinaryExpr>(detach->schemaExpr)) {
    std::fprintf(stderr,"sqlite native: ATTACH/DETACH expressions and KEY payload retained\n");return false;
  }
  return true;
}
}
