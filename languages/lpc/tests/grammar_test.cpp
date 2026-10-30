#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lpc/core/CoreServices.hpp"
#include "lpc/core/ast.hpp"

namespace {

namespace ast = lpc::ast;

int fail(const char *what) {
  std::fprintf(stderr, "lpc: %s\n", what);
  return 1;
}

std::string read(const char *path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

const char *const kCleanExamples[] = {
    CURSIV_EXAMPLE_DIR "/example1.lpc",  CURSIV_EXAMPLE_DIR "/example2.lpc",
    CURSIV_TEST_DIR "/expressions.lpc",  CURSIV_TEST_DIR "/statements.lpc",
    CURSIV_TEST_DIR "/declarations.lpc",
};

template <class T> const T *as(const ast::Expr *e) {
  if (e == nullptr || e->type != &cursiv::ast_type_tag<T>)
    return nullptr;
  return static_cast<const T *>(e);
}

const ast::FuncDecl *findFunc(const ast::Program &program,
                              std::string_view name) {
  for (const ast::Item *const item : program.items) {
    if (item->type != &cursiv::ast_type_tag<ast::FuncDecl>)
      continue;
    const auto *const fn = static_cast<const ast::FuncDecl *>(item);
    if (fn->name == name)
      return fn;
  }
  return nullptr;
}

const ast::VarDecl *findVarDecl(const ast::CompoundStmt &body,
                                std::string_view name) {
  for (const ast::Item *const item : body.items) {
    if (item->type != &cursiv::ast_type_tag<ast::VarDecl>)
      continue;
    const auto *const decl = static_cast<const ast::VarDecl *>(item);
    for (const ast::VarName *const v : decl->names)
      if (v->name == name)
        return decl;
  }
  return nullptr;
}

const ast::VarName *declaredName(const ast::VarDecl &decl,
                                 std::string_view name) {
  for (const ast::VarName *const v : decl.names)
    if (v->name == name)
      return v;
  return nullptr;
}

} // namespace

int main() {
  {
    const auto audit_language =
        cursiv::create_language<lpc::LpcCoreServices,
                                lpc::LpcSharedCoreServices,
                                lpc::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse(
        "void f(int x) { switch(x) { case x=1: break; case 1+2: break; } }");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    bool assignment = false, arithmetic = false;
    for (const auto *node : audit_result.nodes)
      if (const auto *label = cursiv::ast_cast<lpc::ast::CaseStmt>(node)) {
        assignment =
            assignment || cursiv::ast_cast<lpc::ast::AssignExpr>(label->low);
        arithmetic =
            arithmetic || cursiv::ast_cast<lpc::ast::BinaryExpr>(label->low);
      }
    if (!assignment || !arithmetic)
      return 1;
    const auto checked = cursiv::test::validationHelper(
        audit_language.language,
        "void f(int x) { switch(x) { case x=1: break; } }");
    if (checked.diagnostics.size() != 1)
      return 1;
  }

  const auto language =
      cursiv::create_language<lpc::LpcCoreServices, lpc::LpcSharedCoreServices,
                              lpc::install_core_module>();

  for (const auto source : {"float f(){return 1_000.5e-2;}", "float f(){return 1._2;}", R"LPC(string f(){return "\q";})LPC", "int f(){return '\\x';}", "int f(int x : (: 1 :)){return x;}", "mixed f(){return (: efun::write, 1 :);}", "mixed f(){return calls[0](1);}", "int f(){switch(1){case 1..:break;case ..0:break;}return 1;}"}) {
    const auto r = language.language.parser->parse(source);
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) { std::fprintf(stderr, "source: %s\n", source); return fail("source-backed native form rejected"); }
  }
  {
    const auto r = language.language.parser->parse(R"LPC(
async promise<int> f(promise<int> input) {
  int array xs = ({1,2});
  mixed value = map?.item ?? 5;
  value ??= 6;
  mixed key = map?.[0];
  mixed other = map.?[1];
  string text = `value=${value}` "!";
  mixed caught = acatch { await input; };
  mixed slice = xs[<1..<2];
  for(int i=0;i<1;i++){}
  return await input;
}
)LPC");
    bool promise = false, array = false, optionalMember = false, optionalIndex = false;
    bool coalesce = false, interpolation = false, concat = false, reverse = false, initialization = false;
    for (const auto* n : r.nodes) {
      if (const auto* t = cursiv::ast_cast<ast::TypeSpec>(n)) { promise |= t->baseType == "promise" && t->typeArgument; array |= t->arrayKeyword; }
      if (const auto* v = cursiv::ast_cast<ast::MemberExpr>(n)) optionalMember |= v->optional;
      if (const auto* v = cursiv::ast_cast<ast::IndexExpr>(n)) optionalIndex |= v->optional;
      if (const auto* v = cursiv::ast_cast<ast::BinaryExpr>(n)) coalesce |= v->op == "??";
      if (const auto* v = cursiv::ast_cast<ast::TemplatePart>(n)) interpolation |= v->expression != nullptr;
      concat |= cursiv::ast_cast<ast::StringConcat>(n) != nullptr;
      if (const auto* v = cursiv::ast_cast<ast::SliceExpr>(n)) reverse |= v->lowFromEnd && v->highFromEnd && v->exclusive;
      if (const auto* v = cursiv::ast_cast<ast::ForStmt>(n)) initialization |= v->initVar && v->initVarOp == "=";
    }
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !promise || !array || !optionalMember || !optionalIndex ||
        !coalesce || !interpolation || !concat || !reverse || !initialization) return fail("native source-family AST capture lost");
  }
  for (const auto source : {"int if;", "mixed f(){return (:);}", "int asm;", "int f(){return '\\8';}", "int f(){return '\\9';}", "int f(){return '';}", "int f(){return '\\u0041';}", "int f(){int return;}", "int f(){return map?.;}", "int f(){return `unterminated;}"}) {
    const auto r = language.language.parser->parse(source);
    if (r.accepted && !r.cst().recovered() && r.diagnostics.empty()) return fail("native keyword/malformed extension accepted");
  }
  for (const auto source : {"int f(){return await p;}", "mixed f(){return acatch(1);}", "async mixed f(){return catch(await p);}", "async mixed f(){return time_expression(acatch(1));}"}) {
    const auto r = cursiv::test::validationHelper(language.language, source);
    if (r.diagnostics.empty()) return fail("native async context needs diagnostic");
  }
  for (const char *const path : kCleanExamples) {
    const std::string text = read(path);
    if (text.empty())
      return fail(path);

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      std::fprintf(stderr, "lpc: %s does not parse clean\n", path);
      return 1;
    }
    if (!result.diagnostics.empty()) {
      std::fprintf(stderr, "lpc: %s carries a diagnostic\n", path);
      return 1;
    }
    const cursiv::AstNode *const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<ast::Program>) {
      std::fprintf(stderr, "lpc: %s built no Program\n", path);
      return 1;
    }
  }

  // Precedence: `-a * b` must bind the unary minus tighter than `*`, and
  // `a || b && c` must bind `&&` tighter than `||` -- proof the
  // tree shape is right, not just that the file parses.
  {
    const std::string text = read(CURSIV_TEST_DIR "/expressions.lpc");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &program = *result.value_as<ast::Program>();

    const ast::FuncDecl *const fn = findFunc(program, "precedence_test");
    if (fn == nullptr || fn->body == nullptr)
      return fail("precedence_test not found");

    const ast::VarDecl *const r2Decl = findVarDecl(*fn->body, "r2");
    if (r2Decl == nullptr)
      return fail("r2 not found");
    const ast::VarName *const r2 = declaredName(*r2Decl, "r2");
    if (r2 == nullptr || r2->init == nullptr)
      return fail("r2 has no init");
    const auto *const r2Mul = as<ast::BinaryExpr>(r2->init);
    if (r2Mul == nullptr || r2Mul->op != "*")
      return fail("r2 init is not `left * right`");
    const auto *const r2Neg = as<ast::PrefixExpr>(r2Mul->left);
    if (r2Neg == nullptr || r2Neg->op != "-")
      return fail("`-a * b`: `*` root's left is not the negated `a`");

    const ast::VarDecl *const r3Decl = findVarDecl(*fn->body, "r3");
    if (r3Decl == nullptr)
      return fail("r3 not found");
    const ast::VarName *const r3 = declaredName(*r3Decl, "r3");
    if (r3 == nullptr || r3->init == nullptr)
      return fail("r3 has no init");
    const auto *const r3Or = as<ast::BinaryExpr>(r3->init);
    if (r3Or == nullptr || r3Or->op != "||")
      return fail("r3 init is not `left || right`");
    const auto *const r3And = as<ast::BinaryExpr>(r3Or->right);
    if (r3And == nullptr || r3And->op != "&&")
      return fail("`a || b && c`: `||` root's right is not the `&&`");
  }

  // A `class` type names its class by any identifier, and
  // `new(class Point, ...)` is the construct that exercises it.
  {
    const std::string text = read(CURSIV_TEST_DIR "/declarations.lpc");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &program = *result.value_as<ast::Program>();

    const ast::FuncDecl *const fn = findFunc(program, "use_class");
    if (fn == nullptr || fn->body == nullptr)
      return fail("use_class not found");
    const ast::VarDecl *const pDecl = findVarDecl(*fn->body, "p");
    if (pDecl == nullptr)
      return fail("p not found");
    if (pDecl->type == nullptr || pDecl->type->className != "Point")
      return fail("`class Point p`: declared type's className is not Point");
    const ast::VarName *const p = declaredName(*pDecl, "p");
    if (p == nullptr || p->init == nullptr)
      return fail("p has no init");
    const auto *const newExpr = as<ast::NewClassExpr>(p->init);
    if (newExpr == nullptr || newExpr->className != "Point")
      return fail(
          "`new(class Point, ...)`: NewClassExpr.className is not Point");
    if (newExpr->inits.size() != 2 || newExpr->inits[0]->name != "x" ||
        newExpr->inits[1]->name != "y")
      return fail("`new(class Point, x: 1, y: 2)`: inits mismatch");
  }

  // A switch range label (`case 1..5:`) carries both bounds.
  {
    const std::string text = read(CURSIV_TEST_DIR "/statements.lpc");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    const auto &program = *result.value_as<ast::Program>();

    const ast::FuncDecl *const fn = findFunc(program, "switch_ranges");
    if (fn == nullptr || fn->body == nullptr)
      return fail("switch_ranges not found");
    const ast::SwitchStmt *sw = nullptr;
    for (const ast::Item *const item : fn->body->items)
      if (item->type == &cursiv::ast_type_tag<ast::SwitchStmt>)
        sw = static_cast<const ast::SwitchStmt *>(item);
    if (sw == nullptr)
      return fail("switch statement not found");
    const ast::CaseStmt *rangeCase = nullptr;
    for (const ast::Stmt *const item : sw->items)
      if (item->type == &cursiv::ast_type_tag<ast::CaseStmt>) {
        const auto *const c = static_cast<const ast::CaseStmt *>(item);
        if (c->high != nullptr)
          rangeCase = c;
      }
    if (rangeCase == nullptr || rangeCase->low == nullptr)
      return fail("`case 1..5:` did not build a CaseStmt with both bounds");
  }

  std::puts("lpc: every example parses clean");
  return 0;
}
