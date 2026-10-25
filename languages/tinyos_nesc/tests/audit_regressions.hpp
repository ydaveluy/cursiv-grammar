#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "tinyosnesc/core/ast.hpp"
#include "tinyosnesc/generated/TinyosNesc/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = tinyosnesc::generated::TinyosNesc::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = tinyosnesc::ast;
  auto r = parser->parse(
      "generic configuration C(typedef T, int n) {} implementation {}\ngeneric "
      "module M(typedef T) {} implementation {}");
  check(r.accepted && !r.cst().recovered(), "generic component parameters");
  auto *d = as<ast::Document>(r.value);
  auto *c = d && d->components.size() == 2
                ? as<ast::Configuration>(d->components[0])
                : nullptr;
  auto *m = d && d->components.size() == 2 ? as<ast::Module>(d->components[1])
                                           : nullptr;
  check(c && c->hasParameters &&
            c->parameters.find("n") != std::string_view::npos && m &&
            m->hasParameters &&
            m->parameters.find("T") != std::string_view::npos,
        "both parameter clauses retained");
  return ok;
}
} // namespace audit_regressions
