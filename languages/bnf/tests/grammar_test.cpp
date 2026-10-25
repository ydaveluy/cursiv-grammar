// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "bnf/core/CoreServices.hpp"
#include "bnf/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* sample, const char* what) {
  std::fprintf(stderr, "bnf: %s: %s\n", sample, what);
  return 1;
}

struct Sample {
  const char* dir;
  const char* name;
};

constexpr Sample kSamples[] = {
    {CURSIV_EXAMPLE_DIR, "treutwein-algol60.bnf"},
    {CURSIV_EXAMPLE_DIR, "wiki-bnf.bnf"},
    {CURSIV_EXAMPLE_DIR, "unicode-ebnf.bnf"},
    {CURSIV_EXAMPLE_DIR, "ebnf.bnf"},
    {CURSIV_EXAMPLE_DIR, "postal.bnf"},
    {CURSIV_EXAMPLE_DIR, "letter1.bnf"},
    {CURSIV_EXAMPLE_DIR, "shutup.bnf"},
    {CURSIV_EXAMPLE_DIR, "parens.bnf"},
    {CURSIV_EXAMPLE_DIR, "pemdas.bnf"},
    {CURSIV_EXAMPLE_DIR, "exp.bnf"},
    {CURSIV_TEST_DIR, "arrow-forms.bnf"},
};

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<bnf::BnfCoreServices, bnf::BnfSharedCoreServices,
                              bnf::install_core_module>();

  for (const Sample& sample : kSamples) {
    const std::string path = std::string(sample.dir) + "/" + sample.name;
    std::ifstream file(path);
    if (!file) return fail(sample.name, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(sample.name, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bnf::ast::Grammar>)
      return fail(sample.name, "the parse built no grammar");
  }

  for (const auto& [text, count] : {
           std::pair{"x ::=", 1u},
           std::pair{"x ::= | \"a\"", 2u},
           std::pair{"x ::= \"a\" |", 2u},
           std::pair{"x ::= ||", 3u},
           std::pair{"<x> ::= | <x> |", 3u}}) {
    const auto result = language.language.parser->parse(text);
    const auto* grammar = cursiv::ast_cast<bnf::ast::Grammar>(result.value);
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !grammar || grammar->productions.size() != 1 ||
        grammar->productions[0]->name != "x" ||
        grammar->productions[0]->alternatives.size() != count ||
        (count == 3 && !grammar->productions[0]->alternatives[0]->elements.empty()))
      return fail(text, "empty alternatives or single-letter header lost");
  }
  {
    const auto result = language.language.parser->parse(
        "x ::=\nother ::= \"\"\nnext ::= <x> | \"a\"");
    const auto* grammar = cursiv::ast_cast<bnf::ast::Grammar>(result.value);
    if (!result.accepted || result.cst().recovered() || !grammar ||
        grammar->productions.size() != 3 ||
        !grammar->productions[0]->alternatives[0]->elements.empty() ||
        grammar->productions[1]->alternatives[0]->elements.size() != 1 ||
        !grammar->productions[1]->alternatives[0]->elements[0]->text.empty() ||
        grammar->productions[1]->alternatives[0]->elements[0]->nonterminal ||
        grammar->productions[2]->alternatives[0]->elements.size() != 1 ||
        !grammar->productions[2]->alternatives[0]->elements[0]->nonterminal)
      return fail("epsilon", "production boundaries or quoted empty string lost");
  }

  std::puts("bnf: every sample parses");
  return 0;
}
