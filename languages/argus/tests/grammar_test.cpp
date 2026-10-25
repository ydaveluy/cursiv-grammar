// The smoke test of the language: every document under tests/ (this port's
// corpus -- the upstream language has no examples/ directory at all) parses
// clean, and one of them is read back for real field values.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "argus/core/CoreServices.hpp"
#include "argus/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "argus: %s\n", what);
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
      argus::ArgusCoreServices, argus::ArgusSharedCoreServices,
      argus::install_core_module>();

  const char* const docs[] = {
      CURSIV_TEST_DIR "/statements.arg",       //
      CURSIV_TEST_DIR "/expressions.arg",      //
      CURSIV_TEST_DIR "/types.arg",            //
      CURSIV_TEST_DIR "/guardian.arg",         //
      CURSIV_TEST_DIR "/cluster.arg",          //
      CURSIV_TEST_DIR "/iterator.arg",         //
      CURSIV_TEST_DIR "/equates_generic.arg",  //
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
    if (root == nullptr || root->type != &cursiv::ast_type_tag<argus::ast::Module>)
      return fail("the parse built no module");
  }

  // Read real field values back out of statements.arg: a converter that
  // silently yields the wrong value would still pass every structural check
  // above.
  const cursiv::ParseResult result =
      language.language.parser->parse(texts[0]);
  const auto& module =
      *static_cast<const argus::ast::Module*>(result.value);
  if (module.unit == nullptr ||
      module.unit->type != &cursiv::ast_type_tag<argus::ast::Procedure>)
    return fail("statements.arg's unit is not the Demo procedure");

  const auto& demo = *static_cast<const argus::ast::Procedure*>(module.unit);
  if (demo.name != "Demo" || demo.endName != "Demo")
    return fail("the procedure's name/endName were not read back correctly");
  if (demo.args.size() != 1 || demo.args[0]->names.size() != 2 ||
      demo.args[0]->names[0] != "a" || demo.args[0]->names[1] != "b")
    return fail("the procedure's argument names were not read back correctly");
  if (demo.returns == nullptr || demo.returns->types.size() != 1)
    return fail("the procedure's returns clause was not read back correctly");
  if (demo.signals == nullptr || demo.signals->exceptions.size() != 1 ||
      demo.signals->exceptions[0]->name != "oops")
    return fail("the procedure's signals clause was not read back correctly");

  // The first statement is `x: int` -- a declaration with no initializer.
  if (demo.body == nullptr || demo.body->statements.empty() ||
      demo.body->statements[0]->type !=
          &cursiv::ast_type_tag<argus::ast::DeclStatement>)
    return fail("the first statement is not the expected declaration");
  const auto& firstDecl =
      *static_cast<const argus::ast::DeclStatement*>(demo.body->statements[0]);
  if (firstDecl.decls.size() != 1 || firstDecl.decls[0]->names.size() != 1 ||
      firstDecl.decls[0]->names[0] != "x" || !firstDecl.initExprs.empty())
    return fail("the first declaration's fields were not read back correctly");

  // Precedence: `||` shares a level with `+`/`-`, `cand` with `&`, `cor`
  // with `|` (Argus Reference Manual §9.16, Table 9-2) -- not tiers of
  // their own. expressions.arg's E42-E47 probe every mixed pair; each
  // must fold left to right, the second operator applied last (at the
  // root) over the first (nested on the left).
  {
    const cursiv::ParseResult exprResult =
        language.language.parser->parse(texts[1]);
    const auto& exprModule =
        *static_cast<const argus::ast::Module*>(exprResult.value);
    const auto* const equates =
        cursiv::ast_cast<argus::ast::Equates_>(exprModule.unit);
    if (equates == nullptr)
      return fail("expressions.arg's unit is not an equates definition");

    auto find = [&](std::string_view name) -> const argus::ast::Expression* {
      for (const argus::ast::Equate* const e : equates->equates) {
        if (e == nullptr || e->name != name) continue;
        const auto* const expr = std::get_if<argus::ast::Expression*>(&e->value);
        return expr != nullptr ? *expr : nullptr;
      }
      return nullptr;
    };
    auto foldsLeft = [&](std::string_view name, argus::ast::Operator rootOp) {
      const auto* const top =
          cursiv::ast_cast<argus::ast::BinaryExpr>(find(name));
      return top != nullptr && top->op == rootOp &&
             cursiv::ast_cast<argus::ast::BinaryExpr>(top->left) != nullptr;
    };
    if (!foldsLeft("E42", argus::ast::Operator::concat))
      return fail("E42 '1+2||3': '+' and '||' should be one level");
    if (!foldsLeft("E43", argus::ast::Operator::add))
      return fail("E43 '1||2+3': '||' and '+' should be one level");
    if (!foldsLeft("E44", argus::ast::Operator::cand))
      return fail("E44 'true&false cand true': '&' and 'cand' should be one level");
    if (!foldsLeft("E45", argus::ast::Operator::and_))
      return fail("E45 'true cand false&true': 'cand' and '&' should be one level");
    if (!foldsLeft("E46", argus::ast::Operator::cor))
      return fail("E46 'true|false cor true': '|' and 'cor' should be one level");
    if (!foldsLeft("E47", argus::ast::Operator::or_))
      return fail("E47 'true cor false|true': 'cor' and '|' should be one level");
  }

  // Catch-all: `IndexAssignStatement`'s target is any `PostfixExpr` with an
  // optional index, so an unindexed target (`x := 5`) is also a complete
  // `AssignStatement`. Each must take its own reading.
  {
    auto first_stmt_type =
        [&](const std::string& src) -> const cursiv::AstTypeTag* {
      const std::string text = "P = proc ()\n  x: int\n  " + src + "\n  end P\n";
      const cursiv::ParseResult r = language.language.parser->parse(text);
      if (r.cst().recovered() || !r.diagnostics.empty()) return nullptr;
      const auto& m = *static_cast<const argus::ast::Module*>(r.value);
      const auto* const proc = cursiv::ast_cast<argus::ast::Procedure>(m.unit);
      if (proc == nullptr || proc->body == nullptr ||
          proc->body->statements.size() < 2)
        return nullptr;
      return proc->body->statements[1]->type;
    };
    if (first_stmt_type("x := 5") !=
        &cursiv::ast_type_tag<argus::ast::AssignStatement>)
      return fail("'x := 5' should read as AssignStatement, not IndexAssignStatement");
    if (first_stmt_type("x[1] := 5") !=
        &cursiv::ast_type_tag<argus::ast::IndexAssignStatement>)
      return fail("'x[1] := 5' should read as IndexAssignStatement");
  }

  std::puts("argus: every document parses, field values read back correctly");
  return 0;
}
