#pragma once
#include "stl/core/ast.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
namespace stl {
inline std::uint32_t little_u32(std::string_view bytes) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < bytes.size() && i < 4; ++i)
    value |= std::uint32_t(static_cast<unsigned char>(bytes[i])) << (8 * i);
  return value;
}
inline std::optional<double> coordinate(std::string_view text, bool binary) {
  if (binary) {
    if (text.size() != 4)
      return {};
    return static_cast<double>(std::bit_cast<float>(little_u32(text)));
  }
  if (text.empty())
    return {};
  if (text.starts_with('+')) {
    text.remove_prefix(1);
    if (text.starts_with('-') || text.starts_with('+'))
      return {};
  }
  double value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                      value, std::chars_format::general);
  if (result.ptr != text.data() + text.size())
    return {};
  if (result.ec == std::errc{})
    return value;
  if (result.ec != std::errc::result_out_of_range)
    return {};
  const bool negative = text.starts_with('-');
  if (negative)
    text.remove_prefix(1);
  const auto e = text.find_first_of("eE");
  auto mantissa = text.substr(0, e);
  std::int64_t exponent = 0;
  if (e != text.npos) {
    auto digits = text.substr(e + 1);
    const bool minus = digits.starts_with('-');
    if (digits.starts_with('-') || digits.starts_with('+'))
      digits.remove_prefix(1);
    for (char c : digits)
      exponent =
          std::min<std::int64_t>(1000000000000LL, exponent * 10 + c - '0');
    if (minus)
      exponent = -exponent;
  }
  const auto dot = mantissa.find('.');
  std::int64_t zeros = 0;
  for (char c : mantissa) {
    if (c == '.')
      continue;
    if (c != '0')
      break;
    ++zeros;
  }
  const auto order =
      static_cast<std::int64_t>(dot == mantissa.npos ? mantissa.size() : dot) -
      zeros - 1 + exponent;
  return std::copysign(order < 0 ? 0.0
                                 : std::numeric_limits<double>::infinity(),
                       negative ? -1.0 : 1.0);
}
inline std::optional<std::array<double, 3>>
coordinates(const ast::Triple &triple) {
  const auto i = coordinate(triple.i, triple.binary),
             j = coordinate(triple.j, triple.binary),
             k = coordinate(triple.k, triple.binary);
  if (!i || !j || !k)
    return {};
  return std::array<double, 3>{*i, *j, *k};
}
} // namespace stl
