// The smoke test of the language: parse every corpus example, and check
// that a document's fields read back the values it declares.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "key_segments.hpp"
#include "query_axis_cases.hpp"
#include "mdx/core/CoreServices.hpp"
#include "mdx/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "mdx: %s\n", what.c_str());
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const std::vector<const char*> kExamples = {
    CURSIV_EXAMPLE_DIR "/example1.mdx",
    CURSIV_EXAMPLE_DIR "/example2.mdx",
    CURSIV_EXAMPLE_DIR "/example3.mdx",
};

// Constructs the corpus above never exercises: decimal numbers, CASE, the
// full logical/comparison/range ladder, NON EMPTY, DIMENSION/CELL
// PROPERTIES, WITH MEMBER/SET (`constructs.mdx`); function and chained
// method calls (`calls.mdx`); an ampersand-quoted key segment (`amp.mdx`);
// a quote-wrapped formula (`quote.mdx`).
const std::vector<const char*> kOwnDocs = {
    CURSIV_TEST_DIR "/constructs.mdx",
    CURSIV_TEST_DIR "/calls.mdx",
    CURSIV_TEST_DIR "/amp.mdx",
    CURSIV_TEST_DIR "/quote.mdx",
};

}  // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<mdx::MdxCoreServices,
                                mdx::MdxSharedCoreServices,
                                mdx::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "select {[Date].[Year].&[2003]} on 0 from [Cube]\n");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language =
      cursiv::create_language<mdx::MdxCoreServices, mdx::MdxSharedCoreServices,
                              mdx::install_core_module>();

  if (!check_key_segments(language.language))
    return fail("key segment content/range/parent mismatch");
  if (!mdx_query_axes::run(language.language)) return 1;

  for (const char* const path : kExamples) {
    const std::string text = read_file(path);
    if (text.empty()) return fail("an example is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("an example does not parse");
    if (!result.diagnostics.empty())
      return fail("an example carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mdx::ast::SelectStatement>)
      return fail("the parse built no select statement");
  }

  for (const char* const path : kOwnDocs) {
    const std::string text = read_file(path);
    if (text.empty()) return fail("a construct document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      for (const auto& d : result.diagnostics) std::fprintf(stderr, "%s: repair kind=%u at=%u:%u\n", path, static_cast<unsigned>(d.kind), d.begin, d.end);
      return fail("a construct document does not parse");
    }
    if (!result.diagnostics.empty())
      return fail("a construct document carries diagnostics");
  }

  // example1.mdx: two axes (COLUMNS, ROWS), a cube FROM Sales, a slicer.
  {
    const std::string text = read_file(kExamples[0]);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto& select =
        *static_cast<const mdx::ast::SelectStatement*>(result.value);

    if (select.axes.size() != 2) return fail("example1: expected two axes");
    if (select.axes[0]->axisName != "COLUMNS")
      return fail("example1: first axis is not COLUMNS");
    if (select.axes[1]->axisName != "ROWS")
      return fail("example1: second axis is not ROWS");
    if (select.cube == nullptr || select.cube->parts.size() != 1 ||
        select.cube->parts[0] != "Sales")
      return fail("example1: cube is not Sales");
    if (select.slicer == nullptr) return fail("example1: expected a slicer");
  }

  // Microsoft's operator precedence table: `*` tighter than `+` tighter
  // than `=`, so `1 + 2 * 3 = 7` groups as `(1 + (2 * 3)) = 7`.
  {
    const std::string text = "SELECT FROM Sales WHERE 1 + 2 * 3 = 7";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("1 + 2 * 3 = 7: does not parse clean");
    const auto& select =
        *static_cast<const mdx::ast::SelectStatement*>(result.value);
    const auto* const eq =
        cursiv::ast_cast<mdx::ast::BinaryExpr>(select.slicer);
    if (eq == nullptr || eq->op != mdx::ast::BinaryOp::eq)
      return fail("1 + 2 * 3 = 7: expected = at the root");
    const auto* const add = cursiv::ast_cast<mdx::ast::BinaryExpr>(eq->left);
    if (add == nullptr || add->op != mdx::ast::BinaryOp::add)
      return fail("1 + 2 * 3 = 7: expected + under =");
    const auto* const mul = cursiv::ast_cast<mdx::ast::BinaryExpr>(add->right);
    if (mul == nullptr || mul->op != mdx::ast::BinaryOp::multiply)
      return fail("1 + 2 * 3 = 7: expected * under +");
  }

  // Microsoft's table: AND tighter than XOR tighter than OR, each its own
  // level, so `1 AND 2 XOR 3 OR 4` groups as `((1 AND 2) XOR 3) OR 4`.
  {
    const std::string text = "SELECT FROM Sales WHERE 1 AND 2 XOR 3 OR 4";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("1 AND 2 XOR 3 OR 4: does not parse clean");
    const auto& select =
        *static_cast<const mdx::ast::SelectStatement*>(result.value);
    const auto* const or_ =
        cursiv::ast_cast<mdx::ast::BinaryExpr>(select.slicer);
    if (or_ == nullptr || or_->op != mdx::ast::BinaryOp::logical_or)
      return fail("1 AND 2 XOR 3 OR 4: expected OR at the root");
    const auto* const xor_ = cursiv::ast_cast<mdx::ast::BinaryExpr>(or_->left);
    if (xor_ == nullptr || xor_->op != mdx::ast::BinaryOp::logical_xor)
      return fail("1 AND 2 XOR 3 OR 4: expected XOR under OR");
    const auto* const and_ = cursiv::ast_cast<mdx::ast::BinaryExpr>(xor_->left);
    if (and_ == nullptr || and_->op != mdx::ast::BinaryOp::logical_and)
      return fail("1 AND 2 XOR 3 OR 4: expected AND under XOR");
  }

  // Microsoft's table ranks `:` tighter than `+`, so `a:b + c` groups
  // `(a:b) + c`.
  {
    const std::string text = "SELECT FROM Sales WHERE a:b + c";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a:b + c: does not parse clean");
    const auto& select =
        *static_cast<const mdx::ast::SelectStatement*>(result.value);
    const auto* const add =
        cursiv::ast_cast<mdx::ast::BinaryExpr>(select.slicer);
    if (add == nullptr || add->op != mdx::ast::BinaryOp::add)
      return fail("a:b + c: expected + at the root");
    const auto* const range = cursiv::ast_cast<mdx::ast::BinaryExpr>(add->left);
    if (range == nullptr || range->op != mdx::ast::BinaryOp::range)
      return fail("a:b + c: expected : under +");
  }

  // CASE uses the same expression syntax; native scalar type constraints
  // require semantic analysis beyond this parser.
  {
    const std::string text =
        "SELECT FROM Sales WHERE CASE WHEN a = b THEN c ELSE d END";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a CASE without `:` does not parse clean");
    const auto& select =
        *static_cast<const mdx::ast::SelectStatement*>(result.value);
    if (cursiv::ast_cast<mdx::ast::CaseExpr>(select.slicer) == nullptr)
      return fail("a CASE branch did not build a CaseExpr");
  }
  {
    const std::string text =
        "SELECT FROM Sales WHERE CASE WHEN a:b THEN c ELSE d END";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("CASE expression should use the ordinary expression syntax; type checking is deferred");
  }

  for (const auto source : {"WITH MEMBER [Measures].[x] AS 1 SELECT [Measures].[x] ON 0 FROM Sales", "SELECT {} ON AXIS(0) FROM Sales;", "SELECT * FROM Sales", "SELECT {} ON 0 FROM (SELECT {} ON 0 FROM Sales)", "// comment\nSELECT {} ON 0 FROM Sales -- tail\n", "SELECT FROM Sales WHERE a IS b:c^2 * 3 + .5e2", "SELECT FROM Sales WHERE Measures.CurrentMember.Value"}) {
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return fail(std::string("native form rejected: ") + source);
  }
  {
    const auto result = language.language.parser->parse("WITH MEMBER [M] AS 2 SELECT {} ON AXIS(0) FROM (SELECT FROM Sales) WHERE a IS b:c^2 * 3");
    const auto* q = result.value_as<mdx::ast::SelectStatement>();
    const auto* mul = q ? cursiv::ast_cast<mdx::ast::BinaryExpr>(q->slicer) : nullptr;
    const auto* power = mul ? cursiv::ast_cast<mdx::ast::BinaryExpr>(mul->left) : nullptr;
    const auto* range = power ? cursiv::ast_cast<mdx::ast::BinaryExpr>(power->left) : nullptr;
    const auto* identity = range ? cursiv::ast_cast<mdx::ast::BinaryExpr>(range->left) : nullptr;
    if (!q || q->formulas.size() != 1 || !q->subquery || q->axes.size() != 1 || !q->axes[0]->axisFunction ||
        !mul || mul->op != mdx::ast::BinaryOp::multiply || !power || power->op != mdx::ast::BinaryOp::power ||
        !range || range->op != mdx::ast::BinaryOp::range || !identity || identity->op != mdx::ast::BinaryOp::is_)
      return fail("native formula/subquery/axis/operator AST lost");
  }
  for (const auto source : {"SELECT FROM Sales WHERE CASE END", "WITH MEMBER [x] AS 1, SELECT FROM Sales", "SELECT FROM SELECT", "SELECT FROM Sales WHERE CREATE"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) return fail("non-native keyword/CASE/member syntax accepted");
  }
  for (const auto source : {"SELECT {} ON columns, {} ON 0 FROM Sales", "SELECT {} ON 128 FROM Sales", "SELECT {} ON rows FROM Sales", "SELECT {} ON AXIS(0), {} ON COLUMNS FROM Sales", "SELECT {} ON UNKNOWN FROM Sales"}) {
    const auto result = cursiv::test::validationHelper(language.language, source);
    if (result.diagnostics.empty()) return fail("native axis constraint needs diagnostic");
  }
  std::puts("mdx: the examples parse");
  return 0;
}
