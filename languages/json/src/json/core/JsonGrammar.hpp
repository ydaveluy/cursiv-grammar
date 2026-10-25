// The json language: its grammar.
#pragma once

#include <charconv>
#include <limits>
#include <memory_resource>
#include <string_view>
#include <system_error>
#include <variant>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "json/core/ast.hpp"
#include "json/core/JsonString.hpp"

namespace json {

inline constexpr auto unquote = [](std::string_view t, std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos) return body;
  return cursiv::copy_into(mem, decode_string(body));
};
using Unquoted = cursiv::text_conv<unquote>;
inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};
using NullValue = cursiv::text_conv<null_value>;

// RFC 8259 §6 allows a double-range implementation to keep going past it:
// a magnitude from_chars cannot represent saturates to signed infinity
// instead of failing the conversion.
inline constexpr auto to_double = [](std::string_view t) {
  double v = 0.0;
  const std::from_chars_result r =
      std::from_chars(t.data(), t.data() + t.size(), v);
  if (r.ec == std::errc{})
    return v;
  const bool negative = t.starts_with('-');
  if (negative)
    t.remove_prefix(1);
  const auto e = t.find_first_of("eE");
  auto mantissa = t.substr(0, e);
  long long exponent = 0;
  if (e != std::string_view::npos) {
    auto digits = t.substr(e + 1);
    const bool minus = digits.starts_with('-');
    if (!digits.empty() && (digits[0] == '+' || digits[0] == '-'))
      digits.remove_prefix(1);
    for (char c : digits)
      if (exponent < 1000000)
        exponent = exponent * 10 + c - '0';
    if (minus)
      exponent = -exponent;
  }
  const auto dot = mantissa.find('.');
  const auto whole = dot == std::string_view::npos ? mantissa.size() : dot;
  long long digit = 0, first = -1;
  for (char c : mantissa)
    if (c != '.') {
      if (first < 0 && c != '0')
        first = digit;
      ++digit;
    }
  const bool underflow =
      first < 0 || static_cast<long long>(whole) - first - 1 + exponent < 0;
  const double magnitude =
      underflow ? 0.0 : std::numeric_limits<double>::infinity();
  return negative ? -magnitude : magnitude;
};
using ToDouble = cursiv::text_conv<to_double>;

struct Json : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};

  struct NUMBER
      : terminal<option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d))) +
                     option(lit<"."> + some(d)) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 ToDouble> {};

  static constexpr auto escape =
      lit<"\\"> +
      (cr<R"("\\/bfnrt)"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  struct STRING
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\x00-\x1f)">) + lit<"\"">,
                 Unquoted> {};

  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};
  struct NULLK : terminal<kw<"null">, NullValue> {};

  struct ObjectRule;
  struct ArrayRule;
  static constexpr auto value = ref<ObjectRule> | ref<ArrayRule> | ref<STRING> |
                                ref<NUMBER> | ref<BOOL> | ref<NULLK>;

  struct MemberRule : rule<assign<&ast::Member::name>(ref<STRING>) + lit<":"> +
                           assign<&ast::Member::value>(value)> {};

  struct ObjectRule
      : rule<lit<"{"> +
             many(append<&ast::Object::members>(ref<MemberRule>), lit<",">) +
             lit<"}">> {};

  struct ArrayRule
      : rule<lit<"["> + many(append<&ast::Array::items>(value), lit<",">) +
             lit<"]">> {};

  struct DocRule : rule<assign<&ast::Doc::value>(value)> {};

  using entry = DocRule;
  using ignore = WS;
};

} // namespace json
