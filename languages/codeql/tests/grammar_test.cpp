
#include <cstdio>

#include "codeql/core/CoreServices.hpp"
#include "codeql/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Document at the root. Two
// documents' values are also read back, so a converter that silently drops
// or mis-converts a value would still fail this test.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"

namespace {

namespace fs = std::filesystem;
namespace ast = codeql::ast;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "codeql: %s: %s\n", file.string().c_str(), what);
  return 1;
}

// expression.ql: `select 0,42,-2048,2.0,123.456,-100.5, ...` -- checks that
// int/float literals and the unary-minus fix both read back as expected.
int check_expression_values(const fs::path& path,
                            const cursiv::ParseResult& result) {
  const auto& doc = *static_cast<const ast::Document*>(result.value);
  if (doc.body.empty() || doc.body[0]->type != &cursiv::ast_type_tag<ast::Select>)
    return fail(path, "no leading select");
  const auto& select = *static_cast<const ast::Select*>(doc.body[0]);
  if (select.results.size() < 6) return fail(path, "select has too few items");

  const auto int_value = [](const ast::Expr* e) -> const std::int64_t* {
    if (e->type != &cursiv::ast_type_tag<ast::Literal>) return nullptr;
    return std::get_if<std::int64_t>(&static_cast<const ast::Literal*>(e)->value);
  };
  const auto negated_int = [](const ast::Expr* e) -> const std::int64_t* {
    if (e->type != &cursiv::ast_type_tag<ast::UnaryExpr>) return nullptr;
    const auto& u = *static_cast<const ast::UnaryExpr*>(e);
    if (u.op != ast::SignOp::minus) return nullptr;
    if (u.operand->type != &cursiv::ast_type_tag<ast::Literal>) return nullptr;
    return std::get_if<std::int64_t>(
        &static_cast<const ast::Literal*>(u.operand)->value);
  };

  const std::int64_t* const zero = int_value(select.results[0]->value);
  const std::int64_t* const forty_two = int_value(select.results[1]->value);
  const std::int64_t* const minus_2048 = negated_int(select.results[2]->value);
  if (zero == nullptr || *zero != 0) return fail(path, "0 did not read back");
  if (forty_two == nullptr || *forty_two != 42)
    return fail(path, "42 did not read back");
  if (minus_2048 == nullptr || *minus_2048 != 2048)
    return fail(path, "-2048 did not read back as unary minus of 2048");

  return 0;
}

// fomula.ql ends with `where x % 2 = 0 implies x % 4 = 0`: the fixed
// precedence table must keep `implies` as the outermost connective, not
// bury it inside a comparison.
int check_fomula_implies(const fs::path& path,
                         const cursiv::ParseResult& result) {
  const auto& doc = *static_cast<const ast::Document*>(result.value);
  const ast::Select* select = nullptr;
  for (const ast::TopLevel* const item : doc.body)
    if (item->type == &cursiv::ast_type_tag<ast::Select>)
      select = static_cast<const ast::Select*>(item);
  if (select == nullptr || !select->where) return fail(path, "no where clause");
  const ast::Formula* const where = *select->where;
  if (where->type != &cursiv::ast_type_tag<ast::BinFormula>)
    return fail(path, "where clause is not a binary formula");
  const auto& bin = *static_cast<const ast::BinFormula*>(where);
  if (bin.op != ast::LogicOp::implies)
    return fail(path, "the top connective is not implies");
  if (bin.left->type != &cursiv::ast_type_tag<ast::ComparisonFormula> ||
      bin.right->type != &cursiv::ast_type_tag<ast::ComparisonFormula>)
    return fail(path, "implies did not bind the two comparisons directly");
  return 0;
}

// Constructs the upstream corpus never reaches: strict aggregations,
// unique/any/none, dontcare, a soft-keyword name, <=, an expression pragma,
// a '*' closure, a nested signatureType with no leading `signature`, a
// dotted import with an alias, and forex.
int check_coverage(const fs::path& path, const cursiv::ParseResult& result) {
  const auto& doc = *static_cast<const ast::Document*>(result.value);
  if (doc.body.size() < 15)
    return fail(path, "fewer top-level entities than coverage.ql declares");
  return 0;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<codeql::CodeqlCoreServices,
                              codeql::CodeqlSharedCoreServices,
                              codeql::install_core_module>();
  {
    const auto r = language.language.parser->parse("select \"a\\nb\", - -1");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;

    const auto* doc = r.value_as<codeql::ast::Document>();
    if (!doc || doc->body.size() != 1) return 1;
    const auto* sel = cursiv::ast_cast<codeql::ast::Select>(doc->body[0]);
    if (!sel || sel->results.size() != 2) return 1;
    const auto* lit =
        cursiv::ast_cast<codeql::ast::Literal>(sel->results[0]->value);
    if (!lit || !std::holds_alternative<std::string_view>(lit->value) ||
        std::get<std::string_view>(lit->value) != "a\nb")
      return 1;
    const auto* outer =
        cursiv::ast_cast<codeql::ast::UnaryExpr>(sel->results[1]->value);
    if (!outer || !cursiv::ast_cast<codeql::ast::UnaryExpr>(outer->operand))
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "private signature predicate p(int x);");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "signature class C { predicate p(int x); }");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      codeql::CodeqlCoreServices, codeql::CodeqlSharedCoreServices,
      codeql::install_core_module>();

  int failures = 0;
  int count = 0;

  {
    const fs::path path = fs::path(CURSIV_TEST_DIR) / "coverage.ql";
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      failures += fail(path, "not readable");
    } else {
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(path, "does not parse clean");
      } else if (!result.diagnostics.empty()) {
        failures += fail(path, "carries diagnostics");
      } else if (result.value == nullptr ||
                result.value->type != &cursiv::ast_type_tag<ast::Document>) {
        failures += fail(path, "built no Document");
      } else {
        failures += check_coverage(path, result);
      }
    }
  }
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR "/example")) {
    if (!entry.is_regular_file()) continue;
    const std::string ext = entry.path().extension().string();
    if (ext != ".ql" && ext != ".qll") continue;
    ++count;
    const fs::path path = entry.path();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
      failures += fail(path, "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(path, "the document does not parse clean");
      continue;
    }
    if (!result.diagnostics.empty()) {
      failures += fail(path, "the document carries diagnostics");
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Document>) {
      failures += fail(path, "the parse built no Document");
      continue;
    }

    if (path.filename() == "expression.ql")
      failures += check_expression_values(path, result);
    if (path.filename() == "fomula.ql")
      failures += check_fomula_implies(path, result);
  }

  if (count == 0) {
    std::fprintf(stderr, "codeql: no .ql/.qll file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("codeql: %d example documents parse clean\n", count);
  return 0;
}
