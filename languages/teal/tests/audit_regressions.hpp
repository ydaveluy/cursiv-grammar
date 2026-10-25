#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "teal/core/ast.hpp"
#include "teal/generated/Teal/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = teal::generated::Teal::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = teal::ast;
  auto r = parser->parse(
      "local a,b,c: number,string,boolean = 1,'x',true\nlocal function f(): "
      "number,string,boolean return 1,'x',true end");
  check(r.accepted && !r.cst().recovered(), "three annotated and return types");
  auto *b = as<ast::Block>(r.value);
  auto *d = b && !b->statements.empty()
                ? as<ast::LocalAttrAssignStat>(b->statements[0])
                : nullptr;
  check(d && d->types.size() == 3, "all annotated types retained");
  return ok;
}
} // namespace audit_regressions
