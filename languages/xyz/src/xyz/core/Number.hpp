#pragma once

#include "cursiv/core/grammar/Conversion.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace xyz {
inline cursiv::ConversionResult<double> coordinate_value(std::string_view text) {
  if (text.find('_') != text.npos) {
    // Separator placement was already checked by the NUM terminal.
    std::string compact;
    compact.reserve(text.size());
    for (char c : text)
      if (c != '_') compact += c;
    return coordinate_value(compact);
  }
  auto parsed = cursiv::default_convert<double>(text);
  if (parsed)
    return parsed;
  if (text.starts_with('+'))
    text.remove_prefix(1);
  double value = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
  if (error != std::errc::result_out_of_range || end != text.data() + text.size())
    return parsed;
  const bool negative = text.starts_with('-');
  if (negative)
    text.remove_prefix(1);
  const auto e = text.find_first_of("eE");
  const auto mantissa = text.substr(0, e);
  std::int64_t exponent = 0;
  if (e != text.npos) {
    auto digits = text.substr(e + 1);
    const bool minus = digits.starts_with('-');
    if (digits.starts_with('-') || digits.starts_with('+'))
      digits.remove_prefix(1);
    for (char c : digits)
      exponent = std::min<std::int64_t>(1000000000, exponent * 10 + c - '0');
    if (minus)
      exponent = -exponent;
  }
  const auto dot = mantissa.find('.');
  std::int64_t zeros = 0;
  bool nonzero = false;
  for (char c : mantissa) {
    if (c == '.')
      continue;
    if (c != '0') {
      nonzero = true;
      break;
    }
    ++zeros;
  }
  const auto order =
      static_cast<std::int64_t>(dot == mantissa.npos ? mantissa.size() : dot) -
      zeros - 1 + exponent;
  const auto magnitude = !nonzero || order < 0 ? 0.0 :
      std::numeric_limits<double>::infinity();
  return cursiv::ConversionResult<double>::ok(
      std::copysign(magnitude, negative ? -1.0 : 1.0));
}
}  // namespace xyz
