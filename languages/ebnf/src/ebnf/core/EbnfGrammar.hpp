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

  struct Optional;
  struct ZeroOrMore;
  struct OneOrMore;
  struct Text;
  struct Id;

  static constexpr auto element =
      ref<Optional> | ref<ZeroOrMore> | ref<OneOrMore> |
      ref<Text> | ref<Id>;

  struct Alternative
      : rule<many(append<&ast::Alternative::elements>(element))> {};

  struct Optional
      : rule<lit<"["> +
             some(append<&ast::Optional::alternatives>(ref<Alternative>),
                  lit<"|">) +
             lit<"]">> {};

  struct ZeroOrMore
      : rule<lit<"{"> +
             some(append<&ast::ZeroOrMore::alternatives>(ref<Alternative>),
                  lit<"|">) +
             lit<"}">> {};

  struct OneOrMore
      : rule<lit<"("> +
             some(append<&ast::OneOrMore::alternatives>(ref<Alternative>),
                  lit<"|">) +
             lit<")">> {};

  struct Text : rule<assign<&ast::Text::value>(ref<ID>)> {};

  struct Id
      : rule<lit<"<"> + assign<&ast::Id::name>(ref<ID>) + lit<">">> {};

  struct Rule
      : rule<assign<&ast::Rule::lhs>(ref<Id>) + lit<"::="> +
             some(append<&ast::Rule::alternatives>(ref<Alternative>),
                  lit<"|">)> {};

  struct Document
      : rule<many(append<&ast::Document::rules>(ref<Rule>))> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace ebnf
