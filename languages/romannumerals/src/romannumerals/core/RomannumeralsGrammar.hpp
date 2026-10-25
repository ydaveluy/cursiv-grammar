#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "romannumerals/core/ast.hpp"

namespace romannumerals {

struct Romannumerals : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LETTERS : terminal<some(cr<"IVXLCDMNivxlcdmn">)> {};

  struct RomanNumeralRule
      : rule<assign<&ast::RomanNumeral::text>(ref<LETTERS>)> {};

  using entry = RomanNumeralRule;
  using ignore = WS;
};

}  // namespace romannumerals
