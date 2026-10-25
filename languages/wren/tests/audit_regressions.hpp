#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "wren/core/ast.hpp"
#include "wren/generated/Wren/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = wren::generated::Wren::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = wren::ast;
  auto r = parser->parse("System.print(\"value %(foo(\"nested\")) tail\")\n");
  check(r.accepted && !r.cst().recovered(),
        "nested quoted strings inside interpolation");
  return ok;
}
} // namespace audit_regressions
