#include "modern_cases.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "antlr/core/CoreServices.hpp"
#include "antlr/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "antlr: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path &p) {
  std::ifstream file(p);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
  const auto language = cursiv::create_language<antlr::AntlrCoreServices,
                                                antlr::AntlrSharedCoreServices,
                                                antlr::install_core_module>();
  {
    constexpr auto text = R"(lexer grammar T;
@members { void f() { const char* x = "}"; /* } */ } }
A : 'a' { f({1}); };
mode M;
B : 'b';
)";
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("action islands");
    const auto *grammar = result.value_as<antlr::ast::Grammar>();
    if (!grammar || grammar->modes.size() != 1 ||
        grammar->actions.size() != 1 ||
        grammar->actions[0]->text !=
            " void f() { const char* x = \"}\"; /* } */ } ")
      return fail("action text or mode lost");
    bool action = false;
    for (const auto *node : result.nodes)
      if (const auto *value = cursiv::ast_cast<antlr::ast::Action>(node))
        action |= value->text == " f({1}); ";
    if (!action)
      return fail("nested action delimiters lost");
  }

  if (!antlr_audit::modern_cases(language))
    return 1;

  const std::filesystem::path dir = CURSIV_EXAMPLE_DIR;
  int checked = 0;
  for (const auto &entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".g4")
      continue;
    ++checked;
    const std::string text = read_file(entry.path());
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": does not parse");
    if (!result.diagnostics.empty())
      return fail(entry.path().string() + ": parses with diagnostics");
    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<antlr::ast::Grammar>)
      return fail(entry.path().string() + ": the parse built no Grammar");
  }
  if (checked == 0)
    return fail("no .g4 file found under example/");

  // Field values read back out of one document, not just its shape: a
  // converter that silently mis-parses would still leave the structural
  // checks above green.
  const std::string parserText = read_file(dir / "ANTLRv4Parser.g4");
  const cursiv::ParseResult parserResult =
      language.language.parser->parse(parserText);
  const auto &parserGrammar =
      *static_cast<const antlr::ast::Grammar *>(parserResult.value);
  if (parserGrammar.name != "ANTLRv4Parser")
    return fail("ANTLRv4Parser.g4: wrong grammar name");
  if (!parserGrammar.parserOnly || parserGrammar.lexerOnly)
    return fail("ANTLRv4Parser.g4: wrong grammar kind");
  if (parserGrammar.options.size() != 1 ||
      parserGrammar.options[0]->name != "tokenVocab" ||
      parserGrammar.options[0]->value != "ANTLRv4Lexer")
    return fail("ANTLRv4Parser.g4: wrong options { tokenVocab=...; }");

  bool foundGrammarSpec = false;
  for (const antlr::ast::RuleDecl *const rule : parserGrammar.rules) {
    if (rule->type != &cursiv::ast_type_tag<antlr::ast::ParserRule>)
      continue;
    const auto *const pr = static_cast<const antlr::ast::ParserRule *>(rule);
    if (pr->name != "grammarSpec")
      continue;
    foundGrammarSpec = true;
    if (pr->alternatives.size() != 1)
      return fail("grammarSpec: wrong alternative count");
    if (pr->alternatives[0]->elements.size() != 5)
      return fail("grammarSpec: wrong element count");
  }
  if (!foundGrammarSpec)
    return fail("ANTLRv4Parser.g4: rule 'grammarSpec' not found");

  std::printf("antlr: %d example documents parse as expected\n", checked);
  return 0;
}
