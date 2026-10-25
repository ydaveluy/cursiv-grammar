#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "stl/core/ast.hpp"
#include "stl/generated/Stl/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = stl::generated::Stl::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = stl::ast;
  auto r = parser->parse(
      "solid x\nfacet normal 0 0 -1\nouter loop\nvertex -1 0 0\nvertex 1 0 "
      "0\nvertex 0 1 0\nendloop\nendfacet\nendsolid x\n");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "stl valid syntax regression");

  return ok;
}
} // namespace audit_regressions
