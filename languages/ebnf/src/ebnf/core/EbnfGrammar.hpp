// The ebnf language: its grammar. Plain BNF plus `[optional]`,
// `{zero-or-more}` and `(one-or-more)` grouping.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "ebnf/core/ast.hpp"

namespace ebnf {

struct Ebnf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // First char a letter; each further char is a letter, digit, hyphen or
  // space -- a one-letter name and a run of words are both representable.
  struct ID : terminal<cr<"A-Za-z"> + many(cr<"A-Za-z0-9 -">)> {};

  struct OptionalRule;
  struct ZeroOrMoreRule;
  struct OneOrMoreRule;
  struct TextRule;
  struct IdRule;

  static constexpr auto element =
      ref<OptionalRule> | ref<ZeroOrMoreRule> | ref<OneOrMoreRule> |
      ref<TextRule> | ref<IdRule>;

  struct AlternativeRule
      : rule<many(append<&ast::Alternative::elements>(element))> {};

  struct OptionalRule
      : rule<lit<"["> +
             some(append<&ast::Optional::alternatives>(ref<AlternativeRule>),
                  lit<"|">) +
             lit<"]">> {};

  struct ZeroOrMoreRule
      : rule<lit<"{"> +
             some(append<&ast::ZeroOrMore::alternatives>(ref<AlternativeRule>),
                  lit<"|">) +
             lit<"}">> {};

  struct OneOrMoreRule
      : rule<lit<"("> +
             some(append<&ast::OneOrMore::alternatives>(ref<AlternativeRule>),
                  lit<"|">) +
             lit<")">> {};

  struct TextRule : rule<assign<&ast::Text::value>(ref<ID>)> {};

  struct IdRule
      : rule<lit<"<"> + assign<&ast::Id::name>(ref<ID>) + lit<">">> {};

  struct RuleRule
      : rule<assign<&ast::Rule::lhs>(ref<IdRule>) + lit<"::="> +
             some(append<&ast::Rule::alternatives>(ref<AlternativeRule>),
                  lit<"|">)> {};

  struct DocumentRule
      : rule<many(append<&ast::Document::rules>(ref<RuleRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace ebnf
