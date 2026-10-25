#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "wavefront/core/ast.hpp"
#include "wavefront/generated/Wavefront/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = wavefront::generated::Wavefront::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = wavefront::ast;
  auto r = parser->parse("v +1e-3 .5 1.\nmtllib materials/a.mtl b.mtl\n");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "wavefront valid syntax regression");

  return ok;
}
} // namespace audit_regressions
