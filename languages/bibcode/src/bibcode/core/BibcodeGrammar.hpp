#pragma once

#include "bibcode/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bibcode {

struct Bibcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct CHAR : terminal<any> {};

  template <std::size_t Width>
  struct Field : rule<repeat<Width>(ref<CHAR>)> {};

  struct BibcodeRule : rule<option(ref<WS>) +
      assign<&ast::Bibcode::year>(ref<Field<4>>) +
      assign<&ast::Bibcode::publish>(ref<Field<5>>) +
      assign<&ast::Bibcode::volume>(ref<Field<4>>) +
      assign<&ast::Bibcode::section>(ref<Field<1>>) +
      assign<&ast::Bibcode::page>(ref<Field<4>>) +
      assign<&ast::Bibcode::author>(ref<Field<1>>) + option(ref<WS>)> {};

  using entry = BibcodeRule;
};

}  // namespace bibcode
