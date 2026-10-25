#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "trapc/core/ast.hpp"
#include "trapc/generated/Trapc/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = trapc::generated::Trapc::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = trapc::ast;
  auto r = parser->parse(
      "[[nodiscard]] int f() __attribute__((pure)); [[deprecated]];");
  check(r.accepted && !r.cst().recovered(),
        "attributes at declaration and standalone sites");
  auto *u = as<ast::TranslationUnit>(r.value);
  auto *d =
      u && !u->items.empty() ? as<ast::Declaration>(u->items[0]) : nullptr;
  auto *standalone =
      u && u->items.size() == 2 ? as<ast::AttributeDecl>(u->items[1]) : nullptr;
  check(d && d->specifiers && d->specifiers->attributes.size() == 1 &&
            !d->declarators.empty() &&
            d->declarators[0]->declarator->attributes.size() == 1 &&
            standalone && standalone->attributes.size() == 1,
        "attribute spellings and attachment sites retained");
  return ok;
}
} // namespace audit_regressions
