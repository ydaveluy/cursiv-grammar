// The smoke test of the language: parse every file in example/, and check each
// one builds a clean Document.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ebnf/core/CoreServices.hpp"
#include "ebnf/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& path, const char* what) {
  std::fprintf(stderr, "ebnf: %s: %s\n", path.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      ebnf::EbnfCoreServices,
      ebnf::EbnfSharedCoreServices,
      ebnf::install_core_module>();

  int failures = 0;
  int parsed = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".ebnf")
      continue;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<ebnf::ast::Document>) {
      failures += fail(entry.path(), "built no Document");
      continue;
    }

    const auto& document = *static_cast<const ebnf::ast::Document*>(root);
    if (document.rules.empty()) {
      failures += fail(entry.path(), "declares no rule");
      continue;
    }

    ++parsed;
  }

  {
    const auto result = language.language.parser->parse("<a>::=[{(x|<b>)}]|\n<b>::=y");
    const auto* doc = result.value_as<ebnf::ast::Document>();
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !doc || doc->rules.size() != 2 || doc->rules[0]->lhs->name != "a" ||
        doc->rules[0]->alternatives.size() != 2 ||
        !doc->rules[0]->alternatives[1]->elements.empty())
      return fail("native-dialect", "one-letter names or empty alternative were lost");
    const auto* alt = doc->rules[0]->alternatives[0];
    const auto* opt = alt->elements.size() == 1
        ? cursiv::ast_cast<ebnf::ast::Optional>(alt->elements[0]) : nullptr;
    const auto* repeated = opt && opt->alternatives.size() == 1 &&
        opt->alternatives[0]->elements.size() == 1
        ? cursiv::ast_cast<ebnf::ast::ZeroOrMore>(opt->alternatives[0]->elements[0]) : nullptr;
    const auto* some = repeated && repeated->alternatives.size() == 1 &&
        repeated->alternatives[0]->elements.size() == 1
        ? cursiv::ast_cast<ebnf::ast::OneOrMore>(repeated->alternatives[0]->elements[0]) : nullptr;
    if (!some || some->alternatives.size() != 2 ||
        some->alternatives[0]->elements.size() != 1 || some->alternatives[1]->elements.size() != 1)
      return fail("native-dialect", "optional/zero/one repetition nesting changed");
    const auto* literal = cursiv::ast_cast<ebnf::ast::Text>(some->alternatives[0]->elements[0]);
    const auto* reference = cursiv::ast_cast<ebnf::ast::Id>(some->alternatives[1]->elements[0]);
    if (!literal || literal->value != "x" || !reference || reference->name != "b")
      return fail("native-dialect", "terminal/reference values changed");
  }

  if (parsed == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");
  if (failures > 0) return 1;

  std::printf("ebnf: %d example(s) parse\n", parsed);
  return 0;
}
