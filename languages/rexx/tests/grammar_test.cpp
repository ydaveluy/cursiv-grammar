#include "native_regressions.hpp"
#include "parse_options.hpp"
#include <cstdio>
#include <optional>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "rexx/core/CoreServices.hpp"
#include "rexx/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every example, and check the tree.

#include <fstream>
#include <iterator>
#include <string>
#include <type_traits>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int g_failures = 0;

void fail(const char* what) {
  std::fprintf(stderr, "rexx: %s\n", what);
  ++g_failures;
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

std::optional<cursiv::ParseResult> parse_clean(
    const cursiv::CoreLanguage<rexx::RexxCoreServices,
                               rexx::RexxSharedCoreServices>& language,
    const std::string& path, const std::string& text) {
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "rexx: %s does not parse clean\n", path.c_str());
    ++g_failures;
    return std::nullopt;
  }
  if (!result.diagnostics.empty()) {
    std::fprintf(stderr, "rexx: %s carries diagnostics\n", path.c_str());
    ++g_failures;
    return std::nullopt;
  }
  const rexx::ast::Block* const block = as<rexx::ast::Block>(result.value);
  if (block == nullptr) {
    std::fprintf(stderr, "rexx: %s built no block\n", path.c_str());
    ++g_failures;
  }
  return result;
}

std::string slurp(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

// test_concatenation.rex has three say_ pairs per concatenation kind, in
// this order: `||` (explicit, comment, continuation), adjacency (abuttal,
// same three variations), a blank gap (blank, same three variations).
void check_concatenation(const rexx::ast::Block& doc) {
  const rexx::ast::Concat* kinds[9] = {};
  int seen = 0;
  for (const rexx::ast::Stmt* const stmt : doc.body) {
    const auto* const say = as<rexx::ast::SayStmt>(stmt);
    if (say == nullptr || say->value == nullptr) continue;
    if (const auto* const concat = as<rexx::ast::Concat>(say->value);
        concat != nullptr && seen < 9) {
      kinds[seen++] = concat;
    }
  }
  if (seen != 9) {
    fail("test_concatenation.rex: expected nine say concatenations");
    return;
  }
  for (int i = 0; i < 3; ++i) {
    const rexx::ast::Concat& c = *kinds[i];
    if (!c.explicitOp || c.blank) fail("`||` did not read as explicit");
    if (as<rexx::ast::StringLit>(c.left) == nullptr ||
        as<rexx::ast::StringLit>(c.right) == nullptr)
      fail("explicit concatenation operands are not both strings");
  }
  for (int i = 3; i < 6; ++i) {
    const rexx::ast::Concat& c = *kinds[i];
    if (c.explicitOp || c.blank) fail("adjacency did not read as abuttal");
    if (as<rexx::ast::StringLit>(c.left) == nullptr ||
        as<rexx::ast::Call>(c.right) == nullptr)
      fail("abuttal concatenation right operand is not the Summer() call");
  }
  for (int i = 6; i < 9; ++i) {
    const rexx::ast::Concat& c = *kinds[i];
    if (c.explicitOp || !c.blank) fail("a blank gap did not read as blank");
    if (as<rexx::ast::StringLit>(c.left) == nullptr ||
        as<rexx::ast::StringLit>(c.right) == nullptr)
      fail("blank concatenation operands are not both strings");
  }
}

const rexx::ast::Assignment* find_assignment(const rexx::ast::Block& doc,
                                             std::string_view target) {
  for (const rexx::ast::Stmt* const stmt : doc.body) {
    const auto* const a = as<rexx::ast::Assignment>(stmt);
    if (a != nullptr && a->target == target) return a;
  }
  return nullptr;
}

// binary_good1's value is the binary string token itself; binary_bad1's is
// a string blank-concatenated with bare name `b`. Native-invalid bitstring
// attempts remain separate negative regressions.
void check_bitstring(const rexx::ast::Block& doc) {
  const rexx::ast::Assignment* const good1 = find_assignment(doc, "binary_good1");
  if (good1 == nullptr || as<rexx::ast::BinaryLit>(good1->value) == nullptr) {
    fail("binary_good1 is not a binary string literal");
  } else if (as<rexx::ast::BinaryLit>(good1->value)->text != "'1'b") {
    fail("binary_good1's text does not match the source");
  }

  const rexx::ast::Assignment* const bad1 = find_assignment(doc, "binary_bad1");
  const rexx::ast::Concat* const concat =
      bad1 != nullptr ? as<rexx::ast::Concat>(bad1->value) : nullptr;
  if (concat == nullptr || concat->explicitOp || !concat->blank) {
    fail("binary_bad1 is not a blank concatenation");
  } else {
    const auto* const left = as<rexx::ast::StringLit>(concat->left);
    const auto* const right = as<rexx::ast::NameRef>(concat->right);
    if (left == nullptr || left->value != "0 1 0 1")
      fail("binary_bad1's left operand is not the string '0 1 0 1'");
    if (right == nullptr || right->name != "b")
      fail("binary_bad1's right operand is not the bare name b");
  }
}

// tests/docs/coverage.rex: constructs the upstream corpus never exercises.
// A handful of readbacks, not every statement -- the point is that each
// kind of directive (create/assign/append/flag/nest) actually wrote
// what it was asked to, not a full walk of the document.
void check_coverage(const rexx::ast::Block& doc) {
  const rexx::ast::DoStmt* first_do = nullptr;
  const rexx::ast::SelectStmt* select = nullptr;
  const rexx::ast::ParseStmt* first_parse = nullptr;
  const rexx::ast::CallStmt* first_call = nullptr;
  for (const rexx::ast::Stmt* const stmt : doc.body) {
    if (first_do == nullptr) first_do = as<rexx::ast::DoStmt>(stmt);
    if (select == nullptr) select = as<rexx::ast::SelectStmt>(stmt);
    if (first_parse == nullptr) first_parse = as<rexx::ast::ParseStmt>(stmt);
    if (first_call == nullptr) first_call = as<rexx::ast::CallStmt>(stmt);
  }

  // `do i = 1 to 10 by 2 for 3`: all three of TO/BY/FOR, via the counted
  // assignment.
  if (first_do == nullptr || first_do->counter == nullptr ||
      first_do->counter->target != "i" || first_do->clauses.size() != 3) {
    fail("the first do_ does not hold its counter and to/by/for clauses");
  }

  // `select ... when ... when ... otherwise ... end`.
  if (select == nullptr || select->whens.size() != 2 || !select->hasOtherwise ||
      select->otherwiseBody == nullptr) {
    fail("select_ does not hold two whens and an otherwise");
  }

  // `call foo 1, 2` (the first CallStmt) and `call on error name handler`
  // (the fourth): a plain call's bare arg list, and an on-condition's
  // handler name.
  if (first_call == nullptr || first_call->target != "foo" ||
      first_call->args.size() != 2 || first_call->args[0]->value == nullptr ||
      first_call->args[1]->value == nullptr) {
    fail("`call foo 1, 2` does not hold two arguments");
  }
  int call_seen = 0;
  const rexx::ast::CallStmt* call_on = nullptr;
  for (const rexx::ast::Stmt* const stmt : doc.body) {
    const auto* const c = as<rexx::ast::CallStmt>(stmt);
    if (c == nullptr) continue;
    if (++call_seen == 4) call_on = c;
  }
  if (call_on == nullptr || !call_on->onCondition ||
      call_on->condition != "error" || call_on->handlerName != "handler") {
    fail("`call on error name handler` did not read as on-condition");
  }

  // `parse arg t1 . t2 'pat' (varpat) 3 =4 =(posvar) +5 +(relvar) -6
  // -(relvar2)`: every template item kind, in order.
  if (first_parse == nullptr || first_parse->templates.size() != 1) {
    fail("the first parse_ does not hold one template");
  } else {
    const rexx::ast::Template& tpl = *first_parse->templates[0];
    // t1 . t2 'pat' (varpat) 3 =4 =(posvar) +5 +(relvar) -6 -(relvar2)
    if (tpl.items.size() != 12) {
      fail("the first parse_'s template does not hold twelve items");
    } else {
      const auto kind_ok = [&](std::size_t i, auto* expected) {
        using Expected = std::remove_pointer_t<decltype(expected)>;
        return std::holds_alternative<Expected*>(tpl.items[i]);
      };
      if (!kind_ok(0, (rexx::ast::TplVar*)nullptr) ||
          !kind_ok(1, (rexx::ast::TplDrop*)nullptr) ||
          !kind_ok(2, (rexx::ast::TplVar*)nullptr) ||
          !kind_ok(3, (rexx::ast::TplStringPattern*)nullptr) ||
          !kind_ok(4, (rexx::ast::TplVarPattern*)nullptr) ||
          !kind_ok(5, (rexx::ast::TplAbsPos*)nullptr) ||
          !kind_ok(6, (rexx::ast::TplAbsPos*)nullptr) ||
          !kind_ok(7, (rexx::ast::TplAbsPosVar*)nullptr) ||
          !kind_ok(8, (rexx::ast::TplRelPos*)nullptr) ||
          !kind_ok(9, (rexx::ast::TplRelPosVar*)nullptr) ||
          !kind_ok(10, (rexx::ast::TplRelPos*)nullptr) ||
          !kind_ok(11, (rexx::ast::TplRelPosVar*)nullptr))
        fail("the template items are not the eight shapes in order");
    }
  }
}

// `f()` passes no argument; an omitted slot needs a comma beside it, so
// `f(,1)` holds two.
void check_argument_lists(const rexx::ast::Block& doc) {
  if (doc.body.size() != 5) {
    fail("the argument lists document does not hold five statements");
    return;
  }
  const auto call_args = [&](std::size_t i) -> int {
    const auto* const a = as<rexx::ast::Assignment>(doc.body[i]);
    const auto* const call = a != nullptr ? as<rexx::ast::Call>(a->value) : nullptr;
    return call != nullptr ? static_cast<int>(call->args.size()) : -1;
  };
  const auto stmt_args = [&](std::size_t i) -> int {
    const auto* const call = as<rexx::ast::CallStmt>(doc.body[i]);
    return call != nullptr ? static_cast<int>(call->args.size()) : -1;
  };
  if (call_args(0) != 0) fail("`f()` holds an argument");
  if (call_args(1) != 2) fail("`f(,1)` does not hold two slots");
  if (stmt_args(2) != 0) fail("`call g` holds an argument");
  if (stmt_args(3) != 0) fail("`call g()` holds an argument");
  if (stmt_args(4) != 2) fail("`call g ,2` does not hold two slots");
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<rexx::RexxCoreServices,
                                                rexx::RexxSharedCoreServices,
                                                rexx::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "do i = 1 for 2 by 1 to 3\nsay i\nend\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r =
        language.language.parser->parse("do i = 1 to 3 to 4\nsay i\nend\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "do i = 1 to 3 to 4\nsay i\nend\n", "file:///audit.rex");
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
      rexx::RexxCoreServices, rexx::RexxSharedCoreServices,
      rexx::install_core_module>();

  if (!rexx_native_regressions(language)) return 1;
  if (!rexx_parse_options::run(language.language)) return 1;

  const std::string example1 = CURSIV_EXAMPLE_DIR "/example1.rex";
  const std::string bitstring = CURSIV_EXAMPLE_DIR "/test_bitstring.rex";
  const std::string concatenation = CURSIV_EXAMPLE_DIR "/test_concatenation.rex";

  // Keep each ParseResult alive while reading its AST and borrowed source fields.
  {
    const std::string text = slurp(example1.c_str());
    const auto result = parse_clean(language, example1, text);
    const auto* doc = result ? result->value_as<rexx::ast::Block>() : nullptr;
    if (doc != nullptr && doc->body.empty())
      fail("example1.rex built an empty program");
  }
  {
    const std::string text = slurp(bitstring.c_str());
    const auto result = parse_clean(language, bitstring, text);
    const auto* doc = result ? result->value_as<rexx::ast::Block>() : nullptr;
    if (doc != nullptr) check_bitstring(*doc);
  }
  {
    const std::string text = slurp(concatenation.c_str());
    const auto result = parse_clean(language, concatenation, text);
    const auto* doc = result ? result->value_as<rexx::ast::Block>() : nullptr;
    if (doc != nullptr) check_concatenation(*doc);
  }
  {
    const std::string coverage = std::string(CURSIV_TEST_DIR) + "/docs/coverage.rex";
    const std::string text = slurp(coverage.c_str());
    const auto result = parse_clean(language, coverage, text);
    const auto* doc = result ? result->value_as<rexx::ast::Block>() : nullptr;
    if (doc != nullptr) check_coverage(*doc);
  }
  {
    const std::string text = "x = f()\ny = f(,1)\ncall g\ncall g()\ncall g ,2\n";
    const auto result = parse_clean(language, "argument lists", text);
    const auto* doc = result ? result->value_as<rexx::ast::Block>() : nullptr;
    if (doc != nullptr) check_argument_lists(*doc);
  }

  if (g_failures != 0) {
    std::fprintf(stderr, "rexx: %d check(s) failed\n", g_failures);
    return 1;
  }
  std::puts("rexx: the examples parse");
  return 0;
}
