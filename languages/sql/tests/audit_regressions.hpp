#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "sqlite/core/ast.hpp"
#include "sqlite/generated/Sqlite/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = sqlite::generated::Sqlite::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = sqlite::ast;
  auto r = parser->parse("SELECT 9007199254740993 LIMIT 5, 10;");
  check(r.accepted && !r.cst().recovered(),
        "exact integer and LIMIT comma syntax");
  auto *f = as<ast::Script>(r.value);
  if (f && !f->statements.empty()) {
    auto *s = as<ast::Select>(f->statements[0]->statement);
    check(s && as<ast::NumberLit>(s->offset) &&
              as<ast::NumberLit>(s->offset)->value == "5" &&
              as<ast::NumberLit>(s->limit) &&
              as<ast::NumberLit>(s->limit)->value == "10",
          "LIMIT offset,count assignment");
    check(s && !s->parts.empty() && !s->parts[0]->core->columns.empty() &&
              as<ast::NumberLit>(s->parts[0]->core->columns[0]->value)->value ==
                  "9007199254740993",
          "exact integer lexeme");
  } else
    check(false, "SQL script");
  return ok;
}
} // namespace audit_regressions
