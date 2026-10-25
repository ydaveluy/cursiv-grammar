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
        (text[i + 1] == '\\' || text[i + 1] == '"'))
      c = text[++i];
    room[size++] = c;
  }
  return std::string_view(room.data(), size);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Guido : cursiv::GrammarSpec {
  static constexpr auto identifier = cr<"a-zA-Z_"> + many(cr<"a-zA-Z_0-9">);
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"(*"> +
                 many(ref<BLOCK_COMMENT> | !(lit<"(*"> | lit<"*)">)+any) +
                 lit<"*)">> {};
  struct NUMBER : terminal<some(d)> {};
  struct INTEGER : terminal<option(cr<"+-">) + some(d)> {};
  struct FLOAT : terminal<option(cr<"+-">) + many(d) + lit<"."> + some(d)> {};
  struct ID : terminal<identifier> {};
  struct VARIABLE : terminal<lit<"$"> + identifier> {};
  struct TAG_NAME
      : terminal<lit<"\\"> + identifier,
                 text_conv<[](std::string_view s) { return s.substr(1); }>> {};
  struct BAR : terminal<lit<"|">, text_conv<[](std::string_view) {
                          return std::string_view("bar");
                        }>> {};
  struct STRING
      : terminal<
            lit<"\""> + many(lit<"\\"> + any | !lit<"\""> + any) + lit<"\""> |
                lit<"'"> + many(lit<"\\"> + any | !lit<"'"> + any) + lit<"'">,
            Unquoted> {};
  struct NOTENAME
      : terminal<lit<"empty"> | lit<"cis"> | lit<"dis"> | lit<"fis"> |
                 lit<"gis"> | lit<"ais"> | lit<"do"> | lit<"re"> | lit<"mi"> |
                 lit<"fa"> | lit<"sol"> | lit<"la"> | lit<"si"> | lit<"ti"> |
                 cr<"a-h"> | lit<"_">> {};
  struct Sharp : terminal<lit<"#">, value<ast::Accidental::sharp>> {};
  struct Flat : terminal<lit<"&">, value<ast::Accidental::flat>> {};
  struct DOTS : terminal<some(lit<".">)> {};

  struct NumberRule : rule<create<ast::Number>(
                          option(flag<&ast::Number::negative>(lit<"-">) |
                                 flag<&ast::Number::positive>(lit<"+">)) +
                          assign<&ast::Number::value>(ref<NUMBER>))> {};

  struct DurationRule
      : rule<create<ast::Duration>(
            flag<&ast::Duration::asterisk>(lit<"*">) +
                option(assign<&ast::Duration::numerator>(ref<NumberRule>)) +
                option(lit<"/"> + assign<&ast::Duration::denominator>(
                                      ref<NumberRule>) |
                       flag<&ast::Duration::milliseconds>(lit<"ms">)) |
            lit<"/"> + assign<&ast::Duration::denominator>(ref<NumberRule>))> {
  };

  struct NoteRule
      : rule<create<ast::Note>(
            assign<&ast::Note::name>(ref<NOTENAME>) +
            option(lit<"<"> + assign<&ast::Note::annotation>(ref<NUMBER>) +
                   lit<">">) +
            many(append<&ast::Note::accidentals>(ref<Sharp> | ref<Flat>)) +
            option(assign<&ast::Note::octave>(ref<NumberRule>)) +
            option(assign<&ast::Note::duration>(ref<DurationRule>)) +
            option(assign<&ast::Note::dots>(ref<DOTS>)))> {};

  static constexpr auto parameter_value =
      (flag<&ast::Parameter::quoted>(
           assign<&ast::Parameter::text>(ref<STRING>)) |
       flag<&ast::Parameter::variable>(
           assign<&ast::Parameter::text>(ref<VARIABLE>)) |
       (flag<&ast::Parameter::real>(
            assign<&ast::Parameter::text>(ref<FLOAT>)) |
        assign<&ast::Parameter::text>(ref<INTEGER>)) +
           option(assign<&ast::Parameter::unit>(ref<ID>)) |
       flag<&ast::Parameter::bare>(
           assign<&ast::Parameter::text>(ref<ID>)));

  struct ParameterValue : rule<create<ast::Parameter>(parameter_value)> {};

  struct ParameterRule
      : rule<create<ast::Parameter>(
            option(assign<&ast::Parameter::name>(ref<ID>) + lit<"=">) +
            parameter_value)> {};

  struct VariableDeclRule
      : rule<create<ast::VariableDecl>(
            assign<&ast::VariableDecl::name>(ref<VARIABLE>) + lit<"="> +
            assign<&ast::VariableDecl::value>(ref<ParameterValue>) +
            lit<";">)> {};

  struct VariableRefRule
      : rule<create<ast::VariableRef>(
            assign<&ast::VariableRef::name>(ref<VARIABLE>))> {};

  struct TagRule;
  struct ChordRule;
  struct ItemRule : rule<ref<TagRule> | ref<NoteRule> | ref<ChordRule> |
                         ref<VariableRefRule>> {};

  struct TagRule
      : rule<create<ast::Tag>(
            (assign<&ast::Tag::name>(ref<TAG_NAME>) |
             flag<&ast::Tag::bar>(assign<&ast::Tag::name>(ref<BAR>))) +
            option(lit<":"> + assign<&ast::Tag::id>(ref<NUMBER>)) +
            option(lit<"<"> +
                   some(append<&ast::Tag::parameters>(ref<ParameterRule>),
                        lit<",">) +
                   lit<">">) +
            option(flag<&ast::Tag::ranged>(lit<"(">) +
                   many(append<&ast::Tag::notes>(ref<ItemRule>)) + lit<")">))> {
  };

  struct ChordMemberRule
      : rule<create<ast::ChordMember>(some(
            append<&ast::ChordMember::items>(ref<TagRule> | ref<NoteRule>)))> {
  };

  struct ChordRule
      : rule<create<ast::Chord>(
            lit<"{"> +
            some(append<&ast::Chord::members>(ref<ChordMemberRule>), lit<",">) +
            lit<"}">)> {};

  struct SequenceRule
      : rule<create<ast::Sequence>(
            lit<"["> + many(append<&ast::Sequence::items>(ref<ItemRule>)) +
            lit<"]">)> {};

  struct SegmentRule
      : rule<create<ast::Segment>(
            lit<"{"> +
            many(option(lit<",">) +
                 append<&ast::Segment::sequences>(ref<SequenceRule>)) +
            lit<"}">)> {};

  struct ProgRule
      : rule<create<ast::Prog>(
            many(append<&ast::Prog::variables>(ref<VariableDeclRule>)) +
            (some(append<&ast::Prog::segments>(ref<SegmentRule>)) |
             some(append<&ast::Prog::sequences>(ref<SequenceRule>),
                  lit<",">)))> {};

  using entry = ProgRule;
  using ignore = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};

} // namespace guido
