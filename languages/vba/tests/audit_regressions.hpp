#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "vba/core/ast.hpp"
#include "vba/generated/Vba/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = vba::generated::Vba::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = vba::ast;
  auto r = parser->parse(
      "Sub Test()\nx = 2 ^ 3 ^ 2\ny = -2 ^ 2\nz = 2 ^ -2\nEnd Sub\n");
  check(r.accepted && !r.cst().recovered(), "power and signed operands");
  auto *m = as<ast::Module>(r.value);
  auto *p = m && !m->body.empty() ? as<ast::Procedure>(m->body[0]) : nullptr;
  auto *st = p && !p->body.empty() ? as<ast::AssignStmt>(p->body[0]) : nullptr;
  auto *b = st ? as<ast::BinaryExpr>(st->value) : nullptr;
  check(b && b->op == "^" && as<ast::BinaryExpr>(b->left) &&
            as<ast::BinaryExpr>(b->left)->op == "^",
        "exponentiation is left associative");
  return ok;
}
} // namespace audit_regressions
