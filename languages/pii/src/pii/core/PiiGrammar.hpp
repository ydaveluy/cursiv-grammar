// The pii language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pii/core/ast.hpp"

namespace pii {

struct Pii : cursiv::GrammarSpec {
  struct DIGIT : terminal<d> {};

  // '(', ')' and '-' are free-standing separators, not brackets, so any run
  // of them and whitespace is trivia.
  struct Trivia : terminal<some(cr<R"( \t\r\n()-)">)> {};

  struct Digits2 : rule<repeat<2>(ref<DIGIT>)> {};
  struct Digits5 : rule<repeat<5>(ref<DIGIT>)> {};
  struct Digits8 : rule<repeat<8>(ref<DIGIT>)> {};
  struct Digits10 : rule<repeat<10>(ref<DIGIT>)> {};

  struct IssnRule
      : rule<lit<"S"> + assign<&ast::Issn::digits>(ref<Digits8>) +
             assign<&ast::Issn::year>(ref<Digits2>)> {};

  struct IsbnRule
      : rule<lit<"B"> + assign<&ast::Isbn::digits>(ref<Digits10>)> {};

  static constexpr auto identifier = ref<IssnRule> | ref<IsbnRule>;

  struct DocRule
      : rule<assign<&ast::Doc::id>(identifier) +
             assign<&ast::Doc::item>(ref<Digits5>) +
             assign<&ast::Doc::check>(ref<DIGIT> | lit<"X">)> {};

  using entry = DocRule;
  using ignore = Trivia;
};

}  // namespace pii
