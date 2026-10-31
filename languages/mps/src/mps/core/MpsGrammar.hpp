#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mps/core/ast.hpp"

namespace mps {

template <std::size_t Pad>
inline constexpr auto fieldText = [](std::string_view text) {
  text.remove_prefix(Pad);
  const auto first = text.find_first_not_of(" \t");
  return first == text.npos ? text.substr(text.size()) :
      text.substr(first, text.find_last_not_of(" \t") - first + 1);
};

struct Mps : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<R"(^ \t\r\n\f\v)">;
  static constexpr auto space = some(cr<R"( \t\f\v)">);
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct BlankLine : terminal<many(cr<R"( \t\f\v)">) + (lit<"\r\n"> | cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<"*"> + many(cr<R"(^\r\n)">)> {};
  struct WORD : terminal<some(cr<R"(^ \t\r\n\f\v)">)> {};
  struct MarkerKind : terminal<as<[](std::string_view text) {
    return text.substr(1, text.size() - 2);
  }>(lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">)> {};
  template <cursiv::str Text>
  struct Header : terminal<as<[](std::string_view text) {
    const auto first = text.find_first_not_of(" \t\f\v");
    return text.substr(first, text.find_last_not_of(" \t\f\v") - first + 1);
  }>(option(space) + ikw<Text> + option(space) + &(cr<R"(\r\n)"> | eof))> {};
  static constexpr auto gaps = many(ref<BlankLine> | (ref<COMMENT> + (ref<EOL> | eof)));
  static constexpr auto cell = cr<R"(^\r\n)">;
  static constexpr auto numberCell = cr<R"(0-9+.,DEdeINFNAnafin\x2d \t)">;
  static constexpr auto lineEnd = &(cr<R"(\r\n)"> | eof);
  template <std::size_t Pad, std::size_t Width>
  struct Field : terminal<as<fieldText<Pad>>(repeat<Pad>(lit<" ">) +
                                             repeat<Width>(cell))> {};
  template <std::size_t Pad, std::size_t Width>
  struct LastField : terminal<as<fieldText<Pad>>(repeat<Pad>(lit<" ">) +
                                                 repeat<1, Width>(cell))> {};
  template <std::size_t Pad, std::size_t Width, auto Shape>
  struct StartField
      : terminal<as<fieldText<Pad>>(&Shape + repeat<Pad>(lit<" ">) +
                                    repeat<Width>(cell))> {};
  static constexpr auto fixedPair = repeat<8>(cell) + lit<"  "> + repeat<1, 12>(numberCell);
  static constexpr auto fixedCardBody = lit<"    "> + repeat<8>(cell) + lit<"  "> + fixedPair +
      option(lit<"   "> + fixedPair) + option(space) + lineEnd;
  static constexpr auto fixedRowBody = lit<" "> + cr<"ELGN"> + lit<"  "> +
      repeat<1, 8>(cell) + option(space) + lineEnd;
  static constexpr auto fixedMarkerBody = lit<"    "> + repeat<8>(cell) + lit<"  'MARKER'"> +
      space + ref<MarkerKind> + option(space) + lineEnd;
  static constexpr auto fixedBoundBody = lit<" "> + repeat<2>(cell) + lit<" "> +
      repeat<8>(cell) + lit<"  "> +
      ((repeat<8>(cell) + lit<"  "> + repeat<1, 12>(numberCell)) |
       repeat<1, 8>(cell)) + option(space) + lineEnd;
  static constexpr auto end = option(space) + (ref<EOL> | eof) + gaps;
  static constexpr auto boundary =
      (option(space) + (ikw<"NAME"> | ikw<"OBJSENSE"> | ikw<"QSECTION"> |
                        ikw<"QCMATRIX"> | ikw<"CSECTION">)) |
      ref<Header<"ROWS">> | ref<Header<"COLUMNS">> | ref<Header<"RHS">> |
      ref<Header<"RANGES">> | ref<Header<"BOUNDS">> | ref<Header<"QUADOBJ">> |
      ref<Header<"QMATRIX">> | ref<Header<"ENDATA">> | ref<Header<"SOS">> | ref<Header<"SETS">>;
  struct Entry
      : rule<assign<&ast::Entry::row>(ref<WORD>) +
             option(space + assign<&ast::Entry::value>(ref<WORD>))> {};
  struct RowDecl
      : rule<!boundary +
             ((assign<&ast::RowDecl::type>(ref<StartField<1, 1, fixedRowBody>>) +
               assign<&ast::RowDecl::name>(ref<LastField<2, 8>>)) |
              (option(space) + assign<&ast::RowDecl::type>(ref<WORD>) +
               space + assign<&ast::RowDecl::name>(ref<WORD>)))> {};
  template <std::size_t Pad>
  struct FixedEntry
      : rule<assign<&ast::Entry::row>(ref<Field<Pad, 8>>) +
             assign<&ast::Entry::value>(ref<LastField<2, 12>>)> {};
  struct ColumnCard
      : rule<!boundary +
             ((assign<&ast::ColumnCard::name>(ref<StartField<4, 8, fixedCardBody>>) +
               append<&ast::ColumnCard::entries>(ref<FixedEntry<2>>) +
               option(append<&ast::ColumnCard::entries>(ref<FixedEntry<3>>))) |
              (option(space) + assign<&ast::ColumnCard::name>(ref<WORD>) +
               some(space + append<&ast::ColumnCard::entries>(ref<Entry>))))> {};
  struct Marker
      : rule<!boundary +
             ((assign<&ast::Marker::name>(ref<StartField<4, 8, fixedMarkerBody>>) +
               lit<"  'MARKER'"> + space + assign<&ast::Marker::kind>(ref<MarkerKind>)) |
              (option(space) + assign<&ast::Marker::name>(ref<WORD>) + space +
               lit<"'MARKER'"> + space + assign<&ast::Marker::kind>(ref<MarkerKind>)))> {};
  template <std::size_t Pad>
  static constexpr auto fixedDataEntry =
      append<&ast::DataCard::fields>(ref<Field<Pad, 8>>) +
      append<&ast::DataCard::fields>(ref<LastField<2, 12>>);
  struct DataCard
      : rule<!boundary +
             ((append<&ast::DataCard::fields>(ref<StartField<4, 8, fixedCardBody>>) +
               fixedDataEntry<2> + option(fixedDataEntry<3>)) |
              (option(space) + some(append<&ast::DataCard::fields>(ref<WORD>), space)))> {};
  struct BoundCard
      : rule<!boundary +
             ((assign<&ast::BoundCard::key>(ref<StartField<1, 2, fixedBoundBody>>) +
               append<&ast::BoundCard::fields>(ref<Field<1, 8>>) +
               append<&ast::BoundCard::fields>(ref<LastField<2, 8>>) +
               option(append<&ast::BoundCard::fields>(ref<LastField<2, 12>>))) |
              (option(space) + assign<&ast::BoundCard::key>(ref<WORD>) +
               many(space + append<&ast::BoundCard::fields>(ref<WORD>))))> {};
  struct Quadratic
      : rule<(assign<&ast::QuadraticSection::kind>(ref<Header<"QUADOBJ">> | ref<Header<"QMATRIX">>) |
              (option(space) + assign<&ast::QuadraticSection::kind>(ikw<"QSECTION"> | ikw<"QCMATRIX">) +
               space + assign<&ast::QuadraticSection::row>(ref<WORD>))) + end +
             many(append<&ast::QuadraticSection::cards>(ref<ColumnCard>) + end)> {};
  static constexpr auto objectiveSense =
      append<&ast::Model::objectiveSenses>(ikw<"MAX"> | ikw<"MIN">);
  struct Model
      : rule<create<ast::Model> + gaps +
             option(option(space) + ikw<"NAME"> +
                    option(space + assign<&ast::Model::name>(ref<WORD>)) +
                    option(space + flag<&ast::Model::free>(ikw<"FREE">)) + end) +
             many((option(space) + ikw<"OBJSENSE"> + option(space + objectiveSense) + end +
                   many(option(space) + objectiveSense + end)) |
                  (ref<Header<"ROWS">> + end +
                   many(append<&ast::Model::rows>(ref<RowDecl>) + end)) |
                  (ref<Header<"COLUMNS">> + end +
                   many(append<&ast::Model::columns>(ref<Marker> | ref<ColumnCard>) + end)) |
                  (ref<Header<"RHS">> + end +
                   many(append<&ast::Model::rhs>(ref<DataCard>) + end)) |
                  (ref<Header<"RANGES">> + end +
                   many(append<&ast::Model::ranges>(ref<DataCard>) + end)) |
                  (ref<Header<"BOUNDS">> + end +
                   many(append<&ast::Model::bounds>(ref<BoundCard>) + end)) |
                  append<&ast::Model::quadratic>(ref<Quadratic>)) +
             ref<Header<"ENDATA">> + option(end)> {};
  using entry = Model;
};

}  // namespace mps
