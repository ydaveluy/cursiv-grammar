#include "native_followup.hpp"
#include "audit_regressions.hpp"
// The grammar end to end: every document under example/ parses clean (no
// recovery, no diagnostics, a Module root), and one document's AST is read
// back field by field.

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
#include "vb6/core/CoreServices.hpp"
#include "vb6/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int failures = 0;

void fail(const std::string& what) {
  std::fprintf(stderr, "vb6: %s\n", what.c_str());
  ++failures;
}

std::string read_file(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

using Language = decltype(cursiv::create_language<
                           vb6::Vb6CoreServices, vb6::Vb6SharedCoreServices,
                           vb6::install_core_module>());

const vb6::ast::Module* parse_module(const Language& language,
                                      const fs::path& path,
                                      std::string& text, cursiv::ParseResult* out) {
  text = read_file(path);
  *out = language.language.parser->parse(text);
  if (!out->accepted || out->cst().recovered()) {
    fail(path.string() + ": the document does not parse clean");
    return nullptr;
  }
  if (!out->diagnostics.empty()) {
    fail(path.string() + ": the document has diagnostics");
    return nullptr;
  }
  const cursiv::AstNode* const root = out->value;
  if (root == nullptr ||
      root->type != &cursiv::ast_type_tag<vb6::ast::Module>) {
    fail(path.string() + ": the parse built no Module");
    return nullptr;
  }
  return static_cast<const vb6::ast::Module*>(root);
}

void corpus(const Language& language) {
  const fs::path dir = CURSIV_EXAMPLE_DIR;
  int count = 0;
  for (const fs::directory_entry& e : fs::recursive_directory_iterator(dir)) {
    if (!e.is_regular_file()) continue;
    const fs::path& path = e.path();
    const std::string ext = path.extension().string();
    if (ext != ".bas" && ext != ".cls" && ext != ".frm") continue;
    ++count;
    std::string text;
    cursiv::ParseResult result;
    parse_module(language, path, text, &result);
  }
  if (count == 0) fail("no corpus document was found under " + dir.string());
}

// The constructs no upstream corpus example exercises: also
// required to parse clean. They live beside this test, not in the corpus.
void own_examples(const Language& language) {
  const fs::path dir = fs::path(CURSIV_TEST_DIR) / "own";
  int count = 0;
  for (const fs::directory_entry& e : fs::recursive_directory_iterator(dir)) {
    if (!e.is_regular_file()) continue;
    ++count;
    std::string text;
    cursiv::ParseResult result;
    parse_module(language, e.path(), text, &result);
  }
  if (count == 0) fail("no own-written document was found under " + dir.string());
}

// Field-level readback: Calls.cls declares Function1..Function2, Sub1, Sub2
// and a TestSimpleCalls sub whose body calls Function2 both explicitly
// (`Call Function2(1, 2)`) and implicitly (`Function2 1, 2`).
void field_readback(const Language& language) {
  const fs::path path = fs::path(CURSIV_EXAMPLE_DIR) / "calls" / "Calls.cls";
  std::string text;
  cursiv::ParseResult result;
  const vb6::ast::Module* const module = parse_module(language, path, text, &result);
  if (module == nullptr) {
    fail("field_readback: the sample itself failed to parse");
    return;
  }
  const vb6::ast::Procedure* testSimpleCalls = nullptr;
  for (vb6::ast::Item* item : module->body) {
    if (item->type != &cursiv::ast_type_tag<vb6::ast::Procedure>) continue;
    const auto* const proc = static_cast<const vb6::ast::Procedure*>(item);
    if (proc->name == "TestSimpleCalls") {
      testSimpleCalls = proc;
      break;
    }
  }
  if (testSimpleCalls == nullptr) {
    fail("field_readback: TestSimpleCalls was not read back");
    return;
  }
  bool sawExplicitCall = false;
  bool sawImplicitCall = false;
  for (vb6::ast::Item* stmt : testSimpleCalls->body) {
    if (stmt->type != &cursiv::ast_type_tag<vb6::ast::CallStmt>) continue;
    const auto* const call = static_cast<const vb6::ast::CallStmt*>(stmt);
    const auto* const target =
        call->target->type == &cursiv::ast_type_tag<vb6::ast::NameExpr>
            ? static_cast<const vb6::ast::NameExpr*>(call->target)
            : nullptr;
    if (target == nullptr || target->name != "Function2") continue;
    if (call->args.size() != 2) continue;
    if (call->args[0]->value == nullptr || call->args[1]->value == nullptr)
      continue;
    // Read the actual converted text, not just that a value is present: a
    // converter that silently mangles the span would still pass a
    // structural-only check.
    const auto* const first =
        call->args[0]->value->type == &cursiv::ast_type_tag<vb6::ast::IntLit>
            ? static_cast<const vb6::ast::IntLit*>(call->args[0]->value)
            : nullptr;
    const auto* const second =
        call->args[1]->value->type == &cursiv::ast_type_tag<vb6::ast::IntLit>
            ? static_cast<const vb6::ast::IntLit*>(call->args[1]->value)
            : nullptr;
    if (first == nullptr || first->text != "1" || second == nullptr ||
        second->text != "2") {
      continue;
    }
    // The explicit form is written `Call Function2(1, 2)`: the arguments
    // were absorbed into the target's own index-call chain, so CallStmt's
    // own `args` stays empty for it -- only the implicit, paren-less form
    // (`Function2 1, 2`) populates `args` directly.
    sawImplicitCall = true;
  }
  for (vb6::ast::Item* stmt : testSimpleCalls->body) {
    if (stmt->type != &cursiv::ast_type_tag<vb6::ast::CallStmt>) continue;
    const auto* const call = static_cast<const vb6::ast::CallStmt*>(stmt);
    if (call->target->type != &cursiv::ast_type_tag<vb6::ast::IndexExpr>)
      continue;
    const auto* const index =
        static_cast<const vb6::ast::IndexExpr*>(call->target);
    const auto* const target =
        index->target->type == &cursiv::ast_type_tag<vb6::ast::NameExpr>
            ? static_cast<const vb6::ast::NameExpr*>(index->target)
            : nullptr;
    if (target != nullptr && target->name == "Function2" &&
        index->args.size() == 2) {
      sawExplicitCall = true;
    }
  }
  if (!sawExplicitCall)
    fail("field_readback: Call Function2(1, 2) was not read back");
  if (!sawImplicitCall)
    fail("field_readback: Function2 1, 2 was not read back");
}

// `^` binds tighter than unary `+`/`-` (MS-VBAL's operator table):
// `-2 ^ 2` is `-(2 ^ 2)`, and `^`'s own right operand may still carry a sign
// (`2 ^ -2` is `2 ^ (-2)`).
void check_pow_tighter_than_unary(const Language& language) {
  const std::string text =
      "Sub Test()\n"
      "x = -2 ^ 2\n"
      "y = 2 ^ -2\n"
      "End Sub\n";
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("check_pow_tighter_than_unary: the probe does not parse clean");
    return;
  }
  const auto* const module = result.value_as<vb6::ast::Module>();
  if (module == nullptr || module->body.size() != 1) {
    fail("check_pow_tighter_than_unary: expected one top-level Sub");
    return;
  }
  const auto* const proc =
      static_cast<const vb6::ast::Procedure*>(module->body[0]);
  if (proc->body.size() != 2) {
    fail("check_pow_tighter_than_unary: expected two statements in Test");
    return;
  }

  const auto* const letX = static_cast<const vb6::ast::LetStmt*>(proc->body[0]);
  const auto* const negated =
      letX->value->type == &cursiv::ast_type_tag<vb6::ast::UnaryExpr>
          ? static_cast<const vb6::ast::UnaryExpr*>(letX->value)
          : nullptr;
  if (negated == nullptr || negated->op != "-")
    fail("check_pow_tighter_than_unary: `-2 ^ 2` should be a UnaryExpr(-)");
  const auto* const powInsideNeg =
      negated != nullptr &&
              negated->operand->type == &cursiv::ast_type_tag<vb6::ast::BinaryExpr>
          ? static_cast<const vb6::ast::BinaryExpr*>(negated->operand)
          : nullptr;
  if (powInsideNeg == nullptr || powInsideNeg->op != "^")
    fail("check_pow_tighter_than_unary: `-2 ^ 2` should be `-(2 ^ 2)`");

  const auto* const letY = static_cast<const vb6::ast::LetStmt*>(proc->body[1]);
  const auto* const pow =
      letY->value->type == &cursiv::ast_type_tag<vb6::ast::BinaryExpr>
          ? static_cast<const vb6::ast::BinaryExpr*>(letY->value)
          : nullptr;
  if (pow == nullptr || pow->op != "^")
    fail("check_pow_tighter_than_unary: `2 ^ -2` should be a BinaryExpr(^)");
  const auto* const negRight =
      pow != nullptr &&
              pow->right->type == &cursiv::ast_type_tag<vb6::ast::UnaryExpr>
          ? static_cast<const vb6::ast::UnaryExpr*>(pow->right)
          : nullptr;
  if (negRight == nullptr || negRight->op != "-")
    fail("check_pow_tighter_than_unary: `2 ^ -2`'s right operand should be -2");
}

// Integer division `\` has its own tier, looser than `*`/`/` (MS-VBAL):
// `10 \ 3 * 2` is `10 \ (3 * 2)`, not `(10 \ 3) * 2`.
void check_int_div_own_tier(const Language& language) {
  const std::string text = "Sub Test()\nx = 10 \\ 3 * 2\nEnd Sub\n";
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("check_int_div_own_tier: the probe does not parse clean");
    return;
  }
  const auto* const module = result.value_as<vb6::ast::Module>();
  if (module == nullptr || module->body.size() != 1) {
    fail("check_int_div_own_tier: expected one top-level Sub");
    return;
  }
  const auto* const proc =
      static_cast<const vb6::ast::Procedure*>(module->body[0]);
  if (proc->body.size() != 1) {
    fail("check_int_div_own_tier: expected one statement in Test");
    return;
  }
  const auto* const let = static_cast<const vb6::ast::LetStmt*>(proc->body[0]);
  const auto* const intDiv =
      let->value->type == &cursiv::ast_type_tag<vb6::ast::BinaryExpr>
          ? static_cast<const vb6::ast::BinaryExpr*>(let->value)
          : nullptr;
  if (intDiv == nullptr || intDiv->op != "\\") {
    fail("check_int_div_own_tier: `10 \\ 3 * 2` should be a BinaryExpr(\\)");
    return;
  }
  const auto* const mul =
      intDiv->right->type == &cursiv::ast_type_tag<vb6::ast::BinaryExpr>
          ? static_cast<const vb6::ast::BinaryExpr*>(intDiv->right)
          : nullptr;
  if (mul == nullptr || mul->op != "*")
    fail("check_int_div_own_tier: `10 \\ 3 * 2` should be `10 \\ (3 * 2)`");
}

// `End With` closes the block and `With New` opens the next, never a call to
// `End` or `With`; `Time` and `Randomize` open their own statements, a class
// header's `Begin` its configuration block, and a reserved word is still a
// `Dim` name to the grammar (a validator refuses it).
void check_reserved_words(const Language& language) {
  const std::string text =
      "VERSION 1.0 CLASS\n"
      "BEGIN\n"
      "  MultiUse = -1  'True\n"
      "END\n"
      "Sub Test()\n"
      "With o\n"
      ".A\n"
      "End With\n"
      "With New Foo\n"
      ".B\n"
      "End With\n"
      "Time = t\n"
      "Randomize\n"
      "End Sub\n";
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("check_reserved_words: the probe does not parse clean");
    return;
  }
  const auto* const module = result.value_as<vb6::ast::Module>();
  if (module == nullptr || module->config == nullptr ||
      module->body.size() != 1) {
    fail("check_reserved_words: expected a header block and one Sub");
    return;
  }
  const auto* const proc =
      static_cast<const vb6::ast::Procedure*>(module->body[0]);
  if (proc->body.size() != 4 ||
      proc->body[0]->type != &cursiv::ast_type_tag<vb6::ast::WithStmt> ||
      proc->body[1]->type != &cursiv::ast_type_tag<vb6::ast::WithStmt> ||
      proc->body[2]->type != &cursiv::ast_type_tag<vb6::ast::TimeStmt> ||
      proc->body[3]->type != &cursiv::ast_type_tag<vb6::ast::RandomizeStmt>) {
    fail("check_reserved_words: expected With, With, Time, Randomize");
    return;
  }
  const auto* const second =
      static_cast<const vb6::ast::WithStmt*>(proc->body[1]);
  if (second->target->type != &cursiv::ast_type_tag<vb6::ast::NewExpr>)
    fail("check_reserved_words: `With New Foo` should target a NewExpr");
  for (const char* word : {"Next", "Int", "LenB", "InputB", "CDecl", "GoTo"})
    if (!language.language.parser
             ->parse(std::string("Dim ") + word + " As Long\n")
             .diagnostics.empty())
      fail(std::string("check_reserved_words: `") + word +
           "` should be a name");
}

// A name may hold an accented letter, even right behind a keyword (`ifé`);
// `Date` stays a name; `Randomize:` opens no label (MS-VBAL 5.4.1.1).
void check_names(const Language& language) {
  const std::string text =
      "Dim Date As Long\n"
      "Sub Test()\n"
      "D\xc3\xa9j\xc3\xa0 = 1\n"
      "if\xc3\xa9 = D\xc3\xa9j\xc3\xa0\n"
      "\xc3\x89tatX = 2\n"
      "Randomize:\n"
      "End Sub\n";
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("check_names: the probe does not parse clean");
    return;
  }
  const auto* const module = result.value_as<vb6::ast::Module>();
  const auto* const proc =
      module != nullptr && module->body.size() == 2
          ? static_cast<const vb6::ast::Procedure*>(module->body[1])
          : nullptr;
  if (proc == nullptr || proc->body.size() != 4 ||
      proc->body[1]->type != &cursiv::ast_type_tag<vb6::ast::LetStmt> ||
      proc->body[3]->type != &cursiv::ast_type_tag<vb6::ast::RandomizeStmt>)
    fail("check_names: expected three assignments and a Randomize");
}

// A label opens a line (MS-VBAL 5.4.1.1): after a `:` a name is a call, and
// a label on a block's last line is that block's. A date literal is a date
// (MS-VBAL 3.3.3), never the text between two file numbers.
void check_labels(const Language& language) {
  const std::string text =
      "Sub Test()\n"
      "On Error GoTo Fail:\n"
      "x = 1: Foo: y = 2\n"
      "Close #1, #2\n"
      "Fail:\n"
      "End Sub\n";
  cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered() || !result.diagnostics.empty()) {
    fail("check_labels: the probe does not parse clean");
    return;
  }
  const auto* const module = result.value_as<vb6::ast::Module>();
  const auto* const proc =
      module != nullptr && module->body.size() == 1
          ? static_cast<const vb6::ast::Procedure*>(module->body[0])
          : nullptr;
  if (proc == nullptr || proc->body.size() != 6 ||
      proc->body[2]->type != &cursiv::ast_type_tag<vb6::ast::CallStmt> ||
      proc->body[4]->type != &cursiv::ast_type_tag<vb6::ast::CloseStmt> ||
      proc->body[5]->type != &cursiv::ast_type_tag<vb6::ast::LabelStmt>) {
    fail("check_labels: expected On Error, x, Foo, y, Close, a label");
    return;
  }
  const auto* const close =
      static_cast<const vb6::ast::CloseStmt*>(proc->body[4]);
  const auto* const label =
      static_cast<const vb6::ast::LabelStmt*>(proc->body[5]);
  if (close->fileNumbers.size() != 2)
    fail("check_labels: `Close #1, #2` should close two file numbers");
  if (label->name != "Fail") fail("check_labels: the label is not `Fail`");
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      vb6::Vb6CoreServices, vb6::Vb6SharedCoreServices,
      vb6::install_core_module>();

  if (!vb6_native_followup::run(*language.language.parser)) return 1;
  corpus(language);
  own_examples(language);
  field_readback(language);
  check_pow_tighter_than_unary(language);
  check_int_div_own_tier(language);
  check_reserved_words(language);
  check_labels(language);
  check_names(language);

  if (failures != 0) {
    std::fprintf(stderr, "vb6: %d check(s) failed\n", failures);
    return 1;
  }
  std::puts("vb6: the corpus parses");
  return 0;
}
