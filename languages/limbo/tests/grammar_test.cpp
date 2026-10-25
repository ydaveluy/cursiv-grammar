// The grammar's smoke test: the upstream sample and this port's own
// features document both parse clean, and a few real field values read
// back -- a converter that silently gave the wrong value would still pass
// a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "limbo/core/CoreServices.hpp"
#include "limbo/core/ast.hpp"

namespace {

namespace ast = limbo::ast;

int fail(const char* what) {
  std::fprintf(stderr, "limbo: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      limbo::LimboCoreServices, limbo::LimboSharedCoreServices,
      limbo::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  // example1.b: the upstream corpus's own sample.
  {
    const std::string text = read(CURSIV_EXAMPLE_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("example1.b does not parse");
    if (!result.diagnostics.empty())
      return fail("example1.b carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("example1.b built no program");
    if (program->name != "Command")
      return fail("example1.b: the implement name did not read back");
    if (program->decls.size() != 5)
      return fail("example1.b: wrong top-level declaration count");
    const auto* init = as<ast::FunctionDef>(program->decls[4]);
    if (init == nullptr || init->nameParts.size() != 1 ||
        init->nameParts[0] != "init")
      return fail("example1.b: the function name did not read back");
    if (init->signature == nullptr || init->signature->params.size() != 2)
      return fail("example1.b: wrong parameter count");
    const auto* ref = as<ast::RefType>(init->signature->params[0]->type);
    if (ref == nullptr || ref->target == nullptr ||
        ref->target->id1 != "Draw" || ref->target->id2 != "Context")
      return fail("example1.b: the ref/module-qualified type did not read back");
  }

  // features.b: this port's own document, one of each remaining construct.
  {
    const std::string text = read(CURSIV_TEST_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("features.b does not parse");
    if (!result.diagnostics.empty())
      return fail("features.b carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("features.b built no program");
    if (program->name != "Features")
      return fail("features.b: the implement name did not read back");

    const auto* mod = as<ast::ModuleDecl>(program->decls[1]);
    if (mod == nullptr || mod->name != "Math" || mod->members.size() != 2)
      return fail("features.b: the module declaration did not read back");
    const auto* con = as<ast::ConDecl>(mod->members[0]);
    if (con == nullptr || con->names.size() != 1 || con->names[0] != "PI")
      return fail("features.b: the module's con member did not read back");

    const auto* adt = as<ast::AdtDecl>(program->decls[2]);
    if (adt == nullptr || adt->name != "Shape" || adt->members.size() != 4)
      return fail("features.b: the adt declaration did not read back");
    const auto* selfFn = as<ast::VarDecl>(adt->members[2]);
    const auto* fnType =
        selfFn != nullptr ? as<ast::FnType>(selfFn->type) : nullptr;
    if (fnType == nullptr || fnType->signature->params.size() != 1 ||
        !fnType->signature->params[0]->isSelf ||
        !fnType->signature->params[0]->refopt)
      return fail("features.b: the refopt self parameter did not read back");
    const auto* pick = as<ast::PickDecl>(adt->members[3]);
    if (pick == nullptr || pick->arms.size() != 2 ||
        pick->arms[0]->tags.size() != 2 ||
        pick->arms[0]->tags[1] != "Square")
      return fail("features.b: the pick declaration did not read back");

    const auto* mainFn = as<ast::FunctionDef>(program->decls.back());
    if (mainFn == nullptr || mainFn->nameParts[0] != "main")
      return fail("features.b: did not find the main function");

    bool foundCast = false, foundCase = false, foundCons = false,
         foundSlice = false;
    for (const ast::Item* item : mainFn->body) {
      const auto* stmt = as<ast::ExprStmt>(item);
      const auto* decl =
          stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
      const auto* target =
          decl != nullptr ? as<ast::Ident>(decl->target) : nullptr;
      if (target != nullptr && target->name == "n") {
        const auto* cast = as<ast::CastExpr>(decl->value);
        const auto* prim =
            cast != nullptr ? as<ast::PrimType>(cast->type) : nullptr;
        if (prim != nullptr && prim->keyword == "big" &&
            as<ast::IntLit>(cast->operand) != nullptr)
          foundCast = true;
      }
      if (target != nullptr && target->name == "s") {
        const auto* cons = as<ast::BinaryExpr>(decl->value);
        if (cons != nullptr && cons->op == "::") foundCons = true;
      }
      if (target != nullptr && target->name == "p") {
        const auto* idx = as<ast::IndexExpr>(decl->value);
        if (idx != nullptr && idx->isSlice && idx->high != nullptr)
          foundSlice = true;
      }
      if (const auto* c = as<ast::CaseStmt>(item); c != nullptr) {
        if (c->arms.size() == 3 && c->arms[1]->labels[0]->b != nullptr)
          foundCase = true;
      }
    }
    if (!foundCast) return fail("features.b: the cast expression did not read back");
    if (!foundCase) return fail("features.b: the case range label did not read back");
    if (!foundCons) return fail("features.b: the '::' operator did not read back");
    if (!foundSlice) return fail("features.b: the slice expression did not read back");
  }

  // The language manual's table (Section 8.3): `::` sits between `|` and
  // `&&`/`||`, and is right-associative.
  {
    const cursiv::ParseResult result = parser.parse(
        "implement P;\ninit()\n{\n  x := a | b :: c || d;\n}\n");
    if (result.cst().recovered()) return fail("a|b::c||d does not parse");
    if (!result.diagnostics.empty())
      return fail("a|b::c||d carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    const auto* init = program != nullptr
                            ? as<ast::FunctionDef>(program->decls.back())
                            : nullptr;
    const auto* stmt =
        init != nullptr && !init->body.empty() ? as<ast::ExprStmt>(init->body[0]) : nullptr;
    const auto* decl = stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
    const auto* orExpr = decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
    if (orExpr == nullptr || orExpr->op != "||")
      return fail("a|b::c||d: || is not the loosest operator");
    const auto* consExpr = as<ast::BinaryExpr>(orExpr->left);
    if (consExpr == nullptr || consExpr->op != "::")
      return fail("a|b::c||d: :: does not sit tighter than ||");
    if (as<ast::BinaryExpr>(consExpr->left) == nullptr)
      return fail("a|b::c||d: :: does not sit looser than |");
  }
  {
    const cursiv::ParseResult result =
        parser.parse("implement P;\ninit()\n{\n  x := a :: b :: c;\n}\n");
    if (result.cst().recovered()) return fail("a::b::c does not parse");
    if (!result.diagnostics.empty())
      return fail("a::b::c carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    const auto* init = program != nullptr
                            ? as<ast::FunctionDef>(program->decls.back())
                            : nullptr;
    const auto* stmt =
        init != nullptr && !init->body.empty() ? as<ast::ExprStmt>(init->body[0]) : nullptr;
    const auto* decl = stmt != nullptr ? as<ast::DeclareExpr>(stmt->value) : nullptr;
    const auto* cons = decl != nullptr ? as<ast::BinaryExpr>(decl->value) : nullptr;
    if (cons == nullptr || cons->op != "::" ||
        as<ast::BinaryExpr>(cons->left) != nullptr ||
        as<ast::BinaryExpr>(cons->right) == nullptr)
      return fail("a::b::c: :: is not right-associative");
  }

  std::puts("limbo: the samples parse");
  return 0;
}
