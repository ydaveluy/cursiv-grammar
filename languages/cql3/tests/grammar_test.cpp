#include <cstdio>

#include "cql3/core/CoreServices.hpp"
#include "cql3/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Root at the top. One document
// (createTable.cql3) is also read back field by field, since a structural
// pass alone would not catch a converter that silently returns the wrong
// value.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "json_insert.hpp"
#include "terms_selectors.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "cql3: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

int check_create_table(const auto& language, const fs::path& dir) {
  const fs::path path = dir / "example" / "createTable.cql3";
  const std::string text = read(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<cql3::ast::Root>)
    return fail(path, "the parse built no Root");
  const auto& doc = *static_cast<const cql3::ast::Root*>(root);
  if (doc.statements.size() != 1)
    return fail(path, "expected exactly one statement");

  const cql3::ast::Statement* const stmt = doc.statements[0];
  if (stmt == nullptr ||
      stmt->type != &cursiv::ast_type_tag<cql3::ast::CreateTable>)
    return fail(path, "expected a CreateTable statement");
  const auto& table = *static_cast<const cql3::ast::CreateTable*>(stmt);

  if (table.keyspace != "cycling") return fail(path, "wrong keyspace");
  if (table.table != "race_winners") return fail(path, "wrong table name");
  if (table.columns.size() != 3) return fail(path, "wrong column count");
  if (table.columns[0]->name != "race_name" ||
      table.columns[0]->type->name != "text")
    return fail(path, "wrong first column");
  if (table.columns[2]->name != "cyclist_name" ||
      table.columns[2]->type->name != "FROZEN" ||
      table.columns[2]->type->params.size() != 1 ||
      table.columns[2]->type->params[0]->name != "fullname")
    return fail(path, "wrong frozen column type");
  if (table.primaryKey == nullptr ||
      table.primaryKey->partitionColumns.size() != 1 ||
      table.primaryKey->partitionColumns[0] != "race_name" ||
      table.primaryKey->clusteringColumns.size() != 1 ||
      table.primaryKey->clusteringColumns[0] != "race_position")
    return fail(path, "wrong primary key");

  return 0;
}

template <class T>
const T* is(const cursiv::AstNode* node) {
  return node != nullptr && node->type == &cursiv::ast_type_tag<T>
             ? static_cast<const T*>(node)
             : nullptr;
}

// `{}` is a set literal wherever a collection is expected (Cassandra's
// Parser.g forces the empty braces to a set); a map needs one pair.
int check_empty_braces(const auto& language) {
  const fs::path path = "{}";
  const std::string text =
      "UPDATE t SET a = {} WHERE id = 1;\n"
      "INSERT INTO t (id, c) VALUES (1, {});\n"
      "UPDATE t SET b = {1: 2} WHERE id = 1;\n";
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");
  const auto* const root = is<cql3::ast::Root>(result.value);
  if (root == nullptr || root->statements.size() != 3)
    return fail(path, "expected three statements");
  const auto* const set_update = is<cql3::ast::Update>(root->statements[0]);
  const auto* const insert = is<cql3::ast::Insert>(root->statements[1]);
  const auto* const map_update = is<cql3::ast::Update>(root->statements[2]);
  if (set_update == nullptr || set_update->assignments.size() != 1 ||
      is<cql3::ast::SetLit>(set_update->assignments[0]->value) == nullptr)
    return fail(path, "`SET a = {}` is not a set");
  if (insert == nullptr || insert->values.size() != 2 ||
      is<cql3::ast::SetLit>(insert->values[1]) == nullptr)
    return fail(path, "`VALUES (1, {})` is not a set");
  if (map_update == nullptr || map_update->assignments.size() != 1 ||
      is<cql3::ast::MapLit>(map_update->assignments[0]->value) == nullptr)
    return fail(path, "`SET b = {1: 2}` is not a map");
  return 0;
}

// A handful of constructs the upstream corpus never exercises (BEGIN BATCH,
// CREATE TRIGGER, IF NOT EXISTS, ALLOW FILTERING, NORECURSIVE, CONTAINS[
// KEY], a hex literal, SUPERUSER/NOSUPERUSER, ROLE ... OPTIONS, a
// materialized view's trailing `AND` relations, INITCOND's hash and
// nested-list shapes) -- authored here, under tests/, per the corpus rule:
// example/ holds only the upstream corpus.
int check_coverage(const auto& language, const fs::path& dir) {
  const fs::path path = fs::path(dir) / "coverage.cql3";
  const std::string text = read(path);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty())
    return fail(path, "does not parse clean");
  if (result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<cql3::ast::Root>)
    return fail(path, "the parse built no Root");
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<cql3::Cql3CoreServices,
                                                cql3::Cql3SharedCoreServices,
                                                cql3::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "CREATE TABLE \"a\"\"b\" (k int PRIMARY KEY, x "
        "list<frozen<list<int>>>, v vector<float,3>);\nINSERT INTO t (k,s,n,d) "
        "VALUES "
        "(:key,'a\\b',123456789012345678901234567890,-12345678901234567890."
        "123456789e-10);\nUPDATE users SET favs['author'] = ? WHERE id = "
        ":id;\nSELECT * FROM t WHERE k = ? LIMIT :n;");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;

    const auto* doc = r.value_as<cql3::ast::Root>();
    if (!doc || doc->statements.size() != 4) return 1;
    const auto* table =
        cursiv::ast_cast<cql3::ast::CreateTable>(doc->statements[0]);
    if (!table || table->table != "a\"b" || table->columns.size() != 3 ||
        table->columns[1]->type->params.size() != 1 ||
        table->columns[1]->type->params[0]->name != "frozen" ||
        table->columns[1]->type->params[0]->params[0]->params[0]->name !=
            "int" ||
        table->columns[2]->type->params[1]->name != "3")
      return 1;
    const auto* ins = cursiv::ast_cast<cql3::ast::Insert>(doc->statements[1]);
    if (!ins || ins->values.size() != 4) return 1;
    const auto* key = cursiv::ast_cast<cql3::ast::BindMarker>(ins->values[0]);
    const auto* str = cursiv::ast_cast<cql3::ast::StringLit>(ins->values[1]);
    const auto* integer = cursiv::ast_cast<cql3::ast::IntLit>(ins->values[2]);
    const auto* dec = cursiv::ast_cast<cql3::ast::FloatLit>(ins->values[3]);
    if (!key || key->name != "key" || !str || str->value != "a\\b" ||
        !integer || integer->value != "123456789012345678901234567890" ||
        !dec || dec->value != "-12345678901234567890.123456789e-10")
      return 1;
    const auto* update =
        cursiv::ast_cast<cql3::ast::Update>(doc->statements[2]);
    if (!update || update->assignments.size() != 1) return 1;
    const auto* index =
        cursiv::ast_cast<cql3::ast::StringLit>(update->assignments[0]->index);
    if (!index || index->value != "author" ||
        !cursiv::ast_cast<cql3::ast::BindMarker>(update->assignments[0]->value))
      return 1;
    const auto* select =
        cursiv::ast_cast<cql3::ast::Select>(doc->statements[3]);
    if (!select || !cursiv::ast_cast<cql3::ast::BindMarker>(select->limit))
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "CREATE TABLE t (k int PRIMARY KEY, v vector<float,0>);",
        "file:///audit.cql3");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "--without a space\nCREATE TABLE MixedCase (A int, B int, PRIMARY KEY ((A,B))) WITH cdc=true;"
        "INSERT INTO t (k,u,f,d,b) VALUES (1,{Field: 'x'},-Infinity,1h30m,0x);"
        "SELECT * FROM t WHERE k IN :keys AND labels['x'] != 'y' "
        "GROUP BY a,b ORDER BY a DESC,b ASC PER PARTITION LIMIT ? LIMIT 10;"
        "CREATE TRIGGER tr ON ks.tbl USING 'example.Trigger';"
        "CREATE AGGREGATE agg(int,text) SFUNC state STYPE int;"
        "UPDATE t SET a=[1]+a,b=b+[2],c=c+? WHERE k=1;"
        "SELECT Ks.Fn(a) FROM t; --final");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("native-followup", "native statement forms need repair");
    const auto* doc = r.value_as<cql3::ast::Root>();
    if (!doc || doc->statements.size() != 7) return 1;
    const auto* table = cursiv::ast_cast<cql3::ast::CreateTable>(doc->statements[0]);
    const auto* insert = cursiv::ast_cast<cql3::ast::Insert>(doc->statements[1]);
    const auto* select = cursiv::ast_cast<cql3::ast::Select>(doc->statements[2]);
    const auto* trigger = cursiv::ast_cast<cql3::ast::CreateTrigger>(doc->statements[3]);
    const auto* aggregate = cursiv::ast_cast<cql3::ast::CreateAggregate>(doc->statements[4]);
    const auto* update = cursiv::ast_cast<cql3::ast::Update>(doc->statements[5]);
    const auto* call_select = cursiv::ast_cast<cql3::ast::Select>(doc->statements[6]);
    if (!table || table->table != "mixedcase" || !table->primaryKey ||
        table->primaryKey->partitionColumns.size() != 2 ||
        !table->primaryKey->clusteringColumns.empty() || !insert ||
        insert->values.size() != 5 || !select || select->groupBy.size() != 2 ||
        select->orderBy.size() != 2 || !select->orderBy[0]->descending ||
        !*select->orderBy[0]->descending || !select->orderBy[1]->descending ||
        *select->orderBy[1]->descending ||
        !cursiv::ast_cast<cql3::ast::BindMarker>(select->perPartitionLimit) ||
        select->where.size() != 2 || !trigger || trigger->onKeyspace != "ks" ||
        trigger->onTable != "tbl" || !aggregate || aggregate->argTypes.size() != 2 ||
        !aggregate->finalfunc.empty() || aggregate->initcond || !update ||
        update->assignments.size() != 3 ||
        !update->assignments[0]->counterOp ||
        !update->assignments[0]->counterOp->deltaFirst ||
        !update->assignments[1]->counterOp ||
        update->assignments[1]->counterOp->deltaFirst ||
        !update->assignments[2]->counterOp ||
        !cursiv::ast_cast<cql3::ast::BindMarker>(update->assignments[2]->counterOp->deltaOperand) ||
        !call_select || call_select->elements.size() != 1 ||
        !call_select->elements[0]->call ||
        call_select->elements[0]->call->keyspace != "ks" ||
        call_select->elements[0]->call->name != "fn") return 1;
    const auto* udt = cursiv::ast_cast<cql3::ast::UdtLit>(insert->values[1]);
    const auto* duration = cursiv::ast_cast<cql3::ast::DurationLit>(insert->values[3]);
    const auto* in = cursiv::ast_cast<cql3::ast::InRelation>(select->where[0]);
    const auto* compare = cursiv::ast_cast<cql3::ast::CompareRelation>(select->where[1]);
    if (!udt || udt->fields.size() != 1 || udt->fields[0]->field != "field" ||
        !duration || duration->value != "1h30m" || !in || !in->marker ||
        in->marker->name != "keys" || !compare || !compare->index ||
        compare->op != cql3::ast::CompareOp::neq) return 1;
  }
  for (const std::string source : {
           "CREATE AGGREGATE empty() SFUNC state STYPE int INITCOND 0;",
           "INSERT INTO t (d) VALUES (P1Y2M3DT4H5M6S);",
           "INSERT INTO t (d) VALUES (-P0001-02-03T04:05:06);",
           "INSERT INTO t (f) VALUES (NaN);",
           "SELECT JSON DISTINCT k FROM t;",
           "CREATE CUSTOM INDEX i ON t (a,VALUES(b)) USING 'example.Index' WITH OPTIONS={};"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("native-followup", "native literal/aggregate needs repair");
  }
  {
    auto document = language.shared->documentFactory->fromString(
        "CREATE TYPE \"list\" (x int); CREATE TABLE t (k int PRIMARY KEY, "
        "v frozen<\"list\">);", "file:///quoted-type.cql3");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered() ||
        !document->parseResult.diagnostics.empty()) return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    const auto* root = document->parseResult.value_as<cql3::ast::Root>();
    const auto* table = root && root->statements.size() == 2
                            ? cursiv::ast_cast<cql3::ast::CreateTable>(root->statements[1])
                            : nullptr;
    if (!diagnostics.empty() || !table || table->columns.size() != 2 ||
        table->columns[1]->type->params.size() != 1 ||
        !table->columns[1]->type->params[0]->quoted) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      cql3::Cql3CoreServices, cql3::Cql3SharedCoreServices,
      cql3::install_core_module>();

  if (!cql3_json::run(language.language)) return 1;
  if (!cql3_terms_selectors::run(language.language)) return 1;

  const fs::path dir = CURSIV_EXAMPLE_DIR;
  int failures = 0;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(dir / "example")) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cql3")
      continue;
    ++count;

    const std::string text = read(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(entry.path(), "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<cql3::ast::Root>) {
      failures += fail(entry.path(), "the parse built no Root");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "cql3: no .cql3 file found under example/\n");
    return 1;
  }

  failures += check_create_table(language, dir);
  failures += check_empty_braces(language);
  failures += check_coverage(language, CURSIV_TEST_DIR);

  if (failures > 0) return 1;

  std::printf("cql3: %d example documents parse clean\n", count);
  return 0;
}
