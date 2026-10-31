// The smoke test of the language: every document under tests/ (the upstream
// corpus ships no examples/ at all) parses clean, and module.atl is read
// back for real field values.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <tuple>

#include "atl/core/CoreServices.hpp"
#include "atl/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "assignment_regressions.hpp"
#include "drop_reverse.hpp"

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
  if (!atl_assignment_regressions::run(*language.language.parser)) return 1;
  if (!atl_drop_reverse::run(language.language)) return 1;
  // An absent else and an explicit empty else have distinct source syntax.
  for (const auto& test : {
           std::tuple{"if (true) {}", false, 0u, 0u},
           std::tuple{"if (true) {} else {}", true, 0u, 0u},
           std::tuple{"if (true) {} else 1;", true, 0u, 1u},
           std::tuple{"if (true) 1; else 2;", true, 1u, 1u},
           std::tuple{"if (true) { if (false) {} else {} }", false, 1u, 0u}}) {
    const auto& [body, hasElse, thenSize, elseSize] = test;
    const std::string source =
        std::string("module M; create OUT:M from IN:M; rule R { ") +
        "from s:IN!A to t:OUT!B do { " + body + " } }";
    const auto parsed = language.language.parser->parse(source);
    const auto* module = cursiv::ast_cast<atl::ast::Module>(parsed.value);
    const auto* rule = module && module->elements.size() == 1
        ? cursiv::ast_cast<atl::ast::MatchedRule>(module->elements[0]) : nullptr;
    const auto* block = rule ? rule->action : nullptr;
    const auto* branch = block && block->statements.size() == 1
        ? cursiv::ast_cast<atl::ast::IfStmt>(block->statements[0]) : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !branch || branch->hasElse != hasElse || branch->thenStmts.size() != thenSize ||
        branch->elseStmts.size() != elseSize || branch->parent != block ||
        !branch->cond || branch->cond->parent != branch)
      return fail("if statement lost its else presence, children or ownership");
    for (const auto* child : branch->thenStmts)
      if (child->parent != branch) return fail("then statement ownership changed");
    for (const auto* child : branch->elseStmts)
      if (child->parent != branch) return fail("else statement ownership changed");
    if (const auto* nested = thenSize == 1
            ? cursiv::ast_cast<atl::ast::IfStmt>(branch->thenStmts[0]) : nullptr)
      if (!nested->hasElse || !nested->elseStmts.empty())
        return fail("nested empty else presence was lost");
  }
  for (const char* text : {"module M; create OUT:M from IN:M; uses Utils;",
                           "library L; uses Utils; uses Other;",
                           "query Q = 1; uses Utils;"}) {
    const auto result = language.language.parser->parse(text);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty())
      return fail("native unquoted library reference rejected");
  }
  {
    const auto result = language.language.parser->parse("query Q = 00012;");
    const auto* q = result.value_as<atl::ast::Query>();
    const auto* i = q ? cursiv::ast_cast<atl::ast::IntLit>(q->body) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !i || i->value != 12)
      return fail("native decimal with leading zeroes lost its value");
  }
  for (const char* literal : {"1e999", "1e-999"}) {
    const std::string source = std::string("query Q = ") + literal + ";";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || result.diagnostics.empty())
      return fail("out-of-range real literal was silently converted");
  }
  {
    const auto result = language.language.parser->parse("query Q = 1.25d;");
    const auto* query = result.value_as<atl::ast::Query>();
    const auto* value = query ? cursiv::ast_cast<atl::ast::FloatLit>(query->body) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !value || value->value != 1.25)
      return fail("representable suffixed real literal lost its value");
  }
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

  // Both native ATL TCS frontends share comparison priority 4 and boolean
  // priority 5; all operators at a priority associate to the left.
  const struct {
    const char* text;
    const char* rootOp;
  } precedenceProbes[] = {
      {"query Q = 1 = 2 < 3;", "<"},
      {"query Q = true or false and true;", "and"},
      {"query Q = true xor false or true;", "or"},
      {"query Q = true implies false xor true;", "xor"},
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
    if (root.op != probe.rootOp || !cursiv::ast_cast<atl::ast::BinaryExpr>(root.left))
      return fail("native ATL precedence or left associativity is wrong");
  }

  std::puts("atl: every document parses, field values read back correctly");
  return 0;
}
