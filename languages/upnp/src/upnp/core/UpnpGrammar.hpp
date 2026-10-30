#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "upnp/core/ast.hpp"
namespace upnp {
struct Upnp : cursiv::GrammarSpec {
  static constexpr auto PropertyChar = cr<R"(^ \t\r\n\v\f()=!<>"'/#)">;
  static constexpr auto word_boundary = PropertyChar;
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
  struct Property : terminal<some(PropertyChar)> {};
  struct QuotedValue
      : terminal<(lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                  lit<"\"">) |
                 (lit<"'"> + many(cr<"^'">) + lit<"'">)> {};
  struct DerivedFrom
      : terminal<as<ast::BinOp::derived_from>(ikw<"derivedfrom">)> {};
  static constexpr auto BinaryOp = as<ast::BinOp::ne>(lit<"!=">) | as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::ge>(lit<">=">) | as<ast::BinOp::eq>(lit<"=">) |
                                   as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::gt>(lit<">">) | as<ast::BinOp::contains>(ikw<"contains">) |
                                   as<ast::BinOp::does_not_contain>(ikw<"doesNotContain">) | ref<DerivedFrom>;
  struct Comparison : rule<assign<&ast::Comparison::property>(ref<Property>) +
                           assign<&ast::Comparison::op>(BinaryOp) +
                           assign<&ast::Comparison::raw>(ref<QuotedValue>)> {};
  struct Exists
      : rule<assign<&ast::Exists::property>(ref<Property>) + ikw<"exists"> +
             (flag<&ast::Exists::value>(ikw<"true">) | ikw<"false">)> {};
  struct Expr;
  struct Primary : rule<ref<Comparison> | ref<Exists> |
                        (lit<"("> + ref<Expr> + lit<")">)> {};
  struct Expr : infix<Primary, ast::BinaryExpr,
                      levels(left(as<ast::LogOp::and_>(ikw<"and">)), left(as<ast::LogOp::or_>(ikw<"or">)))> {};
  struct SearchCrit : rule<assign<&ast::SearchCrit::expr>(ref<Expr>) |
                               flag<&ast::SearchCrit::wildcard>(lit<"*">)> {};
  using entry = SearchCrit;
};
} // namespace upnp
