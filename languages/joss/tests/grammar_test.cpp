// The smoke test of the language: every document under tests/ (the upstream
// corpus is empty) parses clean, and one is read back for real field values.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "joss/core/CoreServices.hpp"
#include "joss/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "joss: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      joss::JossCoreServices, joss::JossSharedCoreServices,
      joss::install_core_module>();

  const char* const docs[] = {
      CURSIV_TEST_DIR "/statements.joss",   //
      CURSIV_TEST_DIR "/expressions.joss",  //
      CURSIV_TEST_DIR "/readback.joss",     //
  };

  // The texts must outlive every read of the parsed trees below: every
  // string_view in a CST/AST points into them.
  std::string texts[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) texts[i] = read_file(docs[i]);

  for (std::size_t i = 0; i < std::size(docs); ++i) {
    if (texts[i].empty()) return fail("a document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(texts[i]);
    if (result.cst().recovered()) return fail("a document does not parse clean");
    if (!result.diagnostics.empty())
      return fail("a document carries an unexpected diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<joss::ast::Program>)
      return fail("the parse built no program");
  }

  // Read real field values back out of readback.joss: a converter that
  // silently yields the wrong value would still pass every structural check
  // above.
  const cursiv::ParseResult result =
      language.language.parser->parse(texts[2]);
  const auto& program = *static_cast<const joss::ast::Program*>(result.value);
  if (program.statements.size() != 2) return fail("readback.joss has 2 statements");

  if (program.statements[0]->type !=
      &cursiv::ast_type_tag<joss::ast::AssignStmt>)
    return fail("the first statement is not an assignment");
  const auto& first =
      *static_cast<const joss::ast::AssignStmt*>(program.statements[0]);
  if (first.assignment == nullptr || first.assignment->target == nullptr ||
      first.assignment->target->name != "A")
    return fail("the first assignment's target was not read back correctly");
  if (first.assignment->value == nullptr ||
      first.assignment->value->type !=
          &cursiv::ast_type_tag<joss::ast::NumberLit>)
    return fail("the first assignment's value is not a number literal");
  if (static_cast<const joss::ast::NumberLit*>(first.assignment->value)
          ->value != 3.0)
    return fail("the first assignment's number was not read back correctly");

  if (program.statements[1]->type !=
      &cursiv::ast_type_tag<joss::ast::AssignStmt>)
    return fail("the second statement is not an assignment");
  const auto& second =
      *static_cast<const joss::ast::AssignStmt*>(program.statements[1]);
  if (second.assignment == nullptr || second.assignment->target == nullptr ||
      second.assignment->target->name != "B")
    return fail("the second assignment's target was not read back correctly");
  if (second.assignment->value == nullptr ||
      second.assignment->value->type != &cursiv::ast_type_tag<joss::ast::BinMath>)
    return fail("the second assignment's value is not a binary expression");
  const auto& sum =
      *static_cast<const joss::ast::BinMath*>(second.assignment->value);
  if (sum.op != joss::ast::MathOp::add || sum.left == nullptr ||
      sum.left->type != &cursiv::ast_type_tag<joss::ast::Variable>)
    return fail("the second assignment's sum was not read back correctly");

  std::puts("joss: every document parses, field values read back correctly");
  return 0;
}
