#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "stringtemplate/core/ast.hpp"
#include "stringtemplate/generated/Stringtemplate/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = stringtemplate::generated::Stringtemplate::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = stringtemplate::ast;
  auto r = parser->parse("  hello < name > world\n");
  check(r.accepted && !r.cst().recovered(),
        "spaced expression in literal text");
  auto *t = as<ast::Template>(r.value);
  check(t && t->elements.size() == 3 && as<ast::Text>(t->elements[0]) &&
            as<ast::Text>(t->elements[0])->value == "  hello " &&
            as<ast::Text>(t->elements[2]) &&
            as<ast::Text>(t->elements[2])->value == " world\n",
        "literal whitespace retained");
  return ok;
}
} // namespace audit_regressions
