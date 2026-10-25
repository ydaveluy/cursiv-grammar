#include "protobuf/core/Numbers.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
namespace protobuf {
namespace {
std::string_view token(std::string_view text) {
  for (;;) {
    const auto next = text.find_first_not_of(" \t\r\n\f");
    if (next == text.npos)
      return {};
    text.remove_prefix(next);
    if (text.starts_with("/*")) {
      auto end = text.find("*/", 2);
      if (end == text.npos)
        return {};
      text.remove_prefix(end + 2);
    } else if (text.starts_with("//")) {
      auto end = text.find_first_of("\r\n", 2);
      if (end == text.npos)
        return {};
      text.remove_prefix(end);
    } else
      return text;
  }
}
std::optional<double> floating(std::string_view text) {
  if (text == "inf")
    return INFINITY;
  if (text == "nan")
    return std::numeric_limits<double>::quiet_NaN();
  double value = 0;
  auto [end, error] = std::from_chars(text.data(), text.data() + text.size(),
                                      value, std::chars_format::general);
  if (end != text.data() + text.size())
    return {};
  if (error == std::errc{})
    return value;
  if (error != std::errc::result_out_of_range)
    return {};
  const auto e = text.find_first_of("eE");
  const auto mantissa = text.substr(0, e);
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
  auto dot = mantissa.find('.');
  std::int64_t zeros = 0;
  for (char c : mantissa) {
    if (c == '.')
      continue;
    if (c != '0')
      break;
    ++zeros;
  }
  auto order = std::int64_t(dot == mantissa.npos ? mantissa.size() : dot) -
               zeros - 1 + exponent;
  return order < 0 ? 0.0 : INFINITY;
}
} // namespace
std::optional<Number> number_value(std::string_view text, bool real) {
  bool negative = text.starts_with('-');
  if (negative || text.starts_with('+'))
    text = token(text.substr(1));
  if (text.empty())
    return {};
  if (!real) {
    auto digits = text;
    int base = 10;
    if (digits.size() > 1 && digits[0] == '0') {
      base = 8;
      if (digits[1] == 'x' || digits[1] == 'X') {
        base = 16;
        digits.remove_prefix(2);
      } else
        digits.remove_prefix(1);
    }
    std::uint64_t magnitude = 0;
    auto [end, error] = std::from_chars(
        digits.data(), digits.data() + digits.size(), magnitude, base);
    if (error == std::errc{} && end == digits.data() + digits.size()) {
      if (!negative)
        return magnitude;
      const auto limit = std::uint64_t(INT64_MAX) + 1;
      if (magnitude <= limit)
        return magnitude == limit ? INT64_MIN : -std::int64_t(magnitude);
      return -double(magnitude);
    }
    if (base != 10)
      return {};
  }
  auto value = floating(text);
  if (!value)
    return {};
  return negative ? -*value : *value;
}
std::optional<std::int64_t> integer_value(std::string_view text) {
  const auto number = number_value(text);
  if (!number)
    return {};
  if (auto *v = std::get_if<std::int64_t>(&*number))
    return *v;
  if (auto *v = std::get_if<std::uint64_t>(&*number); v && *v <= INT64_MAX)
    return std::int64_t(*v);
  return {};
}
} // namespace protobuf
