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
inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};

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

struct Json : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};

  struct NUMBER : terminal<as<to_double>(
                      option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d))) +
                      option(lit<"."> + some(d)) +
                      option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  static constexpr auto escape =
      lit<"\\"> +
      (cr<R"("\\/bfnrt)"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  // Network JSON is UTF-8 (RFC 8259 §8.1). Keep the allowed scalar set
  // positive, so a complement does not also admit malformed UTF-8 bytes.
  static constexpr auto unescaped =
      cr<R"(\x20-\x21\x23-\x5b\x5d-\u{10ffff})">;
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(escape | unescaped) +
                             lit<"\"">)> {};

  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct NULLK : terminal<as<null_value>(kw<"null">)> {};

  struct Object;
  struct Array;
  static constexpr auto value = ref<Object> | ref<Array> | ref<STRING> |
                                ref<NUMBER> | ref<BOOL> | ref<NULLK>;

  struct Member : rule<assign<&ast::Member::name>(ref<STRING>) + lit<":"> +
                           assign<&ast::Member::value>(value)> {};

  struct Object
      : rule<lit<"{"> +
             many(append<&ast::Object::members>(ref<Member>), lit<",">) +
             lit<"}">> {};

  struct Array
      : rule<lit<"["> + many(append<&ast::Array::items>(value), lit<",">) +
             lit<"]">> {};

  struct Doc : rule<assign<&ast::Doc::value>(value)> {};

  using entry = Doc;
  using ignored = WS;
};

} // namespace json
