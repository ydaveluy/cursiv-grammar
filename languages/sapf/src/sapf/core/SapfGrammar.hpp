// The sapf language: its grammar.
#pragma once

#include <cmath>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sapf/core/ast.hpp"

// sapf's `number` glues an optional sign, mantissa, optional exponent-or-
// fraction and optional unit suffix with no gap allowed anywhere in between
// (`3.4e-3`, `7.5/4`, `4c`): fusing the whole thing into one terminal makes
// that tightness the lexer's maximal munch, instead of a parser-level
// ambiguity between a fraction and a division of two atoms.
inline double sapf_parse_number(std::string_view text) {
  std::size_t pos = 0;
  const std::size_t n = text.size();
  auto mantissa = [&]() -> double {
    bool negative = false;
    if (pos < n && (text[pos] == '-' || text[pos] == '+')) {
      negative = text[pos] == '-';
      ++pos;
    }
    double whole = 0.0;
    while (pos < n && text[pos] >= '0' && text[pos] <= '9') {
      whole = whole * 10.0 + static_cast<double>(text[pos] - '0');
      ++pos;
    }
    if (pos < n && text[pos] == '.') {
      ++pos;
      double frac = 0.0, scale = 1.0;
      while (pos < n && text[pos] >= '0' && text[pos] <= '9') {
        frac = frac * 10.0 + static_cast<double>(text[pos] - '0');
        scale *= 10.0;
        ++pos;
      }
      whole += frac / scale;
    }
    return negative ? -whole : whole;
  };
  double value = mantissa();
  if (pos < n && (text[pos] == 'e' || text[pos] == 'E')) {
    ++pos;
    value *= std::pow(10.0, mantissa());
  } else if (pos < n && text[pos] == '/') {
    ++pos;
    value /= mantissa();
  }
  return value;
}

inline constexpr auto sapf_suffix_of =
    [](std::string_view t) -> sapf::ast::Suffix {
  if (t == "pi") return sapf::ast::Suffix::pi;
  if (t == "M") return sapf::ast::Suffix::M;
  if (t == "k") return sapf::ast::Suffix::k;
  if (t == "h") return sapf::ast::Suffix::h;
  if (t == "c") return sapf::ast::Suffix::c;
  if (t == "m") return sapf::ast::Suffix::m;
  return sapf::ast::Suffix::u;
};

inline constexpr auto sapf_word_prefix_of =
    [](std::string_view t) -> sapf::ast::WordPrefix {
  switch (t.front()) {
    case '`':
      return sapf::ast::WordPrefix::backtick;
    case '\'':
      return sapf::ast::WordPrefix::quote;
    case ',':
      return sapf::ast::WordPrefix::comma;
    default:
      return sapf::ast::WordPrefix::dot;
  }
};

inline constexpr auto sapf_unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

namespace sapf {

struct Sapf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct LITERAL : terminal<cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">)> {};
  struct STRING : terminal<as<sapf_unquote>(through(lit<"\"">, lit<"\"">))> {};

  static constexpr auto digits = some(d);
  static constexpr auto mantissa = (digits + lit<"."> + digits) |
                                    (digits + lit<".">) |
                                    (lit<"."> + digits) | digits;
  static constexpr auto signedMantissa = option(cr<"+-">) + mantissa;
  struct NUMBER : terminal<as<sapf_parse_number>(
                      signedMantissa + option((cr<"eE"> + option(cr<"+-">) + digits) |
                                              (lit<"/"> + signedMantissa)))> {};
  struct SUFFIX
      : terminal<as<sapf_suffix_of>(lit<"pi"> | lit<"M"> | lit<"k"> | lit<"h"> |
                                    lit<"c"> | lit<"m"> | lit<"u">)> {};

  struct WORD_PREFIX : terminal<as<sapf_word_prefix_of>(
                           lit<"`"> | lit<"'"> | lit<","> | lit<".">)> {};

  struct Item;

  struct Operator
      : rule<create<ast::Operator>(assign<&ast::Operator::kind>(
            as<ast::OpKind::add>(lit<"+">) | as<ast::OpKind::subtract>(lit<"-">) | as<ast::OpKind::multiply>(lit<"*">) | as<ast::OpKind::divide>(lit<"/">)))> {};

  struct Number
      : rule<create<ast::NumberLiteral>(
            assign<&ast::NumberLiteral::value>(ref<NUMBER>) +
            option(gap::none +
                   assign<&ast::NumberLiteral::suffix>(ref<SUFFIX>)))> {};

  struct String
      : rule<create<ast::StringLiteral>(
            assign<&ast::StringLiteral::value>(ref<STRING>))> {};

  struct Word
      : rule<create<ast::Word>(
            option(assign<&ast::Word::prefix>(ref<WORD_PREFIX>)) +
            assign<&ast::Word::name>(ref<LITERAL>))> {};

  struct Group
      : rule<create<ast::Group>(
            lit<"("> + many(append<&ast::Group::items>(ref<Item>)) +
            lit<")">)> {};

  struct List
      : rule<create<ast::ListLiteral>(
            option(flag<&ast::ListLiteral::hashPrefix>(lit<"#">)) +
            lit<"["> +
            many(append<&ast::ListLiteral::items>(ref<Item>)) +
            lit<"]">)> {};

  struct Key
      : rule<create<ast::Key>(lit<":"> +
                              assign<&ast::Key::name>(ref<LITERAL>))> {};

  struct Form
      : rule<create<ast::FormLiteral>(
            lit<"{"> +
            many(append<&ast::FormLiteral::items>(ref<Key> |
                                                  ref<Item>)) +
            lit<"}"> +
            option(lit<"="> +
                   assign<&ast::FormLiteral::name>(ref<Word>)))> {};

  struct Function
      : rule<create<ast::FunctionLiteral>(
            lit<"\\"> +
            many(append<&ast::FunctionLiteral::params>(ref<LITERAL>)) +
            option(assign<&ast::FunctionLiteral::doc>(ref<STRING>)) +
            assign<&ast::FunctionLiteral::body>(ref<List>) +
            option(lit<"="> +
                   assign<&ast::FunctionLiteral::name>(ref<Word>)))> {};

  struct Item
      : rule<ref<Operator> | ref<Number> | ref<String> | ref<Word> |
             ref<Group> | ref<List> | ref<Form> |
             ref<Function>> {};

  struct File
      : rule<create<ast::File> +
             many(append<&ast::File::items>(ref<Item>))> {};

  using entry = File;
  using ignored = list<WS, COMMENT>;
};

}  // namespace sapf
