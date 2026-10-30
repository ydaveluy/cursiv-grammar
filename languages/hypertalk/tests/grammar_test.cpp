// The smoke test of the language: parse every file under example/, the way a
// workspace would build one, and read a few real field values back out.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "hypertalk/core/CoreServices.hpp"
#include "hypertalk/core/ast.hpp"
#include "native_followup_cases.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "hypertalk: %s\n", what.c_str());
  return 1;
}

// Relational operators (<, contains, is a/an, is within, is in) bind
// tighter than equality operators (=, <>, is, is not): `a < b = c` is
// `(a<b)=c`, `a contains b is c` is `(a contains b) is c`.
int check_relational_tighter_than_equality(
    hypertalk::HypertalkCoreServices& language) {
  {
    const std::string text = "on m\n  put a < b = c\nend m\n";
    const cursiv::ParseResult result = language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'a < b = c' probe does not parse clean");
    const auto& doc = *static_cast<const hypertalk::ast::Document*>(result.value);
    const auto& put = *static_cast<const hypertalk::ast::PutCmd*>(
        doc.items[0]->handler->body[0]);
    const auto* const top =
        static_cast<const hypertalk::ast::BinaryExpr*>(put.values[0]);
    if (top->op != hypertalk::ast::BinOp::Equal)
      return fail("'a < b = c' is not top-level =");
    if (top->left->type != &cursiv::ast_type_tag<hypertalk::ast::BinaryExpr>)
      return fail("'a < b = c' left operand is not a relational expr");
    const auto* const left =
        static_cast<const hypertalk::ast::BinaryExpr*>(top->left);
    if (left->op != hypertalk::ast::BinOp::Less)
      return fail("'a < b = c' left operand is not <");
  }
  {
    const std::string text = "on m\n  put a contains b is c\nend m\n";
    const cursiv::ParseResult result = language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("'a contains b is c' probe does not parse clean");
    const auto& doc = *static_cast<const hypertalk::ast::Document*>(result.value);
    const auto& put = *static_cast<const hypertalk::ast::PutCmd*>(
        doc.items[0]->handler->body[0]);
    const auto* const top =
        static_cast<const hypertalk::ast::BinaryExpr*>(put.values[0]);
    if (top->op != hypertalk::ast::BinOp::IsEqual)
      return fail("'a contains b is c' is not top-level is");
    if (top->left->type != &cursiv::ast_type_tag<hypertalk::ast::BinaryExpr>)
      return fail("'a contains b is c' left operand is not a relational expr");
    const auto* const left =
        static_cast<const hypertalk::ast::BinaryExpr*>(top->left);
    if (left->op != hypertalk::ast::BinOp::Contains)
      return fail("'a contains b is c' left operand is not contains");
  }
  return 0;
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      hypertalk::HypertalkCoreServices, hypertalk::HypertalkSharedCoreServices,
      hypertalk::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("put 7 div 2 into x\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool found = false;
    for (const auto* node : audit_result.nodes)
      if (const auto* b = cursiv::ast_cast<hypertalk::ast::BinaryExpr>(node)) found = b->op == hypertalk::ast::BinOp::IntegerDivide;
    if (!found) return 1;
  }

  namespace fs = std::filesystem;
  std::vector<fs::path> files;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".hypertalk") files.push_back(entry.path());
  if (files.empty()) return fail("no example files found");

  const auto language = cursiv::create_language<
      hypertalk::HypertalkCoreServices, hypertalk::HypertalkSharedCoreServices,
      hypertalk::install_core_module>();

  if (!hypertalk_native_followup::run(language.language)) return 1;

  if (const int rc = check_relational_tighter_than_equality(language.language);
      rc != 0)
    return rc;

  bool sawSingleLine = false;
  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": the parse needed repair");
    if (!result.diagnostics.empty())
      return fail(path.string() + ": the parse produced diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<hypertalk::ast::Document>)
      return fail(path.string() + ": the parse built no document");

    const auto& doc = *static_cast<const hypertalk::ast::Document*>(root);

    // SingleLine.hypertalk is exactly `answer "Hi, how are you?" with "Fine"
    // or "Dandy"`: one top-level statement, an AnswerCmd with two buttons --
    // read the values back rather than trusting the structural check alone.
    if (path.filename() == "SingleLine.hypertalk") {
      sawSingleLine = true;
      if (doc.items.size() != 1)
        return fail(path.string() + ": expected exactly one statement");
      const hypertalk::ast::TopItem* const item = doc.items[0];
      if (item == nullptr || item->statement == nullptr)
        return fail(path.string() + ": expected a bare statement");
      if (item->statement->type !=
          &cursiv::ast_type_tag<hypertalk::ast::AnswerCmd>)
        return fail(path.string() + ": expected an AnswerCmd");
      const auto& answer =
          *static_cast<const hypertalk::ast::AnswerCmd*>(item->statement);
      if (answer.prompt == nullptr ||
          answer.prompt->type !=
              &cursiv::ast_type_tag<hypertalk::ast::StringLit>)
        return fail(path.string() + ": expected a string prompt");
      const auto& prompt =
          *static_cast<const hypertalk::ast::StringLit*>(answer.prompt);
      if (prompt.value != "Hi, how are you?")
        return fail(path.string() + ": prompt text does not round-trip");
      if (answer.buttons.size() != 2)
        return fail(path.string() + ": expected two buttons");
      const auto& btn0 =
          *static_cast<const hypertalk::ast::StringLit*>(answer.buttons[0]);
      const auto& btn1 =
          *static_cast<const hypertalk::ast::StringLit*>(answer.buttons[1]);
      if (btn0.value != "Fine" || btn1.value != "Dandy")
        return fail(path.string() + ": button text does not round-trip");
    }
  }
  if (!sawSingleLine) return fail("SingleLine.hypertalk was not exercised");

  std::printf("hypertalk: %zu example(s) parse clean\n", files.size());
  return 0;
}
