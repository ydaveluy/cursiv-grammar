#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include "cursiv/core/parser/Parser.hpp"
#include "gql/core/ast.hpp"

namespace gql_native_regressions {
inline bool run(const cursiv::Parser& parser) {
  using namespace gql::ast;
  bool ok = true;
  const auto require = [&](bool value, const char* what) {
    if (!value) { std::fprintf(stderr, "gql native regression: %s\n", what); ok = false; }
  };
  const auto clean = [&](const std::string& source, auto check) {
    const auto parsed = parser.parse(source);
    const bool good = parsed.accepted && !parsed.cst().recovered() &&
                      parsed.diagnostics.empty() && parsed.value;
    if (!good) std::fprintf(stderr, "gql native input: %s\n", source.c_str());
    require(good, "source must parse without recovery/diagnostics");
    if (good) check(parsed);
  };
  clean("LET FORé = 1 MATCH (n:\"RETURN\") RETURN n.\"order\" AS `AS`",
        [&](const auto& parsed) {
    bool name = false, label = false, property = false, alias = false;
    for (const auto* node : parsed.nodes) {
      if (const auto* n = cursiv::ast_cast<LetVarDef>(node)) name = n->name == "FORé";
      if (const auto* n = cursiv::ast_cast<LabelName>(node)) label = n->name == "RETURN";
      if (const auto* n = cursiv::ast_cast<PropertyRef>(node)) property = n->property == "order";
      if (const auto* n = cursiv::ast_cast<ReturnItem>(node)) alias = n->alias == "AS";
    }
    require(name && label && property && alias,
            "Unicode keyword prefixes and decoded delimited names");
  });
  clean(R"(RETURN '\uABCD', '\U01F600', @'\q\n''x')", [&](const auto& parsed) {
    bool values = false;
    for (const auto* node : parsed.nodes)
      if (const auto* result = cursiv::ast_cast<ResultStmt>(node); result && result->items.size() == 3) {
        const std::string_view expected[] = {"ꯍ", "😀", "\\q\\n'x"};
        values = true;
        for (unsigned i = 0; i != 3; ++i) {
          const auto* value = cursiv::ast_cast<StringLit>(result->items[i]->value);
          values = values && value && value->value == expected[i];
        }
      }
    require(values, "Unicode hex, raw backslashes and string ordering retained");
  });
  clean("MATCH (n), (m) RETURN COUNT(DISTINCT n), SAME(n,m), ALL_DIFFERENT(n,m) GROUP BY ()",
        [&](const auto& parsed) {
    bool distinct = false, group = false;
    for (const auto* node : parsed.nodes) {
      if (const auto* n = cursiv::ast_cast<CallExpr>(node))
        distinct = distinct || (n->name == "COUNT" && n->distinct && n->extra == "DISTINCT");
      if (const auto* n = cursiv::ast_cast<ResultStmt>(node))
        group = group || (n->grouped && n->groupBy.empty());
    }
    require(distinct && group, "aggregate DISTINCT and empty GROUP BY presence retained");
  });
  clean("RETURN NOT TRUE IS FALSE", [&](const auto& parsed) {
    const NotExpr* negation = nullptr;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<NotExpr>(node)) negation = n;
    require(negation && cursiv::ast_cast<TruthTest>(negation->operand),
            "IS FALSE binds more tightly than NOT");
  });
  for (const auto* prefix : {"ANY", "ANY SHORTEST", "ALL", "ALL SHORTEST", "SHORTEST $count"}) {
    clean(std::string("MATCH ") + prefix + " (n) RETURN n", [&](const auto& parsed) {
      const PathPrefix* path = nullptr;
      for (const auto* node : parsed.nodes)
        if (const auto* n = cursiv::ast_cast<PathPrefix>(node)) path = n;
      require(path && path->shortest == (std::string_view(prefix).find("SHORTEST") != std::string_view::npos),
              "SHORTEST remains distinguishable from ANY/ALL");
    });
  }
  for (const auto* source : {"MATCH ()<-/L/-() RETURN 1", "MATCH ()~/L/~() RETURN 1",
                             "MATCH ()-/L/->() RETURN 1", "MATCH ()<~/L/~() RETURN 1",
                             "MATCH ()~/L/~>() RETURN 1", "MATCH ()<-/L/->() RETURN 1",
                             "MATCH ()-/L/-() RETURN 1"})
    clean(source, [](const auto&) {});
  clean("MATCH ()-/A B&C*|(D|+|E)?/->() RETURN 1", [&](const auto& parsed) {
    const SimplifiedPathExpr* path = nullptr;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<SimplifiedPathExpr>(node)) path = n;
    const auto* concat = path && path->terms.size() == 2
                             ? cursiv::ast_cast<SimplifiedConcat>(path->terms[0]) : nullptr;
    const auto* conjunction = concat && concat->factors.size() == 2
                                  ? cursiv::ast_cast<LabelBin>(concat->factors[1]) : nullptr;
    const auto* quantified = path && path->terms.size() == 2
                                 ? cursiv::ast_cast<SimplifiedQuantified>(path->terms[1]) : nullptr;
    const auto* nested = quantified ? cursiv::ast_cast<SimplifiedAlternative>(quantified->operand) : nullptr;
    require(path && path->op == "|" && concat && conjunction &&
            cursiv::ast_cast<SimplifiedQuantified>(conjunction->right) && nested &&
            nested->op == "|+|" && nested->terms.size() == 2 &&
            quantified->quantifier && quantified->quantifier->symbol == "?" &&
            nested->parent == quantified, "simplified path precedence and ownership retained");
  });
  clean("MATCH ()-/<A> ~!B> <~C -D E>/->() RETURN 1", [&](const auto& parsed) {
    unsigned directions = 0;
    for (const auto* node : parsed.nodes)
      if (cursiv::ast_cast<SimplifiedDirection>(node)) ++directions;
    require(directions == 5, "all direction overrides retain their own node");
  });
  clean("CREATE GRAPH TYPE g { (:A {a INT})-[:E {e STRING}]->(:B {b BOOL}) }",
        [&](const auto& parsed) {
    const ElementTypeSpec* edge = nullptr;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<ElementTypeSpec>(node); n && n->isEdge) edge = n;
    require(edge && edge->sourceType && edge->destType && edge->labels.size() == 1 &&
            edge->labels[0] == "E" && edge->sourceType->labels.size() == 1 &&
            edge->sourceType->labels[0] == "A" && edge->destType->labels.size() == 1 &&
            edge->destType->labels[0] == "B" && edge->sourceType->properties.size() == 1 &&
            edge->destType->properties.size() == 1 && edge->properties.size() == 1 &&
            edge->sourceType->parent == edge && edge->destType->parent == edge,
            "endpoint type fillers must not overwrite edge labels/properties");
  });
  clean("SESSION SET SCHEMA ../../app SESSION RESET SCHEMA", [&](const auto& parsed) {
    bool parents = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<CatalogRef>(node))
        parents = parents || (n->relative && n->parentSteps.size() == 2 && n->segments.size() == 1);
    require(parents, "relative schema depth retained");
  });
  clean("SESSION SET SCHEMA /", [&](const auto& parsed) {
    bool root = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<CatalogRef>(node)) root = n->absolute && n->segments.empty();
    require(root, "root schema is an absolute path");
  });
  clean("SESSION SET GRAPH /app/g", [&](const auto& parsed) {
    bool schema = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<CatalogRef>(node))
        schema = schema || (n->schema && n->schema->absolute && n->schema->segments.size() == 1 &&
                            n->schema->segments[0] == "app" && n->segments.size() == 1 && n->segments[0] == "g");
    require(schema, "object references retain explicit schema separately");
  });
  clean("RETURN CAST(NULL AS UNSIGNED INTEGER64), CAST(NULL AS LOCAL TIME), CAST(NULL AS LOCAL DATETIME), CAST(NULL AS ANY GRAPH)",
        [&](const auto& parsed) {
    bool sign = false, time = false, datetime = false, graph = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<TypeRef>(node)) {
        sign = sign || (n->name == "INTEGER64" && n->signedness == "UNSIGNED");
        time = time || (n->name == "LOCAL" && n->qualifier == "TIME");
        datetime = datetime || (n->name == "LOCAL" && n->qualifier == "DATETIME");
        graph = graph || (n->name == "ANY" && n->graphType);
      }
    require(sign && time && datetime && graph, "native type distinctions retained");
  });
  clean("MATCH (n) YIELD NO BINDINGS RETURN 1", [&](const auto& parsed) {
    bool noBindings = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<YieldClause>(node)) noBindings = n->noBindings && n->items.empty();
    require(noBindings, "YIELD NO BINDINGS retained");
  });
  clean("RETURN X'AB' /* joining separator */ 'CD'", [&](const auto& parsed) {
    bool joined = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<ByteStringLit>(node))
        joined = n->text == "X'AB' /* joining separator */ 'CD'";
    require(joined, "byte literal chunks preserve their complete raw spelling");
  });
  clean("MATCH (TEMP n) RETURN n", [&](const auto& parsed) {
    bool temporary = false;
    for (const auto* node : parsed.nodes)
      if (const auto* n = cursiv::ast_cast<ElementFiller>(node))
        temporary = temporary || (n->temporary && n->variable == "n");
    require(temporary, "temporary element variable declaration retained");
  });
  clean("RETURN 0XAB, 0O7, 0B1", [](const auto&) {});
  clean("MATCH (TEMP) RETURN TEMP", [](const auto&) {});
  clean(R"(RETURN $@"a\q")", [](const auto&) {});
  for (const auto* source : {
         "LET RETURN = 1 RETURN 1", "RETURN 1 AS IF", "RETURN 1 < 2 < 3",
         "RETURN NOT NOT TRUE", "RETURN ++1", "MATCH (n) RETURN SAME(n)",
         "MATCH (n) RETURN ALL_DIFFERENT(n)", R"(RETURN '\uD800')", R"(RETURN '\U110000')",
         "MATCH ()-/A|B|+|C/->() RETURN 1", "MATCH ()-/%/->() RETURN 1",
         "MATCH ()->{1.5}() RETURN 1", "RETURN CAST(1 AS STRING(1.5))",
         "RETURN CAST(1 AS DURATION(YEAR TO SECOND))", "RETURN TRIM('x' 'y')", "RETURN DURATION()",
         "RETURN CAST(1 AS TIME)",
         "RETURN 1 OTHERWISE ALL RETURN 2", "MATCH (n:!!A) RETURN n",
         "INSERT (n WHERE TRUE)", "INSERT (n:A|B)", "MATCH (n {}) RETURN n",
         "OPTIONAL { LET x = 1 } RETURN 1"}) {
    const auto parsed = parser.parse(source);
    require(!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty(),
            source);
  }
  return ok;
}
}  // namespace gql_native_regressions
