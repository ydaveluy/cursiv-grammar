#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "xyz/core/ast.hpp"
#include "xyz/generated/Xyz/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = xyz::generated::Xyz::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = xyz::ast;
  auto r = parser->parse("1\n\nH 1e-3 +.5 1.\n");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "xyz valid syntax regression");
  auto *d = as<ast::Document>(r.value);
  check(d && d->frames.size() == 1 && d->frames[0]->description.empty() &&
            d->frames[0]->atoms.size() == 1 &&
            d->frames[0]->atoms[0]->x == 0.001 &&
            d->frames[0]->atoms[0]->y == 0.5,
        "empty description and numeric fields");

  return ok;
}
} // namespace audit_regressions
