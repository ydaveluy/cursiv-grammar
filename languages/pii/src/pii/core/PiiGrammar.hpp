// The pii language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pii/core/ast.hpp"

namespace pii {

struct Pii : cursiv::GrammarSpec {
  struct DIGIT : terminal<d> {};
  struct CHECK : terminal<cr<"0-9X">> {};

  // Accept compact identifiers and the publisher's journal/book separators.
  struct Trivia : terminal<some(cr<R"( \t\r\n()./-)">)> {};

  struct Digits2 : rule<repeat<2>(ref<DIGIT>)> {};
  struct IssnNumber : rule<repeat<7>(ref<DIGIT>) + ref<CHECK>> {};
  struct IsbnNumber
      : rule<repeat<13>(ref<DIGIT>) |
             (repeat<9>(ref<DIGIT>) + ref<CHECK>)> {};
  struct ItemNumber
      : rule<ref<CHECK> + repeat<4>(ref<DIGIT>)> {};

  struct Issn
      : rule<lit<"S"> + assign<&ast::Issn::digits>(ref<IssnNumber>) +
             assign<&ast::Issn::year>(ref<Digits2>)> {};

  struct Isbn
      : rule<lit<"B"> + assign<&ast::Isbn::digits>(ref<IsbnNumber>)> {};

  static constexpr auto identifier = ref<Issn> | ref<Isbn>;

  struct Doc
      : rule<assign<&ast::Doc::id>(identifier) +
             assign<&ast::Doc::item>(ref<ItemNumber>) +
             assign<&ast::Doc::check>(ref<CHECK>)> {};

  using entry = Doc;
  using ignored = Trivia;
};

}  // namespace pii
