// The smoke test of the language: build the container the way a workspace
// does, read every document of example/, and check each tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "pl0/core/CoreServices.hpp"
#include "pl0/core/ast.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "pl0: %s: %s\n", file, what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      pl0::Pl0CoreServices, pl0::Pl0SharedCoreServices,
      pl0::install_core_module>();

  for (const auto source : {"VAR BEGIN;.", "CONST IF=1;.", "PROCEDURE CALL; ;.",
                             "VAR a,b;a:=a+-b.", "VAR a,b;a:=a*-b."}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty())
      return fail(source, "native-invalid reserved name/sign accepted");
  }
  {
    const std::string digits(80, '9');
    const auto r = language.language.parser->parse("CONST huge=" + digits + "; !" + digits + ".");
    const auto* p = r.value_as<pl0::ast::Program>();
    const auto* print = p && p->block ? cursiv::ast_cast<pl0::ast::PrintStmt>(p->block->body) : nullptr;
    const auto* number = print ? cursiv::ast_cast<pl0::ast::NumberExpr>(print->value) : nullptr;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !p ||
        p->block->consts.size()!=1 || p->block->consts[0]->value!=digits || !number || number->value!=digits)
      return fail("large numeric text", "native lexeme was rejected or truncated");
  }

  static constexpr const char* kExamples[] = {
      CURSIV_EXAMPLE_DIR "/example/example1.pl0",
      CURSIV_EXAMPLE_DIR "/example/example2.pl0",
      CURSIV_EXAMPLE_DIR "/example/example3.pl0",
      CURSIV_TEST_DIR "/constructs.pl0",
  };

  for (const char* path : kExamples) {
    const std::string text = read_file(path);
    if (text.empty()) return fail(path, "the sample document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty()) return fail(path, "the sample carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<pl0::ast::Program>)
      return fail(path, "the parse built no program");
  }

  std::puts("pl0: every sample parses");
  return 0;
}
