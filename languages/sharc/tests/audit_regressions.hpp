#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "sharc/core/ast.hpp"
#include "sharc/generated/Sharc/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sharc::generated::Sharc::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sharc::ast;
  auto r = parser->parse(".var n = (1 + 2) * 3;\nr0 = n + 1;\n");
  check(r.accepted && !r.cst().recovered(),
        "structured and opaque operands share expression syntax");
  return ok;
}
} // namespace audit_regressions
