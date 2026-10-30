// The telephone language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "telephone/core/ast.hpp"

namespace telephone {

struct Telephone : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n)">)> {};

  struct DIGIT : terminal<d> {};

  struct Digits3 : rule<repeat<3>(ref<DIGIT>)> {};
  struct Digits4 : rule<repeat<4>(ref<DIGIT>)> {};

  struct Nanp
      : rule<create<ast::Nanp> + lit<"011"> +
             assign<&ast::Variation::areaCode>(ref<Digits3>) +
             assign<&ast::Variation::exchange>(ref<Digits3>) +
             assign<&ast::Variation::subscriber>(ref<Digits4>)> {};

  struct Japan
      : rule<create<ast::Japan> + lit<"010"> +
             assign<&ast::Variation::areaCode>(ref<Digits3>) +
             assign<&ast::Variation::exchange>(ref<Digits3>) +
             assign<&ast::Variation::subscriber>(ref<Digits4>)> {};

  static constexpr auto variation = ref<Nanp> | ref<Japan>;

  struct Number
      : rule<option(flag<&ast::Number::hasCountryCode>(lit<"+1">)) +
             option(flag<&ast::Number::hasPlus>(lit<"+">)) +
             assign<&ast::Number::variation>(variation)> {};

  using entry = Number;
  using ignored = WS;
};

}  // namespace telephone
