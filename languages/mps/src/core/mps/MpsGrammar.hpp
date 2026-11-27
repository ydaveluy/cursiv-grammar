#pragma once

#include <unordered_set>

#include "cursiv/grammar/GrammarSpec.hpp"
#include "cursiv/utils/CaseFold.hpp"
#include "mps/ast.hpp"

namespace mps {

template <std::size_t Pad>
inline constexpr auto fieldText = [](std::string_view text) {
  text.remove_prefix(Pad);
  const auto first = text.find_first_not_of(" \t");
  return first == text.npos ? text.substr(text.size()) :
      text.substr(first, text.find_last_not_of(" \t") - first + 1);
};

// An aligned free card can spell a different row or column in fixed format.
inline std::size_t fixedReferences(std::string_view input) {
  enum class Section { Other, Rows, Columns, Data, Bounds };
  Section section = Section::Other;
  std::unordered_set<std::string_view> rows, columns;
  const auto field = [](std::string_view line, std::size_t offset) {
    return offset < line.size() ? fieldText<0>(line.substr(offset, 8)) : std::string_view{};
  };
  while (!input.empty()) {
    const auto end = input.find_first_of("\r\n");
    const auto line = input.substr(0, end);
    input.remove_prefix(end == input.npos ? input.size() : end + 1);
    if (line.empty() || line.front() == '*') continue;
    if (line.front() != ' ') {
      const auto header = line.substr(0, line.find_first_of(" \t"));
      const auto is = [header](std::string_view key) { return cursiv::foldedEqual(header, key); };
      if (is("NAME")) {
        const auto name = line.find_first_not_of(' ', 4);
        if (name != line.npos && name < 14) return 0;
      }
      if (is("ROWS")) section = Section::Rows;
      else if (is("COLUMNS")) section = Section::Columns;
      else if (is("RHS") || is("RANGES")) section = Section::Data;
      else if (is("BOUNDS")) section = Section::Bounds;
      else section = Section::Other;
    } else if (section == Section::Rows) {
      if (auto row = field(line, 4); !row.empty()) rows.insert(row);
    } else if (section == Section::Columns || section == Section::Data) {
      const auto row = field(line, 14), other = field(line, 39);
      if (section == Section::Columns) {
        if (row == "'MARKER'") continue;
        if (auto column = field(line, 4); !column.empty()) columns.insert(column);
      }
      if ((!row.empty() && !rows.contains(row)) || (!other.empty() && !rows.contains(other)))
        return 0;
    } else if (section == Section::Bounds) {
      if (auto column = field(line, 14); !column.empty() && !columns.contains(column)) return 0;
    }
  }
  return 1;
}

template <bool Fixed>
constexpr auto cardFormat(auto fixed, auto free) {
  if constexpr (Fixed) return fixed;
  else return free;
}

struct Mps : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<R"(^ \t\r\n\f\v)">;
  static constexpr auto space = some(cr<R"( \t\f\v)">);
  struct EOL : terminal<eol> {};
  struct BlankLine : terminal<(option(space) + eol) | (space + eof)> {};
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
  static constexpr auto cell = cr<R"(^\t\f\v\r\n)">;
  static constexpr auto numberChar = cr<R"(0-9+.,DEdeINFNAnafin\x2d)">;
  static constexpr auto lineEnd = &(cr<R"(\r\n)"> | eof);
  static constexpr auto cardEnd = option(space) + lineEnd;
  static constexpr auto sign = option(cr<"+-">);
  static constexpr auto decimal = (some(d) + option(lit<"."> + many(d))) | (lit<"."> + some(d));
  static constexpr auto number = sign + ((decimal + option(cr<"DdEe"> + sign + some(d))) |
                                         ikw<"inf"> | ikw<"nan">);
  template <std::size_t Min>
  static constexpr auto numberField = &(repeat<0, 11>(lit<" ">) + number) +
                                      repeat<Min, 12>(lit<" "> | numberChar);
  static constexpr auto lastNumber = &(many(lit<" ">) + number + cardEnd) + numberField<1>;
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
  static constexpr auto nameField = repeat<8>(cell) + lit<"  ">;
  static constexpr auto hasName = &(repeat<0, 7>(lit<" ">) + word_boundary);
  static constexpr auto fixedCardBody = lit<"    "> + nameField + hasName + nameField +
      option(numberField<12> + lit<"   "> + hasName + nameField) + lastNumber + cardEnd;
  static constexpr auto fixedRowBody = lit<" "> + cr<"ELGN"> + lit<"  "> +
      repeat<1, 8>(cell) + option(space) + lineEnd;
  static constexpr auto fixedMarkerBody = lit<"    "> + repeat<8>(cell) + lit<"  'MARKER'"> +
      space + ref<MarkerKind> + option(space) + lineEnd;
  static constexpr auto boundSet = lit<" "> + nameField + hasName;
  static constexpr auto fixedBoundBody = lit<" "> +
      (((lit<"UP"> | lit<"LO"> | lit<"FX"> | lit<"LI"> | lit<"UI"> | lit<"SC"> | lit<"SI">) +
        boundSet + nameField + lastNumber) |
       ((lit<"FR"> | lit<"BV"> | lit<"MI"> | lit<"PL">) + boundSet + repeat<1, 8>(cell))) +
      cardEnd;
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
  template <bool Fixed>
  struct RowDecl
      : rule<!boundary +
             cardFormat<Fixed>(
                 assign<&ast::RowDecl::type>(ref<StartField<1, 1, fixedRowBody>>) +
                     assign<&ast::RowDecl::name>(ref<LastField<2, 8>>),
                 option(space) + assign<&ast::RowDecl::type>(ref<WORD>) +
                 space + assign<&ast::RowDecl::name>(ref<WORD>))> {};
  template <std::size_t Pad>
  struct FixedEntry
      : rule<assign<&ast::Entry::row>(ref<Field<Pad, 8>>) +
             assign<&ast::Entry::value>(ref<LastField<2, 12>>)> {};
  template <bool Fixed>
  struct ColumnCard
      : rule<!boundary +
             cardFormat<Fixed>(
                 assign<&ast::ColumnCard::name>(ref<StartField<4, 8, fixedCardBody>>) +
                     append<&ast::ColumnCard::entries>(ref<FixedEntry<2>>) +
                     option(append<&ast::ColumnCard::entries>(ref<FixedEntry<3>>)),
                 option(space) + assign<&ast::ColumnCard::name>(ref<WORD>) +
                     some(space + append<&ast::ColumnCard::entries>(ref<Entry>)))> {};
  template <bool Fixed>
  struct Marker
      : rule<!boundary +
             cardFormat<Fixed>(
                 assign<&ast::Marker::name>(ref<StartField<4, 8, fixedMarkerBody>>) +
                     lit<"  'MARKER'"> + space + assign<&ast::Marker::kind>(ref<MarkerKind>),
                 option(space) + assign<&ast::Marker::name>(ref<WORD>) + space +
                     lit<"'MARKER'"> + space + assign<&ast::Marker::kind>(ref<MarkerKind>))> {};
  template <std::size_t Pad>
  static constexpr auto fixedDataEntry =
      append<&ast::DataCard::fields>(ref<Field<Pad, 8>>) +
      append<&ast::DataCard::fields>(ref<LastField<2, 12>>);
  template <bool Fixed>
  struct DataCard
      : rule<!boundary +
             cardFormat<Fixed>(
                 append<&ast::DataCard::fields>(ref<StartField<4, 8, fixedCardBody>>) +
                     fixedDataEntry<2> + option(fixedDataEntry<3>),
                 option(space) + some(append<&ast::DataCard::fields>(ref<WORD>), space))> {};
  template <bool Fixed>
  struct BoundCard
      : rule<!boundary +
             cardFormat<Fixed>(
                 assign<&ast::BoundCard::key>(ref<StartField<1, 2, fixedBoundBody>>) +
                     append<&ast::BoundCard::fields>(ref<Field<1, 8>>) +
                     append<&ast::BoundCard::fields>(ref<LastField<2, 8>>) +
                     option(append<&ast::BoundCard::fields>(ref<LastField<2, 12>>)),
                 option(space) + assign<&ast::BoundCard::key>(ref<WORD>) +
                     many(space + append<&ast::BoundCard::fields>(ref<WORD>)))> {};
  template <bool Fixed>
  struct Quadratic
      : rule<(assign<&ast::QuadraticSection::kind>(ref<Header<"QUADOBJ">> | ref<Header<"QMATRIX">>) |
              (option(space) + assign<&ast::QuadraticSection::kind>(ikw<"QSECTION"> | ikw<"QCMATRIX">) +
               space + assign<&ast::QuadraticSection::row>(ref<WORD>))) + end +
             many(append<&ast::QuadraticSection::cards>(ref<ColumnCard<Fixed>>) + end)> {};
  static constexpr auto objectiveSense =
      append<&ast::Model::objectiveSenses>(ikw<"MAX"> | ikw<"MIN">);
  template <bool Fixed>
  static constexpr auto cards =
      option(option(space) + ikw<"NAME"> +
             option(space + assign<&ast::Model::name>(ref<WORD>)) +
             cardFormat<Fixed>(end, option(space + flag<&ast::Model::free>(ikw<"FREE">)) + end)) +
      many((option(space) + ikw<"OBJSENSE"> + option(space + objectiveSense) + end +
            many(option(space) + objectiveSense + end)) |
           (ref<Header<"ROWS">> + end +
            many(append<&ast::Model::rows>(ref<RowDecl<Fixed>>) + end)) |
           (ref<Header<"COLUMNS">> + end +
            many(append<&ast::Model::columns>(ref<Marker<Fixed>> | ref<ColumnCard<Fixed>>) + end)) |
           (ref<Header<"RHS">> + end +
            many(append<&ast::Model::rhs>(ref<DataCard<Fixed>>) + end)) |
           (ref<Header<"RANGES">> + end +
            many(append<&ast::Model::ranges>(ref<DataCard<Fixed>>) + end)) |
           (ref<Header<"BOUNDS">> + end +
            many(append<&ast::Model::bounds>(ref<BoundCard<Fixed>>) + end)) |
           append<&ast::Model::quadratic>(ref<Quadratic<Fixed>>));
  template <bool Fixed>
  struct Deck : rule<create<ast::Model> + cards<Fixed> + ref<Header<"ENDATA">> + option(end)> {};
  struct FixedReferences : terminal<&scan<fixedReferences> + boundary> {};
  static constexpr auto fixed = &ref<FixedReferences> + &ref<Deck<true>>;
  struct Model : rule<gaps + ((&ref<FixedReferences> + ref<Deck<true>>) |
                              (!fixed + ref<Deck<false>>))> {};
  using entry = Model;
};

}  // namespace mps
