#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "wat/core/ast.hpp"
#include "wat/generated/Wat/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = wat::generated::Wat::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = wat::ast;
  auto r = parser->parse("(module (; outer (; inner ;) end ;) )");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "wat valid syntax regression");

  return ok;
}
} // namespace audit_regressions
