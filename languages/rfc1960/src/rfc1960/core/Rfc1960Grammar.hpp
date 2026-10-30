#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc1960/core/ast.hpp"

namespace rfc1960 {

struct Rfc1960 : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct ATTRIBUTE : terminal<!(s | cr<"&|!">)+some(cr<R"(^():~<>=)">)> {};
  struct QUALIFIER : terminal<some(cr<R"(^():~<>= \t\r\n)">)> {};
  struct OCTETSTRING : terminal<some((lit<"\\"> + any) | cr<R"(^()*\\)">)> {};
  struct COMPARE_VALUE : terminal<some((lit<"\\"> + any) | cr<R"(^()\\)">)> {};
  struct Value : rule<option(ref<OCTETSTRING>)> {};
  struct CompareValue : rule<option(ref<COMPARE_VALUE>)> {};
  struct Attribute : rule<option(ref<ATTRIBUTE>)> {};
  struct Filter;

  struct And : rule<lit<"&"> +
                        many(many(ref<WS>) +
                             append<&ast::And::operands>(ref<Filter>)) +
                        many(ref<WS>)> {};
  struct Or
      : rule<lit<"|"> +
             many(many(ref<WS>) + append<&ast::Or::operands>(ref<Filter>)) +
             many(ref<WS>)> {};
  struct Not : rule<lit<"!"> + many(ref<WS>) +
                        option(assign<&ast::Not::operand>(ref<Filter>) +
                               many(ref<WS>))> {};
  struct Simple
      : rule<assign<&ast::Simple::attr>(ref<Attribute>) +
             ((assign<&ast::Simple::op>(as<ast::CompareOp::equal>(lit<"=">)) +
               assign<&ast::Simple::value>(ref<Value>)) |
              (assign<&ast::Simple::op>(as<ast::CompareOp::approx>(lit<"~=">) | as<ast::CompareOp::greater_or_equal>(lit<">=">) |
                                        as<ast::CompareOp::less_or_equal>(lit<"<=">)) +
               assign<&ast::Simple::value>(ref<CompareValue>)))> {};
  struct Substring
      : rule<assign<&ast::Substring::attr>(ref<Attribute>) + lit<"="> +
             option(assign<&ast::Substring::initial>(ref<OCTETSTRING>)) +
             lit<"*"> +
             many(append<&ast::Substring::middle>(ref<Value>) + lit<"*">) +
             option(assign<&ast::Substring::final_>(ref<OCTETSTRING>))> {};
  struct Extensible
      : rule<option(assign<&ast::Extensible::attr>(ref<ATTRIBUTE>)) +
             many(lit<":"> +
                  append<&ast::Extensible::qualifiers>(ref<QUALIFIER>)) +
             lit<":="> +
             assign<&ast::Extensible::value>(ref<CompareValue>)> {};
  struct Filter
      : rule<lit<"("> + many(ref<WS>) +
             (ref<And> | ref<Or> | ref<Not> | ref<Extensible> |
              ref<Simple> | ref<Substring>)+lit<")">> {};
  struct Root : rule<many(ref<WS>) + ref<Filter> + many(ref<WS>)> {};

  using entry = Root;
};

} // namespace rfc1960
