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
  {
    const auto result = parser->parse("<\\n><\\t><\\ ><\\u0041><\"a\nb\"><true><false>");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "ST4 escapes, multiline strings and boolean literals");
    const auto* root = as<ast::Template>(result.value);
    check(root && root->elements.size() == 7, "all native escape and value nodes retained");
    if (root && root->elements.size() == 7) {
      const char* expected[] = {"\n", "\t", " ", "A"};
      for (std::size_t i = 0; i < 4; ++i) {
        const auto* text = as<ast::Text>(root->elements[i]);
        check(text && text->value == expected[i], "ST4 escaped text value");
      }
      const auto* tag = as<ast::ExprTag>(root->elements[4]);
      const auto* map = tag ? as<ast::MapExpr>(tag->value) : nullptr;
      const auto* string = map && map->sources.size() == 1
          ? as<ast::StringExpr>(map->sources[0]) : nullptr;
      check(string && string->value == "a\nb", "native multiline string preserves its newline");
    }
  }
  for (const auto source : {"<\\u41>", "<\\u>", "<\\b>", "<\\r>",
                            "<\\z>", "<é>", "<if>", "<super>"}) {
    const auto result = parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "ST4 rejects invalid escapes, non-ASCII IDs and isolated control keywords");
  }
  return ok;
}
} // namespace audit_regressions
