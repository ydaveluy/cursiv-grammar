#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "promql/core/CoreServices.hpp"
#include "promql/core/ast.hpp"
#include "audit_regressions.hpp"
// The grammar end to end: every document under example/ parses clean
// (recovered() false, no diagnostics, the right root type), and a few real
// field values come back out of one document's AST.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "promql/generated/Promql/Module.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const std::string& what) {
  std::fprintf(stderr, "promql: %s\n", what.c_str());
  return 1;
}

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      promql::generated::Promql::create_parser();
  return *p;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<promql::PromqlCoreServices,
                              promql::PromqlSharedCoreServices,
                              promql::install_core_module>();
  {
    const auto r = language.language.parser->parse("foo @ 123 + bar");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* q = r.value_as<promql::ast::Query>();
    const auto* bin =
        q ? cursiv::ast_cast<promql::ast::BinaryExpr>(q->expression) : nullptr;
    if (!bin || !cursiv::ast_cast<promql::ast::Timestamped>(bin->left))
      return 1;
  }
  for (const char* text : {"foo @ 123 ^ bar", "foo ^ bar @ 123"}) {
    const auto r = language.language.parser->parse(text);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* query = r.value_as<promql::ast::Query>();
    const auto* power =
        query ? cursiv::ast_cast<promql::ast::BinaryExpr>(query->expression)
              : nullptr;
    if (!power || power->op != promql::ast::Operator::pow) return 1;
    const bool left = std::string_view(text).starts_with("foo @");
    if (!cursiv::ast_cast<promql::ast::Timestamped>(left ? power->left
                                                         : power->right))
      return 1;
  }
  {
    const auto r = language.language.parser->parse("foo @ 123 ^ -2");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* query = r.value_as<promql::ast::Query>();
    const auto* power =
        query ? cursiv::ast_cast<promql::ast::BinaryExpr>(query->expression)
              : nullptr;
    if (!power || !cursiv::ast_cast<promql::ast::Timestamped>(power->left) ||
        !cursiv::ast_cast<promql::ast::UnaryExpr>(power->right))
      return 1;
  }
  {
    const auto r = language.language.parser->parse("foo @ start() offset 5m");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "sum(foo) @ 1", "file:///audit.promql");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "foo @ timestamp(bar)", "file:///audit.promql");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  for (const char* text : {"foo[up]", "foo offset up"}) {
    auto doc = language.shared->documentFactory->fromString(text, "file:///duration.promql");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty()) return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

int main() {
  if (!promql_audit::run(engine())) return 1;
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  std::vector<fs::path> docs;
  // CURSIV_TEST_DIR holds documents written for this port, exercising
  // constructs the upstream corpus never reaches; CURSIV_EXAMPLE_DIR is the
  // corpus itself.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".promql")
        docs.push_back(entry.path());
  if (docs.empty()) return fail("no example documents found");

  fs::path readback_doc;
  for (const fs::path& doc : docs) {
    const std::string text = read_file(doc);
    const cursiv::ParseResult result = engine().parse(text);
    if (!result.accepted)
      return fail(doc.string() + ": the parse was refused");
    if (result.cst().recovered())
      return fail(doc.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(doc.string() + ": the parse left diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<promql::ast::Query>)
      return fail(doc.string() + ": the parse built no query");
    if (doc.filename() == "vector_match_operation.promql")
      readback_doc = doc;
  }

  if (readback_doc.empty())
    return fail("vector_match_operation.promql was not found among the examples");

  // Read real field values out of one document's AST: a converter or a
  // grouping that silently misfires would still leave `recovered()` false
  // and the root type right, so only reading the fields back catches it.
  //
  //   max by (very, second)(foo_metric{very=~'important', something='else'})
  //     > 0 AND ON(host,project)(metric_name) > 0
  const std::string text = read_file(readback_doc);
  const cursiv::ParseResult result = engine().parse(text);
  const auto& query = *static_cast<const promql::ast::Query*>(result.value);

  if (query.expression == nullptr ||
      query.expression->type != &cursiv::ast_type_tag<promql::ast::BinaryExpr>)
    return fail("vector_match_operation: expected a binary expression");
  const auto& top =
      *static_cast<const promql::ast::BinaryExpr*>(query.expression);
  if (top.op != promql::ast::Operator::logical_and)
    return fail("vector_match_operation: expected AND at the top");
  if (top.matching == nullptr || top.matching->ignoring ||
      top.matching->labels.size() != 2 || top.matching->labels[0] != "host" ||
      top.matching->labels[1] != "project")
    return fail("vector_match_operation: wrong ON(...) matching clause");

  if (top.left == nullptr ||
      top.left->type != &cursiv::ast_type_tag<promql::ast::BinaryExpr>)
    return fail("vector_match_operation: expected a comparison on the left");
  const auto& leftCompare = *static_cast<const promql::ast::BinaryExpr*>(top.left);
  if (leftCompare.op != promql::ast::Operator::gt)
    return fail("vector_match_operation: expected > on the left");
  if (leftCompare.left == nullptr ||
      leftCompare.left->type != &cursiv::ast_type_tag<promql::ast::AggregationExpr>)
    return fail("vector_match_operation: expected an aggregation");
  const auto& agg =
      *static_cast<const promql::ast::AggregationExpr*>(leftCompare.left);
  if (agg.op != "max" || !agg.hasGrouping || agg.without ||
      agg.groupBy.size() != 2 || agg.groupBy[0] != "very" ||
      agg.groupBy[1] != "second")
    return fail("vector_match_operation: wrong aggregation clause");
  if (agg.args.size() != 1 ||
      agg.args[0]->type != &cursiv::ast_type_tag<promql::ast::Selector>)
    return fail("vector_match_operation: wrong aggregation argument");
  const auto& sel = *static_cast<const promql::ast::Selector*>(agg.args[0]);
  if (sel.metric != "foo_metric" || sel.matchers.size() != 2)
    return fail("vector_match_operation: wrong selector");
  if (sel.matchers[0]->name != "very" ||
      sel.matchers[0]->op != promql::ast::Operator::re ||
      sel.matchers[0]->value != "important")
    return fail("vector_match_operation: wrong first label matcher");
  if (sel.matchers[1]->name != "something" ||
      sel.matchers[1]->op != promql::ast::Operator::eq ||
      sel.matchers[1]->value != "else")
    return fail("vector_match_operation: wrong second label matcher");

  if (top.right == nullptr ||
      top.right->type != &cursiv::ast_type_tag<promql::ast::BinaryExpr>)
    return fail("vector_match_operation: expected a comparison on the right");
  const auto& rightCompare = *static_cast<const promql::ast::BinaryExpr*>(top.right);
  if (rightCompare.op != promql::ast::Operator::gt ||
      rightCompare.left == nullptr ||
      rightCompare.left->type != &cursiv::ast_type_tag<promql::ast::Selector>)
    return fail("vector_match_operation: wrong right-hand comparison");
  const auto& rightSel =
      *static_cast<const promql::ast::Selector*>(rightCompare.left);
  if (rightSel.metric != "metric_name")
    return fail("vector_match_operation: wrong right-hand metric name");

  std::printf("promql: %zu documents parse clean\n", docs.size());
  return 0;
}
