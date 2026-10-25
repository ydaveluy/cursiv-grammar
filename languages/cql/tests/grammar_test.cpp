// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Cql at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cql/core/CoreServices.hpp"
#include "cql/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "cql: %s: %s\n", file.string().c_str(), what);
  return 1;
}

int fail(const char* what) {
  std::fprintf(stderr, "cql: %s\n", what);
  return 1;
}

const cql::ast::BooleanExpr* as_boolean_expr(const cql::ast::Clause* node) {
  return (node != nullptr &&
          node->type == &cursiv::ast_type_tag<cql::ast::BooleanExpr>)
             ? static_cast<const cql::ast::BooleanExpr*>(node)
             : nullptr;
}

// The document text must outlive every read of the tree below, so it stays
// local to the caller, never returned from here.
const cql::ast::Clause* top_clause(const cursiv::ParseResult& result) {
  if (result.cst().recovered() || result.value == nullptr ||
      result.value->type != &cursiv::ast_type_tag<cql::ast::Cql>)
    return nullptr;
  return static_cast<const cql::ast::Cql*>(result.value)->clause;
}

// CQL gives AND/OR/NOT/PROX no relative precedence: every boolean group
// chains at one level, strictly left to right (searchRetrieve: Part 5,
// CQL v1.2, section 3.7, "Boolean operators all have the same precedence;
// they are evaluated left-to-right").
int check_boolean_left_to_right(const auto& language) {
  {
    const std::string text = "a AND b OR c";
    const auto* top =
        as_boolean_expr(top_clause(language.language.parser->parse(text)));
    if (top == nullptr) return fail("'a AND b OR c' does not parse to a BooleanExpr");
    if (top->group->op != "OR") return fail("'a AND b OR c' is not top-level OR");
    const auto* left = as_boolean_expr(top->left);
    if (left == nullptr || left->group->op != "AND")
      return fail("'a AND b OR c' left operand is not 'a AND b'");
  }
  {
    const std::string text = "a OR b AND c";
    const auto* top =
        as_boolean_expr(top_clause(language.language.parser->parse(text)));
    if (top == nullptr) return fail("'a OR b AND c' does not parse to a BooleanExpr");
    if (top->group->op != "AND") return fail("'a OR b AND c' is not top-level AND");
    const auto* left = as_boolean_expr(top->left);
    if (left == nullptr || left->group->op != "OR")
      return fail("'a OR b AND c' left operand is not 'a OR b'");
  }
  {
    // NOT is binary (set difference), never a unary prefix: `a NOT b` is
    // its own BooleanExpr, not a marker on `b`.
    const std::string text = "a NOT b AND c";
    const auto* top =
        as_boolean_expr(top_clause(language.language.parser->parse(text)));
    if (top == nullptr) return fail("'a NOT b AND c' does not parse to a BooleanExpr");
    if (top->group->op != "AND") return fail("'a NOT b AND c' is not top-level AND");
    const auto* left = as_boolean_expr(top->left);
    if (left == nullptr || left->group->op != "NOT")
      return fail("'a NOT b AND c' left operand is not 'a NOT b'");
  }
  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      cql::CqlCoreServices, cql::CqlSharedCoreServices,
      cql::install_core_module>();

  int failures = 0;
  if (const int rc = check_boolean_left_to_right(language); rc != 0)
    return rc;

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".cql")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

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
        root->type != &cursiv::ast_type_tag<cql::ast::Cql>) {
      failures += fail(entry.path(), "the parse built no Cql");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "cql: no .cql file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("cql: %d example documents parse clean\n", count);
  return 0;
}
