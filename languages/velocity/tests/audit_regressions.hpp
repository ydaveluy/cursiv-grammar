#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "velocity/core/ast.hpp"
#include "velocity/generated/Velocity/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = velocity::generated::Velocity::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = velocity::ast;
  auto r = parser->parse(
      "  hello $name world\n#set($x = (1 + 2) * 3)\n#if(1 < 2 == 3 < 4)x#end");
  check(r.accepted && !r.cst().recovered(),
        "literal whitespace, grouped and relational expressions");
  auto *b = as<ast::Block>(r.value);
  check(b && !b->atoms.empty() &&
            std::get<std::string_view>(b->atoms[0]) == "  hello ",
        "literal leading and inter-word spaces retained");
  auto prose = parser->parse("a $name. next");
  auto *text = as<ast::Block>(prose.value);
  check(prose.accepted && !prose.cst().recovered() && text &&
            text->atoms.size() == 4 &&
            std::holds_alternative<std::string_view>(text->atoms[2]) &&
            std::get<std::string_view>(text->atoms[2]) == "." &&
            std::get<std::string_view>(text->atoms[3]) == " next",
        "reference chains do not consume prose across spaces");
  auto loop = parser->parse(
      "#foreach( $n in [1, 2, 3] )\n#if( $n == 2 )\n#break\n#end\n#end\n");
  check(loop.accepted && !loop.cst().recovered(),
        "spaced variable references inside directives");
  return ok;
}
} // namespace audit_regressions
