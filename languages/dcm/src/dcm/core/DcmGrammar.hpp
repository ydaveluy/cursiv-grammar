// The dcm language: its grammar. A DCM file is line-oriented: '\n' is a real
// token throughout, not trivia, so blank lines and line boundaries are
// spelled with NEWLINE.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "dcm/core/ast.hpp"

namespace dcm {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Dcm : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\f)">)> {};

  // Comments are entire lines. Keeping the comment pattern inside line
  // boundaries prevents a fractional number such as .5 becoming trivia.
  static constexpr auto comment =
      cr<"*!."> + many(cr<R"(^\n)">) + (lit<"\n"> | eof);
  static constexpr auto blank_line = many(cr<R"( \t\r\f)">) +
                                     (comment | lit<"\n">);
  struct INITIAL_COMMENT : terminal<comment> {};
  struct NEWLINE : terminal<lit<"\n"> + many(blank_line)> {};
  static constexpr auto line_end = ref<NEWLINE> | eof;

  struct NAME : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_.[]">)> {};

  static constexpr auto escape = lit<"\\"> + cr<R"(btnfr"'\\)">;
  struct TEXT : terminal<as<unquote>(lit<"\""> + many(escape | cr<R"(^\\")">) +
                                     lit<"\"">)> {};

  struct INT
      : terminal<as<std::int64_t>(option(cr<"+-">) + some(d))> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT
      : terminal<as<double>(
            option(cr<"+-">) +
            ((some(d) + option(lit<"."> + many(d))) |
             (lit<"."> + some(d))) +
            option(exponent))> {};

  // Numeric values have the native floating-point domain; only array
  // dimensions use the checked integer conversion.
  static constexpr auto realzahl = ref<FLOAT>;

  struct VarAbh
      : rule<assign<&ast::VarAbh::name>(ref<NAME>) + lit<"="> +
             assign<&ast::VarAbh::value>(ref<NAME>)> {};

  struct KgrInfo
      : rule<create<ast::KgrInfo> +
             option(kw<"LANGNAME"> +
                    assign<&ast::KgrInfo::langname>(ref<TEXT>) +
                    line_end) +
             option(kw<"DISPLAYNAME"> +
                    assign<&ast::KgrInfo::displayName>(ref<NAME> |
                                                        ref<TEXT>) +
                    line_end) +
             option(kw<"VAR"> +
                    some(append<&ast::KgrInfo::dependencies>(ref<VarAbh>),
                         lit<",">) +
                    line_end) +
             option(kw<"FUNKTION"> +
                    some(append<&ast::KgrInfo::functions>(ref<NAME>)) +
                    line_end)> {};

  struct SupportPoints
      : rule<(lit<"ST/X"> +
              some(append<&ast::SupportPoints::numbers>(realzahl)) +
              line_end) |
             (lit<"ST_TX/X"> +
              some(append<&ast::SupportPoints::texts>(ref<TEXT>)) +
              line_end)> {};

  struct ValueRow
      : rule<kw<"WERT"> + some(append<&ast::ValueRow::values>(realzahl)) +
             line_end> {};

  struct ValueBlock
      : rule<(kw<"WERT"> +
              some(append<&ast::ValueBlock::numbers>(realzahl)) +
              line_end) |
             (kw<"TEXT"> +
              some(append<&ast::ValueBlock::texts>(ref<TEXT>)) +
              line_end)> {};

  struct MapRowNum
      : rule<lit<"ST/Y"> + assign<&ast::MapRow::y>(realzahl) + line_end +
             some(append<&ast::MapRow::rows>(ref<ValueRow>))> {};

  struct MapRowText
      : rule<lit<"ST_TX/Y"> + assign<&ast::MapRow::yText>(ref<TEXT>) +
             line_end +
             some(append<&ast::MapRow::rows>(ref<ValueRow>))> {};

  struct MapRowsWithoutAxis
      : rule<some(append<&ast::MapRow::rows>(ref<ValueRow>))> {};

  struct Kennwert
      : rule<kw<"FESTWERT"> + assign<&ast::Kennwert::name>(ref<NAME>) +
             line_end + assign<&ast::Kennwert::info>(ref<KgrInfo>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennwert::unit>(ref<TEXT>) + line_end) +
             ((kw<"WERT"> + assign<&ast::Kennwert::value>(realzahl) +
               line_end) |
              (kw<"TEXT"> + assign<&ast::Kennwert::text>(ref<TEXT>) +
               line_end)) +
             kw<"END"> + line_end> {};

  struct Kennwerteblock
      : rule<kw<"FESTWERTEBLOCK"> +
             assign<&ast::Kennwerteblock::name>(ref<NAME>) +
             assign<&ast::Kennwerteblock::count>(ref<INT>) +
             option(lit<"@"> + assign<&ast::Kennwerteblock::countY>(ref<INT>)) +
             line_end +
             assign<&ast::Kennwerteblock::info>(ref<KgrInfo>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennwerteblock::unit>(ref<TEXT>) +
                    line_end) +
             some(append<&ast::Kennwerteblock::blocks>(ref<ValueBlock>)) +
             kw<"END"> + line_end> {};


  struct Kennlinie
      : rule<assign<&ast::Kennlinie::kind>(
                 as<ast::CurveKind::Normal>(kw<"KENNLINIE">) | as<ast::CurveKind::Fixed>(kw<"FESTKENNLINIE">) |
                 as<ast::CurveKind::Group>(kw<"GRUPPENKENNLINIE">)) +
             assign<&ast::Kennlinie::name>(ref<NAME>) +
             assign<&ast::Kennlinie::count>(ref<INT>) + line_end +
             assign<&ast::Kennlinie::info>(ref<KgrInfo>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Kennlinie::unitX>(ref<TEXT>) +
                    line_end) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennlinie::unitW>(ref<TEXT>) +
                    line_end) +
             many(append<&ast::Kennlinie::supportX>(ref<SupportPoints>)) +
             some(append<&ast::Kennlinie::values>(ref<ValueRow>)) +
             kw<"END"> + line_end> {};


  struct Kennfeld
      : rule<assign<&ast::Kennfeld::kind>(
                 as<ast::CurveKind::Normal>(kw<"KENNFELD">) | as<ast::CurveKind::Fixed>(kw<"FESTKENNFELD">) | as<ast::CurveKind::Group>(kw<"GRUPPENKENNFELD">)) +
             assign<&ast::Kennfeld::name>(ref<NAME>) +
             assign<&ast::Kennfeld::countX>(ref<INT>) +
             assign<&ast::Kennfeld::countY>(ref<INT>) + line_end +
             assign<&ast::Kennfeld::info>(ref<KgrInfo>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Kennfeld::unitX>(ref<TEXT>) + line_end) +
             option(kw<"EINHEIT_Y"> +
                    assign<&ast::Kennfeld::unitY>(ref<TEXT>) + line_end) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennfeld::unitW>(ref<TEXT>) + line_end) +
             many(append<&ast::Kennfeld::supportX>(ref<SupportPoints>)) +
             (some(append<&ast::Kennfeld::rows>(ref<MapRowNum>)) |
              some(append<&ast::Kennfeld::rows>(ref<MapRowText>)) |
              append<&ast::Kennfeld::rows>(ref<MapRowsWithoutAxis>)) +
             kw<"END"> + line_end> {};

  struct Gruppenstuetzstellen
      : rule<kw<"STUETZSTELLENVERTEILUNG"> +
             assign<&ast::Gruppenstuetzstellen::name>(ref<NAME>) +
             assign<&ast::Gruppenstuetzstellen::count>(ref<INT>) +
             line_end +
             assign<&ast::Gruppenstuetzstellen::info>(ref<KgrInfo>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Gruppenstuetzstellen::unitX>(ref<TEXT>) +
                    line_end) +
             some(append<&ast::Gruppenstuetzstellen::supportX>(
                 ref<SupportPoints>)) +
             kw<"END"> + line_end> {};

  struct Kenntext
      : rule<kw<"TEXTSTRING"> + assign<&ast::Kenntext::name>(ref<NAME>) +
             line_end + assign<&ast::Kenntext::info>(ref<KgrInfo>) +
             kw<"TEXT"> + assign<&ast::Kenntext::text>(ref<TEXT>) +
             line_end + kw<"END"> + line_end> {};

  struct ModHeaderLine
      : rule<kw<"MODULKOPF"> + assign<&ast::ModHeaderLine::name>(ref<NAME>) +
             append<&ast::ModHeaderLine::values>(ref<TEXT>) +
             line_end +
             many(kw<"MODULKOPF"> +
                  append<&ast::ModHeaderLine::values>(ref<TEXT>) +
                  line_end)> {};

  struct FunctionEntry
      : rule<kw<"FKT"> + assign<&ast::FunctionEntry::name>(ref<NAME>) +
             assign<&ast::FunctionEntry::version>(ref<TEXT>) +
             assign<&ast::FunctionEntry::longName>(ref<TEXT>) +
             line_end> {};

  struct VariantCriterion
      : rule<kw<"KRITERIUM"> +
             assign<&ast::VariantCriterion::name>(ref<NAME>) +
             many(append<&ast::VariantCriterion::values>(ref<NAME>)) +
             line_end> {};

  struct Document
      : rule<create<ast::Document> +
             many(ref<NEWLINE> | ref<INITIAL_COMMENT>) +
             kw<"KONSERVIERUNG_FORMAT"> + lit<"2.0"> + line_end +
             option(some(append<&ast::Document::moduleHeader>(
                 ref<ModHeaderLine>))) +
             option(kw<"FUNKTIONEN"> + line_end +
                    some(append<&ast::Document::functions>(
                        ref<FunctionEntry>)) +
                    kw<"END"> + line_end) +
             option(kw<"VARIANTENKODIERUNG"> + line_end +
                    some(append<&ast::Document::variants>(
                        ref<VariantCriterion>)) +
                    kw<"END"> + line_end) +
             many(append<&ast::Document::items>(
                 ref<Kennwert> | ref<Kennwerteblock> |
                 ref<Kennlinie> | ref<Kennfeld> |
                 ref<Gruppenstuetzstellen> | ref<Kenntext>))> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace dcm
