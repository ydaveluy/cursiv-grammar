#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "graphemes/core/ast.hpp"
#include "graphemes/generated/Graphemes/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = graphemes::generated::Graphemes::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = graphemes::ast;
  auto r = parser->parse("́");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "unicode valid syntax regression");

  return ok;
}
} // namespace audit_regressions
