#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "guido/core/ast.hpp"
#include <string_view>

namespace guido {

inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource *memory) {
  text = text.substr(1, text.size() - 2);
  if (text.find('\\') == text.npos)
    return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    if (c == '\\' && i + 1 < text.size() &&
        (text[i + 1] == '\\' || text[i + 1] == '"' || text[i + 1] == ':'))
      c = text[++i];
    room[size++] = c;
  }
  return std::string_view(room.data(), size);
};

struct Guido : cursiv::GrammarSpec {
  static constexpr auto identifier = cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">);
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"(*"> +
                 many(ref<BLOCK_COMMENT> | (!(lit<"(*"> | lit<"*)">)+any)) +
                 lit<"*)">> {};
  struct NUMBER : terminal<some(d)> {};
  struct INTEGER : terminal<option(cr<"+-">) + some(d)> {};
  struct FLOAT : terminal<option(cr<"+-">) + many(d) + lit<"."> + some(d)> {};
  struct ID : terminal<identifier> {};
  struct VARIABLE : terminal<lit<"$"> + identifier> {};
  struct TAG_NAME : terminal<as<[](std::string_view s) { return s.substr(1); }>(
                        lit<"\\"> + identifier)> {};
  struct BAR
      : terminal<as<[](std::string_view) { return std::string_view("bar"); }>(
            lit<"|">)> {};
  struct STRING
      : terminal<as<unquote>(
            (lit<"\""> + many((lit<"\\"> + cr<R"(^\n)">) | cr<R"(^"\\)">) +
             lit<"\"">) |
            (lit<"'"> + many((lit<"\\"> + cr<R"(^\n)">) | cr<R"(^'\\)">) +
             lit<"'">))> {};
  struct NOTENAME
      : terminal<lit<"empty"> | lit<"cis"> | lit<"dis"> | lit<"fis"> |
                 lit<"gis"> | lit<"ais"> | lit<"do"> | lit<"re"> | lit<"mi"> |
                 lit<"fa"> | lit<"sol"> | lit<"la"> | lit<"si"> | lit<"ti"> |
                 cr<"a-h"> | lit<"_">> {};
  struct DOTS : terminal<some(lit<".">)> {};
  struct TAB : terminal<lit<"s"> + cr<"1-6">> {};
  struct FRETS : terminal<as<unquote>(
      lit<":"> + many(lit<"\\:"> | cr<"^:">) + lit<":">)> {};

  struct Number : rule<create<ast::Number>(
                          option(flag<&ast::Number::negative>(lit<"-">) |
                                 flag<&ast::Number::positive>(lit<"+">)) +
                          assign<&ast::Number::value>(ref<NUMBER>))> {};

  struct Duration
      : rule<create<ast::Duration>(
            (flag<&ast::Duration::asterisk>(lit<"*">) +
                option(assign<&ast::Duration::numerator>(ref<Number>)) +
                option((lit<"/"> + assign<&ast::Duration::denominator>(
                                      ref<Number>)) |
                       flag<&ast::Duration::milliseconds>(lit<"ms">))) |
            (lit<"/"> + assign<&ast::Duration::denominator>(ref<Number>)))> {
  };

  struct Note
      : rule<create<ast::Note>(
            assign<&ast::Note::name>(ref<NOTENAME>) +
            option(lit<"<"> + assign<&ast::Note::annotation>(ref<NUMBER>) +
                   lit<">">) +
            many(append<&ast::Note::accidentals>(as<ast::Accidental::sharp>(lit<"#">) | as<ast::Accidental::flat>(lit<"&">))) +
            option(assign<&ast::Note::octave>(ref<Number>)) +
            option(assign<&ast::Note::duration>(ref<Duration>)) +
            option(assign<&ast::Note::dots>(ref<DOTS>)))> {};

  struct Tablature : rule<create<ast::Tablature>(
      assign<&ast::Tablature::string>(ref<TAB>) +
      assign<&ast::Tablature::frets>(ref<FRETS>) +
      option(assign<&ast::Tablature::duration>(ref<Duration>)) +
      option(assign<&ast::Tablature::dots>(ref<DOTS>)))> {};

  static constexpr auto quoted_parameter = flag<&ast::Parameter::quoted>(
      assign<&ast::Parameter::text>(ref<STRING>));
  static constexpr auto numeric_parameter =
      flag<&ast::Parameter::real>(
          assign<&ast::Parameter::text>(ref<FLOAT>)) |
      assign<&ast::Parameter::text>(ref<INTEGER>);
  // In native PARAM, UNIT/MLS/SEC win their equal-length match over IDT.
  static constexpr auto ParameterName =
      !(kw<"m"> | kw<"cm"> | kw<"mm"> | kw<"in"> | kw<"pt"> | kw<"pc"> |
        kw<"hs"> | kw<"rl"> | kw<"ms"> | kw<"s">) + ref<ID>;
  static constexpr auto parameter_value =
      (quoted_parameter |
       flag<&ast::Parameter::variable>(
           assign<&ast::Parameter::text>(ref<VARIABLE>)) |
       (numeric_parameter +
           option(assign<&ast::Parameter::unit>(ref<ID>))) |
       flag<&ast::Parameter::bare>(
           assign<&ast::Parameter::text>(ParameterName)));

  struct ParameterValue
      : rule<create<ast::Parameter>(quoted_parameter | numeric_parameter)> {};

  struct Parameter
      : rule<create<ast::Parameter>(
            option(assign<&ast::Parameter::name>(ParameterName) + lit<"=">) +
            parameter_value)> {};

  struct VariableDecl
      : rule<create<ast::VariableDecl>(
            assign<&ast::VariableDecl::name>(ref<VARIABLE>) + lit<"="> +
            assign<&ast::VariableDecl::value>(ref<ParameterValue>) +
            lit<";">)> {};

  struct VariableRef
      : rule<assign<&ast::VariableRef::name>(ref<VARIABLE>)> {};

  struct Tag;
  struct Chord;
  struct Item : rule<ref<Tag> | ref<Note> | ref<Tablature> | ref<Chord> |
                         ref<VariableRef>> {};

  struct Tag
      : rule<create<ast::Tag>(
            ((assign<&ast::Tag::name>(ref<TAG_NAME>) +
              option(lit<":"> + assign<&ast::Tag::id>(ref<NUMBER>))) |
             flag<&ast::Tag::bar>(assign<&ast::Tag::name>(ref<BAR>))) +
            option(lit<"<"> +
                   some(append<&ast::Tag::parameters>(ref<Parameter>),
                        lit<",">) +
                   lit<">">) +
            option(flag<&ast::Tag::ranged>(lit<"(">) +
                   many(append<&ast::Tag::notes>(ref<Item>)) + lit<")">))> {
  };

  struct ChordMember
      : rule<create<ast::ChordMember>(some(
            append<&ast::ChordMember::items>(ref<Tag> | ref<Note> | ref<Tablature>)))> {
  };

  struct Chord
      : rule<create<ast::Chord>(
            lit<"{"> +
            some(append<&ast::Chord::members>(ref<ChordMember>), lit<",">) +
            lit<"}">)> {};

  struct Sequence
      : rule<create<ast::Sequence>(
            lit<"["> + many(append<&ast::Sequence::items>(ref<Item>)) +
            lit<"]">)> {};

  struct Segment
      : rule<create<ast::Segment>(
            lit<"{"> +
            option(some(append<&ast::Segment::sequences>(ref<Sequence>),
                        lit<",">)) +
            lit<"}">)> {};

  struct Prog
      : rule<create<ast::Prog>(
            many(append<&ast::Prog::variables>(ref<VariableDecl>)) +
            (append<&ast::Prog::segments>(ref<Segment>) |
             append<&ast::Prog::sequences>(ref<Sequence>)))> {};

  using entry = Prog;
  using ignored = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};

} // namespace guido
