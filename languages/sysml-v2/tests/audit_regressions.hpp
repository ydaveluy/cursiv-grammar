#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "sysmlv2/core/ast.hpp"
#include "sysmlv2/generated/SysmlV2/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sysmlv2::generated::SysmlV2::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sysmlv2::ast;
  auto r = parser->parse(
      "send s to r; connect end a : Node to end b : Node; transition t first "
      "s1 accept e : Signal if ready then s2; first x then y;");
  check(r.accepted && !r.cst().recovered(),
        "typed ends, direct send target and transition clauses");
  auto *d = as<ast::RootNamespace>(r.value);
  auto *e = d && d->members.size() > 1 ? d->members[1] : nullptr;
  check(e && e->kind == "connect" && e->connectorEnds.size() == 2 &&
            e->connectorEnds[0]->isEnd &&
            e->connectorEnds[0]->specs.size() == 1 &&
            e->connectorEnds[1]->name == "b",
        "typed endpoints retained");
  auto *t = d && d->members.size() > 2 ? d->members[2] : nullptr;
  check(t && t->clauses.size() == 4 && t->clauses[1]->kind == "accept" &&
            t->clauses[1]->endpoint->specs.size() == 1 &&
            t->clauses[2]->expression,
        "transition clauses retained");
  return ok;
}
} // namespace audit_regressions
