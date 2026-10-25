#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "inf/core/ast.hpp"

namespace inf {

inline constexpr auto section_name = [](std::string_view text) {
  return text.substr(1);
};
using SectionName = cursiv::text_conv<section_name>;

struct Inf : cursiv::GrammarSpec {
  static constexpr auto Space =
      cr<R"( \t\v\f\r)"
             "\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000">;
  static constexpr auto Newline = lit<"\n">;
  static constexpr auto RestOfLine = cr<R"(^\n\x00\x1a)">;
  struct WS : terminal<some(Space)> {};
  struct COMMENT : terminal<lit<";"> + many(RestOfLine)> {};
  struct EOL : terminal<Newline> {};
  struct STOP : terminal<cr<R"(\x00\x1a)"> + many(any)> {};

  static constexpr auto Continuation = lit<"\\"> + many(lit<"\\"> | Space) +
                                       option(lit<";"> + many(RestOfLine)) +
                                       (Newline | eof);
  static constexpr auto QuotedBody = lit<"\"\""> | cr<R"(^"\n\x00\x1a)">;
  static constexpr auto Quoted = lit<"\""> + many(QuotedBody) + lit<"\"">;
  static constexpr auto IncompleteQuoted = lit<"\""> + many(QuotedBody);
  static constexpr auto FirstChar = cr<R"(^,=;"\n\x00\x1a)">;
  static constexpr auto ValueChar = cr<R"(^,;"\n\x00\x1a)">;
  static constexpr auto FirstHeadChar =
      cr<R"(^,=;"[ \t\v\f\r\n\x00\x1a)"
             "\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000">;
  static constexpr auto FirstHead = Continuation | Quoted | FirstHeadChar;
  static constexpr auto FirstPart = Continuation | Quoted | FirstChar;
  static constexpr auto ValuePart = Continuation | Quoted | ValueChar;
  struct FIRST
      : terminal<(FirstHead + many(FirstPart) + option(IncompleteQuoted)) |
                 IncompleteQuoted> {};
  struct VALUE : terminal<(some(ValuePart) + option(IncompleteQuoted)) |
                          IncompleteQuoted> {};

  static constexpr auto InitialChar = cr<
      R"(^,=;"[ \t\v\f\r\n\x00\x1a)"
      "\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000\ufeff">;
  struct INITIAL : terminal<((Continuation | Quoted | InitialChar) +
                             many(FirstPart) + option(IncompleteQuoted)) |
                            IncompleteQuoted> {};
  template <class First>
  struct FirstItem : rule<assign<&ast::Item::raw>(ref<First>)> {};
  template <class First>
  struct KeyItem
      : rule<create<ast::Item> + option(assign<&ast::Item::raw>(ref<First>))> {
  };

  struct ValueItem
      : rule<create<ast::Item> + option(assign<&ast::Item::raw>(ref<VALUE>))> {
  };

  static constexpr auto Fields =
      append<&ast::Line::fields>(ref<ValueItem>) +
      many(lit<","> + append<&ast::Line::fields>(ref<ValueItem>));
  static constexpr auto line_end = some(ref<EOL>) | &ref<STOP> | eof;
  template <class First>
  struct LineRule
      : rule<((assign<&ast::Line::key>(ref<KeyItem<First>>) + lit<"="> +
               Fields) |
              (append<&ast::Line::fields>(ref<FirstItem<First>>) +
               many(lit<","> + append<&ast::Line::fields>(ref<ValueItem>))) |
              (append<&ast::Line::fields>(create<ast::Item>) +
               some(lit<","> + append<&ast::Line::fields>(ref<ValueItem>)))) +
             line_end> {};

  struct HEADER_OPEN
      : terminal<lit<"["> + many(cr<R"(^]\n\x00\x1a)">), SectionName> {};
  struct HEADER_CLOSE : terminal<lit<"]"> + many(RestOfLine)> {};
  struct SectionRule
      : rule<assign<&ast::Section::name>(ref<HEADER_OPEN>) +
             assign<&ast::Section::closing>(ref<HEADER_CLOSE>) + line_end +
             many(append<&ast::Section::lines>(ref<LineRule<FIRST>>))> {};

  struct DocRule
      : rule<create<ast::Document> +
             option(flag<&ast::Document::has_bom>(lit<"﻿">)) +
             many(ref<EOL>) +
             many(append<&ast::Document::preamble>(ref<LineRule<INITIAL>>)) +
             many(append<&ast::Document::sections>(ref<SectionRule>)) +
             option(assign<&ast::Document::trailer>(ref<STOP>))> {};

  using entry = DocRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace inf
