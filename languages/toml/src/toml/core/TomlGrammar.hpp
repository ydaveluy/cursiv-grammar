// The toml language: its grammar. Lines are separated by NL, and each table
// header (`[key]`/`[[key]]`) opens a new run of entries until the next one;
// only inside `[...]` (an array) do NL and comments become trivia -- an
// inline table stays single-line.
#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "toml/core/TomlString.hpp"
#include "toml/core/ast.hpp"

namespace toml {

inline constexpr auto string_value = [](std::string_view t,
                                        std::pmr::memory_resource *memory) {
  const auto decoded = decode_string(t);
  return cursiv::copy_into(memory, decoded.text);
};
using StringConv = cursiv::text_conv<string_value>;

// `from_chars` takes a leading '-' but never a leading '+' -- unlike
// `strtod`/`strtoll` -- so a `+`-prefixed literal drops the sign here too.
inline constexpr auto strip_to = [](std::string_view t, std::size_t skip,
                                    std::pmr::memory_resource *m) {
  if (skip < t.size() && t[skip] == '+')
    ++skip;
  char *buf = static_cast<char *>(m->allocate(t.size() - skip));
  std::size_t n = 0;
  for (char c : t.substr(skip))
    if (c != '_')
      buf[n++] = c;
  return std::string_view{buf, n};
};

inline constexpr auto dec_int_value =
    [](std::string_view t, std::pmr::memory_resource *m) -> std::int64_t {
  const std::string_view s = strip_to(t, 0, m);
  std::int64_t v = 0;
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
  // The original token remains in the CST for the runtime range check.
  return ec == std::errc{} && ptr == s.data() + s.size() ? v : 0;
};
using DecIntConv = cursiv::text_conv<dec_int_value>;

template <int Base>
inline constexpr auto radix_int_value =
    [](std::string_view t, std::pmr::memory_resource *m) -> std::int64_t {
  const std::string_view s = strip_to(t, 2, m);
  std::uint64_t v = 0;
  const auto [ptr, ec] =
      std::from_chars(s.data(), s.data() + s.size(), v, Base);
  return ec == std::errc{} && ptr == s.data() + s.size() &&
                 v <= static_cast<std::uint64_t>(
                          std::numeric_limits<std::int64_t>::max())
             ? static_cast<std::int64_t>(v)
             : 0;
};
using HexIntConv = cursiv::text_conv<radix_int_value<16>>;
using OctIntConv = cursiv::text_conv<radix_int_value<8>>;
using BinIntConv = cursiv::text_conv<radix_int_value<2>>;

inline constexpr auto float_value = [](std::string_view t,
                                       std::pmr::memory_resource *m) -> double {
  const std::string_view s = strip_to(t, 0, m);
  double v = 0;
  auto t2 = s;
  const std::from_chars_result r =
      std::from_chars(s.data(), s.data() + s.size(), v);
  if (r.ec == std::errc{})
    return v;
  const bool negative = t2.starts_with('-');
  if (negative)
    t2.remove_prefix(1);
  const auto e = t2.find_first_of("eE");
  auto mantissa = t2.substr(0, e);
  long long exponent = 0;
  if (e != std::string_view::npos) {
    auto digits = t2.substr(e + 1);
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
using FloatConv = cursiv::text_conv<float_value>;

inline constexpr auto inf_value = [](std::string_view t) {
  return t.front() == '-' ? -std::numeric_limits<double>::infinity()
                          : std::numeric_limits<double>::infinity();
};
using InfConv = cursiv::text_conv<inf_value>;

inline constexpr auto nan_value = [](std::string_view t) {
  return t.starts_with('-') ? -std::numeric_limits<double>::quiet_NaN()
                            : std::numeric_limits<double>::quiet_NaN();
};
using NanConv = cursiv::text_conv<nan_value>;

template <ast::DateTimeKind Kind>
inline constexpr auto date_time_value =
    [](std::string_view t) { return ast::DateTime{Kind, t}; };
using OffsetDateTimeConv =
    cursiv::text_conv<date_time_value<ast::DateTimeKind::OffsetDateTime>>;
using LocalDateTimeConv =
    cursiv::text_conv<date_time_value<ast::DateTimeKind::LocalDateTime>>;
using LocalDateConv =
    cursiv::text_conv<date_time_value<ast::DateTimeKind::LocalDate>>;
using LocalTimeConv =
    cursiv::text_conv<date_time_value<ast::DateTimeKind::LocalTime>>;

struct Toml : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct NL : terminal<some(option(lit<"\r">) + lit<"\n">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};

  struct UNQUOTED_KEY : terminal<some(cr<"A-Za-z0-9_-">)> {};

  static constexpr auto hex_digit = cr<"0-9A-Fa-f">;
  // Escape validity belongs to the runtime checker, which still sees the
  // complete token. The parser only needs to keep escaped delimiters inside.
  static constexpr auto esc = lit<"\\"> + any;
  struct BASIC_STRING
      : terminal<lit<"\""> + many(esc | cr<R"(^"\\\n)">) + lit<"\"">,
                 StringConv> {};
  struct LITERAL_STRING
      : terminal<lit<"'"> + many(cr<R"(^'\n)">) + lit<"'">, StringConv> {};
  struct ML_BASIC_STRING
      : terminal<lit<"\"\"\""> +
                     many(esc | cr<R"(^"\\)"> | lit<"\"\"", !lit<"\"">> |
                          lit<"\"", !lit<"\"">>) +
                     lit<"\"\"\""> + repeat<0, 2>(lit<"\"">),
                 StringConv> {};
  struct ML_LITERAL_STRING
      : terminal<through(lit<"'''">, lit<"'''">)+repeat<0, 2>(lit<"'">),
                 StringConv> {};

  static constexpr auto zero_prefixable_int = d + many(d | (lit<"_"> + d));
  static constexpr auto dec_int_body =
      option(cr<"+-">) + (lit<"0"> | (cr<"1-9"> + many(d | (lit<"_"> + d))));
  static constexpr auto exponent =
      cr<"eE"> + option(cr<"+-">) + zero_prefixable_int;
  static constexpr auto frac = lit<"."> + zero_prefixable_int;

  struct DEC_INT : terminal<dec_int_body, DecIntConv> {};
  struct HEX_INT : terminal<lit<"0x"> + hex_digit +
                                many(hex_digit | (lit<"_"> + hex_digit)),
                            HexIntConv> {};
  struct OCT_INT : terminal<lit<"0o"> + cr<"0-7"> +
                                many(cr<"0-7"> | (lit<"_"> + cr<"0-7">)),
                            OctIntConv> {};
  struct BIN_INT : terminal<lit<"0b"> + cr<"0-1"> +
                                many(cr<"0-1"> | (lit<"_"> + cr<"0-1">)),
                            BinIntConv> {};
  struct FLOAT : terminal<dec_int_body + (exponent | (frac + option(exponent))),
                          FloatConv> {};
  struct INF : terminal<option(cr<"+-">) + lit<"inf">, InfConv> {};
  struct NAN_TOK : terminal<option(cr<"+-">) + lit<"nan">, NanConv> {};

  struct BOOLEAN : terminal<kw<"true"> | kw<"false">, bool> {};

  static constexpr auto year_month_day =
      repeat<4>(d) + lit<"-"> + repeat<2>(d) + lit<"-"> + repeat<2>(d);
  static constexpr auto partial_time = repeat<2>(d) + lit<":"> + repeat<2>(d) +
                                       lit<":"> + repeat<2>(d) +
                                       option(lit<"."> + some(d));
  static constexpr auto num_offset =
      cr<"+-"> + repeat<2>(d) + lit<":"> + repeat<2>(d);
  static constexpr auto offset = cr<"Zz"> | num_offset;
  static constexpr auto delim = lit<"T"> | lit<"t"> | lit<" ">;

  struct OFFSET_DATE_TIME
      : terminal<year_month_day + delim + partial_time + offset,
                 OffsetDateTimeConv> {};
  struct LOCAL_DATE_TIME
      : terminal<year_month_day + delim + partial_time, LocalDateTimeConv> {};
  struct LOCAL_DATE : terminal<year_month_day, LocalDateConv> {};
  struct LOCAL_TIME : terminal<partial_time, LocalTimeConv> {};

  static constexpr auto simple_key =
      ref<BASIC_STRING> | ref<LITERAL_STRING> | ref<UNQUOTED_KEY>;
  struct DottedKey
      : rule<some(append<&ast::Key::parts>(simple_key), lit<".">)> {};

  struct ArrayRule;
  struct InlineTableRule;

  static constexpr auto value =
      ref<BASIC_STRING> | ref<ML_BASIC_STRING> | ref<LITERAL_STRING> |
      ref<ML_LITERAL_STRING> | ref<FLOAT> | ref<INF> | ref<NAN_TOK> |
      ref<DEC_INT> | ref<HEX_INT> | ref<OCT_INT> | ref<BIN_INT> | ref<BOOLEAN> |
      ref<OFFSET_DATE_TIME> | ref<LOCAL_DATE_TIME> | ref<LOCAL_DATE> |
      ref<LOCAL_TIME> | ref<ArrayRule> | ref<InlineTableRule>;

  struct KeyValueRule : rule<assign<&ast::KeyValue::key>(ref<DottedKey>) +
                             lit<"="> + assign<&ast::KeyValue::value>(value)> {
  };

  // `skip()` regions may only narrow the ambient trivia, never widen it (a
  // state cannot un-skip a token on exit), so a multi-line array spells its
  // own padding: NL and COMMENT, zero or more, wherever whitespace could sit.
  static constexpr auto array_pad = many(ref<NL> | ref<COMMENT>);
  static constexpr auto array_item = append<&ast::Array::items>(value);

  struct ArrayRule
      : rule<lit<"["> + array_pad +
             option(array_item + array_pad +
                    many(lit<","> + array_pad + array_item + array_pad) +
                    option(lit<","> + array_pad)) +
             lit<"]">> {};

  struct InlineTableRule
      : rule<lit<"{"> +
             many(append<&ast::InlineTable::entries>(ref<KeyValueRule>),
                  lit<",">) +
             lit<"}">> {};

  static constexpr auto trailing_comment =
      tight(option(ref<COMMENT>));
  static constexpr auto line_body =
      option(append<&ast::Table::entries>(ref<KeyValueRule>)) +
      trailing_comment;

  static constexpr auto array_table_marker = lit<"["> + lit<"[">;

  struct TableBlockRule
      : rule<((flag<&ast::Table::is_array>(array_table_marker) +
               assign<&ast::Table::key>(ref<DottedKey>) + lit<"]"> + lit<"]">) |
              (lit<"["> + assign<&ast::Table::key>(ref<DottedKey>) +
               lit<"]">)) +
             line_body + many(ref<NL> + line_body)> {};

  struct RootBlockRule : rule<line_body + many(ref<NL> + line_body)> {};

  struct DocumentRule
      : rule<append<&ast::Document::tables>(ref<RootBlockRule>) +
             many(append<&ast::Document::tables>(ref<TableBlockRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

} // namespace toml
