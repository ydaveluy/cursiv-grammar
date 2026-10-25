#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "squirrel/core/ast.hpp"
#include "squirrel/generated/Squirrel/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = squirrel::generated::Squirrel::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = squirrel::ast;
  auto r = parser->parse("local xs = [\n1,\n2\n]\nlocal y = "
                         "foo(\n1,\n2\n)\nlocal z = (\n1 +\n2\n)\n");
  check(r.accepted && !r.cst().recovered(),
        "newlines inside delimited expressions and after operators");
  return ok;
}
} // namespace audit_regressions
