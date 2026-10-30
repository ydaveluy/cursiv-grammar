// The grammar end to end: every document under example/ (the upstream
// corpus) and tests/ (this port's own coverage, never counted as corpus)
// parses clean, and one document's AST is read back to check the
// converters (StringLit, NumberLit, PathExpr).

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "jpa/core/CoreServices.hpp"
#include "jpa/core/ast.hpp"
#include "native_followup_cases.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "jpa: %s: %s\n", file.string().c_str(), what);
  return 1;
}

std::string read(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      jpa::JpaCoreServices, jpa::JpaSharedCoreServices,
      jpa::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("select e from Employee e\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      jpa::JpaCoreServices, jpa::JpaSharedCoreServices,
      jpa::install_core_module>();

  if (!jpa_native_followup::run(language)) return 1;

  // example1.jpql ("SELECT f FROM Country") has no alias -- JPQL always
  // requires one in a range_variable_declaration. Required
  // to carry a diagnostic, not merely tolerated.
  static constexpr std::string_view kExpectedFailure = "example1.jpql";

  int failures = 0;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR "/example")) {
    if (!entry.is_regular_file() || entry.path().extension() != ".jpql")
      continue;
    ++count;
    const bool wantsDiagnostic = entry.path().filename() == kExpectedFailure;

    const std::string text = read(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const bool clean = !result.cst().recovered() && result.diagnostics.empty();
    if (clean == wantsDiagnostic) {
      failures += fail(entry.path(), wantsDiagnostic
                                          ? "the document parses clean now"
                                          : "the document does not parse clean");
      continue;
    }
    if (wantsDiagnostic) continue;
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<jpa::ast::Document>) {
      failures += fail(entry.path(), "the parse built no Document");
      continue;
    }
  }
  if (count == 0) {
    std::fprintf(stderr, "jpa: no .jpql file found under example/\n");
    return 1;
  }

  // This port's own coverage documents, one statement per file: never
  // counted as corpus, so they live here rather than under example/.
  static constexpr const char* kCoverage[] = {
      "coverage_select.jpql",   "coverage_functions.jpql",
      "coverage_joins.jpql",    "coverage_subquery_alias.jpql",
      "coverage_update.jpql",   "coverage_delete.jpql",
  };
  for (const char* const name : kCoverage) {
    const fs::path path = fs::path(CURSIV_TEST_DIR) / name;
    const std::string text = read(path);
    if (text.empty()) {
      failures += fail(path, "not readable");
      continue;
    }
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
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<jpa::ast::Document>) {
      failures += fail(path, "the parse built no Document");
      continue;
    }
  }

  // Read real field values back out of the select coverage document: a
  // converter that silently yields the wrong value passes every structural
  // check above.
  {
    const fs::path path = fs::path(CURSIV_TEST_DIR) / "coverage_select.jpql";
    const std::string text = read(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& doc = *static_cast<const jpa::ast::Document*>(result.value);
    if (doc.statement == nullptr ||
        doc.statement->type != &cursiv::ast_type_tag<jpa::ast::SelectStatement>) {
      failures += fail(path, "the statement is not a SelectStatement");
    } else {
      const auto& select =
          *static_cast<const jpa::ast::SelectStatement*>(doc.statement);
      if (!select.distinct) failures += fail(path, "DISTINCT was not read");
      if (select.from.empty() ||
          select.from[0]->type != &cursiv::ast_type_tag<jpa::ast::IdVarDecl>) {
        failures += fail(path, "the from-clause did not build an IdVarDecl");
      } else {
        const auto& idvar =
            *static_cast<const jpa::ast::IdVarDecl*>(select.from[0]);
        if (idvar.range == nullptr || idvar.range->schemaName != "Order" ||
            idvar.range->alias != "o") {
          failures += fail(path, "the range variable was read wrong");
        }
        if (idvar.joins.size() != 1 || idvar.joins[0]->alias != "i" ||
            idvar.joins[0]->kind != jpa::ast::JoinKind::left) {
          failures += fail(path, "the join step was read wrong");
        }
      }
      if (select.orderBy.size() != 1 || !select.orderBy[0]->descending) {
        failures += fail(path, "ORDER BY ... DESC was read wrong");
      }
    }
  }

  // comparison_expression is one relation, not a chain (Jakarta Persistence
  // Query Language BNF): `o.a = o.b = o.c` must carry a diagnostic, the
  // same way `o.a = o.b AND o.b = o.c` would have to be spelled out.
  {
    const std::string text = "SELECT o FROM Order o WHERE o.a = o.b = o.c";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.cst().recovered() && result.diagnostics.empty())
      failures += fail("<chained comparison>",
                        "o.a = o.b = o.c parses clean, should not chain");
  }

  if (failures > 0) return 1;

  std::printf("jpa: %d example documents and %zu coverage documents parse clean\n",
              count, sizeof(kCoverage) / sizeof(kCoverage[0]));
  return 0;
}
