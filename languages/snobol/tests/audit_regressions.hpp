#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "snobol/core/ast.hpp"
#include "snobol/generated/Snobol/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = snobol::generated::Snobol::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = snobol::ast;
  auto r = parser->parse(" X = replace('abc','a','b','extra')\nEND\n");
  check(r.accepted && !r.cst().recovered(),
        "builtin arguments are a runtime concern");
  auto *d = as<ast::Program>(r.value);
  auto *st = d && !d->lines.empty() ? as<ast::Statement>(d->lines[0]) : nullptr;
  auto *c = st && !st->replacement.empty()
                ? as<ast::FunctionCall>(st->replacement[0])
                : nullptr;
  check(c && c->fn == ast::Builtin::replace && c->args.size() == 4,
        "all call arguments retained");
  return ok;
}
} // namespace audit_regressions
