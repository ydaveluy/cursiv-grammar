#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "xpath/core/ast.hpp"
#include "xpath/generated/Xpath/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = xpath::generated::Xpath::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = xpath::ast;
  auto r = parser->parse("é");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "xpath valid syntax regression");

  return ok;
}
} // namespace audit_regressions
