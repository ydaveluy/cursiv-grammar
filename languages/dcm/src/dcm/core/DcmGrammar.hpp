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
using Unquoted = cursiv::text_conv<unquote>;

struct Dcm : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\f)">)> {};

  struct NEWLINE : terminal<lit<"\n">> {};

  struct COMMENT : terminal<cr<"*!."> + many(cr<R"(^\n)">) + lit<"\n">> {};

  struct NAME : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_.[]">)> {};

  static constexpr auto escape = lit<"\\"> + cr<R"(btnfr"'\\)">;
  struct TEXT
      : terminal<lit<"\""> + many(escape | cr<R"(^\\")">) + lit<"\"">,
                 Unquoted> {};

  struct INT
      : terminal<option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d))),
                 std::int64_t> {};

  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT
      : terminal<option(lit<"-">) +
                     ((some(d) + lit<"."> + many(d) + option(exponent)) |
                      (lit<"."> + some(d) + option(exponent)) |
                      (some(d) + exponent)),
                 double> {};

  static constexpr auto realzahl = ref<INT> | ref<FLOAT>;

  struct VarAbhRule
      : rule<assign<&ast::VarAbh::name>(ref<NAME>) + lit<"="> +
             assign<&ast::VarAbh::value>(ref<NAME>)> {};

  struct KgrInfoRule
      : rule<create<ast::KgrInfo> +
             option(kw<"LANGNAME"> +
                    assign<&ast::KgrInfo::langname>(ref<TEXT>) +
                    ref<NEWLINE>) +
             option(kw<"DISPLAYNAME"> +
                    assign<&ast::KgrInfo::displayName>(ref<NAME> |
                                                        ref<TEXT>) +
                    ref<NEWLINE>) +
             option(kw<"VAR"> +
                    some(append<&ast::KgrInfo::dependencies>(ref<VarAbhRule>),
                         lit<",">) +
                    ref<NEWLINE>) +
             option(kw<"FUNKTION"> +
                    some(append<&ast::KgrInfo::functions>(ref<NAME>)) +
                    ref<NEWLINE>)> {};

  struct SupportPointsRule
      : rule<(lit<"ST/X"> +
              some(append<&ast::SupportPoints::numbers>(realzahl)) +
              ref<NEWLINE>) |
             (lit<"ST_TX/X"> +
              some(append<&ast::SupportPoints::texts>(ref<TEXT>)) +
              ref<NEWLINE>)> {};

  struct ValueRowRule
      : rule<kw<"WERT"> + some(append<&ast::ValueRow::values>(realzahl)) +
             ref<NEWLINE>> {};

  struct ValueBlockRule
      : rule<(kw<"WERT"> +
              some(append<&ast::ValueBlock::numbers>(realzahl)) +
              ref<NEWLINE>) |
             (kw<"TEXT"> +
              some(append<&ast::ValueBlock::texts>(ref<TEXT>)) +
              ref<NEWLINE>)> {};

  struct MapRowNumRule
      : rule<lit<"ST/Y"> + assign<&ast::MapRow::y>(realzahl) + ref<NEWLINE> +
             some(append<&ast::MapRow::rows>(ref<ValueRowRule>))> {};

  struct MapRowTextRule
      : rule<lit<"ST_TX/Y"> + assign<&ast::MapRow::yText>(ref<TEXT>) +
             ref<NEWLINE> +
             some(append<&ast::MapRow::rows>(ref<ValueRowRule>))> {};

  struct KennwertRule
      : rule<kw<"FESTWERT"> + assign<&ast::Kennwert::name>(ref<NAME>) +
             ref<NEWLINE> + assign<&ast::Kennwert::info>(ref<KgrInfoRule>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennwert::unit>(ref<TEXT>) + ref<NEWLINE>) +
             ((kw<"WERT"> + assign<&ast::Kennwert::value>(realzahl) +
               ref<NEWLINE>) |
              (kw<"TEXT"> + assign<&ast::Kennwert::text>(ref<TEXT>) +
               ref<NEWLINE>)) +
             kw<"END"> + some(ref<NEWLINE>)> {};

  struct KennwerteblockRule
      : rule<kw<"FESTWERTEBLOCK"> +
             assign<&ast::Kennwerteblock::name>(ref<NAME>) +
             assign<&ast::Kennwerteblock::count>(ref<INT>) + ref<NEWLINE> +
             assign<&ast::Kennwerteblock::info>(ref<KgrInfoRule>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennwerteblock::unit>(ref<TEXT>) +
                    ref<NEWLINE>) +
             some(append<&ast::Kennwerteblock::blocks>(ref<ValueBlockRule>)) +
             kw<"END"> + some(ref<NEWLINE>)> {};

  struct CurveKindNormal : rule<kw<"KENNLINIE">, value<ast::CurveKind::Normal>> {};
  struct CurveKindFixed : rule<kw<"FESTKENNLINIE">, value<ast::CurveKind::Fixed>> {};
  struct CurveKindGroup : rule<kw<"GRUPPENKENNLINIE">, value<ast::CurveKind::Group>> {};

  struct KennlinieRule
      : rule<assign<&ast::Kennlinie::kind>(
                 ref<CurveKindNormal> | ref<CurveKindFixed> |
                 ref<CurveKindGroup>) +
             assign<&ast::Kennlinie::name>(ref<NAME>) +
             assign<&ast::Kennlinie::count>(ref<INT>) + ref<NEWLINE> +
             assign<&ast::Kennlinie::info>(ref<KgrInfoRule>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Kennlinie::unitX>(ref<TEXT>) +
                    ref<NEWLINE>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennlinie::unitW>(ref<TEXT>) +
                    ref<NEWLINE>) +
             some(append<&ast::Kennlinie::supportX>(ref<SupportPointsRule>)) +
             some(append<&ast::Kennlinie::values>(ref<ValueRowRule>)) +
             kw<"END"> + some(ref<NEWLINE>)> {};

  struct MapKindNormal : rule<kw<"KENNFELD">, value<ast::CurveKind::Normal>> {};
  struct MapKindFixed : rule<kw<"FESTKENNFELD">, value<ast::CurveKind::Fixed>> {};
  struct MapKindGroup : rule<kw<"GRUPPENKENNFELD">, value<ast::CurveKind::Group>> {};

  struct KennfeldRule
      : rule<assign<&ast::Kennfeld::kind>(
                 ref<MapKindNormal> | ref<MapKindFixed> | ref<MapKindGroup>) +
             assign<&ast::Kennfeld::name>(ref<NAME>) +
             assign<&ast::Kennfeld::countX>(ref<INT>) +
             assign<&ast::Kennfeld::countY>(ref<INT>) + ref<NEWLINE> +
             assign<&ast::Kennfeld::info>(ref<KgrInfoRule>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Kennfeld::unitX>(ref<TEXT>) + ref<NEWLINE>) +
             option(kw<"EINHEIT_Y"> +
                    assign<&ast::Kennfeld::unitY>(ref<TEXT>) + ref<NEWLINE>) +
             option(kw<"EINHEIT_W"> +
                    assign<&ast::Kennfeld::unitW>(ref<TEXT>) + ref<NEWLINE>) +
             some(append<&ast::Kennfeld::supportX>(ref<SupportPointsRule>)) +
             (some(append<&ast::Kennfeld::rows>(ref<MapRowNumRule>)) |
              some(append<&ast::Kennfeld::rows>(ref<MapRowTextRule>))) +
             kw<"END"> + some(ref<NEWLINE>)> {};

  struct GruppenstuetzstellenRule
      : rule<kw<"STUETZSTELLENVERTEILUNG"> +
             assign<&ast::Gruppenstuetzstellen::name>(ref<NAME>) +
             assign<&ast::Gruppenstuetzstellen::count>(ref<INT>) +
             ref<NEWLINE> +
             assign<&ast::Gruppenstuetzstellen::info>(ref<KgrInfoRule>) +
             option(kw<"EINHEIT_X"> +
                    assign<&ast::Gruppenstuetzstellen::unitX>(ref<TEXT>) +
                    ref<NEWLINE>) +
             some(append<&ast::Gruppenstuetzstellen::supportX>(
                 ref<SupportPointsRule>)) +
             kw<"END"> + some(ref<NEWLINE>)> {};

  struct KenntextRule
      : rule<kw<"TEXTSTRING"> + assign<&ast::Kenntext::name>(ref<NAME>) +
             ref<NEWLINE> + assign<&ast::Kenntext::info>(ref<KgrInfoRule>) +
             kw<"TEXT"> + assign<&ast::Kenntext::text>(ref<TEXT>) +
             ref<NEWLINE> + kw<"END"> + some(ref<NEWLINE>)> {};

  struct ModHeaderLineRule
      : rule<kw<"MODULKOPF"> + assign<&ast::ModHeaderLine::name>(ref<NAME>) +
             append<&ast::ModHeaderLine::values>(ref<TEXT>) +
             some(ref<NEWLINE>) +
             many(kw<"MODULKOPF"> +
                  append<&ast::ModHeaderLine::values>(ref<TEXT>) +
                  some(ref<NEWLINE>))> {};

  struct FunctionEntryRule
      : rule<kw<"FKT"> + assign<&ast::FunctionEntry::name>(ref<NAME>) +
             assign<&ast::FunctionEntry::version>(ref<TEXT>) +
             assign<&ast::FunctionEntry::longName>(ref<TEXT>) +
             some(ref<NEWLINE>)> {};

  struct VariantCriterionRule
      : rule<kw<"KRITERIUM"> +
             assign<&ast::VariantCriterion::name>(ref<NAME>) +
             many(append<&ast::VariantCriterion::values>(ref<NAME>)) +
             some(ref<NEWLINE>)> {};

  struct DocumentRule
      : rule<create<ast::Document> + many(ref<NEWLINE>) +
             lit<"KONSERVIERUNG_FORMAT 2.0"> + some(ref<NEWLINE>) +
             option(some(append<&ast::Document::moduleHeader>(
                 ref<ModHeaderLineRule>))) +
             option(kw<"FUNKTIONEN"> + ref<NEWLINE> +
                    some(append<&ast::Document::functions>(
                        ref<FunctionEntryRule>)) +
                    kw<"END"> + some(ref<NEWLINE>)) +
             option(kw<"VARIANTENKODIERUNG"> + ref<NEWLINE> +
                    some(append<&ast::Document::variants>(
                        ref<VariantCriterionRule>)) +
                    kw<"END"> + some(ref<NEWLINE>)) +
             many(append<&ast::Document::items>(
                 ref<KennwertRule> | ref<KennwerteblockRule> |
                 ref<KennlinieRule> | ref<KennfeldRule> |
                 ref<GruppenstuetzstellenRule> | ref<KenntextRule>))> {};

  using entry = DocumentRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace dcm
