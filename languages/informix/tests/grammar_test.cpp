// The smoke test of the language: every corpus document parses clean, and
// one is read back field by field to catch a converter that silently
// mismaps its value.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "informix/core/CoreServices.hpp"
#include "informix/core/ast.hpp"
#include "native_followup_cases.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "informix: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char* const kExamples[] = {
    "example1.4gl",
    "example2.4gl",
    "example3.4gl",
};

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      informix::InformixCoreServices, informix::InformixSharedCoreServices,
      informix::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("MAIN\n DEFINE X INTEGER\n LET X = 1\nEND MAIN\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      informix::InformixCoreServices, informix::InformixSharedCoreServices,
      informix::install_core_module>();
  if (!informix_native_followup::run(language)) return 1;

  for (const char* const name : kExamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    const std::string text = read_file(path);
    if (text.empty())
      return fail((std::string(name) + ": empty or unreadable").c_str());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail((std::string(name) + ": does not parse clean").c_str());
    if (!result.diagnostics.empty())
      return fail((std::string(name) + ": has diagnostics").c_str());

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<informix::ast::Program>)
      return fail((std::string(name) + ": built no Program").c_str());

    const auto& program = *static_cast<const informix::ast::Program*>(root);
    if (program.main == nullptr)
      return fail((std::string(name) + ": Program holds no MAIN block").c_str());
  }

  // coverage.4gl (not part of the upstream corpus): SQL DDL/DML,
  // cursors, dynamic SQL, transactions, records/arrays/LIKE types,
  // datetime/interval qualifiers, screen and menu statements, a labelled
  // GOTO and a REPORT with FORMAT -- none of which the three-file corpus
  // reaches.
  {
    const std::string text = read_file(std::string(CURSIV_TEST_DIR) +
                                       "/coverage.4gl");
    if (text.empty()) return fail("coverage.4gl: empty or unreadable");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("coverage.4gl: does not parse clean");
    if (!result.diagnostics.empty()) return fail("coverage.4gl: has diagnostics");
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<informix::ast::Program>)
      return fail("coverage.4gl: built no Program");
    const auto& program = *static_cast<const informix::ast::Program*>(root);
    if (program.definitions.size() != 3)
      return fail("coverage.4gl: expected 3 definitions (2 functions, 1 report)");
  }

  // example3.4gl, read back field by field: three DEFINE groups sharing one
  // DEFINE, and the statement sequence a bare juxtaposition
  // ("LET i = f + 1 DISPLAY i") folds into two statements.
  const std::string text =
      read_file(std::string(CURSIV_EXAMPLE_DIR) + "/example3.4gl");
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& program = *static_cast<const informix::ast::Program*>(result.value);
  const informix::ast::MainBlock& main = *program.main;

  if (main.types.size() != 1 || main.types[0]->groups.size() != 3)
    return fail("example3.4gl: expected one DEFINE with three groups");
  const informix::ast::VarGroup& g2 = *main.types[0]->groups[2];
  if (g2.names.size() != 1 || g2.names[0] != "c")
    return fail("example3.4gl: third group is not `c`");
  if (g2.type->type != &cursiv::ast_type_tag<informix::ast::ScalarType>)
    return fail("example3.4gl: `c`'s type is not scalar");
  const auto& cType = *static_cast<const informix::ast::ScalarType*>(g2.type);
  if (cType.keyword != "CHAR" || !cType.size1.has_value() ||
      *cType.size1 != 20)
    return fail("example3.4gl: `c` is not CHAR(20)");

  if (main.body.size() != 9)
    return fail("example3.4gl: expected 9 statements");
  if (main.body[0]->type != &cursiv::ast_type_tag<informix::ast::LetStmt>)
    return fail("example3.4gl: first statement is not LET");
  const auto& let0 = *static_cast<const informix::ast::LetStmt*>(main.body[0]);
  if (let0.target->type != &cursiv::ast_type_tag<informix::ast::NameRef>)
    return fail("example3.4gl: LET target is not a name");
  const auto& target0 =
      *static_cast<const informix::ast::NameRef*>(let0.target);
  if (target0.name != "c") return fail("example3.4gl: LET target is not `c`");
  if (let0.values.size() != 1 ||
      let0.values[0]->type != &cursiv::ast_type_tag<informix::ast::StringLit>)
    return fail("example3.4gl: LET value is not one string literal");
  const auto& lit0 =
      *static_cast<const informix::ast::StringLit*>(let0.values[0]);
  if (lit0.value != "99") return fail("example3.4gl: LET value is not \"99\"");

  // BETWEEN/LIKE/IS NULL/comparisons take the full arithmetic/concatenation
  // expression as their operand, not a bare atom: `a + b BETWEEN c AND d`
  // is `(a + b) BETWEEN c AND d`, not `a + (b BETWEEN c AND d)`.
  {
    const std::string precText =
        "MAIN\n"
        "  DEFINE a, b, c, d, x SMALLINT\n"
        "  LET x = a + b BETWEEN c AND d\n"
        "END MAIN\n";
    const cursiv::ParseResult precResult =
        language.language.parser->parse(precText);
    if (precResult.cst().recovered() || !precResult.diagnostics.empty())
      return fail("<precedence>: does not parse clean");
    const auto& precProgram =
        *static_cast<const informix::ast::Program*>(precResult.value);
    if (precProgram.main == nullptr || precProgram.main->body.size() != 1 ||
        precProgram.main->body[0]->type !=
            &cursiv::ast_type_tag<informix::ast::LetStmt>)
      return fail("<precedence>: expected one LET statement");
    const auto& precLet =
        *static_cast<const informix::ast::LetStmt*>(precProgram.main->body[0]);
    if (precLet.values.size() != 1 ||
        precLet.values[0]->type !=
            &cursiv::ast_type_tag<informix::ast::BetweenExpr>)
      return fail("<precedence>: a + b BETWEEN c AND d should be BETWEEN at top");
    const auto& precBetween =
        *static_cast<const informix::ast::BetweenExpr*>(precLet.values[0]);
    if (precBetween.operand == nullptr ||
        precBetween.operand->type != &cursiv::ast_type_tag<informix::ast::BinaryExpr>)
      return fail("<precedence>: BETWEEN's operand should be a + b, not just b");
  }

  std::puts("informix: the corpus parses");
  return 0;
}
