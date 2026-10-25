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
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
  struct Property : terminal<some(PropertyChar)> {};
  struct QuotedValue
      : terminal<(lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                  lit<"\"">) |
                 (lit<"'"> + many(cr<"^'">) + lit<"'">)> {};
  struct Eq : terminal<lit<"=">, value<ast::BinOp::eq>> {};
  struct Ne : terminal<lit<"!=">, value<ast::BinOp::ne>> {};
  struct Le : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct Lt : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct Ge : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct Gt : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct Contains : terminal<ikw<"contains">, value<ast::BinOp::contains>> {};
  struct DoesNotContain
      : terminal<ikw<"doesNotContain">, value<ast::BinOp::does_not_contain>> {};
  struct DerivedFrom
      : terminal<ikw<"derivedfrom">, value<ast::BinOp::derived_from>> {};
  static constexpr auto BinaryOp = ref<Ne> | ref<Le> | ref<Ge> | ref<Eq> |
                                   ref<Lt> | ref<Gt> | ref<Contains> |
                                   ref<DoesNotContain> | ref<DerivedFrom>;
  struct AndOp : rule<ikw<"and">, value<ast::LogOp::and_>> {};
  struct OrOp : rule<ikw<"or">, value<ast::LogOp::or_>> {};
  struct Comparison : rule<assign<&ast::Comparison::property>(ref<Property>) +
                           assign<&ast::Comparison::op>(BinaryOp) +
                           assign<&ast::Comparison::raw>(ref<QuotedValue>)> {};
  struct Exists
      : rule<assign<&ast::Exists::property>(ref<Property>) + ikw<"exists"> +
             (flag<&ast::Exists::value>(ikw<"true">) | ikw<"false">)> {};
  struct Expr;
  struct Primary : rule<ref<Comparison> | ref<Exists> |
                        (lit<"("> + ref<Expr> + lit<")">)> {};
  struct Expr
      : infix<Primary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(ref<AndOp>) >> left(ref<OrOp>))> {
  };
  struct SearchCritRule : rule<assign<&ast::SearchCrit::expr>(ref<Expr>) |
                               flag<&ast::SearchCrit::wildcard>(lit<"*">)> {};
  using entry = SearchCritRule;
};
} // namespace upnp
