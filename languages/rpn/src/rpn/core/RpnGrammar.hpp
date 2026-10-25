// The rpn language: its grammar.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include "cursiv/core/grammar/Conversion.hpp"
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rpn/core/ast.hpp"

namespace rpn {

inline double decimal_value(std::string_view text) {
  if (const auto value = cursiv::default_convert<double>(text))
    return *value.value;
  const bool negative = text.starts_with('-');
  if (text.starts_with('+') || negative) text.remove_prefix(1);
  const auto e = text.find_first_of("eE");
  const auto mantissa = text.substr(0, e);
  std::int64_t exponent = 0;
  if (e != text.npos) {
    auto digits = text.substr(e + 1);
    const bool minus = digits.starts_with('-');
    if (digits.starts_with('+') || minus) digits.remove_prefix(1);
    for (char c : digits)
      exponent = std::min<std::int64_t>(1000000000, exponent * 10 + c - '0');
    if (minus) exponent = -exponent;
  }
  std::int64_t zeros = 0;
  bool nonzero = false;
  for (char c : mantissa) {
    if (c == '.') continue;
    if (c != '0') { nonzero = true; break; }
    ++zeros;
  }
  const auto dot = mantissa.find('.');
  const auto order = static_cast<std::int64_t>(dot == mantissa.npos ? mantissa.size() : dot) -
      zeros - 1 + exponent;
  const double magnitude = !nonzero || order < 0 ? 0.0 :
      std::numeric_limits<double>::infinity();
  return std::copysign(magnitude, negative ? -1.0 : 1.0);
}

inline constexpr auto parse_scientific = [](std::string_view text) {
  const auto e = text.find_first_of("eE");
  if (e == text.npos || text.substr(e + 1).find('.') == text.npos)
    return decimal_value(text);
  const double mantissa = decimal_value(text.substr(0, e));
  const double exponent = decimal_value(text.substr(e + 1));
  return mantissa == 0 ? mantissa : mantissa * std::pow(10.0, exponent);
};
using Scientific = cursiv::text_conv<parse_scientific>;

struct Rpn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  struct SCIENTIFIC_NUMBER
      : terminal<some(d) + option(lit<"."> + some(d)) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d) +
                            option(lit<"."> + some(d))),
                 Scientific> {};

  struct VARIABLE : terminal<cr<"a-zA-Z_"> + many(w)> {};

  // Each operator denotes its own constant; `signs` below reuses the same
  // terminals (and the same enumerators) for the unary +/- prefix, since
  // `+`/`-` is one token read in two rules, never two tokens for one glyph.
  struct POW : terminal<lit<"^">, value<ast::Operator::pow>> {};
  struct PLUS : terminal<lit<"+">, value<ast::Operator::plus>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Operator::minus>> {};
  struct TIMES : terminal<lit<"*">, value<ast::Operator::times>> {};
  struct DIV : terminal<lit<"/">, value<ast::Operator::div>> {};
  static constexpr auto oper = ref<POW> | ref<PLUS> | ref<MINUS> | ref<TIMES> | ref<DIV>;

  struct SignedAtomRule
      : rule<many(append<&ast::SignedAtom::signs>(ref<PLUS> | ref<MINUS>)) +
             assign<&ast::SignedAtom::value>(ref<SCIENTIFIC_NUMBER> |
                                             ref<VARIABLE>)> {};

  static constexpr auto term = ref<SignedAtomRule> | oper;

  struct ExpressionRule
      : rule<append<&ast::Expression::items>(ref<SignedAtomRule>) +
             many(append<&ast::Expression::items>(term))> {};

  using entry = ExpressionRule;
  using ignore = WS;
};

}  // namespace rpn
