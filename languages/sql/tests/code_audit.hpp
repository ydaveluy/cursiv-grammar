#pragma once
#include <cstdio>
#include <functional>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "sqlite/core/CoreServices.hpp"
#include "sqlite/core/ast.hpp"
namespace code_audit {
namespace ast = sqlite::ast;
inline bool run() {
  auto language = cursiv::create_language<sqlite::SqliteCoreServices,
                                          sqlite::SqliteSharedCoreServices,
                                          sqlite::install_core_module>();
  bool ok = true;
  auto check = [&](bool v, const char* m) {
    if (!v) {
      std::fprintf(stderr, "sqlite code audit: %s\n", m);
      ok = false;
    }
  };
  auto parse = [&](std::string_view source) {
    auto r = language.language.parser->parse(source);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "clean parse");
    check(r.value_as<ast::Script>() != nullptr, "Script root");
    std::unordered_set<const cursiv::AstNode*> reached;
    std::function<void(const cursiv::AstNode*, const cursiv::AstNode*)> walk =
        [&](const auto* n, const auto* parent) {
          if (!n) return;
          check(n->parent == parent, "AST parent");
          if (!reached.insert(n).second) {
            check(false, "unique AST ownership");
            return;
          }
          language.language.astReflection->visitMembers(
              *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& c) {
                walk(&c, n);
              });
        };
    walk(r.value, nullptr);
    check(reached.size() == r.nodes.size(),
          "every allocated AST node is reachable");
    return r;
  };
  auto select = [](const cursiv::ParseResult& r) {
    const auto* root = r.value_as<ast::Script>();
    return root && !root->statements.empty()
               ? cursiv::ast_cast<ast::Select>(root->statements[0]->statement)
               : nullptr;
  };
  auto expression = [&](const cursiv::ParseResult& r,
                        std::size_t i = 0) -> const ast::Expr* {
    const auto* s = select(r);
    return s && !s->parts.empty() && i < s->parts[0]->core->columns.size()
               ? s->parts[0]->core->columns[i]->value
               : nullptr;
  };
  {
    const auto r = parse(
        "SELECT a IN (1,2), a IN (SELECT b FROM t2), a IN main.t2, a NOT IN "
        "f(3); ");
    const auto* list = cursiv::ast_cast<ast::InExpr>(expression(r));
    const auto* sub = cursiv::ast_cast<ast::InExpr>(expression(r, 1));
    const auto* table = cursiv::ast_cast<ast::InExpr>(expression(r, 2));
    const auto* fn = cursiv::ast_cast<ast::InExpr>(expression(r, 3));
    check(list && list->list.size() == 2 &&
              cursiv::ast_cast<ast::NumberLit>(list->list[1])->value == "2",
          "IN list retains values");
    check(sub && sub->subquery && sub->subquery->parts.size() == 1 &&
              sub->subquery->parts[0]->core->from->first->name == "t2",
          "IN retains its complete subquery");
    check(table && table->schema == "main" && table->table == "t2",
          "IN retains qualified table");
    check(fn && fn->negated && fn->isTableFunction && fn->table == "f" &&
              fn->tableArgs.size() == 1,
          "IN retains table function arguments");
  }
  {
    const auto r = parse(
        "SELECT a BETWEEN b=1 AND c, a = NOT b=1, -NOT a+1 FROM t1 HAVING "
        "sum(a)>0;");
    const auto* between = cursiv::ast_cast<ast::BetweenExpr>(expression(r));
    const auto* low =
        between ? cursiv::ast_cast<ast::BinaryExpr>(between->low) : nullptr;
    check(low && low->op == ast::Operator::eq,
          "BETWEEN lower bound has equality precedence");
    const auto* eq = cursiv::ast_cast<ast::BinaryExpr>(expression(r, 1));
    const auto* neg =
        eq ? cursiv::ast_cast<ast::UnaryExpr>(eq->right) : nullptr;
    check(eq && eq->op == ast::Operator::eq && neg && neg->op == "NOT" &&
              cursiv::ast_cast<ast::BinaryExpr>(neg->operand),
          "prefix NOT is legal on an operator right side");
    check(select(r) && select(r)->parts[0]->core->having &&
              select(r)->parts[0]->core->groupBy.empty(),
          "HAVING without GROUP BY");
  }
  {
    const auto r = parse("SELECT $ns::name(suffix), :123, @a$b, #abc, ?42;");
    for (std::size_t i = 0; i < 5; ++i) {
      const auto* p = cursiv::ast_cast<ast::BindParam>(expression(r, i));
      constexpr std::string_view expected[] = {"$ns::name(suffix)", ":123",
                                               "@a$b", "#abc", "?42"};
      check(p && p->text == expected[i], "full native parameter spelling");
    }
  }
  {
    const auto r = parse(
        "CREATE TABLE 😀('some column' 'custom type', a$b DEFAULT abc, § "
        "DEFAULT -'12', g GENERATED ALWAYS AS (a$b+1)); SELECT "
        "\"m\".\"t\".\"c\";");
    const auto* root = r.value_as<ast::Script>();
    const auto* t = root && root->statements.size() == 2
                        ? cursiv::ast_cast<ast::CreateTableStmt>(
                              root->statements[0]->statement)
                        : nullptr;
    check(t && t->name == "😀" && t->columns.size() == 4 &&
              t->columns[0]->name == "some column" &&
              t->columns[1]->name == "a$b" && t->columns[2]->name == "§",
          "native identifier and type-name forms");
    check(t && t->columns[3]->constraints.size() == 1 &&
              t->columns[3]->constraints[0]->generatedAlways,
          "generated-always flag belongs to its constraint");
    const auto* s =
        root && root->statements.size() == 2
            ? cursiv::ast_cast<ast::Select>(root->statements[1]->statement)
            : nullptr;
    const auto* q = s ? cursiv::ast_cast<ast::QualifiedRef>(
                            s->parts[0]->core->columns[0]->value)
                      : nullptr;
    check(q && q->schema == "m" && q->table == "t" && q->column == "c",
          "qualified identifier components are decoded separately");
  }
  {
    const auto r = parse(
        "ALTER TABLE t ADD c TEXT; SELECT count(ALL a),CAST(12 AS) FROM t1; "
        "CREATE TABLE x(a INTEGER,b,PRIMARY KEY(a AUTOINCREMENT) UNIQUE(b) "
        "CHECK(a>0) ON CONFLICT IGNORE);");
    const auto* root = r.value_as<ast::Script>();
    const auto* a = root && root->statements.size() == 3
                        ? cursiv::ast_cast<ast::AlterTableStmt>(
                              root->statements[0]->statement)
                        : nullptr;
    check(a && a->action == ast::AlterTableStmt::Action::add_column &&
              a->addedColumn && a->addedColumn->name == "c",
          "ADD has optional COLUMN keyword");
    const auto* t = root && root->statements.size() == 3
                        ? cursiv::ast_cast<ast::CreateTableStmt>(
                              root->statements[2]->statement)
                        : nullptr;
    check(t && t->constraints.size() == 3 && t->constraints[0]->autoincrement &&
              t->constraints[2]->conflict == ast::ConflictAction::ignore,
          "native table constraint forms and fields");
  }
  {
    const auto r = parse(
        "SELECT * FROM t1 LEFT NATURAL JOIN t2 LEFT RIGHT JOIN t3; BEGIN "
        "TRANSACTION named; COMMIT TRANSACTION named; ROLLBACK TRANSACTION "
        "named TO SAVEPOINT sp;");
    const auto* s = select(r);
    const auto* j = s ? s->parts[0]->core->from : nullptr;
    check(j && j->steps.size() == 2 && j->steps[0]->modifiers.size() == 2 &&
              j->steps[0]->modifiers[0] == "LEFT" &&
              j->steps[0]->modifiers[1] == "NATURAL" &&
              j->steps[1]->modifiers[1] == "RIGHT",
          "join modifiers preserve native order");
    const auto* root = r.value_as<ast::Script>();
    check(
        root && root->statements.size() == 4 &&
            cursiv::ast_cast<ast::BeginStmt>(root->statements[1]->statement)
                    ->name == "named" &&
            cursiv::ast_cast<ast::CommitStmt>(root->statements[2]->statement)
                    ->name == "named" &&
            cursiv::ast_cast<ast::RollbackStmt>(root->statements[3]->statement)
                    ->savepoint == "sp",
        "transaction labels and savepoint survive");
  }
  {
    const auto r = parse(
        "CREATE VIRTUAL TABLE x USING module(a(\"(\", ')'), \"a,b\", [a,b], "
        "`a,b`,,); ");
    const auto* root = r.value_as<ast::Script>();
    const auto* v = root && !root->statements.empty()
                        ? cursiv::ast_cast<ast::CreateVirtualTableStmt>(
                              root->statements[0]->statement)
                        : nullptr;
    check(
        v && v->moduleArgs.size() == 6 &&
            v->moduleArgs[0].find("a(\"(\", ')')") != std::string_view::npos &&
            v->moduleArgs[1].find("\"a,b\"") != std::string_view::npos &&
            v->moduleArgs[4].empty() && v->moduleArgs[5].empty(),
        "module arguments preserve quoted delimiters and empty positions");
  }
  {
    const auto r = parse(
        "\xef\xbb\xbfSELECT 1 -- comment\ronly\n; /* comment without a closer");
    check(select(r) && select(r)->parts.size() == 1,
          "BOM, LF-terminated comments and EOF block comments");
  }
  {
    const auto r = parse(
        "SELECT row_number() OVER \"w\",sum(a) OVER ([w]) FROM t1 WINDOW \"w\" "
        "AS (),[w] AS ();");
    const auto* s = select(r);
    const auto* f = cursiv::ast_cast<ast::FuncCall>(expression(r));
    const auto* chained = cursiv::ast_cast<ast::FuncCall>(expression(r, 1));
    check(f && f->overName == "\"w\"" && chained && chained->overDefn &&
              chained->overDefn->baseWindow == "[w]" && s &&
              s->parts[0]->core->windows.size() == 2 &&
              s->parts[0]->core->windows[0]->name == "\"w\"" &&
              s->parts[0]->core->windows[1]->name == "[w]",
          "window names retain native quote-sensitive spelling");
  }
  {
    const auto r = parse(
        "CREATE VIRTUAL TABLE x USING module(a /*, (*/ b,c(-- ) , \n x));");
    const auto* root = r.value_as<ast::Script>();
    const auto* v = root && !root->statements.empty()
                        ? cursiv::ast_cast<ast::CreateVirtualTableStmt>(
                              root->statements[0]->statement)
                        : nullptr;
    check(v && v->moduleArgs.size() == 2 &&
              v->moduleArgs[0].find("/*, (*/") != std::string_view::npos &&
              v->moduleArgs[1].find("-- ) ,") != std::string_view::npos,
          "module comments hide delimiter characters");
  }
  {
    const auto r = parse(
        "CREATE TABLE x(a DEFERRABLE,b NOT DEFERRABLE INITIALLY IMMEDIATE,c "
        "CONSTRAINT marker,CONSTRAINT table_marker);");
    const auto* root = r.value_as<ast::Script>();
    const auto* t = root && !root->statements.empty()
                        ? cursiv::ast_cast<ast::CreateTableStmt>(
                              root->statements[0]->statement)
                        : nullptr;
    check(t && t->columns.size() == 3 && t->constraints.size() == 1 &&
              t->columns[0]->constraints.size() == 1 &&
              t->columns[0]->constraints[0]->kind ==
                  ast::ColumnConstraintKind::deferral &&
              t->columns[1]->constraints[0]->notDeferrable &&
              t->columns[1]->constraints[0]->initially ==
                  ast::InitiallyMode::immediate &&
              t->columns[2]->constraints[0]->kind ==
                  ast::ColumnConstraintKind::name_only &&
              t->columns[2]->constraints[0]->name == "marker" &&
              t->constraints[0]->kind == ast::TableConstraintKind::name_only &&
              t->constraints[0]->name == "table_marker",
          "native standalone constraint markers and deferral are retained");
  }
  {
    const auto r = parse(
        "WITH RECURSIVE c(x) AS NOT MATERIALIZED (VALUES(1) UNION ALL SELECT "
        "x+1 FROM c WHERE x<3) SELECT * FROM c ORDER BY x LIMIT 2 OFFSET 1;");
    const auto* s = select(r);
    check(s && s->recursive && s->ctes.size() == 1 && s->ctes[0]->name == "c" &&
              s->ctes[0]->columns.size() == 1 &&
              s->ctes[0]->columns[0] == "x" &&
              s->ctes[0]->materialized == false &&
              s->ctes[0]->query->parts.size() == 2 &&
              s->ctes[0]->query->parts[1]->op == ast::CompoundOp::union_all &&
              s->orderBy.size() == 1 &&
              cursiv::ast_cast<ast::NumberLit>(s->limit)->value == "2" &&
              cursiv::ast_cast<ast::NumberLit>(s->offset)->value == "1",
          "CTE flags, columns, compound operator and query limits survive");
  }
  {
    const auto r = parse(
        "SELECT 1 AS \"\",2; SELECT 1 IN \"\",1 IN (),1 IN \"\"(2); SELECT "
        "\"\".* FROM \"\" AS \"\" INDEXED BY \"\"; SELECT \"\".t.\"\" FROM "
        "\"\".t;");
    const auto* root = r.value_as<ast::Script>();
    const auto* first = select(r);
    check(first && first->parts[0]->core->columns[0]->alias &&
              first->parts[0]->core->columns[0]->alias->empty() &&
              !first->parts[0]->core->columns[1]->alias,
          "empty alias differs from absent alias");
    if (root && root->statements.size() == 4) {
      const auto* second =
          cursiv::ast_cast<ast::Select>(root->statements[1]->statement);
      const auto& columns = second->parts[0]->core->columns;
      const auto* table = cursiv::ast_cast<ast::InExpr>(columns[0]->value);
      const auto* list = cursiv::ast_cast<ast::InExpr>(columns[1]->value);
      const auto* call = cursiv::ast_cast<ast::InExpr>(columns[2]->value);
      check(table && table->table && table->table->empty() && list &&
                !list->table && call && call->table && call->table->empty() &&
                call->isTableFunction && call->tableArgs.size() == 1,
            "IN retains empty table and function names distinctly from an "
            "empty list");
      const auto* third =
          cursiv::ast_cast<ast::Select>(root->statements[2]->statement);
      const auto* item = third->parts[0]->core->from->first;
      check(third->parts[0]->core->columns[0]->table &&
                third->parts[0]->core->columns[0]->table->empty() &&
                item->alias && item->alias->empty() && item->indexedBy &&
                item->indexedBy->empty(),
            "empty star qualifier, table alias and index name survive");
      const auto* fourth =
          cursiv::ast_cast<ast::Select>(root->statements[3]->statement);
      const auto* qualified = cursiv::ast_cast<ast::QualifiedRef>(
          fourth->parts[0]->core->columns[0]->value);
      check(qualified && qualified->schema && qualified->schema->empty() &&
                qualified->column.empty() &&
                fourth->parts[0]->core->from->first->schema &&
                fourth->parts[0]->core->from->first->schema->empty(),
            "empty schemas and column names survive separately");
    } else
      check(false, "four empty-name queries");
  }
  {
    const auto r = parse(
        "ANALYZE \"\"; REINDEX \"\"; VACUUM \"\"; ROLLBACK TRANSACTION \"\" TO "
        "SAVEPOINT \"\"; ALTER TABLE \"\".t ADD CONSTRAINT \"\" CHECK(1);");
    const auto* root = r.value_as<ast::Script>();
    if (root && root->statements.size() == 5) {
      const auto* a =
          cursiv::ast_cast<ast::AnalyzeStmt>(root->statements[0]->statement);
      const auto* re =
          cursiv::ast_cast<ast::ReindexStmt>(root->statements[1]->statement);
      const auto* v =
          cursiv::ast_cast<ast::VacuumStmt>(root->statements[2]->statement);
      const auto* rollback =
          cursiv::ast_cast<ast::RollbackStmt>(root->statements[3]->statement);
      const auto* alter =
          cursiv::ast_cast<ast::AlterTableStmt>(root->statements[4]->statement);
      check(a && a->name && a->name->empty() && re && re->name &&
                re->name->empty() && v && v->schema && v->schema->empty() &&
                rollback && rollback->name && rollback->name->empty() &&
                rollback->savepoint && rollback->savepoint->empty() && alter &&
                alter->schema && alter->schema->empty() &&
                alter->constraintName && alter->constraintName->empty(),
            "empty optional command names retain presence");
    } else
      check(false, "five optional-name statements");
  }
  {
    const std::string source =
        "SELECT X'0',X'gg',?0,:;SELECT * FROM t1 OUTER JOIN t2;";
    cursiv::Document doc(
        cursiv::TextDocument::create("memory:sqlite", "sql", 0, source));
    doc.parseResult = parse(source);
    unsigned errors = 0;
    auto sink = [&](cursiv::Diagnostic&&) { ++errors; };
    cursiv::ValidationAcceptor accept(sink, doc, source);
    std::function<void(const cursiv::AstNode*)> walk = [&](const auto* n) {
      if (!n) return;
      if (const auto* b = cursiv::ast_cast<ast::BlobLit>(n))
        language.language.validator->checkBlob(*b, accept);
      if (const auto* b = cursiv::ast_cast<ast::BindParam>(n))
        language.language.validator->checkBind(*b, accept);
      if (const auto* j = cursiv::ast_cast<ast::JoinStep>(n))
        language.language.validator->checkJoin(*j, accept);
      language.language.astReflection->visitMembers(
          *n, [&](std::string_view, std::uint32_t, cursiv::AstNode& c) {
            walk(&c);
          });
    };
    walk(doc.parseResult.value);
    check(errors == 5,
          "blob, parameter and join errors validate after clean parsing");
  }
  return ok;
}
}  // namespace code_audit
