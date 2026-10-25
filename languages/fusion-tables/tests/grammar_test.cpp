// The grammar's smoke test: every document under tests/ (the upstream
// corpus is empty) parses clean, and real field values read
// back -- a converter that silently gave the wrong value would still pass a
// structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "fusiontables/core/CoreServices.hpp"
#include "fusiontables/core/ast.hpp"

namespace {

namespace ast = fusiontables::ast;

int fail(const char* what) {
  std::fprintf(stderr, "fusiontables: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      fusiontables::FusionTablesCoreServices,
      fusiontables::FusionTablesSharedCoreServices,
      fusiontables::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  const char* const docs[] = {
      CURSIV_TEST_DIR "/select.fql",
      CURSIV_TEST_DIR "/ddl.fql",
      CURSIV_TEST_DIR "/dml.fql",
  };
  std::string texts[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) texts[i] = read(docs[i]);

  cursiv::ParseResult results[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) {
    if (texts[i].empty()) return fail("a document is not readable");
    results[i] = parser.parse(texts[i]);
    if (results[i].cst().recovered())
      return fail("a document does not parse clean");
    if (!results[i].diagnostics.empty())
      return fail("a document carries an unexpected diagnostic");
  }

  // select.fql
  {
    const auto* doc = as<ast::Document>(results[0].value);
    if (doc == nullptr || doc->statements.size() != 12)
      return fail("select: wrong statement count");

    const auto* s0 = as<ast::SelectStmt>(doc->statements[0]);
    if (s0 == nullptr || s0->items.size() != 1 || !s0->items[0]->star ||
        s0->from == nullptr || s0->from->name != "Cities")
      return fail("select[0]: the star select did not read back");

    const auto* s1 = as<ast::SelectStmt>(doc->statements[1]);
    if (s1 == nullptr || s1->items.size() != 3)
      return fail("select[1]: wrong item count");
    if (s1->items[0]->column == nullptr ||
        s1->items[0]->column->table != "Cities" ||
        s1->items[0]->column->column != "Name")
      return fail("select[1]: the qualified column did not read back");
    if (s1->items[1]->tableStar != "Cities")
      return fail("select[1]: the table-star column did not read back");
    if (s1->items[2]->aggregate == nullptr ||
        s1->items[2]->aggregate->func != ast::AggregateFunc::Count ||
        s1->items[2]->aggregate->column->column != "Population")
      return fail("select[1]: the aggregate call did not read back");
    if (s1->from->name != "Cities" || s1->from->alias != "c")
      return fail("select[1]: the table alias did not read back");
    if (s1->joins.size() != 1 || s1->joins[0]->table->name != "Countries" ||
        s1->joins[0]->left->column != "CountryId" ||
        s1->joins[0]->right->table != "Countries")
      return fail("select[1]: the join did not read back");
    const auto* where1 = as<ast::BinaryExpr>(s1->where);
    if (where1 == nullptr || where1->op != ast::LogicalOp::And)
      return fail("select[1]: the AND chain did not read back");
    if (as<ast::CompareCond>(where1->left) == nullptr ||
        as<ast::CompareCond>(where1->left)->op != ast::CompareOp::Greater)
      return fail("select[1]: the compare condition did not read back");
    if (as<ast::TextCond>(where1->right) == nullptr ||
        as<ast::TextCond>(where1->right)->op != ast::TextOp::Like)
      return fail("select[1]: the LIKE condition did not read back");
    if (s1->groupBy.size() != 2 || s1->groupBy[1]->column != "CountryId")
      return fail("select[1]: group by did not read back");
    if (s1->orderBy.size() != 2 || s1->orderBy[0]->distanceFrom == nullptr ||
        s1->orderBy[0]->distanceFrom->lat != 37.7 ||
        s1->orderBy[0]->distanceFrom->lng != -122.4 ||
        !s1->orderBy[0]->descending)
      return fail("select[1]: the ST_DISTANCE order term did not read back");
    if (s1->orderBy[1]->distanceFrom != nullptr || s1->orderBy[1]->descending)
      return fail("select[1]: the plain ASC order term did not read back");
    if (!s1->offset.has_value() || *s1->offset != 10.0 ||
        !s1->limit.has_value() || *s1->limit != 20.0)
      return fail("select[1]: offset/limit did not read back");

    const auto* s2 = as<ast::SelectStmt>(doc->statements[2]);
    const auto* where2 = as<ast::BinaryExpr>(s2->where);
    if (where2 == nullptr || where2->op != ast::LogicalOp::Or ||
        as<ast::IntersectsCond>(where2->left) == nullptr ||
        as<ast::IntersectsCond>(where2->right) == nullptr)
      return fail("select[2]: the geometry OR chain did not read back");
    const auto* circle =
        as<ast::Circle>(as<ast::IntersectsCond>(where2->left)->shape);
    if (circle == nullptr || circle->radius != 5.0)
      return fail("select[2]: the circle did not read back");
    const auto* rect =
        as<ast::Rectangle>(as<ast::IntersectsCond>(where2->right)->shape);
    if (rect == nullptr || rect->corner2->lat != 10.0)
      return fail("select[2]: the rectangle did not read back");

    const auto* s3 = as<ast::SelectStmt>(doc->statements[3]);
    const auto* in3 = as<ast::InCond>(s3->where);
    if (in3 == nullptr || in3->values.size() != 3 ||
        in3->values[2]->value != "east")
      return fail("select[3]: IN did not read back");

    const auto* s4 = as<ast::SelectStmt>(doc->statements[4]);
    const auto* between4 = as<ast::BetweenCond>(s4->where);
    if (between4 == nullptr ||
        as<ast::NumberLit>(between4->low)->value != 1.0 ||
        as<ast::NumberLit>(between4->high)->value != 100.0)
      return fail("select[4]: BETWEEN did not read back");

    constexpr ast::TextOp kOps[] = {
        ast::TextOp::StartsWith,           ast::TextOp::EndsWith,
        ast::TextOp::ContainsIgnoringCase, ast::TextOp::DoesNotContain,
        ast::TextOp::NotEqualTo,           ast::TextOp::Matches,
        ast::TextOp::Like,
    };
    for (std::size_t i = 0; i < std::size(kOps); ++i) {
      const auto* s = as<ast::SelectStmt>(doc->statements[5 + i]);
      const auto* cond = s == nullptr ? nullptr : as<ast::TextCond>(s->where);
      if (cond == nullptr || cond->op != kOps[i])
        return fail("select: a text-condition operator did not read back");
    }
  }

  // ddl.fql
  {
    const auto* doc = as<ast::Document>(results[1].value);
    if (doc == nullptr || doc->statements.size() != 6)
      return fail("ddl: wrong statement count");

    const auto* alter = as<ast::AlterTableStmt>(doc->statements[0]);
    if (alter == nullptr || alter->table != "OldCities" ||
        alter->newName != "Cities")
      return fail("ddl[0]: ALTER TABLE did not read back");

    const auto* view = as<ast::CreateViewStmt>(doc->statements[1]);
    if (view == nullptr || view->name != "BigCities" ||
        view->items.size() != 2 || view->from->name != "Cities")
      return fail("ddl[1]: CREATE VIEW did not read back");
    if (as<ast::CompareCond>(view->where) == nullptr)
      return fail("ddl[1]: the view's WHERE did not read back");
    if (view->joins.size() != 1 || view->joins[0]->table->name != "Countries")
      return fail("ddl[1]: the view's JOIN did not read back");

    const auto* cta = as<ast::CreateTableAsSelectStmt>(doc->statements[2]);
    if (cta == nullptr || cta->name != "Snapshot" || cta->from != "Cities")
      return fail("ddl[2]: CREATE TABLE AS SELECT did not read back");

    const auto* drop = as<ast::DropTableStmt>(doc->statements[3]);
    if (drop == nullptr || drop->table != "OldSnapshot")
      return fail("ddl[3]: DROP TABLE did not read back");

    const auto* describe = as<ast::DescribeStmt>(doc->statements[4]);
    if (describe == nullptr || describe->table != "Cities")
      return fail("ddl[4]: DESCRIBE did not read back");

    if (as<ast::ShowTablesStmt>(doc->statements[5]) == nullptr)
      return fail("ddl[5]: SHOW TABLES did not read back");
  }

  // dml.fql
  {
    const auto* doc = as<ast::Document>(results[2].value);
    if (doc == nullptr || doc->statements.size() != 7)
      return fail("dml: wrong statement count");

    const auto* ins0 = as<ast::InsertStmt>(doc->statements[0]);
    if (ins0 == nullptr || ins0->table != "Cities" ||
        ins0->columns.size() != 3 || ins0->columns[1] != "Population" ||
        ins0->values.size() != 3 ||
        as<ast::StringLit>(ins0->values[0])->value != "San Francisco" ||
        as<ast::NumberLit>(ins0->values[1])->value != 883305.0)
      return fail("dml[0]: INSERT did not read back");

    const auto* upd = as<ast::UpdateStmt>(doc->statements[1]);
    if (upd == nullptr || upd->table != "Cities" ||
        upd->assignments.size() != 2 ||
        upd->assignments[0]->column != "Population" ||
        as<ast::NumberLit>(upd->assignments[0]->value)->value != 883305.0 ||
        upd->assignments[1]->column != "Region" ||
        as<ast::StringLit>(upd->assignments[1]->value)->value != "west" ||
        upd->whereColumn != "Name" || upd->whereValue->value != "San Francisco")
      return fail("dml[1]: UPDATE did not read back");

    const auto* del2 = as<ast::DeleteStmt>(doc->statements[2]);
    if (del2 == nullptr || del2->whereColumn != "Name" ||
        as<ast::StringLit>(del2->whereValue)->value != "Obsolete City")
      return fail("dml[2]: DELETE with a string WHERE did not read back");

    const auto* del3 = as<ast::DeleteStmt>(doc->statements[3]);
    if (del3 == nullptr || del3->whereColumn != "Population" ||
        as<ast::NumberLit>(del3->whereValue) == nullptr ||
        as<ast::NumberLit>(del3->whereValue)->value != 100.0)
      return fail("dml[3]: DELETE with a numeric WHERE did not read back");

    const auto* del4 = as<ast::DeleteStmt>(doc->statements[4]);
    if (del4 == nullptr || del4->table != "TempTable" ||
        !del4->whereColumn.empty() || del4->whereValue != nullptr)
      return fail("dml[4]: DELETE without WHERE did not read back");

    const auto* ins5 = as<ast::InsertStmt>(doc->statements[5]);
    if (ins5 == nullptr ||
        as<ast::StringLit>(ins5->values[0])->value != "O'Fallon")
      return fail("dml[5]: the doubled-quote escape did not read back");

    const auto* ins6 = as<ast::InsertStmt>(doc->statements[6]);
    if (ins6 == nullptr || ins6->table != "my-table" ||
        ins6->columns[0] != "col-a" || ins6->columns[1] != "col-b" ||
        as<ast::NumberLit>(ins6->values[0])->value != -3.5 ||
        as<ast::NumberLit>(ins6->values[1])->value != 42.0)
      return fail("dml[6]: dashed identifiers/signed numbers did not read back");
  }

  // A statement missing its result column list must not parse clean.
  {
    const cursiv::ParseResult result = parser.parse("SELECT FROM Cities;");
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("a select with no columns was accepted as clean");
  }

  // A block comment never closed runs to the end of the text, as the
  // reference's `('*/' | EOF)` reads it.
  {
    const cursiv::ParseResult result =
        parser.parse("SELECT * FROM Cities; /* unterminated");
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("an unterminated block comment did not close at the end");
  }

  std::puts("fusiontables: every document parses, field values read back correctly");
  return 0;
}
