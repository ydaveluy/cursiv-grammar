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
