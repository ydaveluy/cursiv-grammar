#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lisp/core/ast.hpp"

namespace lisp {
struct Lisp : cursiv::GrammarSpec {
  static constexpr auto whitespace = cr<R"( \t\r\n\f\v)">;
  static constexpr auto delimiter = whitespace | cr<R"D(()"'`,;)D">;
  static constexpr auto plain = ~(delimiter | cr<R"(|\\)">);
  static constexpr auto escaped = lit<"\\"> + any;
  static constexpr auto symbol_piece =
      plain | escaped | (lit<"|"> + many(escaped | cr<R"(^|\\)">) + lit<"|">);
  struct WS : terminal<some(whitespace)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"#|"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"#|"> + !lit<"|#"> + any)) +
                 lit<"|#">> {};
  struct SYMBOL : terminal<((!lit<"#"> + !(lit<"."> + !plain)) |
                            lit<"#:">)+some(symbol_piece)> {};
  static constexpr auto sign = option(cr<"+-">);
  static constexpr auto exponent = cr<"eEsSfFdDlL"> + sign + some(d);
  struct NUMBER : terminal<(sign + ((some(d) + lit<"/"> + some(d)) |
                                   (((some(d) + option(lit<"."> + many(d))) |
                                     (lit<"."> + some(d))) +
                                    option(exponent)))) |
                           (lit<"#"> + option(some(d)) + cr<"bBoOxXrR"> + sign +
                            some(cr<"0-9A-Za-z/+-">))> {};
  struct STRING
      : terminal<lit<"\""> + many(escaped | cr<R"(^"\\)">) + lit<"\"">> {};
  struct CHARACTER : terminal<lit<"#\\"> + any + many(~delimiter)> {};
  struct BIT_VECTOR : terminal<lit<"#"> + many(d) + lit<"*"> + many(cr<"01">)> {
  };
  struct GRAPH_REF : terminal<lit<"#"> + some(d) + lit<"#">> {};
  struct PREFIX
      : terminal<lit<"'"> | lit<"`"> | lit<",@"> | lit<","> | lit<"#'"> |
                 lit<"#."> | (lit<"#"> + option(some(d)) + cr<"pPcCsSaA">) |
                 (lit<"#"> + some(d) + lit<"=">)> {};
  struct CONDITIONAL : terminal<lit<"#+"> | lit<"#-">> {};
  struct OPEN : terminal<option(lit<"#"> + many(d)) + lit<"(">> {};
  struct NumberRule : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct SymbolRule : rule<assign<&ast::Symbol::text>(ref<SYMBOL>)> {};
  struct StringRule : rule<assign<&ast::String::text>(ref<STRING>)> {};
  struct CharacterRule : rule<assign<&ast::Character::text>(ref<CHARACTER>)> {};
  struct BitVectorRule : rule<assign<&ast::BitVector::text>(ref<BIT_VECTOR>)> {
  };
  struct GraphRefRule : rule<assign<&ast::GraphRef::text>(ref<GRAPH_REF>)> {};
  struct ListRule;
  struct PrefixRule;
  struct ConditionalRule;
  static constexpr auto sexpr =
      ref<NumberRule> | ref<StringRule> | ref<CharacterRule> |
      ref<BitVectorRule> | ref<GraphRefRule> | ref<ListRule> | ref<PrefixRule> |
      ref<ConditionalRule> | ref<SymbolRule>;
  struct ListRule
      : rule<assign<&ast::List::opener>(ref<OPEN>) +
             many(append<&ast::List::items>(sexpr)) +
             option(lit<"."> + assign<&ast::List::tail>(sexpr)) + lit<")">> {};
  struct PrefixRule : rule<assign<&ast::Prefix::marker>(ref<PREFIX>) +
                           assign<&ast::Prefix::value>(sexpr)> {};
  struct ConditionalRule
      : rule<assign<&ast::Conditional::marker>(ref<CONDITIONAL>) +
             assign<&ast::Conditional::feature>(sexpr) +
             assign<&ast::Conditional::value>(sexpr)> {};
  struct DocRule
      : rule<create<ast::Doc> + many(append<&ast::Doc::expressions>(sexpr))> {};
  using entry = DocRule;
  using ignore = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};
} // namespace lisp
