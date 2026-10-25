#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "v/core/ast.hpp"
#include "v/generated/V/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = v::generated::V::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = v::ast;
  auto r = parser->parse(
      "enum Color {\nred\nblue\n}\nfn main() {\nx := foo() or { return "
      "}\nmatch x {\n.red { return }\nelse { return }\n}\n}\n");
  check(r.accepted && !r.cst().recovered(), "enum, match and or blocks");
  auto *d = as<ast::SourceFile>(r.value);
  auto *e = d && !d->decls.empty() ? as<ast::EnumDecl>(d->decls[0]) : nullptr;
  check(e && e->members.size() == 2 && e->members[1]->name == "blue",
        "enum members retained");
  auto *f =
      d && d->decls.size() == 2 ? as<ast::FunctionDecl>(d->decls[1]) : nullptr;
  auto *var = f && f->body && !f->body->stmts.empty()
                  ? as<ast::VarDecl>(f->body->stmts[0])
                  : nullptr;
  auto *handler =
      var && !var->values.empty() ? as<ast::OrExpr>(var->values[0]) : nullptr;
  auto *matchStmt = f && f->body && f->body->stmts.size() == 2
                        ? as<ast::ExprStmt>(f->body->stmts[1])
                        : nullptr;
  auto *match = matchStmt ? as<ast::MatchExpr>(matchStmt->value) : nullptr;
  check(handler && handler->handler && handler->handler->stmts.size() == 1 &&
            match && match->arms.size() == 2 && match->arms[1]->isElse,
        "or handler and match branches retained");
  return ok;
}
} // namespace audit_regressions
