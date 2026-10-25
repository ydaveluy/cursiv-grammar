#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "sgf/core/ast.hpp"
#include "sgf/generated/Sgf/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sgf::generated::Sgf::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sgf::ast;
  auto r = parser->parse("(;A[hello])");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "sgf valid syntax regression");

  return ok;
}
} // namespace audit_regressions
