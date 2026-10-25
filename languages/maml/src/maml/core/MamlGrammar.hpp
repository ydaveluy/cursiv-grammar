// The maml language: its grammar.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "maml/core/ast.hpp"

namespace maml {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto unquote_raw = [](std::string_view t) {
  return t.substr(3, t.size() - 6);
};
using UnquotedRaw = cursiv::text_conv<unquote_raw>;

inline constexpr auto null_value = [](std::string_view) {
  return std::monostate{};
};
using NullValue = cursiv::text_conv<null_value>;

// A key is stored as its raw spelling regardless of which alternative
// matched -- a quoted key's delimiters are the only thing this strips, since
// none of the other spellings (identifier, digits, a keyword) ever start
// with one.
inline constexpr auto key_text = [](std::string_view t) -> std::string_view {
  if (!t.empty() && t.front() == '"') return t.substr(1, t.size() - 2);
  return t;
};
using KeyText = cursiv::text_conv<key_text>;

struct Maml : cursiv::GrammarSpec {
  // IDENTIFIER admits '-', past what the default word class covers: without
  // this, kw<"true"> would read only "true" out of "true-ish" and leave
  // "-ish" behind instead of losing to the longer identifier.
  static constexpr auto word_boundary = cr<"A-Za-z0-9_-">;

  struct WS : terminal<some(cr<R"( \t)">)> {};

  // The terminating newline is left for NL, which still acts as a separator.
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};

  struct NL : terminal<some(option(lit<"\r">) + lit<"\n">)> {};

  struct TRUE : terminal<kw<"true">, bool> {};
  struct FALSE : terminal<kw<"false">, bool> {};
  struct NULLK : terminal<kw<"null">, NullValue> {};

  // Raw strings are bounded by """ and preserve their contents literally, at
  // least one character of them -- empty raw strings are spelled "" instead.
  // Checked before STRING so the delimiter is never read as "" plus a quote.
  struct RAW_STRING
      : terminal<through(lit<"\"\"\""> + any, lit<"\"\"\"">), UnquotedRaw> {};

  static constexpr auto hexd = cr<"0-9a-fA-F">;
  static constexpr auto escape =
      lit<"\\"> +
      (cr<R"("\\tnr)"> | (lit<"u{"> + repeat<1, 6>(hexd) + lit<"}">));
  struct STRING
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\x00-\b\n-\x1f)">) +
                     lit<"\"">,
                 Unquoted> {};

  struct NUMBER
      : terminal<option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d))) +
                     option(lit<"."> + some(d)) +
                     option(cr<"eE"> + option(cr<"+-">) + some(d)),
                 double> {};

  // Identifier keys: only A-Z, a-z, 0-9, _ and -.
  struct IDENTIFIER : terminal<some(cr<"A-Za-z0-9_-">)> {};

  struct KeyRule
      : rule<ref<IDENTIFIER> | ref<STRING> | ref<NUMBER> | ref<TRUE> |
                 ref<FALSE> | ref<NULLK>,
             KeyText> {};

  // A comma optionally followed by newlines, or one or more newlines alone --
  // never both, and never two commas running.
  static constexpr auto separator =
      (lit<","> + many(ref<NL>)) | some(ref<NL>);

  struct ObjectRule;
  struct ArrayRule;
  static constexpr auto value =
      ref<ObjectRule> | ref<ArrayRule> | ref<STRING> | ref<RAW_STRING> |
      ref<NUMBER> | ref<TRUE> | ref<FALSE> | ref<NULLK>;

  struct MemberRule
      : rule<assign<&ast::Member::key>(ref<KeyRule>) + many(ref<NL>) +
                 lit<":"> + many(ref<NL>) +
                 assign<&ast::Member::value>(value)> {};

  struct ObjectRule
      : rule<lit<"{"> + many(ref<NL>) +
                 option(some(append<&ast::Object::members>(ref<MemberRule>),
                             separator) +
                        option(separator)) +
                 lit<"}">> {};

  struct ArrayRule
      : rule<lit<"["> + many(ref<NL>) +
                 option(some(append<&ast::Array::items>(value), separator) +
                        option(separator)) +
                 lit<"]">> {};

  struct DocRule
      : rule<many(ref<NL>) + assign<&ast::Doc::value>(value) +
                 many(ref<NL>)> {};

  using entry = DocRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace maml
