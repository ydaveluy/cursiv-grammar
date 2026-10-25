#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "useragent/core/ast.hpp"
#include "useragent/generated/Useragent/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = useragent::generated::Useragent::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = useragent::ast;
  auto r = parser->parse("MyClient my-client/1.0-beta");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "useragent valid syntax regression");

  return ok;
}
} // namespace audit_regressions
