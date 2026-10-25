#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "stellaris/core/ast.hpp"
#include "stellaris/generated/Stellaris/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = stellaris::generated::Stellaris::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = stellaris::ast;
  auto r = parser->parse("x = -0.5\ny = 1.25");
  check(r.accepted && !r.cst().recovered(), "decimal scalars");
  auto *d = as<ast::Document>(r.value);
  check(d && d->items.size() == 2, "two decimal entries");
  if (d && d->items.size() == 2) {
    const auto* first = cursiv::ast_cast<ast::KeyValue>(d->items[0]);
    const auto* scalar = first ? cursiv::ast_cast<ast::Scalar>(first->value) : nullptr;
    check(scalar && scalar->text == "-0.5" && !scalar->quoted,
          "decimal spelling remains one scalar value");
  }
  return ok;
}
} // namespace audit_regressions
