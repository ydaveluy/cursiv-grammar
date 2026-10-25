// The smoke test of the language: parse every document in example/ the way a
// workspace does, and check each tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "pegen/core/CoreServices.hpp"
#include "pegen/core/ast.hpp"

namespace {

constexpr const char* kExamples[] = {
    CURSIV_EXAMPLE_DIR "/python-10.gram",
    CURSIV_EXAMPLE_DIR "/python-11.gram",
    CURSIV_EXAMPLE_DIR "/python-11-from-md.gram",
    CURSIV_TEST_DIR "/features.gram",
};

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "pegen: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      pegen::PegenCoreServices, pegen::PegenSharedCoreServices,
      pegen::install_core_module>();

  for (const char* path : kExamples) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the sample parses with diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pegen::ast::Grammar>)
      return fail(path, "the parse built no grammar");

    const auto& grammar = *static_cast<const pegen::ast::Grammar*>(root);
    if (grammar.rules.empty()) return fail(path, "the grammar declares no rule");
  }

  const std::string source = R"GRAM(@empty ''
_entrée[List[Tuple[str, int]]](memo):
    | a[Token *]=NAME 'match' "match" { {"}": [a]} # ignored }
       }
)GRAM";
  const auto result = language.language.parser->parse(source);
  const auto* grammar = cursiv::ast_cast<pegen::ast::Grammar>(result.value);
  if (!grammar || result.cst().recovered() || !result.diagnostics.empty() ||
      grammar->rules.size() != 1 || grammar->metas.size() != 1)
    return fail("regression", "typed rule and quoted action do not parse");
  const auto* rule = grammar->rules[0];
  if (rule->name != "_entrée" || rule->returnType != "List[Tuple[str, int]]" ||
      !rule->memo || grammar->metas[0]->valueString != "''")
    return fail("regression", "rule type or metadata spelling was lost");
  const auto* alt = rule->body->alts[0];
  const auto* binding = cursiv::ast_cast<pegen::ast::Named>(alt->items[0]);
  const auto* hard = cursiv::ast_cast<pegen::ast::Named>(alt->items[1]);
  const auto* soft = cursiv::ast_cast<pegen::ast::Named>(alt->items[2]);
  if (!binding || binding->attrName != "a" || binding->attrType != "Token *" ||
      !hard || !soft ||
      cursiv::ast_cast<pegen::ast::StringLit>(hard->value->value)->value != "'match'" ||
      cursiv::ast_cast<pegen::ast::StringLit>(soft->value->value)->value != "\"match\"" ||
      alt->action.find("# ignored }") == std::string_view::npos)
    return fail("regression", "binding, keyword kind or action text was lost");
  const std::string duplicate = "start: NAME\nstart: STRING\n";
  const auto validation = cursiv::test::validationHelper(language.language, duplicate);
  bool diagnosed = false;
  for (const auto& diagnostic : validation.diagnostics)
    diagnosed |= diagnostic.code && diagnostic.code->isString() &&
                 diagnostic.code->string() == "pegen.duplicateRule" &&
                 diagnostic.range.begin == 12 && diagnostic.range.end == 17;
  if (!diagnosed) return fail("regression", "duplicate rule has no precise diagnostic");

  std::puts("pegen: every sample parses");
  return 0;
}
