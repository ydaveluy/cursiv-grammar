#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "cypher/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace cypher_legacy {
inline bool run(const auto& language) {
  namespace a = cypher::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "cypher legacy: %s\n", message); ok = false; }
  };
  auto clean = [&](const cursiv::ParseResult& parsed, std::string_view source) {
    const bool valid = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    if (!valid) {
      std::fprintf(stderr, "cypher legacy: accepted=%u recovered=%u source=%.*s\n",
          unsigned(parsed.accepted), unsigned(parsed.cst().recovered()), int(source.size()), source.data());
      for (const auto& d : parsed.diagnostics)
        std::fprintf(stderr, "cypher legacy: kind=%u bytes=%u..%u element=%u\n",
            unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    }
    return valid;
  };
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
  constexpr std::string_view source = R"C(LOAD CSV WITH HEADERS FROM 'file:///people.csv' AS row FIELDTERMINATOR ';'
MATCH p = shortestPath((a:Person)-[:KNOWS*1..3]->(b:Person))
USING INDEX a:Person(name) USING JOIN ON a,b USING SCAN b:Person
WHERE a.name=row.name
FOREACH (x IN [1,2] | CREATE (n:Value {value:x}) SET n.seen=true
  FOREACH (y IN [x] | SET n.y=y))
RETURN p, allShortestPaths((a)-->(b)))C";
  const auto parsed = language.parser->parse(source);
  if (!clean(parsed, source)) return false;
  const auto* script = cursiv::ast_cast<a::Script>(parsed.value);
  const auto* regular = script ? std::get_if<a::RegularQuery*>(&script->query) : nullptr;
  const auto* query = regular && *regular ? (*regular)->first : nullptr;
  const auto* part = query && query->parts.size() == 1 ? query->parts[0] : nullptr;
  check(script && script->query.index() == 0 && part && part->statements.size() == 3,
        "regular query index and source clause order stay intact");
  if (part && part->statements.size() == 3) {
    const auto* loadValue = std::get_if<a::LoadCsvClause*>(&part->statements[0]);
    const auto* matchValue = std::get_if<a::MatchClause*>(&part->statements[1]);
    const auto* foreachValue = std::get_if<a::ForeachClause*>(&part->statements[2]);
    const auto* load = loadValue ? *loadValue : nullptr;
    const auto* url = load ? cursiv::ast_cast<a::StringLit>(load->source) : nullptr;
    check(part->statements[0].index() == 8 && part->statements[1].index() == 0 &&
          part->statements[2].index() == 9 && load && load->headers && load->variable == "row" &&
          url && url->value == "file:///people.csv" && load->fieldTerminator &&
          load->fieldTerminator->value == ";", "CSV settings/expressions and old variant indices are preserved");
    const auto* match = matchValue ? *matchValue : nullptr;
    const auto* path = match && match->pattern && match->pattern->parts.size() == 1
        ? cursiv::ast_cast<a::ShortestPattern>(match->pattern->parts[0]->elem) : nullptr;
    const auto* inner = path ? cursiv::ast_cast<a::SimplePatternElem>(path->inner) : nullptr;
    check(match && match->pattern && !match->pattern->parts.empty() &&
          match->pattern->parts[0]->variable == "p" && match->where_ &&
          path && !path->all && inner && inner->chain.size() == 1,
          "bound shortest path keeps its pattern and following WHERE");
    if (inner && inner->chain.size() == 1) {
      const auto* step = std::get_if<a::RelChainStep*>(&inner->chain[0]);
      const auto* rel = step && *step ? (*step)->rel : nullptr;
      const auto* range = rel && rel->detail ? rel->detail->range : nullptr;
      check(rel && rel->pointsRight && !rel->pointsLeft && range && range->hasRange &&
            range->min == "1" && range->max == "3", "shortest path retains direction and length bounds");
    }
    if (match && match->hints.size() == 3) {
      const auto* index = cursiv::ast_cast<a::IndexHint>(match->hints[0]);
      const auto* join = cursiv::ast_cast<a::JoinHint>(match->hints[1]);
      const auto* scan = cursiv::ast_cast<a::ScanHint>(match->hints[2]);
      check(index && index->variable == "a" && index->label == "Person" && index->property == "name" &&
            join && join->variables.size() == 2 && join->variables[0] == "a" && join->variables[1] == "b" &&
            scan && scan->variable == "b" && scan->label == "Person", "all three hint modes retain source order and names");
    } else check(false, "three MATCH hints are present");
    const auto* foreach = foreachValue ? *foreachValue : nullptr;
    check(foreach && foreach->variable == "x" && cursiv::ast_cast<a::ListLit>(foreach->source) &&
          foreach->updates.size() == 3, "FOREACH keeps binding/source and three ordered updates");
    if (foreach && foreach->updates.size() == 3) {
      const auto* create = std::get_if<a::CreateClause*>(&foreach->updates[0]);
      const auto* set = std::get_if<a::SetClause*>(&foreach->updates[1]);
      const auto* nested = std::get_if<a::ForeachClause*>(&foreach->updates[2]);
      check(create && *create && set && *set && nested && *nested && (*nested)->variable == "y" &&
            (*nested)->updates.size() == 1 && std::get_if<a::SetClause*>(&(*nested)->updates[0]),
            "nested FOREACH remains an update with its own AST binding and child");
    }
    const auto* ret = part->ret && part->ret->body ? part->ret->body : nullptr;
    const auto* allPaths = ret && ret->items.size() == 2
        ? cursiv::ast_cast<a::ShortestPathExpr>(ret->items[1]->value) : nullptr;
    check(allPaths && allPaths->all && allPaths->pattern, "all-shortest expression is distinct from one-shortest pattern");
  }
  check(owned(parsed), "new clauses/hints/paths have complete unique parent and CST ownership");

  constexpr std::string_view bulkSource =
      "USING PERIODIC COMMIT 1000 LOAD CSV FROM 'file:///x.csv' AS row CREATE (:Person {name:row[0]})";
  const auto bulkParsed = language.parser->parse(bulkSource);
  if (!clean(bulkParsed, bulkSource)) return false;
  const auto* bulkScript = cursiv::ast_cast<a::Script>(bulkParsed.value);
  const auto* bulkValue = bulkScript ? std::get_if<a::BulkImportQuery*>(&bulkScript->query) : nullptr;
  const auto* bulk = bulkValue ? *bulkValue : nullptr;
  check(bulkScript && bulkScript->query.index() == 2 && bulk && bulk->batchSize &&
        *bulk->batchSize == "1000" && bulk->load && !bulk->load->headers &&
        !bulk->load->fieldTerminator && bulk->load->variable == "row" && bulk->query &&
        bulk->query->parts.size() == 1 && bulk->query->parts[0]->statements.size() == 1 &&
        owned(bulkParsed), "periodic import owns a distinct CSV clause and continuation query");

  constexpr std::string_view nameSource = "WITH 'name' AS shortestPath RETURN shortestPath";
  const auto named = language.parser->parse(nameSource);
  check(clean(named, nameSource) && owned(named), "path function names retain ordinary contextual identifier use");
  for (const auto source : {
      "LOAD CSV FROM $url AS row RETURN row", "FOREACH(x IN [1] | CREATE (n))",
      "MATCH p=allShortestPaths((a)-->(b)) RETURN p",
      "RETURN shortestPath((a)-->(b))", "MATCH shortestPath((a)-->(b)) RETURN a",
      "USING PERIODIC COMMIT LOAD CSV FROM $url AS row RETURN row",
      "CREATE (a) WITH a MATCH (a) RETURN a", "CALL db.labels",
      "MATCH (n) USING SCAN n:Label RETURN n"}) {
    const auto r = language.parser->parse(source);
    check(clean(r, source) && owned(r), "native legacy or existing query control rejected");
    if (std::string_view(source).starts_with("USING PERIODIC COMMIT LOAD")) {
      bool absent = false;
      for (const auto* node : r.nodes)
        if (const auto* bulk = cursiv::ast_cast<a::BulkImportQuery>(node))
          absent |= !bulk->batchSize && bulk->load && bulk->query;
      check(absent, "absent periodic batch size is distinct from an explicit size");
    }
  }
  for (const auto source : {
      "LOAD CSV HEADERS FROM $url AS row RETURN row", "LOAD CSV FROM $url RETURN row",
      "LOAD CSV FROM $url AS row FIELDTERMINATOR 1 RETURN row", "LOAD CSV FROM $url AS row",
      "FOREACH(x IN [1] |)", "FOREACH(x IN [1] | MATCH (n) RETURN n)",
      "FOREACH(x IN [1] | CALL db.p())", "FOREACH(x IN [1] | WITH x CREATE(n))",
      "MATCH (a) USING INDEX a:Label() RETURN a", "MATCH(a) USING INDEX a:Label(x,y) RETURN a",
      "MATCH(a) USING JOIN ON RETURN a", "MATCH(a) USING SCAN a RETURN a",
      "MATCH p=shortestPath(1) RETURN p", "MATCH p=allShortestPaths() RETURN p",
      "USING PERIODIC COMMIT 1.5 LOAD CSV FROM $url AS row RETURN row",
      "USING PERIODIC COMMIT RETURN 1", "USING PERIODIC COMMIT LOAD CSV FROM $url AS row",
      "CREATE(a) MATCH(a) WITH a RETURN a",
      "RETURN 1 UNION USING PERIODIC COMMIT LOAD CSV FROM $url AS row RETURN row"}) {
    const auto r = language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "cypher legacy: unexpectedly clean: %s\n", source);
    check(rejected, "native legacy clause context or shape must not be bypassed");
  }
  return ok;
}
} // namespace cypher_legacy
