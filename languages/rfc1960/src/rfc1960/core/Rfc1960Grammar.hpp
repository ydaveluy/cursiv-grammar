#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc1960/core/ast.hpp"

namespace rfc1960 {

struct Rfc1960 : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct EQUAL : terminal<lit<"=">, value<ast::CompareOp::equal>> {};
  struct APPROX : terminal<lit<"~=">, value<ast::CompareOp::approx>> {};
  struct GREATER
      : terminal<lit<">=">, value<ast::CompareOp::greater_or_equal>> {};
  struct LESS : terminal<lit<"<=">, value<ast::CompareOp::less_or_equal>> {};
  struct ATTRIBUTE : terminal<!(s | cr<"&|!">)+some(cr<R"(^():~<>=)">)> {};
  struct QUALIFIER : terminal<some(cr<R"(^():~<>= \t\r\n)">)> {};
  struct OCTETSTRING : terminal<some((lit<"\\"> + any) | cr<R"(^()*\\)">)> {};
  struct COMPARE_VALUE : terminal<some((lit<"\\"> + any) | cr<R"(^()\\)">)> {};
  struct ValueRule : rule<option(ref<OCTETSTRING>)> {};
  struct CompareValueRule : rule<option(ref<COMPARE_VALUE>)> {};
  struct AttributeRule : rule<option(ref<ATTRIBUTE>)> {};
  struct FilterRule;

  struct AndRule : rule<lit<"&"> +
                        many(many(ref<WS>) +
                             append<&ast::And::operands>(ref<FilterRule>)) +
                        many(ref<WS>)> {};
  struct OrRule
      : rule<lit<"|"> +
             many(many(ref<WS>) + append<&ast::Or::operands>(ref<FilterRule>)) +
             many(ref<WS>)> {};
  struct NotRule : rule<lit<"!"> + many(ref<WS>) +
                        option(assign<&ast::Not::operand>(ref<FilterRule>) +
                               many(ref<WS>))> {};
  struct SimpleRule
      : rule<assign<&ast::Simple::attr>(ref<AttributeRule>) +
             ((assign<&ast::Simple::op>(ref<EQUAL>) +
               assign<&ast::Simple::value>(ref<ValueRule>)) |
              (assign<&ast::Simple::op>(ref<APPROX> | ref<GREATER> |
                                        ref<LESS>) +
               assign<&ast::Simple::value>(ref<CompareValueRule>)))> {};
  struct SubstringRule
      : rule<assign<&ast::Substring::attr>(ref<AttributeRule>) + lit<"="> +
             option(assign<&ast::Substring::initial>(ref<OCTETSTRING>)) +
             lit<"*"> +
             many(append<&ast::Substring::middle>(ref<ValueRule>) + lit<"*">) +
             option(assign<&ast::Substring::final_>(ref<OCTETSTRING>))> {};
  struct ExtensibleRule
      : rule<option(assign<&ast::Extensible::attr>(ref<ATTRIBUTE>)) +
             many(lit<":"> +
                  append<&ast::Extensible::qualifiers>(ref<QUALIFIER>)) +
             lit<":="> +
             assign<&ast::Extensible::value>(ref<CompareValueRule>)> {};
  struct FilterRule
      : rule<lit<"("> + many(ref<WS>) +
             (ref<AndRule> | ref<OrRule> | ref<NotRule> | ref<ExtensibleRule> |
              ref<SimpleRule> | ref<SubstringRule>)+lit<")">> {};
  struct Root : rule<many(ref<WS>) + ref<FilterRule> + many(ref<WS>)> {};

  using entry = Root;
};

} // namespace rfc1960
