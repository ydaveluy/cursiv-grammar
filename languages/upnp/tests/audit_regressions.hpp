#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "upnp/core/ast.hpp"
#include "upnp/generated/Upnp/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = upnp::generated::Upnp::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = upnp::ast;
  auto r = parser->parse("dc:title = \"a\" or dc:title = \"b\" and dc:title "
                         "doesNotContain \"2001: A Space Odyssey\"");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "upnp valid syntax regression");
  auto *d = as<ast::SearchCrit>(r.value);
  auto *o = d ? as<ast::BinaryExpr>(d->expr) : nullptr;
  check(o && o->op == ast::LogOp::or_ && as<ast::BinaryExpr>(o->right) &&
            as<ast::BinaryExpr>(o->right)->op == ast::LogOp::and_,
        "AND binds before OR");

  return ok;
}
} // namespace audit_regressions
