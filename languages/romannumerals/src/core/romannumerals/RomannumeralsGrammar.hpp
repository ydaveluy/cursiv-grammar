#pragma once

#include "cursiv/grammar/GrammarSpec.hpp"
#include "romannumerals/ast.hpp"

namespace romannumerals {

struct Romannumerals : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LETTERS : terminal<some(cr<"IVXLCDMNivxlcdmn">)> {};

  struct RomanNumeral
      : rule<assign<&ast::RomanNumeral::text>(ref<LETTERS>)> {};

  using entry = RomanNumeral;
  using ignored = WS;
};

}  // namespace romannumerals
