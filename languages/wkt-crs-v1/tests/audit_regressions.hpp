#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "wktcrsv1/core/ast.hpp"
#include "wktcrsv1/generated/WktCrsV1/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = wktcrsv1::generated::WktCrsV1::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = wktcrsv1::ast;
  auto r = parser->parse(
      "TEST=LOCAL_CS[\"local\",LOCAL_DATUM[\"datum\",0],UNIT[\"metre\",1],AXIS["
      "\"x\",EAST]]\nOTHER=LOCAL_CS[\"local\",LOCAL_DATUM[\"datum\",0],UNIT["
      "\"metre\",1],AXIS[\"x\",EAST],AUTHORITY[\"CUSTOM\",\"abc\"]]\n");
  check(r.accepted && !r.cst().recovered(), "optional and custom authorities");
  return ok;
}
} // namespace audit_regressions
