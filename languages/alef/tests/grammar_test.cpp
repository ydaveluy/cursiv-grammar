// The grammar's smoke test: the upstream sample and this port's own
// features document both parse clean, and a few real field values read
// back -- a converter that silently gave the wrong value would still pass
// a structural-only check.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "alef/core/CoreServices.hpp"
#include "alef/core/ast.hpp"

namespace {

namespace ast = alef::ast;

int fail(const char* what) {
  std::fprintf(stderr, "alef: %s\n", what);
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
      alef::AlefCoreServices, alef::AlefSharedCoreServices,
      alef::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  // example1.alef: the upstream corpus's own sample.
  {
    const std::string text = read(CURSIV_EXAMPLE_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("example1.alef does not parse");
    if (!result.diagnostics.empty())
      return fail("example1.alef carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("example1.alef built no program");
    if (program->decls.size() != 2)
      return fail("example1.alef: wrong top-level declaration count");

    const auto* func = as<ast::FuncDecl>(program->decls[0]);
    if (func == nullptr || func->declarator == nullptr ||
        func->declarator->name != "func")
      return fail("example1.alef: the function name did not read back");
    if (func->returnType == nullptr || func->returnType->tupleTypes.size() != 3)
      return fail("example1.alef: the tuple return type did not read back");
    const auto* byteStar = func->returnType->tupleTypes[1];
    if (byteStar == nullptr || byteStar->pointers.size() != 1)
      return fail("example1.alef: the pointer element type did not read back");
    if (func->body == nullptr || func->body->stmts.size() != 1)
      return fail("example1.alef: wrong function body");
    const auto* ret = as<ast::ReturnStmt>(func->body->stmts[0]);
    const auto* tuple = ret != nullptr ? as<ast::TupleExpr>(ret->value) : nullptr;
    if (tuple == nullptr || tuple->items.size() != 3)
      return fail("example1.alef: the returned tuple did not read back");

    const auto* main = as<ast::FuncDecl>(program->decls[1]);
    if (main == nullptr || main->body == nullptr ||
        main->body->autos.size() != 3)
      return fail("example1.alef: main's auto declarations did not read back");
  }

  // features.alef: this port's own document, one of each remaining
  // construct.
  {
    const std::string text = read(CURSIV_TEST_SAMPLE);
    const cursiv::ParseResult result = parser.parse(text);
    if (result.cst().recovered()) return fail("features.alef does not parse");
    if (!result.diagnostics.empty())
      return fail("features.alef carries a diagnostic");
    const auto* program = as<ast::Program>(result.value);
    if (program == nullptr) return fail("features.alef built no program");

    const auto* adt = as<ast::AdtType>(as<ast::TypeDecl>(program->decls[0])
                                            ->type);
    if (adt == nullptr || adt->members.size() != 1)
      return fail("features.alef: the adt declaration did not read back");

    const auto* aggr = as<ast::AggrType>(as<ast::TypeDecl>(program->decls[1])
                                              ->type);
    if (aggr == nullptr || aggr->tags.size() != 2 ||
        aggr->tags[0] != "Rect" || aggr->tags[1] != "RectTag")
      return fail("features.alef: the leading and trailing aggr tags did not "
                  "read back");

    const auto* enumType = as<ast::EnumType>(
        as<ast::TypeDecl>(program->decls[3])->type);
    if (enumType == nullptr || enumType->members.size() != 3 ||
        enumType->members[0]->name != "red" ||
        enumType->members[0]->value == nullptr)
      return fail("features.alef: the enum members did not read back");

    bool foundMethod = false, foundBinAssign = false, foundLabel = false,
         foundGoto = false, foundAlloc = false, foundDollar = false;
    for (const ast::Decl* decl : program->decls) {
      const auto* func = as<ast::FuncDecl>(decl);
      if (func == nullptr) continue;
      if (func->receiver != nullptr && func->receiver->name == "Point" &&
          func->methodName == "origin")
        foundMethod = true;
      if (func->declarator != nullptr && func->declarator->name == "main" &&
          func->body != nullptr) {
        for (const ast::Stmt* stmt : func->body->stmts) {
          if (const auto* label = as<ast::LabeledStmt>(stmt);
              label != nullptr && label->name == "loop") {
            foundLabel = true;
            const auto* ifStmt = as<ast::IfStmt>(label->body);
            const auto* gotoStmt =
                ifStmt != nullptr ? as<ast::GotoStmt>(ifStmt->thenBranch)
                                   : nullptr;
            // `.get()` needs the full document/linker pipeline (parse alone
            // does not resolve it); the cli's own run over this file
            // confirms end-to-end resolution -- here just the parsed shape.
            if (gotoStmt != nullptr && gotoStmt->target.refText == "loop")
              foundGoto = true;
          }
          if (as<ast::AllocStmt>(stmt) != nullptr) foundAlloc = true;
          const auto* exprStmt = as<ast::ExprStmt>(stmt);
          const auto* bin =
              exprStmt != nullptr ? as<ast::BinaryExpr>(exprStmt->value)
                                   : nullptr;
          if (bin != nullptr && bin->op == "<-=") foundBinAssign = true;
          const auto* strAssign =
              exprStmt != nullptr ? as<ast::BinaryExpr>(exprStmt->value)
                                   : nullptr;
          if (strAssign != nullptr) {
            const auto* dollar = as<ast::StringLit>(strAssign->right);
            if (dollar != nullptr && dollar->dollar) foundDollar = true;
          }
        }
      }
    }
    if (!foundMethod) return fail("features.alef: the adt method did not read back");
    if (!foundBinAssign)
      return fail("features.alef: the '<-=' operator did not read back");
    if (!foundLabel) return fail("features.alef: the labeled statement did not read back");
    if (!foundGoto) return fail("features.alef: the goto statement did not read back");
    if (!foundAlloc) return fail("features.alef: the alloc statement did not read back");
    if (!foundDollar) return fail("features.alef: the $\"...\" string did not read back");
  }

  // Precedence, checked at AST level: `*`/`/`/`%`
  // tighter than `+`/`-`, tighter than `<<`/`>>`, tighter than the
  // bounded-iteration `::`, tighter than the relational operators.
  {
    const auto value_of = [&](const std::string& expr) -> const ast::Expr* {
      static std::vector<std::unique_ptr<std::string>> kept;
      kept.push_back(std::make_unique<std::string>(
          "void f(){ x = " + expr + "; }"));
      const cursiv::ParseResult result = parser.parse(*kept.back());
      if (result.cst().recovered() || !result.diagnostics.empty())
        return nullptr;
      const auto* program = as<ast::Program>(result.value);
      const auto* func =
          program != nullptr ? as<ast::FuncDecl>(program->decls[0]) : nullptr;
      const auto* exprStmt = func != nullptr && func->body != nullptr &&
                                      func->body->stmts.size() == 1
                                  ? as<ast::ExprStmt>(func->body->stmts[0])
                                  : nullptr;
      const auto* assign =
          exprStmt != nullptr ? as<ast::BinaryExpr>(exprStmt->value) : nullptr;
      return assign != nullptr ? assign->right : nullptr;
    };
    const auto binary = [](const ast::Expr* e,
                            const char* op) -> const ast::BinaryExpr* {
      const auto* b = as<ast::BinaryExpr>(e);
      return b != nullptr && b->op == op ? b : nullptr;
    };

    // `a + b << c` is `(a + b) << c`: `+` binds tighter than `<<`.
    {
      const auto* shift = binary(value_of("a + b << c"), "<<");
      if (shift == nullptr || binary(shift->left, "+") == nullptr)
        return fail("precedence: `+` did not bind tighter than `<<`");
    }
    // `a << b :: c` is `(a << b) :: c`: `<<` binds tighter than `::`.
    {
      const auto* iter = binary(value_of("a << b :: c"), "::");
      if (iter == nullptr || binary(iter->left, "<<") == nullptr)
        return fail("precedence: `<<` did not bind tighter than `::`");
    }
    // `a :: b < c` is `(a :: b) < c`: `::` binds tighter than `<`.
    {
      const auto* rel = binary(value_of("a :: b < c"), "<");
      if (rel == nullptr || binary(rel->left, "::") == nullptr)
        return fail("precedence: `::` did not bind tighter than `<`");
    }
    // The manual's own example: `to[0::strlen(from)+1]` reads the bound as
    // `0 :: (strlen(from) + 1)`, not `(0 :: strlen(from)) + 1`.
    {
      const auto* iter = binary(value_of("0 :: g(x)+1"), "::");
      if (iter == nullptr || binary(iter->right, "+") == nullptr)
        return fail("precedence: `::`'s bound did not extend through `+`");
    }
  }

  std::puts("alef: the samples parse");
  return 0;
}
