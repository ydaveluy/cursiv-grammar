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
  if (t.find('\\') == std::string_view::npos) return t;
  const auto room = cursiv::text_room(mem, t.size());
  std::size_t n = 0;
  const auto hex = [](std::string_view digits) {
    std::uint32_t cp = 0;
    std::from_chars(digits.data(), digits.data() + digits.size(), cp, 16);
    return cp;
  };
  const auto append = [&](std::uint32_t cp) {
    if (cp < 0x80) room[n++] = static_cast<char>(cp);
    else {
      if (cp >= 0x10000) room[n++] = static_cast<char>(0xf0 | (cp >> 18));
      else if (cp >= 0x800) room[n++] = static_cast<char>(0xe0 | (cp >> 12));
      else room[n++] = static_cast<char>(0xc0 | (cp >> 6));
      if (cp >= 0x10000)
        room[n++] = static_cast<char>(0x80 | ((cp >> 12) & 63));
      if (cp >= 0x800)
        room[n++] = static_cast<char>(0x80 | ((cp >> 6) & 63));
      room[n++] = static_cast<char>(0x80 | (cp & 63));
    }
  };
  for (std::size_t i = 0; i < t.size(); ++i) {
    if (t[i] != '\\') { room[n++] = t[i]; continue; }
    const char escaped = t[++i];
    if (escaped >= '0' && escaped <= '7') {
      unsigned value = escaped - '0';
      for (unsigned count = 1;
           count < 3 && i + 1 < t.size() && t[i + 1] >= '0' && t[i + 1] <= '7';
           ++count)
        value = value * 8 + t[++i] - '0';
      // Clojure's octal escape denotes a Java character, not a UTF-8 byte.
      append(value);
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
      append(cp);
    } else {
      switch (escaped) {
      case 'b': room[n++] = '\b'; break;
      case 'f': room[n++] = '\f'; break;
      case 'n': room[n++] = '\n'; break;
      case 'r': room[n++] = '\r'; break;
      case 't': room[n++] = '\t'; break;
      default: room[n++] = escaped; break;
      }
    }
  }
  return std::string_view(room.data(), n);
}
inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource *mem) {
  return decode_escapes(t.substr(1, t.size() - 2), mem);
};
inline constexpr auto regex_text = [](std::string_view t) {
  return t.substr(2, t.size() - 3);
};

struct Clojure : cursiv::GrammarSpec {
  // java.lang.Character.isWhitespace excludes these three Zs characters.
  static constexpr auto Space =
      cr<R"(\x09-\x0d\x1c-\x20,\u1680\u2000-\u2006\u2008-\u200a\u2028\u2029\u205f\u3000)">;
  static constexpr auto Stop = Space | cr<R"(";\\()[]{}@^`~)">;
  static constexpr auto word_boundary = ~Stop;
  static constexpr auto EndToken = &Stop | eof;
  static constexpr auto ParamBody =
      lit<"%"> + option((cr<"1-9"> + many(d)) | lit<"&">);
  struct SYMBOL
      : terminal<!(cr<"+-"> + d) + !(ParamBody + EndToken) +
                 ~(Stop | d | cr<R"(#:')">) + many(~Stop)> {};
  struct KEYWORD_NAME : terminal<~(Stop | cr<":">) + many(~Stop)> {};
  static constexpr auto symbol_text = ref<SYMBOL>;

  static constexpr auto hexd = cr<"0-9a-fA-F">;
  static constexpr auto octald = cr<"0-7">;
  // The native string reader consumes up to three octal digits, stopping
  // early only at whitespace or a reader macro, and rejects values > 0377.
  static constexpr auto OctalStop = Space | cr<R"(";'@^`~()[]{}\\%#)">;
  static constexpr auto StringOctal =
      (cr<"0-3"> + repeat<2>(octald)) |
      (repeat<1, 2>(octald) + &OctalStop);
  static constexpr auto StringEscape =
      lit<"\\"> + (cr<R"(trn\\"bf)"> | (lit<"u"> + repeat<4>(hexd)) |
                       StringOctal);
  struct STRING
      : terminal<as<unquote>(
            lit<"\""> + many(StringEscape | cr<R"(^"\\)">) + lit<"\"">)> {};
  struct REGEX
      : terminal<as<regex_text>(
            lit<"#\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {
  };

  static constexpr auto float_decimal = lit<"."> + many(d);
  static constexpr auto float_exp = cr<"eE"> + option(cr<"+-">) + some(d);
  static constexpr auto float_tail =
      (float_decimal + option(float_exp)) | float_exp;
  struct FLOAT : terminal<option(cr<"+-">) + some(d) + float_tail + EndToken> {};
  struct SYMBOLIC_FLOAT
      : terminal<lit<"##"> + (lit<"Inf"> | lit<"-Inf"> | lit<"NaN">) + EndToken> {};

  struct RATIO : terminal<option(cr<"+-">) + some(d) + lit<"/"> + many(lit<"0">) +
                                 cr<"1-9"> + many(d) + EndToken> {};
  struct DECIMAL
      : terminal<option(cr<"+-">) + some(d) + option(lit<"."> + many(d)) +
                 option(float_exp) + lit<"M"> + EndToken> {};
  struct RatioLit : rule<assign<&ast::RatioLit::text>(ref<RATIO>)> {};
  struct DecimalLit : rule<assign<&ast::DecimalLit::text>(ref<DECIMAL>)> {};

  struct HEX : terminal<option(cr<"+-">) + lit<"0"> + cr<"xX"> + some(hexd) +
                        option(lit<"N">) + EndToken> {};
  static constexpr auto IntegerBody =
      (lit<"0"> + many(octald)) | (cr<"1-9"> + many(d));
  struct LONG : terminal<option(cr<"+-">) + IntegerBody + EndToken> {};
  struct BIGN : terminal<option(cr<"+-">) + IntegerBody + lit<"N"> + EndToken> {};

  struct RADIX : terminal<option(cr<"+-">) + cr<"1-9"> + option(d) + cr<"rR"> +
                          some(cr<"0-9a-zA-Z">) + EndToken> {};
  struct CHAR_OCT : terminal<lit<"\\o"> + ((cr<"0-3"> + repeat<2>(octald)) |
                                       repeat<1, 2>(octald)) + EndToken> {};
  struct CHAR_U : terminal<lit<"\\u"> +
                           !(cr<"dD"> + cr<"8-9a-fA-F">) +
                           repeat<4>(hexd) + EndToken> {};
  struct CHAR_NAMED
      : terminal<lit<"\\"> + (kw<"newline"> | kw<"return"> | kw<"space"> |
                              kw<"tab"> | kw<"formfeed"> | kw<"backspace">) + EndToken> {};
  struct CHAR_ANY : terminal<lit<"\\"> + any + EndToken> {};

  struct NIL : terminal<kw<"nil">> {};
  struct BOOLEAN : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  struct PARAM_NAME
      : terminal<ParamBody + EndToken> {};

  static constexpr auto not_eol = ~cr<R"(\r\n)">;
  struct COMMENT : terminal<(lit<";"> | lit<"#!">) + many(not_eol)> {};
  struct WS : terminal<some(Space)> {};

  struct StringLit;
  struct FloatLit;
  struct HexLit;
  struct BignLit;
  struct LongLit;
  struct CharLit;
  struct NilLit;
  struct BoolLit;
  struct Keyword;
  struct Symbol;
  struct ParamName;
  struct List;
  struct Vector;
  struct Map;
  struct Set;
  struct Lambda;
  struct MetaData;
  struct Regex;
  struct VarQuote;
  struct Eval;
  struct Tag;
  struct Discard;
  struct Dispatch;
  struct Deref;
  struct Quote;
  struct Backtick;
  struct Unquote;
  struct UnquoteSplicing;
  struct Conditional;
  struct RadixLit;

  static constexpr auto undiscarded_form =
      ref<RatioLit> | ref<DecimalLit> | ref<StringLit> |
      ref<FloatLit> | ref<HexLit> | ref<BignLit> |
      ref<LongLit> | ref<CharLit> | ref<NilLit> | ref<BoolLit> |
      ref<Keyword> | ref<Symbol> | ref<ParamName> | ref<List> |
      ref<Vector> | ref<Map> | ref<Set> | ref<Lambda> |
      ref<MetaData> | ref<Regex> | ref<VarQuote> |
      ref<Eval> | ref<Tag> | ref<Dispatch> | ref<Deref> |
      ref<Quote> | ref<Backtick> | ref<Unquote> |
      ref<UnquoteSplicing> | ref<Conditional> | ref<RadixLit>;

  static constexpr auto form = undiscarded_form | ref<Discard>;
  template <auto Member>
  static constexpr auto OperandInto =
      many(append<&ast::Form::discards>(ref<Discard>)) +
      assign<Member>(undiscarded_form);

  struct StringLit : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  struct FloatLit
      : rule<assign<&ast::FloatLit::text>(ref<FLOAT> | ref<SYMBOLIC_FLOAT>)> {};
  struct HexLit : rule<assign<&ast::HexLit::text>(ref<HEX>)> {};
  struct RadixLit : rule<assign<&ast::RadixLit::text>(ref<RADIX>)> {};
  struct BignLit : rule<assign<&ast::BignLit::text>(ref<BIGN>)> {};
  struct LongLit : rule<assign<&ast::LongLit::text>(ref<LONG>)> {};
  struct CharLit
      : rule<assign<&ast::CharLit::text>(ref<CHAR_NAMED> | ref<CHAR_U> |
                                         ref<CHAR_OCT> | ref<CHAR_ANY>)> {};
  struct NilLit : rule<create<ast::NilLit>(ref<NIL>)> {};
  struct BoolLit : rule<assign<&ast::BoolLit::value>(ref<BOOLEAN>)> {};

  struct Keyword
      : rule<lit<":"> + gap::none +
             option(flag<&ast::Keyword::macro>(lit<":">)) + gap::none +
             assign<&ast::Keyword::name>(ref<KEYWORD_NAME>)> {};
  struct Symbol : rule<assign<&ast::Symbol::text>(symbol_text)> {};
  struct ParamName : rule<assign<&ast::ParamName::text>(ref<PARAM_NAME>)> {
  };

  struct List
      : rule<lit<"("> + many(append<&ast::ListForm::items>(form)) + lit<")">> {
  };
  struct Vector
      : rule<lit<"["> + many(append<&ast::VectorForm::items>(form)) +
             lit<"]">> {};
  struct Set
      : rule<lit<"#{"> + many(append<&ast::SetForm::items>(form)) + lit<"}">> {
  };

  struct MapEntry
      : rule<assign<&ast::MapEntry::key>(undiscarded_form) +
             many(append<&ast::MapEntry::discards>(ref<Discard>)) +
             assign<&ast::MapEntry::value>(undiscarded_form)> {};
  struct Map
      : rule<option(lit<"#:"> + gap::none +
                    ((flag<&ast::MapForm::autoNamespace>(lit<":">) +
                      option(gap::none +
                             assign<&ast::MapForm::namespace_>(symbol_text))) |
                     assign<&ast::MapForm::namespace_>(symbol_text))) +
             lit<"{"> +
             many(append<&ast::Form::discards>(ref<Discard>) |
                  append<&ast::MapForm::entries>(ref<MapEntry>)) +
             lit<"}">> {};

  struct Quote : rule<lit<"'"> + OperandInto<&ast::QuoteForm::form>> {};
  struct Backtick : rule<lit<"`"> + OperandInto<&ast::BacktickForm::form>> {
  };
  struct UnquoteSplicing
      : rule<lit<"~@"> + OperandInto<&ast::UnquoteSplicingForm::form>> {};
  struct Unquote : rule<lit<"~"> + OperandInto<&ast::UnquoteForm::form>> {};
  struct Tag : rule<lit<"^"> + OperandInto<&ast::TagForm::tag> +
                        OperandInto<&ast::TagForm::form>> {};
  struct Deref : rule<lit<"@"> + OperandInto<&ast::DerefForm::form>> {};
  struct Lambda
      : rule<lit<"#("> + many(append<&ast::LambdaForm::items>(form)) +
             lit<")">> {};
  struct MetaData : rule<lit<"#^"> + OperandInto<&ast::MetaDataForm::meta> +
                             OperandInto<&ast::MetaDataForm::form>> {};
  struct Conditional
      : rule<lit<"#?"> +
             option(flag<&ast::ConditionalForm::splicing>(lit<"@">)) +
             OperandInto<&ast::ConditionalForm::form>> {};
  struct VarQuote
      : rule<lit<"#'"> + OperandInto<&ast::VarQuoteForm::target>> {};
  struct Eval : rule<lit<"#="> + OperandInto<&ast::EvalForm::form>> {};
  struct Discard : rule<lit<"#_"> + OperandInto<&ast::DiscardForm::form>> {
  };
  struct Dispatch
      : rule<lit<"#"> + assign<&ast::DispatchForm::tag>(ref<Symbol>) +
             OperandInto<&ast::DispatchForm::form>> {};
  struct RegexPattern : rule<create<ast::StringLit> +
                                 assign<&ast::StringLit::text>(ref<REGEX>)> {};
  struct Regex
      : rule<assign<&ast::RegexForm::pattern>(ref<RegexPattern>)> {};

  struct File : rule<many(append<&ast::File::forms>(form))> {};

  using entry = File;
  using hidden = list<WS, COMMENT>;
};

} // namespace clojure
