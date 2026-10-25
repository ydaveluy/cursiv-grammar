#include "audit_regressions.hpp"
// The smoke test of the language: every document under example/ parses
// clean, and the sample builds a Specification with the definitions it
// declares.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "oncrpc/core/CoreServices.hpp"
#include "oncrpc/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "oncrpc: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      oncrpc::OncrpcCoreServices, oncrpc::OncrpcSharedCoreServices,
      oncrpc::install_core_module>();

  for (const char* path : {CURSIV_SAMPLE, CURSIV_EXAMPLE_TYPES}) {
    const std::string text = read(path);
    if (text.empty()) return fail("an example document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail("an example does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<oncrpc::ast::Specification>)
      return fail("the parse built no specification");
  }

  const std::string text = read(CURSIV_SAMPLE);
  const cursiv::ParseResult result = language.language.parser->parse(text);
  const auto& spec =
      *static_cast<const oncrpc::ast::Specification*>(result.value);
  if (spec.items.size() != 2) return fail("the sample does not hold two top-level definitions");
  if (spec.items[0]->type != &cursiv::ast_type_tag<oncrpc::ast::StructDef>)
    return fail("the sample's first definition is not the struct");
  if (spec.items[1]->type != &cursiv::ast_type_tag<oncrpc::ast::ProgramDef>)
    return fail("the sample's second definition is not the program");

  std::puts("oncrpc: the examples parse");
  return 0;
}
