// The smoke test of the language: build the container the way a workspace
// does, parse every file under example/, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "karel/core/CoreServices.hpp"
#include "karel/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "karel: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      karel::KarelCoreServices, karel::KarelSharedCoreServices,
      karel::install_core_module>();

  // example1.karel: one subroutine, two top-level statements (an ITERATE and
  // a call that nothing requires to resolve).
  {
    const std::string text = read_file(CURSIV_EXAMPLE1);
    if (text.empty()) return fail("example1.karel is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("example1.karel does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<karel::ast::Program>)
      return fail("example1.karel built no Program");

    const auto& program = *static_cast<const karel::ast::Program*>(root);
    if (program.definitions.size() != 1)
      return fail("example1.karel: wrong definition count");
    if (program.definitions[0]->name != "turnright")
      return fail("example1.karel: wrong definition name");
    if (program.statements.size() != 2)
      return fail("example1.karel: wrong statement count");
  }

  // patrol.karel: every instruction, every condition, WHILE, ITERATE, a
  // nested block and a dangling ELSE, two subroutines, one calling the other.
  {
    const std::string text = read_file(CURSIV_PATROL);
    if (text.empty()) return fail("patrol.karel is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail("patrol.karel does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<karel::ast::Program>)
      return fail("patrol.karel built no Program");

    const auto& program = *static_cast<const karel::ast::Program*>(root);
    if (program.definitions.size() != 2)
      return fail("patrol.karel: wrong definition count");
    if (program.statements.empty())
      return fail("patrol.karel: no statements");

    // The dangling ELSE binds to the nearer IF (LEFT-IS-CLEAR), not the
    // outer one (FRONT-IS-CLEAR): the nested BEGIN block holds one
    // statement, a Conditional carrying its own else branch.
    const auto* const block =
        std::get<karel::ast::Block*>(program.statements[3]);
    if (block == nullptr || block->statements.size() != 1)
      return fail("patrol.karel: unexpected nested block shape");
    const auto* const outer =
        std::get<karel::ast::Conditional*>(block->statements[0]);
    if (outer == nullptr || outer->elseBranch.has_value())
      return fail("patrol.karel: outer IF should carry no ELSE of its own");
    const auto* const inner =
        std::get<karel::ast::Conditional*>(outer->thenBranch);
    if (inner == nullptr || !inner->elseBranch.has_value())
      return fail("patrol.karel: dangling ELSE did not bind to the inner IF");
  }

  std::puts("karel: the examples parse");
  return 0;
}
