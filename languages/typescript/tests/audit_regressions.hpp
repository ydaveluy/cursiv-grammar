#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "typescript/core/ast.hpp"
#include "typescript/generated/TypeScript/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = typescript::generated::TypeScript::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = typescript::ast;
  auto r = parser->parse(
      "const n=5.; const f2=f<string>; const a=[,1,,]; const Ⅻ = 1; type S = "
      "unique symbol; const v=a as number + 1 * (2 + 3);");
  check(r.accepted && !r.cst().recovered(),
        "instantiation, decimal, elisions, Unicode, unique, assertion "
        "continuation");
  auto *f = as<ast::Program>(r.value);
  if (f && f->body.size() == 6) {
    auto *generic = as<ast::VarDecl>(f->body[1]);
    auto *instance =
        generic && !generic->declarations.empty()
            ? as<ast::InstantiationExpr>(generic->declarations[0]->init)
            : nullptr;
    check(instance && instance->typeArgs.size() == 1 &&
              as<ast::Ident>(instance->expression),
          "bare generic instantiation retained");
    auto *d = as<ast::VarDecl>(f->body[2]);
    auto *ar = d && !d->declarations.empty()
                   ? as<ast::ArrayLit>(d->declarations[0]->init)
                   : nullptr;
    check(ar && ar->elements.size() == 3 && as<ast::Elision>(ar->elements[0]) &&
              as<ast::Elision>(ar->elements[2]),
          "elision positions retained");
    auto *td = as<ast::TypeAliasDecl>(f->body[4]);
    auto *st = td ? as<ast::PredefinedType>(td->type) : nullptr;
    check(st && st->unique, "unique modifier retained");
    auto *vd = as<ast::VarDecl>(f->body[5]);
    auto *sum = vd && !vd->declarations.empty()
                    ? as<ast::BinaryExpr>(vd->declarations[0]->init)
                    : nullptr;
    auto *product = sum ? as<ast::BinaryExpr>(sum->right) : nullptr;
    auto *assertion = sum ? as<ast::AsExpr>(sum->left) : nullptr;
    check(sum && sum->op == "+" && assertion && assertion->target &&
              product && product->op == "*",
          "assertion continuation retains arithmetic precedence");
    auto *group = product ? as<ast::BinaryExpr>(product->right) : nullptr;
    check(group && group->op == "+", "parenthesized operand remains grouped");
  } else
    check(false, "six declarations");
  auto member = parser->parse("const x=5..toString();");
  check(member.accepted && !member.cst().recovered(),
        "member access after trailing-dot literal");
  auto names = parser->parse("const constⅫ = 1; const a‌ = 2;");
  check(names.accepted && !names.cst().recovered(),
        "Unicode continuation also bounds keywords");
  auto ordinary = parser->parse("const x = a + 1 * 2; const y = a + 1 as "
                                "number; const z = a as number as number + 1;");
  check(ordinary.accepted && !ordinary.cst().recovered(),
        "ordinary precedence and chained assertions");
  auto *normal = as<ast::Program>(ordinary.value);
  auto *decl = normal && normal->body.size() == 3
                   ? as<ast::VarDecl>(normal->body[0])
                   : nullptr;
  auto *sum = decl && !decl->declarations.empty()
                  ? as<ast::BinaryExpr>(decl->declarations[0]->init)
                  : nullptr;
  check(sum && sum->op == "+" && as<ast::BinaryExpr>(sum->right) &&
            as<ast::BinaryExpr>(sum->right)->op == "*",
        "ordinary binary tree preserves precedence");
  auto *asserted = normal && normal->body.size() == 3
                       ? as<ast::VarDecl>(normal->body[1])
                       : nullptr;
  check(asserted && !asserted->declarations.empty() &&
            as<ast::AsExpr>(asserted->declarations[0]->init),
        "ordinary assertion retains its AST node");
  return ok;
}
} // namespace audit_regressions
