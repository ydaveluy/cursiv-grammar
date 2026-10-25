#include "audit_regressions.hpp"
// The smoke test of the language: every document under example/ parses clean
// (no recovery) and builds a CompilationUnit with at least one program.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cobol85/core/CoreServices.hpp"
#include "cobol85/core/ast.hpp"

namespace {

int fail(const std::string& file, const char* what) {
  std::fprintf(stderr, "cobol85: %s: %s\n", file.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      cobol85::Cobol85CoreServices, cobol85::Cobol85SharedCoreServices,
      cobol85::install_core_module>();

  int failures = 0;
  int checked = 0;
  // data-division.cbl and procedure-division.cbl live beside this test, not
  // in the corpus: they exercise clauses and statements example1.cbl alone
  // does not reach.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".cbl")
        continue;
      const std::string path = entry.path().string();
      std::ifstream file(path);
      if (!file) {
        failures += fail(path, "not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result =
          language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(path, "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(path, "the document leaves diagnostics");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type !=
              &cursiv::ast_type_tag<cobol85::ast::CompilationUnit>) {
        failures += fail(path, "the parse built no compilation unit");
        continue;
      }
      const auto& unit =
          *static_cast<const cobol85::ast::CompilationUnit*>(root);
      if (unit.programs.empty()) {
        failures += fail(path, "the compilation unit declares no program");
        continue;
      }
      ++checked;
    }

  if (checked == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  // AND binds tighter than OR in a condition (IBM Enterprise COBOL Language
  // Reference, "Combined Conditions"): `A > B OR A = C AND D > 0` reads
  // `(A > B) OR ((A = C) AND (D > 0))`, not a flat left-to-right chain.
  {
    const std::string text =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. Probe.\n"
        "PROCEDURE DIVISION.\n"
        "IF A > B OR A = C AND D > 0\n"
        "    DISPLAY \"one\"\n"
        "END-IF.\n"
        "STOP RUN.\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<precedence probe>", "does not parse clean");
    const auto& unit =
        *static_cast<const cobol85::ast::CompilationUnit*>(result.value);
    const auto& proc = *unit.programs[0]->procedure;
    const auto& ifStmt =
        *static_cast<const cobol85::ast::IfStatement*>(proc.statements[0]);
    const auto& root =
        *static_cast<const cobol85::ast::CondBinary*>(ifStmt.cond);
    if (root.op != "OR")
      return fail("<precedence probe>", "OR is not the root operator");
    const auto& right =
        *static_cast<const cobol85::ast::CondBinary*>(root.right);
    if (right.op != "AND")
      return fail("<precedence probe>", "AND does not nest under OR");
  }

  // Abbreviated combined relation condition: `A > (B AND C)` is short for
  // `A > B AND A > C` -- the parenthesized list distributes the same left
  // operand and relational operator over each element.
  {
    const std::string text =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. Probe.\n"
        "PROCEDURE DIVISION.\n"
        "IF A > (B AND C)\n"
        "    DISPLAY \"one\"\n"
        "END-IF.\n"
        "STOP RUN.\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<combined relation probe>", "does not parse clean");
    const auto& unit =
        *static_cast<const cobol85::ast::CompilationUnit*>(result.value);
    const auto& proc = *unit.programs[0]->procedure;
    const auto& ifStmt =
        *static_cast<const cobol85::ast::IfStatement*>(proc.statements[0]);
    const auto& root =
        *static_cast<const cobol85::ast::CondBinary*>(ifStmt.cond);
    if (root.op != "AND")
      return fail("<combined relation probe>", "AND is not the root operator");
    const auto& left =
        *static_cast<const cobol85::ast::RelationCond*>(root.left);
    if (left.op != ">")
      return fail("<combined relation probe>",
                   "the first comparison did not read back");
    const auto& right2 =
        *static_cast<const cobol85::ast::AbbrevCond*>(root.right);
    if (!right2.op.empty() || right2.negated)
      return fail("<combined relation probe>",
                   "the abbreviated element carries an operator/NOT");
  }

  // AND binds tighter than OR inside the parenthesized list too: `A > (B
  // AND C OR D)` reads `((A > B) AND (A > C)) OR (A > D)`, not a flat
  // left-to-right chain (IBM's Enterprise COBOL reference ties an
  // abbreviated combined relation condition to the same element-sequence
  // rule as an ordinary combined condition).
  {
    const std::string text =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. Probe.\n"
        "PROCEDURE DIVISION.\n"
        "IF A > (B AND C OR D)\n"
        "    DISPLAY \"one\"\n"
        "END-IF.\n"
        "STOP RUN.\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<combined relation AND/OR probe>", "does not parse clean");
    const auto& unit =
        *static_cast<const cobol85::ast::CompilationUnit*>(result.value);
    const auto& proc = *unit.programs[0]->procedure;
    const auto& ifStmt =
        *static_cast<const cobol85::ast::IfStatement*>(proc.statements[0]);
    const auto& root =
        *static_cast<const cobol85::ast::CondBinary*>(ifStmt.cond);
    if (root.op != "OR")
      return fail("<combined relation AND/OR probe>", "OR is not the root operator");
    const auto& right3 =
        *static_cast<const cobol85::ast::AbbrevCond*>(root.right);
    if (right3.value == nullptr)
      return fail("<combined relation AND/OR probe>", "D did not read back");
    const auto& left2 =
        *static_cast<const cobol85::ast::CondBinary*>(root.left);
    if (left2.op != "AND")
      return fail("<combined relation AND/OR probe>", "AND does not nest under OR");
    const auto& innerLeft =
        *static_cast<const cobol85::ast::RelationCond*>(left2.left);
    if (innerLeft.op != ">")
      return fail("<combined relation AND/OR probe>",
                   "the first comparison did not read back");
  }

  {
    const std::string text =
        "IDENTIFICATION DIVISION.\n"
        "PROGRAM-ID. Probe.\n"
        "DATA DIVISION.\n"
        "WORKING-STORAGE SECTION.\n"
        "01  WS-A PIC 9(4) COMP PIC X(4).\n"
        "PROCEDURE DIVISION.\n"
        "STOP RUN.\n";
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("<repeated clause probe>", "does not parse clean");
    const auto& unit =
        *static_cast<const cobol85::ast::CompilationUnit*>(result.value);
    const auto& item = *static_cast<const cobol85::ast::DataItem*>(
        unit.programs[0]->data->workingStorage[0]);
    if (item.pictures.size() != 2 || item.pictures[0] != "9(4)" ||
        item.pictures[1] != "X(4)")
      return fail("<repeated clause probe>",
                  "PICTURE did not keep both values");
  }

  if (failures > 0) return 1;
  std::printf("cobol85: %d example(s) parse clean\n", checked);
  return 0;
}
