#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "wkt/core/ast.hpp"
#include "wkt/generated/Wkt/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = wkt::generated::Wkt::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = wkt::ast;
  auto r = parser->parse("POINT (+1 .5)");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "wkt valid syntax regression");

  return ok;
}
} // namespace audit_regressions
