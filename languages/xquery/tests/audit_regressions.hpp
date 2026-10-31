#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "xquery/core/ast.hpp"
#include "xquery/generated/XQuery/Module.hpp"
#include <cstdio>
#include <string>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = xquery::generated::XQuery::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = xquery::ast;
  auto r = parser->parse("declare variable $é := 1; $é");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "xquery valid syntax regression");

  auto numbers = parser->parse("(1.0000000000000000001, 1.0e0)");
  check(numbers.accepted && !numbers.cst().recovered(),
        "lossless decimal and double");
  auto destruct = parser->parse("let $( $a, $b ) := (1,2), $[ $c, $d ] := "
                                "[1,2], ${ $e, $f } := map{} return $a");
  check(destruct.accepted && !destruct.cst().recovered(),
        "destructuring delimiters");
  auto body = [](cursiv::AstNode *root) -> ast::Expr * {
    auto *q = as<ast::QueryList>(root);
    auto *seq = q && !q->modules.empty() && q->modules[0]->main
                    ? as<ast::SequenceExpr>(q->modules[0]->main->body)
                    : nullptr;
    return seq && !seq->items.empty() ? seq->items[0] : nullptr;
  };
  auto *paren = as<ast::ParenExpr>(body(numbers.value));
  auto *seq = paren ? paren->body : nullptr;
  auto *decimal = seq && seq->items.size() == 2
                      ? as<ast::DecimalLiteral>(seq->items[0])
                      : nullptr;
  auto *floating = seq && seq->items.size() == 2
                       ? as<ast::DecimalLiteral>(seq->items[1])
                       : nullptr;
  check(decimal && floating && decimal->value == "1.0000000000000000001" &&
            !decimal->isDouble && floating->isDouble &&
            floating->value == "1.0e0",
        "decimal precision and numeric kind retained");
  auto *flwor = as<ast::FlworExpr>(body(destruct.value));
  auto *let = flwor && !flwor->clauses.empty()
                  ? std::get_if<ast::LetClause *>(&flwor->clauses[0])
                  : nullptr;
  check(let && *let && (*let)->bindings.size() == 3 &&
            (*let)->bindings[0]->destructuring == "(" &&
            (*let)->bindings[1]->destructuring == "[" &&
            (*let)->bindings[2]->destructuring == "{",
        "destructuring shapes remain distinct");
  for (const char* source : {"0x7fff_ffff_ffff_ffff", "0b111_111", "6__3"}) {
    const auto number_result = parser->parse(source);
    const auto* number = cursiv::ast_cast<ast::IntLiteral>(body(number_result.value));
    check(number_result.accepted && !number_result.cst().recovered() &&
              number_result.diagnostics.empty() && number &&
              number->value == (source[1] == 'x' ? INT64_MAX : 63),
          "radix integer conversion preserves the boundary and binary separators");
  }
  for (const char* source : {"1_000.2_5", "1_0e+2__0"}) {
    const auto number_result = parser->parse(source);
    const auto* number = cursiv::ast_cast<ast::DecimalLiteral>(body(number_result.value));
    check(number_result.accepted && !number_result.cst().recovered() &&
              number_result.diagnostics.empty() && number && number->value == source,
          "XQuery 4.0 digit separators preserve decimal and double spelling");
  }
  for (const char* source : {"0x_1", "0x1_", "0b_1", "0b1_", "1_", "1e2_"}) {
    const auto invalid = parser->parse(source);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "digit separators must occur between digits");
  }
  for (const std::string& source : {std::string("0x8000000000000000"),
                                  std::string("0b1") + std::string(63, '0')}) {
    const auto overflow = parser->parse(source);
    check(!overflow.diagnostics.empty(),
          "radix integer overflow reports a conversion error without wrapping or zero");
  }
  return ok;
}
} // namespace audit_regressions
