#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "vhdl/core/ast.hpp"
#include "vhdl/generated/Vhdl/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = vhdl::generated::Vhdl::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = vhdl::ast;
  auto r = parser->parse(
      "package p is constant x : integer := -2 ** 2; end package p;");
  check(r.accepted && !r.cst().recovered(), "signed power expression");
  auto *d = as<ast::DesignFile>(r.value);
  auto *u = d && !d->units.empty() ? as<ast::DesignUnit>(d->units[0]) : nullptr;
  auto *p = u ? as<ast::PackageDecl>(u->unit) : nullptr;
  auto *c = p && !p->decls.empty() ? as<ast::ObjectDecl>(p->decls[0]) : nullptr;
  auto *neg = c ? as<ast::UnaryExpr>(c->defaultValue) : nullptr;
  check(neg && neg->op == "-" && as<ast::BinaryExpr>(neg->operand) &&
            as<ast::BinaryExpr>(neg->operand)->op == "**",
        "minus wraps power");
  return ok;
}
} // namespace audit_regressions
