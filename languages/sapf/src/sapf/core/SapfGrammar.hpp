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
    if (pos < n && text[pos] == '-') {
      negative = true;
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
  if (pos < n && text[pos] == 'e') {
    ++pos;
    value *= std::pow(10.0, mantissa());
  } else if (pos < n && text[pos] == '/') {
    ++pos;
    value /= mantissa();
  }
  return value;
}
using NumberConv = cursiv::text_conv<sapf_parse_number>;

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
using SuffixConv = cursiv::text_conv<sapf_suffix_of>;

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
using WordPrefixConv = cursiv::text_conv<sapf_word_prefix_of>;

inline constexpr auto sapf_unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<sapf_unquote>;

namespace sapf {

struct Sapf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct LITERAL : terminal<some(cr<"a-zA-Z_">)> {};
  struct STRING : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  static constexpr auto digits = some(d);
  static constexpr auto mantissa = (digits + lit<"."> + digits) |
                                    (digits + lit<".">) |
                                    (lit<"."> + digits) | digits;
  static constexpr auto signedMantissa = option(lit<"-">) + mantissa;
  struct NUMBER
      : terminal<signedMantissa + option((lit<"e"> + signedMantissa) |
                                         (lit<"/"> + signedMantissa)),
                 NumberConv> {};
  struct SUFFIX
      : terminal<lit<"pi"> | lit<"M"> | lit<"k"> | lit<"h"> | lit<"c"> |
                     lit<"m"> | lit<"u">,
                 SuffixConv> {};

  struct WORD_PREFIX
      : terminal<lit<"`"> | lit<"'"> | lit<","> | lit<".">, WordPrefixConv> {
  };

  struct PLUS : terminal<lit<"+">, value<ast::OpKind::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::OpKind::subtract>> {};
  struct TIMES : terminal<lit<"*">, value<ast::OpKind::multiply>> {};
  struct DIV : terminal<lit<"/">, value<ast::OpKind::divide>> {};

  struct ItemRule;

  struct OperatorRule
      : rule<create<ast::Operator>(assign<&ast::Operator::kind>(
            ref<PLUS> | ref<MINUS> | ref<TIMES> | ref<DIV>))> {};

  struct NumberRule
      : rule<create<ast::NumberLiteral>(
            assign<&ast::NumberLiteral::value>(ref<NUMBER>) +
            option(gap::none +
                   assign<&ast::NumberLiteral::suffix>(ref<SUFFIX>)))> {};

  struct WordRule
      : rule<create<ast::Word>(
            option(assign<&ast::Word::prefix>(ref<WORD_PREFIX>)) +
            assign<&ast::Word::name>(ref<LITERAL>))> {};

  struct GroupRule
      : rule<create<ast::Group>(
            lit<"("> + some(append<&ast::Group::items>(ref<ItemRule>)) +
            lit<")">)> {};

  struct ListRule
      : rule<create<ast::ListLiteral>(
            option(flag<&ast::ListLiteral::hashPrefix>(lit<"#">)) +
            lit<"["> +
            some(append<&ast::ListLiteral::items>(ref<ItemRule>)) +
            lit<"]">)> {};

  struct KeyRule
      : rule<create<ast::Key>(lit<":"> +
                              assign<&ast::Key::name>(ref<LITERAL>))> {};

  struct FormRule
      : rule<create<ast::FormLiteral>(
            lit<"{"> +
            some(append<&ast::FormLiteral::items>(ref<KeyRule> |
                                                  ref<ItemRule>)) +
            lit<"}"> +
            option(lit<"="> +
                   assign<&ast::FormLiteral::name>(ref<WordRule>)))> {};

  struct FunctionRule
      : rule<create<ast::FunctionLiteral>(
            lit<"\\"> +
            many(append<&ast::FunctionLiteral::params>(ref<LITERAL>)) +
            option(assign<&ast::FunctionLiteral::doc>(ref<STRING>)) +
            assign<&ast::FunctionLiteral::body>(ref<ListRule>) +
            option(lit<"="> +
                   assign<&ast::FunctionLiteral::name>(ref<WordRule>)))> {};

  struct ItemRule
      : rule<ref<OperatorRule> | ref<NumberRule> | ref<WordRule> |
             ref<GroupRule> | ref<ListRule> | ref<FormRule> |
             ref<FunctionRule>> {};

  struct FileRule
      : rule<create<ast::File> +
             many(append<&ast::File::items>(ref<ItemRule>))> {};

  using entry = FileRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace sapf
