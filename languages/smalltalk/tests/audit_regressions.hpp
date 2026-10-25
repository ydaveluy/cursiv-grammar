#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "smalltalk/core/ast.hpp"
#include "smalltalk/generated/Smalltalk/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = smalltalk::generated::Smalltalk::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = smalltalk::ast;
  auto r = parser->parse("'can''t'. $z");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "smalltalk valid syntax regression");
  auto *d = as<ast::Sequence>(r.value);
  check(d && d->statements.size() == 2 &&
            as<ast::StringLiteral>(d->statements[0]) &&
            as<ast::StringLiteral>(d->statements[0])->value == "can't" &&
            as<ast::CharLiteral>(d->statements[1]) &&
            as<ast::CharLiteral>(d->statements[1])->value == "z",
        "decoded string and character AST");

  return ok;
}
} // namespace audit_regressions
