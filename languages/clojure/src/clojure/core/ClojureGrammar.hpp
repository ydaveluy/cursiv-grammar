// The clojure language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <memory_resource>

#include "clojure/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace clojure {

inline std::string_view decode_escapes(std::string_view t,
                                       std::pmr::memory_resource *mem) {
  if (t.find('\\') == std::string_view::npos)
    return t;
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\' || i + 1 == t.size()) {
      room[n++] = t[i];
      continue;
    }
    const char escaped = t[++i];
    if (escaped >= '0' && escaped <= '7') {
      unsigned value = escaped - '0';
      for (unsigned count = 1;
           count < 3 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7';
           ++count)
        value = value * 8 + t[++i] - '0';
      room[n++] = static_cast<char>(value);
      continue;
    }
    if (escaped == '\n' || escaped == '\r') {
      if (escaped == '\r' && i + 1 < t.size() && t[i + 1] == '\n')
        ++i;
      continue;
    }
    if (t.substr(i, 3) == "\xe2\x80\xa8" || t.substr(i, 3) == "\xe2\x80\xa9") {
      i += 2;
      continue;
    }
    if (escaped == 'x') {
      room[n++] = static_cast<char>(hex(t.substr(i + 1, 2)));
      i += 2;
    } else if (escaped == 'u') {
      std::uint32_t cp = hex(t.substr(i + 1, 4));
      i += 4;
      if (cp >= 0xd800 && cp <= 0xdbff && t.substr(i + 1, 2) == "\\u" &&
          i + 7 <= t.size()) {
        const auto low = hex(t.substr(i + 3, 4));
        if (low >= 0xdc00 && low <= 0xdfff) {
          cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          i += 6;
        }
      }
      if (cp < 0x80)
        room[n++] = static_cast<char>(cp);
      else {
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0xf0 | (cp >> 18));
        else if (cp >= 0x800)
          room[n++] = static_cast<char>(0xe0 | (cp >> 12));
        else
          room[n++] = static_cast<char>(0xc0 | (cp >> 6));
        if (cp >= 0x10000)
          room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
        if (cp >= 0x800)
          room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
        room[n++] = static_cast<char>(0x80 | (cp & 63));
      }
    } else {
      switch (escaped) {
      case '0':
        room[n++] = '\0';
        break;
      case 'v':
        room[n++] = '\v';
        break;
      case 'b':
        room[n++] = '\b';
        break;
      case 'f':
        room[n++] = '\f';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 'r':
        room[n++] = '\r';
        break;
      case 't':
        room[n++] = '\t';
        break;
      default:
        room[n++] = escaped;
        break;
      }
    }
  }
  return std::string_view(room.data(), n);
}
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource *mem) {
  return decode_escapes(t.substr(1, t.size() - 2), mem);
};
using Unquoted = cursiv::text_conv<unquote>;
inline constexpr auto regex_text = [](std::string_view t) {
  return t.substr(2, t.size() - 3);
};
using RegexText = cursiv::text_conv<regex_text>;

struct Clojure : cursiv::GrammarSpec {
  static constexpr auto Space =
      cr<R"( \t\r\n,\f\v)"> | uc::Zs | uc::Zl | uc::Zp;
  static constexpr auto Stop = Space | cr<R"(";\\()[]{}@^`~)">;
  static constexpr auto word_boundary = ~Stop;
  struct SYMBOL
      : terminal<!(cr<"+-"> + d) + ~(Stop | d | cr<R"(#:'%)">)+many(~Stop)> {};
  struct KEYWORD_NAME : terminal<some(~Stop)> {};
  static constexpr auto symbol_text = ref<SYMBOL>;

  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                               lit<"\"">,
                           Unquoted> {};
  struct REGEX : terminal<lit<"#\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                              lit<"\"">,
                          RegexText> {};

  static constexpr auto float_decimal = lit<"."> + many(d);
  static constexpr auto float_exp = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto float_tail =
      (float_decimal + option(float_exp)) | float_exp;
  struct FLOAT : terminal<option(cr<"+-">) + some(d) + float_tail> {};
  struct SYMBOLIC_FLOAT
      : terminal<lit<"##"> + (lit<"Inf"> | lit<"-Inf"> | lit<"NaN">)> {};

  struct RATIO : terminal<option(cr<"+-">) + some(d) + lit<"/"> + some(d)> {};
  struct DECIMAL
      : terminal<option(cr<"+-">) + some(d) + option(lit<"."> + many(d)) +
                 option(float_exp) + lit<"M">> {};
  struct RatioLitRule : rule<assign<&ast::RatioLit::text>(ref<RATIO>)> {};
  struct DecimalLitRule : rule<assign<&ast::DecimalLit::text>(ref<DECIMAL>)> {};

  static constexpr auto hexd = cr<"0-9a-fA-F">;
  struct HEX : terminal<option(cr<"+-">) + lit<"0"> + cr<"xX"> + some(hexd) +
                        option(lit<"N">)> {};
  struct BIN : terminal<lit<"0"> + cr<"bB"> + some(cr<"01">)> {};
  struct LONG : terminal<option(cr<"+-">) + some(d) + option(cr<"lL">)> {};
  struct BIGN : terminal<option(cr<"+-">) + some(d) + cr<"nN">> {};

  struct RADIX : terminal<option(cr<"+-">) + repeat<1, 2>(d) + cr<"rR"> +
                          some(cr<"0-9a-zA-Z">)> {};
  struct CHAR_OCT : terminal<lit<"\\o"> + repeat<1, 3>(cr<"0-7">)> {};
  struct CHAR_U : terminal<lit<"\\u"> + repeat<4>(hexd)> {};
  struct CHAR_NAMED
      : terminal<lit<"\\"> + (kw<"newline"> | kw<"return"> | kw<"space"> |
                              kw<"tab"> | kw<"formfeed"> | kw<"backspace">)> {};
  struct CHAR_ANY : terminal<lit<"\\"> + any> {};

  struct NIL : terminal<kw<"nil">> {};
  struct BOOLEAN : terminal<kw<"true"> | kw<"false">, bool> {};

  struct PARAM_NAME
      : terminal<lit<"%"> + option((cr<"1-9"> + many(d)) | lit<"&">)> {};

  static constexpr auto not_eol = ~cr<R"(\r\n)">;
  struct COMMENT : terminal<lit<";"> + many(not_eol)> {};
  struct WS : terminal<some(Space)> {};

  struct StringLitRule;
  struct FloatLitRule;
  struct HexLitRule;
  struct BinLitRule;
  struct BignLitRule;
  struct LongLitRule;
  struct CharLitRule;
  struct NilLitRule;
  struct BoolLitRule;
  struct KeywordRule;
  struct SymbolRule;
  struct ParamNameRule;
  struct ListRule;
  struct VectorRule;
  struct MapRule;
  struct SetRule;
  struct LambdaRule;
  struct MetaDataRule;
  struct RegexRule;
  struct VarQuoteRule;
  struct HostExprRule;
  struct TagRule;
  struct DiscardRule;
  struct DispatchRule;
  struct DerefRule;
  struct QuoteRule;
  struct BacktickRule;
  struct UnquoteRule;
  struct UnquoteSplicingRule;
  struct ConditionalRule;
  struct RadixLitRule;

  static constexpr auto undiscarded_form =
      ref<RatioLitRule> | ref<DecimalLitRule> | ref<StringLitRule> |
      ref<FloatLitRule> | ref<HexLitRule> | ref<BinLitRule> | ref<BignLitRule> |
      ref<LongLitRule> | ref<CharLitRule> | ref<NilLitRule> | ref<BoolLitRule> |
      ref<KeywordRule> | ref<SymbolRule> | ref<ParamNameRule> | ref<ListRule> |
      ref<VectorRule> | ref<MapRule> | ref<SetRule> | ref<LambdaRule> |
      ref<MetaDataRule> | ref<RegexRule> | ref<VarQuoteRule> |
      ref<HostExprRule> | ref<TagRule> | ref<DispatchRule> | ref<DerefRule> |
      ref<QuoteRule> | ref<BacktickRule> | ref<UnquoteRule> |
      ref<UnquoteSplicingRule> | ref<ConditionalRule> | ref<RadixLitRule>;

  static constexpr auto form = undiscarded_form | ref<DiscardRule>;
  template <auto Member>
  static constexpr auto OperandInto =
      many(append<&ast::Form::discards>(ref<DiscardRule>)) +
      assign<Member>(undiscarded_form);

  struct StringLitRule : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  struct FloatLitRule
      : rule<assign<&ast::FloatLit::text>(ref<FLOAT> | ref<SYMBOLIC_FLOAT>)> {};
  struct HexLitRule : rule<assign<&ast::HexLit::text>(ref<HEX>)> {};
  struct RadixLitRule : rule<assign<&ast::RadixLit::text>(ref<RADIX>)> {};
  struct BinLitRule : rule<assign<&ast::BinLit::text>(ref<BIN>)> {};
  struct BignLitRule : rule<assign<&ast::BignLit::text>(ref<BIGN>)> {};
  struct LongLitRule : rule<assign<&ast::LongLit::text>(ref<LONG>)> {};
  struct CharLitRule
      : rule<assign<&ast::CharLit::text>(ref<CHAR_NAMED> | ref<CHAR_U> |
                                         ref<CHAR_OCT> | ref<CHAR_ANY>)> {};
  struct NilLitRule : rule<create<ast::NilLit>(ref<NIL>)> {};
  struct BoolLitRule : rule<assign<&ast::BoolLit::value>(ref<BOOLEAN>)> {};

  struct KeywordRule
      : rule<lit<":"> + option(flag<&ast::Keyword::macro>(lit<":">)) +
             assign<&ast::Keyword::name>(ref<KEYWORD_NAME>)> {};
  struct SymbolRule : rule<assign<&ast::Symbol::text>(symbol_text)> {};
  struct ParamNameRule : rule<assign<&ast::ParamName::text>(ref<PARAM_NAME>)> {
  };

  struct ListRule
      : rule<lit<"("> + many(append<&ast::ListForm::items>(form)) + lit<")">> {
  };
  struct VectorRule
      : rule<lit<"["> + many(append<&ast::VectorForm::items>(form)) +
             lit<"]">> {};
  struct SetRule
      : rule<lit<"#{"> + many(append<&ast::SetForm::items>(form)) + lit<"}">> {
  };

  struct MapEntryRule
      : rule<assign<&ast::MapEntry::key>(undiscarded_form) +
             many(append<&ast::MapEntry::discards>(ref<DiscardRule>)) +
             assign<&ast::MapEntry::value>(undiscarded_form)> {};
  struct MapRule
      : rule<option(lit<"#:"> +
                    option(flag<&ast::MapForm::autoNamespace>(lit<":">)) +
                    option(assign<&ast::MapForm::namespace_>(symbol_text))) +
             lit<"{"> +
             many(append<&ast::Form::discards>(ref<DiscardRule>) |
                  append<&ast::MapForm::entries>(ref<MapEntryRule>)) +
             lit<"}">> {};

  struct QuoteRule : rule<lit<"'"> + OperandInto<&ast::QuoteForm::form>> {};
  struct BacktickRule : rule<lit<"`"> + OperandInto<&ast::BacktickForm::form>> {
  };
  struct UnquoteSplicingRule
      : rule<lit<"~@"> + OperandInto<&ast::UnquoteSplicingForm::form>> {};
  struct UnquoteRule : rule<lit<"~"> + OperandInto<&ast::UnquoteForm::form>> {};
  struct TagRule : rule<lit<"^"> + OperandInto<&ast::TagForm::tag> +
                        OperandInto<&ast::TagForm::form>> {};
  struct DerefRule : rule<lit<"@"> + OperandInto<&ast::DerefForm::form>> {};
  struct LambdaRule
      : rule<lit<"#("> + many(append<&ast::LambdaForm::items>(form)) +
             lit<")">> {};
  struct MetaDataRule : rule<lit<"#^"> + OperandInto<&ast::MetaDataForm::meta> +
                             OperandInto<&ast::MetaDataForm::form>> {};
  struct ConditionalRule
      : rule<lit<"#?"> +
             option(flag<&ast::ConditionalForm::splicing>(lit<"@">)) +
             OperandInto<&ast::ConditionalForm::form>> {};
  struct VarQuoteRule
      : rule<lit<"#'"> + OperandInto<&ast::VarQuoteForm::target>> {};
  struct HostExprRule : rule<lit<"#+"> + OperandInto<&ast::HostExprForm::a> +
                             OperandInto<&ast::HostExprForm::b>> {};
  struct DiscardRule : rule<lit<"#_"> + OperandInto<&ast::DiscardForm::form>> {
  };
  struct DispatchRule
      : rule<lit<"#"> + assign<&ast::DispatchForm::tag>(ref<SymbolRule>) +
             OperandInto<&ast::DispatchForm::form>> {};
  struct RegexPatternRule : rule<create<ast::StringLit> +
                                 assign<&ast::StringLit::text>(ref<REGEX>)> {};
  struct RegexRule
      : rule<assign<&ast::RegexForm::pattern>(ref<RegexPatternRule>)> {};

  struct FileRule : rule<many(append<&ast::File::forms>(form))> {};

  using entry = FileRule;
  using hidden = list<WS, COMMENT>;
};

} // namespace clojure
