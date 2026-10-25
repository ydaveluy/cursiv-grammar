#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "zig/core/ast.hpp"
#include "zig/generated/Zig/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = zig::generated::Zig::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = zig::ast;
  auto r = parser->parse("//\nconst x = 1;\n");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "zig valid syntax regression");
  auto *d = as<ast::ContainerMembers>(r.value);
  check(d && d->members.size() == 1, "empty comment preserves declaration");

  return ok;
}
} // namespace audit_regressions
