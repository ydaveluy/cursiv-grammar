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

  // MIT Argus Reference Manual (1987), sections 6.2 and 9.16: exponent
  // association and lexical forms missing from the former narrow terminals.
  {
    const auto r = language.language.parser->parse(R"arg(
E = equates
  Power = 2 ** 3 ** 4
  A = .0314E+2
  B = 3.
  C = 314e-2
  D = '\''
  S = "quoted \" text"
  Octal = \177
  Hex = \#FF
end E
)arg");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("native literal forms do not parse clean");
    const auto* m = cursiv::ast_cast<argus::ast::Module>(r.value);
    const auto* eq = m ? cursiv::ast_cast<argus::ast::Equates_>(m->unit) : nullptr;
    if (eq == nullptr || eq->equates.size() != 8)
      return fail("native literal forms lost equates");
    const auto* powerExpr =
        std::get_if<argus::ast::Expression*>(&eq->equates[0]->value);
    const auto* power = powerExpr
                            ? cursiv::ast_cast<argus::ast::BinaryExpr>(*powerExpr)
                            : nullptr;
    if (power == nullptr || power->op != argus::ast::Operator::power ||
        cursiv::ast_cast<argus::ast::BinaryExpr>(power->right) == nullptr ||
        cursiv::ast_cast<argus::ast::BinaryExpr>(power->left) != nullptr)
      return fail("native power must associate right");
    for (std::size_t i = 1; i <= 3; ++i) {
      const auto* value =
          std::get_if<argus::ast::Expression*>(&eq->equates[i]->value);
      if (value == nullptr || cursiv::ast_cast<argus::ast::RealLit>(*value) == nullptr)
        return fail("native real literals lost their AST classification");
    }
    for (std::size_t i = 6; i <= 7; ++i) {
      const auto* value =
          std::get_if<argus::ast::Expression*>(&eq->equates[i]->value);
      if (value == nullptr || cursiv::ast_cast<argus::ast::IntLit>(*value) == nullptr)
        return fail("native based integers lost their AST classification");
    }
    const auto upper = language.language.parser->parse(
        "_Identity = PROC (_value: INT) RETURNS(INT) "
        "RETURN _value END _Identity");
    if (!upper.accepted || upper.cst().recovered() ||
        !upper.diagnostics.empty())
      return fail("native keywords are case-insensitive and names may start with underscore");
    const auto bindings = language.language.parser->parse(
        "P = proc(xs: sequence[int with {copy: copier}]) end P");
    if (!bindings.accepted || bindings.cst().recovered() ||
        !bindings.diagnostics.empty())
      return fail("native braced operation bindings do not parse clean");
    bool foundBinding = false;
    for (const auto* node : bindings.nodes)
      if (const auto* actual = cursiv::ast_cast<argus::ast::TypeActual>(node))
        if (actual->bindings.size() == 1 &&
            actual->bindings[0]->names.size() == 1 &&
            actual->bindings[0]->names[0] == "copy")
          foundBinding = true;
    if (!foundBinding)
      return fail("native braced operation bindings lost their AST");
  }

  // Appendix I groups parameters, exception payloads and returned values
  // with explicit delimiters; preserve their individual members.
  {
    const auto r = language.language.parser->parse(R"arg(
Allowed = {t | t has copy: proctype(t) returns(t)}
P = proc [t: type](xs: atomic array[int], f: proctype())
    returns(int, int) signals(oops(int, string))
  own counter: int := 0
  a: atomic record[x: int]
  b: atomic variant[x: int]
  x: int := f()[1]
  c: proctype(int) := bind f(1)
  tagcase value
    tag a (x: int): return(1, 2)
  end
  tagtest value
    wtag a (x: int): signal oops(1, "text")
  end
  tagwait value
    tag a (x: int): yield(1, 2)
  end
  f() except
    when oops(x: int, y: string): exit done(1, 2)
    others(e: string): return(0, 0)
  end
  return(1, 2)
end P
)arg");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty())
      return fail("native declaration and statement delimiters failed");
    bool own = false, index = false, bind = false, formals = false,
         signals = false, returned = false, signalled = false, yielded = false,
         exited = false, tag = false, atomicTag = false, handled = false,
         others = false, typeSet = false;
    for (const auto* node : r.nodes) {
      if (const auto* body = cursiv::ast_cast<argus::ast::Body>(node))
        own |= body->ownVars.size() == 1;
      index |= cursiv::ast_cast<argus::ast::IndexExpr>(node) != nullptr;
      if (const auto* closure = cursiv::ast_cast<argus::ast::BindExpr>(node))
        bind |= closure->args.size() == 1 &&
                cursiv::ast_cast<argus::ast::Reference>(closure->target) != nullptr;
      if (const auto* p = cursiv::ast_cast<argus::ast::Procedure>(node)) {
        formals |= p->parms.size() == 1;
        signals |= p->signals && p->signals->exceptions.size() == 1 &&
                   p->signals->exceptions[0]->types.size() == 2;
      }
      if (const auto* s = cursiv::ast_cast<argus::ast::ReturnStatement>(node))
        returned |= s->values.size() == 2;
      if (const auto* s = cursiv::ast_cast<argus::ast::SignalStatement>(node))
        signalled |= s->values.size() == 2;
      if (const auto* s = cursiv::ast_cast<argus::ast::YieldStatement>(node))
        yielded |= s->values.size() == 2;
      if (const auto* s = cursiv::ast_cast<argus::ast::ExitStatement>(node))
        exited |= s->values.size() == 2;
      if (const auto* t = cursiv::ast_cast<argus::ast::TagArm>(node))
        tag |= t->declName == "x" && t->declType;
      if (const auto* t = cursiv::ast_cast<argus::ast::ATagArm>(node))
        atomicTag |= t->decls.size() == 1;
      if (const auto* h = cursiv::ast_cast<argus::ast::WhenHandler>(node))
        handled |= h->decls.size() == 2;
      if (const auto* h = cursiv::ast_cast<argus::ast::OthersHandler>(node))
        others |= h->decls.size() == 1;
      if (const auto* set = cursiv::ast_cast<argus::ast::TypeSet>(node))
        typeSet |= set->binder == "t" && set->elements.size() == 1;
    }
    if (!own || !index || !bind || !formals || !signals || !returned ||
        !signalled || !yielded || !exited || !tag || !atomicTag || !handled ||
        !others || !typeSet) {
      std::fprintf(stderr,
                   "argus native fields: own=%d index=%d bind=%d formals=%d "
                   "signals=%d return=%d signal=%d yield=%d exit=%d tag=%d "
                   "atomicTag=%d when=%d others=%d typeSet=%d\n",
                   own, index, bind, formals, signals, returned, signalled,
                   yielded, exited, tag, atomicTag, handled, others, typeSet);
      return fail("native declaration/statement fields were not preserved");
    }
  }

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
