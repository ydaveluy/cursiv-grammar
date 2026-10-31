#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cql3/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace cql3_terms_selectors {
inline bool run(const auto& language) {
  namespace a = cql3::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "cql terms/selectors: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"CQL(INSERT INTO t (a,b,c,d,e,f) VALUES
(1+2*3, (int)?, CAST((1+2) AS int), -(1+2), [1+2,ks.f((int)3)], {f:2*3});
SELECT a+2*b AS total, CAST(a+2 AS double) AS casted, (frozen<list<int>>) values,
doc.addresses[?].city, xs[1..5], xs[..5], xs[1..], ks.fn(a+1), COUNT(*),
TTL(doc.field), [a,b*2], {a:b}, (a,b), - -a FROM t;
SELECT k FROM t WHERE (a,b)>? AND (a,b) IN :tuples AND (a,b) IN (?,:second) AND (a,b) IN ();
)CQL";
  const auto result = language.parser->parse(source);
  const auto* root = cursiv::ast_cast<a::Root>(result.value);
  const bool clean = result.accepted && !result.cst().recovered() &&
                     result.diagnostics.empty() && root && root->statements.size() == 3;
  check(clean, "native arithmetic, casts, selection chains and tuple markers parse cleanly");
  if (!clean) {
    std::fprintf(stderr, "cql terms/selectors: accepted=%u recovered=%u statements=%zu\n",
        unsigned(result.accepted), unsigned(result.cst().recovered()), root ? root->statements.size() : 0);
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "cql terms/selectors: kind=%u bytes=%u..%u element=%u\n",
          unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    return false;
  }
  const auto* insert = cursiv::ast_cast<a::Insert>(root->statements[0]);
  check(insert && insert->values.size() == 6, "INSERT retains six term expressions");
  if (insert && insert->values.size() == 6) {
    const auto* add = cursiv::ast_cast<a::BinaryExpr>(insert->values[0]);
    const auto* mul = add ? cursiv::ast_cast<a::BinaryExpr>(add->right) : nullptr;
    const auto* hint = cursiv::ast_cast<a::CastExpr>(insert->values[1]);
    const auto* cast = cursiv::ast_cast<a::CastExpr>(insert->values[2]);
    const auto* nested = cast ? cursiv::ast_cast<a::TupleLit>(cast->value) : nullptr;
    const auto* unary = cursiv::ast_cast<a::UnaryExpr>(insert->values[3]);
    const auto* list = cursiv::ast_cast<a::ListLit>(insert->values[4]);
    const auto* udt = cursiv::ast_cast<a::UdtLit>(insert->values[5]);
    check(add && add->op == "+" && mul && mul->op == "*" && hint &&
          !hint->functionStyle && hint->type && hint->type->name == "int" &&
          cursiv::ast_cast<a::BindMarker>(hint->value) && cast && cast->functionStyle &&
          cast->type && cast->type->name == "int" && nested && nested->items.size() == 1 &&
          cursiv::ast_cast<a::BinaryExpr>(nested->items[0]) && unary && unary->op == "-" &&
          list && list->items.size() == 2 && cursiv::ast_cast<a::FuncCall>(list->items[1]) &&
          udt && udt->fields.size() == 1 && cursiv::ast_cast<a::BinaryExpr>(udt->fields[0]->value),
          "term precedence/cast style/types/collections preserve all values");
  }
  const auto* select = cursiv::ast_cast<a::Select>(root->statements[1]);
  check(select && select->elements.size() == 14, "SELECT retains fourteen selectors");
  if (select && select->elements.size() == 14) {
    const auto* sum = cursiv::ast_cast<a::BinaryExpr>(select->elements[0]->expression);
    const auto* cast = cursiv::ast_cast<a::CastExpr>(select->elements[1]->expression);
    const auto* hint = cursiv::ast_cast<a::CastExpr>(select->elements[2]->expression);
    const auto* field = cursiv::ast_cast<a::FieldExpr>(select->elements[3]->expression);
    const auto* element = field ? cursiv::ast_cast<a::ElementExpr>(field->object) : nullptr;
    check(sum && sum->op == "+" && cursiv::ast_cast<a::ColumnRef>(sum->left) &&
          select->elements[0]->alias == "total" && cast && cast->functionStyle &&
          cursiv::ast_cast<a::BinaryExpr>(cast->value) && select->elements[1]->alias == "casted" &&
          hint && !hint->functionStyle && hint->type && hint->type->params.size() == 1 &&
          field && field->field == "city" && element && cursiv::ast_cast<a::BindMarker>(element->index) &&
          cursiv::ast_cast<a::FieldExpr>(element->object),
          "selection arithmetic, full CAST operand, type hint and chained field/index are typed");
    const auto* bounded = cursiv::ast_cast<a::SliceExpr>(select->elements[4]->expression);
    const auto* to = cursiv::ast_cast<a::SliceExpr>(select->elements[5]->expression);
    const auto* from = cursiv::ast_cast<a::SliceExpr>(select->elements[6]->expression);
    const auto* lower = bounded ? cursiv::ast_cast<a::IntLit>(bounded->from) : nullptr;
    const auto* upper = bounded ? cursiv::ast_cast<a::IntLit>(bounded->to) : nullptr;
    const auto* call = select->elements[7]->call;
    const auto* count = select->elements[8]->call;
    const auto* ttl = select->elements[9]->call;
    const auto* list = cursiv::ast_cast<a::ListLit>(select->elements[10]->expression);
    const auto* map = cursiv::ast_cast<a::MapLit>(select->elements[11]->expression);
    const auto* tuple = cursiv::ast_cast<a::TupleLit>(select->elements[12]->expression);
    const auto* unary = cursiv::ast_cast<a::UnaryExpr>(select->elements[13]->expression);
    check(lower && lower->value == "1" && upper && upper->value == "5" &&
          to && !to->from && to->to && from && from->from && !from->to &&
          call && call->keyspace == "ks" && call->name == "fn" && call->args.size() == 1 &&
          cursiv::ast_cast<a::BinaryExpr>(call->args[0]) && count && count->star &&
          count->name == "count" && ttl && ttl->name == "ttl" && ttl->args.size() == 1 &&
          cursiv::ast_cast<a::FieldExpr>(ttl->args[0]) && list && list->items.size() == 2 &&
          map && map->entries.size() == 1 && tuple && tuple->items.size() == 2 &&
          unary && cursiv::ast_cast<a::UnaryExpr>(unary->operand),
          "slices/builtins/selection collections and recursive selection negation retain values");
    for (const auto* item : select->elements) {
      const unsigned alternatives = unsigned(!item->name.empty()) + unsigned(item->call != nullptr) +
                                    unsigned(item->expression != nullptr);
      check(alternatives == 1, "selector owns exactly one name/call/complex expression representation");
    }
  }
  const auto* markers = cursiv::ast_cast<a::Select>(root->statements[2]);
  check(markers && markers->where.size() == 4, "tuple relation modes retain four clauses");
  if (markers && markers->where.size() == 4) {
    const auto* cmp = cursiv::ast_cast<a::TupleCompareRelation>(markers->where[0]);
    const auto* whole = cursiv::ast_cast<a::TupleInRelation>(markers->where[1]);
    const auto* list = cursiv::ast_cast<a::TupleInRelation>(markers->where[2]);
    const auto* empty = cursiv::ast_cast<a::TupleInRelation>(markers->where[3]);
    check(cmp && cmp->marker && cmp->values.empty() && cmp->marker->name.empty() &&
          whole && whole->marker && whole->marker->name == "tuples" &&
          list && !list->marker && list->values.empty() && list->markers.size() == 2 &&
          list->markers[0]->name.empty() && list->markers[1]->name == "second" &&
          empty && !empty->marker && empty->values.empty() && empty->markers.empty(),
          "whole tuple markers, marker lists and empty IN remain distinct");
  }
  auto owned = [&](const cursiv::ParseResult& parsed) {
    if (!parsed.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
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
  check(owned(result), "all expression/call/cast/tuple-marker nodes have unique parents and CST ownership");
  for (const auto source : {
      "SELECT a - 1, 1- 2, a- -1 FROM t;", "SELECT xs[1...5] FROM t;",
      "SELECT WRITETIME(doc.f),MAXWRITETIME(xs[1]),COUNT(a),cast FROM t;",
      "SELECT cast.fn(a),\"cast\"(a),\"Case\"(a) FROM t;",
      "INSERT INTO t(k) VALUES ((frozen<list<int>>)[1+2]);",
      "SELECT * FROM t WHERE token(a,b)>token(?,?) AND (a,b)> (1+2,3);",
      "UPDATE t SET a=(int)?+a,b=[1+2]+b WHERE k=1;",
      "UPDATE t SET a=a-3 WHERE k=1;", "SELECT ks.fn(a+1) AS f FROM t;",
      "SELECT k FROM t PER PARTITION LIMIT ? LIMIT :rows;",
      "INSERT INTO t(k) VALUES (1+2) USING TTL ? AND TIMESTAMP :ts;"}) {
    const auto r = language.parser->parse(source);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && owned(r);
    if (!clean) std::fprintf(stderr, "cql terms/selectors: positive failed: %s\n", source);
    check(clean, "native term/selection/legacy control rejected");
  }
  for (const auto source : {
      "INSERT INTO t(k) VALUES (a);", "INSERT INTO t(k) VALUES (f(a));",
      "INSERT INTO t(k) VALUES (CAST(1+2 AS int));", "INSERT INTO t(k) VALUES (CAST(? AS list<int>));",
      "INSERT INTO t(k) VALUES (- -?);", "INSERT INTO t(k) VALUES (+1);",
      "INSERT INTO t(k) VALUES (+NaN);", "INSERT INTO t(k) VALUES (+Infinity);",
      "SELECT xs[..] FROM t;", "SELECT xs[1..5,6] FROM t;", "SELECT a..b FROM t;",
      "SELECT f(*) FROM t;", "SELECT ks.count(*) FROM t;", "SELECT ttl(1+2) FROM t;",
      "SELECT ttl(a,b) FROM t;", "SELECT CAST(a AS \"int\") FROM t;",
      "SELECT * FROM t WHERE f(a)>1;", "SELECT * FROM t WHERE token(1)>0;",
      "SELECT * FROM t WHERE (a,b) IN ((1,2),?);",
      "SELECT * FROM t WHERE (a,b)>(1,2),(3,4);", "SELECT 1-2 FROM t;",
      "UPDATE t SET a=[1]-a WHERE k=1;",
      "INSERT INTO t(k) VALUES (1-2);", "SELECT a-1 FROM t;",
      "SELECT a-NaN FROM t;", "SELECT a-1h FROM t;", "SELECT a+ +1 FROM t;",
      "SELECT a - 1, 1- 2, a--1 FROM t;",
      "SELECT k FROM t LIMIT 1+2;", "SELECT k FROM t PER PARTITION LIMIT 1.5;",
      "INSERT INTO t(k) VALUES (1) USING TTL 1.5;",
      "DELETE FROM t USING TIMESTAMP CAST(1 AS bigint) WHERE k=1;"}) {
    const auto r = language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "cql terms/selectors: unexpectedly clean: %s\n", source);
    check(rejected, "native term/selection/token boundary must not be bypassed");
  }
  return ok;
}
} // namespace cql3_terms_selectors
