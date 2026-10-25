#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "webidl/core/ast.hpp"
#include "webidl/generated/Webidl/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = webidl::generated::Webidl::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = webidl::ast;
  auto r = parser->parse("[A, A()] interface Foo {};");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "webidl valid syntax regression");
  auto *d = as<ast::Document>(r.value);
  check(d && d->definitions.size() == 1 &&
            d->definitions[0]->attrs.size() == 2 &&
            !d->definitions[0]->attrs[0]->hasArguments &&
            d->definitions[0]->attrs[1]->hasArguments,
        "extended attribute argument-list presence");

  return ok;
}
} // namespace audit_regressions
