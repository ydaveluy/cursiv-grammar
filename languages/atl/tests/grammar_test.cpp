// The smoke test of the language: every document under tests/ (the upstream
// corpus ships no examples/ at all) parses clean, and module.atl is read
// back for real field values.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "atl/core/CoreServices.hpp"
#include "atl/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "atl: %s\n", what);
  return 1;
}

std::string read_file(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<atl::AtlCoreServices, atl::AtlSharedCoreServices,
                              atl::install_core_module>();
  {
    const auto result = language.language.parser->parse("module M; create OUT: M from IN: M; lazy unique rule R { from x : IN!A to y : OUT!B }");
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const atl::ast::MatchedRule* declaration = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<atl::ast::MatchedRule>(node)) declaration = value;
    if (!declaration || declaration->modifiers.size() != 2) return 1;
    const auto invalid = cursiv::test::validationHelper(language.language, "module M; create OUT: M from IN: M; lazy lazy rule R { from x : IN!A }");
    if (invalid.diagnostics.size() != 1) return 1;
  }


  const char* const docs[] = {
      CURSIV_TEST_DIR "/module.atl",         //
      CURSIV_TEST_DIR "/library.atl",        //
      CURSIV_TEST_DIR "/query.atl",          //
      CURSIV_TEST_DIR "/chained_calls.atl",  //
  };

  // The texts must outlive every read of the parsed trees below: every
  // string_view in a CST/AST points into them.
  std::string texts[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i)
    texts[i] = read_file(docs[i]);

  for (std::size_t i = 0; i < std::size(docs); ++i) {
    if (texts[i].empty()) return fail("a document is not readable");

    const cursiv::ParseResult result =
        language.language.parser->parse(texts[i]);
    if (result.cst().recovered())
      return fail("a document does not parse clean");
    if (!result.diagnostics.empty())
      return fail("a document carries an unexpected diagnostic");
  }

  // module.atl: check the module header and a matched rule's shape.
  const cursiv::ParseResult moduleResult =
      language.language.parser->parse(texts[0]);
  const cursiv::AstNode* const root = moduleResult.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<atl::ast::Module>)
    return fail("module.atl did not build a Module");

  const auto& module = *static_cast<const atl::ast::Module*>(root);
  if (module.name != "Families2Persons") return fail("module name misread");
  if (module.targetModels.size() != 2 || module.sourceModels.size() != 2)
    return fail("module target/source model counts misread");
  if (module.targetModels[0]->alias != "OUT" ||
      module.targetModels[0]->modelName != "Persons")
    return fail("first target model misread");
  if (module.mode != "from") return fail("transformation mode misread");
  if (module.uses.size() != 1 || module.uses[0] != "Utils")
    return fail("uses clause misread");

  const atl::ast::MatchedRule* man = nullptr;
  const atl::ast::CalledRule* main = nullptr;
  for (const atl::ast::ModuleElement* const elem : module.elements) {
    if (elem->type == &cursiv::ast_type_tag<atl::ast::MatchedRule>) {
      const auto& rule = *static_cast<const atl::ast::MatchedRule*>(elem);
      if (rule.name == "Man") man = &rule;
    } else if (elem->type == &cursiv::ast_type_tag<atl::ast::CalledRule>) {
      const auto& rule = *static_cast<const atl::ast::CalledRule*>(elem);
      if (rule.name == "Main") main = &rule;
    }
  }
  if (man == nullptr) return fail("rule Man was not found");
  if (man->inPattern == nullptr || man->inPattern->elements.size() != 1 ||
      man->inPattern->elements[0]->inModels.size() != 2)
    return fail("rule Man's in-pattern misread");
  if (man->outPattern == nullptr || man->outPattern->elements.size() != 1 ||
      man->outPattern->elements[0]->name != "t")
    return fail("rule Man's out-pattern misread");
  if (man->action == nullptr || man->action->statements.size() != 2)
    return fail("rule Man's action block misread");

  if (main == nullptr) return fail("rule Main was not found");
  if (!main->isEntrypoint || main->params.size() != 1 ||
      main->variables.size() != 2)
    return fail("rule Main's header misread");

  // OCL precedence (tightest to loosest): relational binds tighter than
  // equality; "and" tighter than "or" tighter than "xor" tighter than
  // "implies"; all left-associative.
  const struct {
    const char* text;
    const char* rootOp;
  } precedenceProbes[] = {
      {"query Q = 1 = 2 < 3;", "="},
      {"query Q = true or false and true;", "or"},
      {"query Q = true xor false or true;", "xor"},
      {"query Q = true implies false xor true;", "implies"},
      {"query Q = true and false and true;", "and"},
  };
  for (const auto& probe : precedenceProbes) {
    const std::string text = probe.text;
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty())
      return fail("a precedence probe does not parse clean");
    if (result.value == nullptr ||
        result.value->type != &cursiv::ast_type_tag<atl::ast::Query>)
      return fail("a precedence probe did not build a Query");
    const auto& query = *static_cast<const atl::ast::Query*>(result.value);
    if (query.body == nullptr ||
        query.body->type != &cursiv::ast_type_tag<atl::ast::BinaryExpr>)
      return fail("a precedence probe's body is not a BinaryExpr");
    const auto& root = *static_cast<const atl::ast::BinaryExpr*>(query.body);
    if (root.op != probe.rootOp)
      return fail("a precedence probe's root operator is wrong");
  }

  std::puts("atl: every document parses, field values read back correctly");
  return 0;
}
