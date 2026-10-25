#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "terraform/core/ast.hpp"
#include "terraform/generated/Terraform/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = terraform::generated::Terraform::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = terraform::ast;
  auto r = parser->parse("locals { a = [for k, v in xs : v if v > 0] b = {for "
                         "k, v in xs : k => v... if v > 0} }");
  check(r.accepted && !r.cst().recovered(), "tuple and object comprehensions");
  return ok;
}
} // namespace audit_regressions
