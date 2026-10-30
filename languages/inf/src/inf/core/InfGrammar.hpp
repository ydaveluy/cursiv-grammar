#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "inf/core/ast.hpp"

namespace inf {

inline constexpr auto section_name = [](std::string_view text) {
  return text.substr(1);
};

struct Inf : cursiv::GrammarSpec {
  static constexpr auto Space =
      cr<R"( \t\v\f)"
             "\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000">;
  static constexpr auto Newline = lit<"\r\n"> | lit<"\r"> | lit<"\n">;
  // Input is decoded UTF-8. Intersect valid scalars with the text alphabet
  // by complementing their exclusions; raw invalid-byte paths otherwise
  // meet both a UTF-8 refusal and the EOF refusal of Continuation.
  static constexpr auto TextChar = ~(
      cr<R"(,=;"%[ \t\v\f\r\n\x00\x1a)"
         "\u0085\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000\ufeff"> |
      ~cr<R"(\x00-\u{10ffff})">);
  static constexpr auto RestOfLine = TextChar | Space | cr<R"(,=;"%[)"> | lit<"\ufeff">;
  struct WS : terminal<some(Space)> {};
  struct COMMENT : terminal<lit<";"> + many(RestOfLine)> {};
  struct EOL : terminal<Newline> {};
  struct STOP : terminal<cr<R"(\x00\x1a)"> + many(any)> {};

  static constexpr auto Continuation = lit<"\\"> + many(lit<"\\"> | Space) +
                                       option(lit<";"> + many(RestOfLine)) +
                                       (Newline | eof);
  static constexpr auto QuotedBody = lit<"\"\""> | TextChar | Space |
      cr<R"(,=;%[)"> | lit<"\ufeff">;
  static constexpr auto Quoted = lit<"\""> + many(QuotedBody) + lit<"\"">;
  static constexpr auto IncompleteQuoted = lit<"\""> + many(QuotedBody);
  static constexpr auto FirstChar = TextChar | Space | cr<"%["> | lit<"\ufeff">;
  static constexpr auto ValueChar = FirstChar | lit<"=">;
  static constexpr auto FirstHeadChar = TextChar | lit<"%"> | lit<"\ufeff">;
  // A semicolon inside a string-key token is not a comment delimiter.
  static constexpr auto StringToken =
      lit<"%"> + many(TextChar | Space | cr<R"(,=;"[)"> | lit<"\ufeff">) + lit<"%">;
  static constexpr auto FirstHead = Continuation | Quoted | StringToken | FirstHeadChar;
  static constexpr auto FirstPart = Continuation | Quoted | StringToken | FirstChar;
  static constexpr auto ValuePart = Continuation | Quoted | StringToken | ValueChar;
  struct FIRST
      : terminal<(FirstHead + many(FirstPart) + option(IncompleteQuoted)) |
                 IncompleteQuoted> {};
  struct VALUE : terminal<(some(ValuePart) + option(IncompleteQuoted)) |
                          IncompleteQuoted> {};

  static constexpr auto InitialChar = TextChar | lit<"%">;
  struct INITIAL : terminal<((Continuation | Quoted | StringToken | InitialChar) +
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
      some(append<&ast::Line::fields>(ref<ValueItem>), lit<",">);
  static constexpr auto line_end = some(ref<EOL>) | &ref<STOP> | eof;
  template <class First>
  struct Line
      : rule<((assign<&ast::Line::key>(ref<KeyItem<First>>) + lit<"="> +
               Fields) |
              (append<&ast::Line::fields>(ref<FirstItem<First>>) +
               many(lit<","> + append<&ast::Line::fields>(ref<ValueItem>))) |
              (append<&ast::Line::fields>(create<ast::Item>) +
               some(lit<","> + append<&ast::Line::fields>(ref<ValueItem>)))) +
             line_end> {};

  struct HEADER_OPEN
      : terminal<as<section_name>(lit<"["> + many(cr<R"(^]\r\n\x00\x1a)">))> {};
  struct HEADER_CLOSE : terminal<lit<"]"> + many(RestOfLine)> {};
  struct Section
      : rule<assign<&ast::Section::name>(ref<HEADER_OPEN>) +
             assign<&ast::Section::closing>(ref<HEADER_CLOSE>) + line_end +
             many(append<&ast::Section::lines>(ref<Line<FIRST>>))> {};

  struct Doc
      : rule<create<ast::Document> +
             option(flag<&ast::Document::has_bom>(lit<"﻿">)) +
             many(ref<EOL>) +
             many(append<&ast::Document::preamble>(ref<Line<INITIAL>>)) +
             many(append<&ast::Document::sections>(ref<Section>)) +
             option(assign<&ast::Document::trailer>(ref<STOP>))> {};

  using entry = Doc;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace inf
