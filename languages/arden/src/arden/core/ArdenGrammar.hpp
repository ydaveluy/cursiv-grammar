// The arden language: its grammar.
#pragma once

#include "arden/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace arden {

inline constexpr auto slot_text_value = [](std::string_view t) {
  std::string_view body = t.substr(1, t.size() - 3);  // ':' and trailing ";;" off
  while (!body.empty() && (body.front() == ' ' || body.front() == '\t' ||
                           body.front() == '\r' || body.front() == '\n'))
    body.remove_prefix(1);
  while (!body.empty() && (body.back() == ' ' || body.back() == '\t' ||
                           body.back() == '\r' || body.back() == '\n'))
    body.remove_suffix(1);
  return body;
};
using SlotTextConv = cursiv::text_conv<slot_text_value>;

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto unbrace = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unbraced = cursiv::text_conv<unbrace>;

struct Arden : cursiv::GrammarSpec {
  // ---- Trivia ----
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  // Discarded wherever it appears, e.g. "within the past 3 days".
  struct THE : terminal<ikw<"the">> {};

  // ---- Basic lexical vocabulary ----
  struct ID : terminal<!ikw<"the"> + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct NUMBER
      : terminal<(some(d) + option(lit<".", !lit<".">> + many(d)) + option(Exponent)) |
                     (lit<"."> + some(d) + option(Exponent)),
                 double> {};

  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">, Unquoted> {};
  // A raw single-quoted term: no escape, closes on the first `'`.
  struct TERM : terminal<lit<"'"> + many(cr<"^'">) + lit<"'">, Unquoted> {};

  static constexpr auto Digit2 = d + d;
  struct TIME_OF_DAY
      : terminal<Digit2 + lit<":"> + Digit2 +
                     option(lit<":"> + Digit2 + option(lit<"."> + some(d))) +
                     option(cr<"zZ"> | (cr<"+-"> + Digit2 + lit<":"> + Digit2))> {
  };
  struct ISO_DATE
      : terminal<Digit2 + Digit2 + lit<"-"> + Digit2 + lit<"-"> + Digit2> {};
  struct ISO_DATE_TIME
      : terminal<Digit2 + Digit2 + lit<"-"> + Digit2 + lit<"-"> + Digit2 +
                     cr<"tT"> + Digit2 + lit<":"> + Digit2 + lit<":"> + Digit2 +
                     option(lit<"."> + some(d)) +
                     option(cr<"zZ"> | (cr<"+-"> + Digit2 + lit<":"> + Digit2))> {
  };

  // "title:" then free text then ";;" as one terminal: `through` closes on the
  // first `;;`, so free text may never itself contain that pair.
  struct SlotText : terminal<through(lit<":">, lit<";;">), SlotTextConv> {};

  struct MlmId : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_.-">)> {};
  struct TwoCharCode
      : terminal<cr<"a-z"> + cr<"a-z"> + option(lit<"_"> + cr<"A-Z"> + cr<"A-Z">)> {
  };
  struct ValidationCode
      : terminal<ikw<"production"> | ikw<"research"> | ikw<"testing"> | ikw<"expired">> {
  };
  // Glued compounds ("data-driven", "mlm-self"): each full spelling is its
  // own alternative so the word-boundary guard lands only at the true end.
  struct TypeCode : terminal<ikw<"data-driven"> | ikw<"data_driven">> {};
  struct MlmSelf : terminal<ikw<"mlm-self"> | ikw<"mlm_self">> {};

  // The `{...}` of a data mapping: raw bytes, `\x` escapes one.
  struct DataMapping
      : terminal<lit<"{"> + many((lit<"\\"> + any) | cr<"^}">) + lit<"}">, Unbraced> {
  };

  // ==== Vocabularies (one terminal per keyword-choice family) ====

  struct OfReadFuncOp
      : terminal<ikw<"average"> | ikw<"avg"> | ikw<"count"> | ikw<"exists"> |
                     ikw<"exist"> | ikw<"sum"> | ikw<"median">> {};
  struct OfNoReadFuncOpSimple
      : terminal<ikw<"slope"> | ikw<"stddev"> | ikw<"variance"> | ikw<"interval"> |
                     ikw<"arccos"> | ikw<"arcsin"> | ikw<"arctan"> |
                     ikw<"cosine"> | ikw<"cos"> | ikw<"sine"> | ikw<"sin"> |
                     ikw<"tangent"> | ikw<"tan"> | ikw<"exp"> | ikw<"floor"> |
                     ikw<"int"> | ikw<"round"> | ikw<"ceiling"> |
                     ikw<"truncate"> | ikw<"log10"> | ikw<"log"> | ikw<"abs"> |
                     ikw<"sqrt"> | ikw<"string"> | ikw<"reverse"> |
                     ikw<"length"> | ikw<"clone"> | ikw<"applicability"> |
                     ikw<"defuzzified">> {};
  struct FromOfFuncOp
      : terminal<ikw<"minimum"> | ikw<"max"> | ikw<"maximum"> | ikw<"min"> |
                     ikw<"last"> | ikw<"first"> | ikw<"earliest"> | ikw<"latest">> {
  };
  struct IndexFromOfFuncOp
      : terminal<ikw<"minimum"> | ikw<"max"> | ikw<"maximum"> | ikw<"min"> |
                     ikw<"earliest"> | ikw<"latest">> {};
  struct DurationOp
      : terminal<ikw<"years"> | ikw<"year"> | ikw<"months"> | ikw<"month"> |
                     ikw<"weeks"> | ikw<"week"> | ikw<"days"> | ikw<"day"> |
                     ikw<"hours"> | ikw<"hour"> | ikw<"minutes"> | ikw<"minute"> |
                     ikw<"seconds"> | ikw<"second">> {};
  struct TimePart
      : terminal<ikw<"years"> | ikw<"year"> | ikw<"months"> | ikw<"month"> |
                     ikw<"days"> | ikw<"day"> | ikw<"hours"> | ikw<"hour"> |
                     ikw<"minutes"> | ikw<"minute"> | ikw<"seconds"> |
                     ikw<"second">> {};

  static constexpr auto any_istrue_text = [] { return std::string_view("any istrue"); };
  struct AnyIsTrue
      : rule<ikw<"any"> + option(ikw<"istrue">), cursiv::text_conv<any_istrue_text>> {};
  static constexpr auto all_aretrue_text =
      [] { return std::string_view("all aretrue"); };
  struct AllAreTrue
      : rule<ikw<"all"> + option(ikw<"aretrue">), cursiv::text_conv<all_aretrue_text>> {
  };
  static constexpr auto no_istrue_text = [] { return std::string_view("no istrue"); };
  struct NoIsTrue
      : rule<ikw<"no"> + option(ikw<"istrue">), cursiv::text_conv<no_istrue_text>> {};
  static constexpr auto percent_increase_text =
      [] { return std::string_view("percent increase"); };
  struct PercentIncrease
      : rule<option(ikw<"percent"> | lit<"%">) + ikw<"increase">,
             cursiv::text_conv<percent_increase_text>> {};
  static constexpr auto percent_decrease_text =
      [] { return std::string_view("percent decrease"); };
  struct PercentDecrease
      : rule<option(ikw<"percent"> | lit<"%">) + ikw<"decrease">,
             cursiv::text_conv<percent_decrease_text>> {};
  static constexpr auto time_of_day_text = [] { return std::string_view("time of day"); };
  struct TimeOfDayFn
      : rule<ikw<"time"> + option(ikw<"of"> + ikw<"day">),
             cursiv::text_conv<time_of_day_text>> {};
  static constexpr auto day_of_week_text = [] { return std::string_view("day of week"); };
  struct DayOfWeekFn
      : rule<ikw<"day"> + ikw<"of"> + ikw<"week">, cursiv::text_conv<day_of_week_text>> {
  };
  static constexpr auto attr_names_text =
      [] { return std::string_view("attribute names"); };
  struct AttrNamesOp
      : rule<ikw<"attribute"> + ikw<"names">, cursiv::text_conv<attr_names_text>> {};

  static constexpr auto OfNoReadFuncOp =
      ref<OfNoReadFuncOpSimple> | ref<AnyIsTrue> | ref<AllAreTrue> | ref<NoIsTrue> |
      ref<PercentIncrease> | ref<PercentDecrease> | ref<TimeOfDayFn> | ref<DayOfWeekFn>;

  // ---- Operator vocabularies for the comparison/function ladders ----

  static constexpr auto nearest_text = [] { return std::string_view("nearest"); };
  struct NearestOp : rule<ikw<"nearest">, cursiv::text_conv<nearest_text>> {};
  static constexpr auto at_least_text = [] { return std::string_view("at least"); };
  struct AtLeastOp : rule<ikw<"at"> + ikw<"least">, cursiv::text_conv<at_least_text>> {};
  static constexpr auto at_most_text = [] { return std::string_view("at most"); };
  struct AtMostOp : rule<ikw<"at"> + ikw<"most">, cursiv::text_conv<at_most_text>> {};

  struct POWER_OP : terminal<lit<"**">, cursiv::value<ast::BinOp::power>> {};

  static constexpr auto truth_value_type_text =
      [] { return std::string_view("truth value"); };
  struct TruthValueTypeOp
      : rule<ikw<"truth"> + ikw<"value">,
             cursiv::text_conv<truth_value_type_text>> {};

  struct BeforeRelOp
      : terminal<ikw<"before">, cursiv::value<ast::CompareOp::before_rel>> {};
  struct AfterRelOp : terminal<ikw<"after">, cursiv::value<ast::CompareOp::after_rel>> {};
  struct FromRelOp : terminal<ikw<"from">, cursiv::value<ast::CompareOp::from_rel>> {};

  struct TcPresent : terminal<ikw<"present">, cursiv::value<ast::TypeCheck::present>> {};
  struct TcNull : terminal<ikw<"null">, cursiv::value<ast::TypeCheck::is_null>> {};
  struct TcBoolean : terminal<ikw<"boolean">, cursiv::value<ast::TypeCheck::boolean_>> {};
  struct TcTruthValue
      : rule<ikw<"truth"> + ikw<"value">, cursiv::value<ast::TypeCheck::truth_value>> {
  };
  struct TcNumber : terminal<ikw<"number">, cursiv::value<ast::TypeCheck::number_>> {};
  struct TcString : terminal<ikw<"string">, cursiv::value<ast::TypeCheck::string_>> {};
  struct TcTimeOfDay
      : rule<ikw<"time"> + ikw<"of"> + ikw<"day">,
             cursiv::value<ast::TypeCheck::time_of_day>> {};
  struct TcTime : terminal<ikw<"time">, cursiv::value<ast::TypeCheck::time_>> {};
  struct TcDuration
      : terminal<ikw<"duration">, cursiv::value<ast::TypeCheck::duration_>> {};
  struct TcList : terminal<ikw<"list">, cursiv::value<ast::TypeCheck::list_>> {};
  struct TcObject : terminal<ikw<"object">, cursiv::value<ast::TypeCheck::object_>> {};
  struct TcFuzzy : terminal<ikw<"fuzzy">, cursiv::value<ast::TypeCheck::fuzzy_>> {};
  struct TcCrisp : terminal<ikw<"crisp">, cursiv::value<ast::TypeCheck::crisp_>> {};
  struct TcLinguisticVariable
      : rule<ikw<"linguistic"> + ikw<"variable">,
             cursiv::value<ast::TypeCheck::linguistic_variable>> {};
  static constexpr auto TypeCheckOp =
      ref<TcTruthValue> | ref<TcTimeOfDay> | ref<TcLinguisticVariable> |
      ref<TcPresent> | ref<TcNull> | ref<TcBoolean> | ref<TcNumber> | ref<TcString> |
      ref<TcTime> | ref<TcDuration> | ref<TcList> | ref<TcObject> | ref<TcFuzzy> |
      ref<TcCrisp>;

  struct CoLessOrEqual
      : rule<ikw<"less"> + ikw<"than"> + ikw<"or"> + ikw<"equal">,
             cursiv::value<ast::CompareOp::less_or_equal>> {};
  struct CoLessThan
      : rule<ikw<"less"> + ikw<"than">, cursiv::value<ast::CompareOp::less_than>> {};
  struct CoGreaterOrEqual
      : rule<ikw<"greater"> + ikw<"than"> + ikw<"or"> + ikw<"equal">,
             cursiv::value<ast::CompareOp::greater_or_equal>> {};
  struct CoGreaterThan
      : rule<ikw<"greater"> + ikw<"than">,
             cursiv::value<ast::CompareOp::greater_than>> {};
  struct CoIn : terminal<ikw<"in">, cursiv::value<ast::CompareOp::in_list>> {};
  static constexpr auto BinaryCompOp = ref<CoLessOrEqual> | ref<CoLessThan> |
                                       ref<CoGreaterOrEqual> | ref<CoGreaterThan> |
                                       ref<CoIn>;

  struct ReTo : terminal<ikw<"to">, cursiv::value<ast::RangeEdge::to>> {};
  struct RePreceding
      : terminal<ikw<"preceding">, cursiv::value<ast::RangeEdge::preceding>> {};
  struct ReFollowing
      : terminal<ikw<"following">, cursiv::value<ast::RangeEdge::following>> {};
  struct ReSurrounding
      : terminal<ikw<"surrounding">, cursiv::value<ast::RangeEdge::surrounding>> {};
  static constexpr auto RangeEdgeOp =
      ref<ReTo> | ref<RePreceding> | ref<ReFollowing> | ref<ReSurrounding>;

  struct OccEqual : terminal<ikw<"equal">, cursiv::value<ast::CompareOp::occurs_equal>> {
  };
  struct OccBefore
      : terminal<ikw<"before">, cursiv::value<ast::CompareOp::occurs_before>> {};
  struct OccAfter : terminal<ikw<"after">, cursiv::value<ast::CompareOp::occurs_after>> {
  };
  struct OccWithinPast
      : rule<ikw<"within"> + ikw<"past">,
             cursiv::value<ast::CompareOp::occurs_within_past>> {};
  struct OccWithinSameDay
      : rule<ikw<"within"> + ikw<"same"> + ikw<"day"> + ikw<"as">,
             cursiv::value<ast::CompareOp::occurs_within_sameday>> {};
  static constexpr auto OccurBinaryOp = ref<OccWithinSameDay> | ref<OccWithinPast> |
                                        ref<OccEqual> | ref<OccBefore> | ref<OccAfter>;
  struct OccAt : terminal<ikw<"at">, cursiv::value<ast::CompareOp::occurs_at>> {};

  struct CmpLe : terminal<lit<"<="> | ikw<"le">, cursiv::value<ast::CompareOp::le>> {};
  struct CmpGe : terminal<lit<">="> | ikw<"ge">, cursiv::value<ast::CompareOp::ge>> {};
  struct CmpNe : terminal<lit<"<>"> | ikw<"ne">, cursiv::value<ast::CompareOp::ne>> {};
  struct CmpEq : terminal<lit<"="> | ikw<"eq">, cursiv::value<ast::CompareOp::eq>> {};
  struct CmpLt : terminal<lit<"<"> | ikw<"lt">, cursiv::value<ast::CompareOp::lt>> {};
  struct CmpGt : terminal<lit<">"> | ikw<"gt">, cursiv::value<ast::CompareOp::gt>> {};
  static constexpr auto SimpleCompOp = ref<CmpLe> | ref<CmpGe> | ref<CmpNe> |
                                       ref<CmpEq> | ref<CmpLt> | ref<CmpGt>;

  static constexpr auto OccurKw = ikw<"occurred"> | ikw<"occurs"> | ikw<"occur">;
  static constexpr auto IsKw =
      ikw<"is"> | ikw<"are"> | ikw<"was"> | ikw<"were">;

  // ==== Expression ladder ====
  //
  // Arden Syntax 3.0 Annex A4 ("Operator Precedence and Associativity"):
  // groups run loosest to tightest; this ladder builds tightest first, so
  // each struct below is cited with its A4 group in loosest-to-tightest
  // reading order. A named operator's secondary argument may read a wider
  // expression than its A4 group; the ladder step itself keeps that
  // precedence.

  struct ExprList;
  struct ExprCommaList;
  struct ExprMerge;
  struct ExprSort;
  struct ExprAddList;
  struct ExprRemoveList;
  struct ExprWhere;
  struct ExprOrAnd;
  struct ExprNot;
  struct ExprComparison;
  struct ExprConcat;
  struct ExprFormatted;
  struct ExprStringPrefix;
  struct ExprLocalizedBare;
  struct ExprNegation;
  struct ExprAddSub;
  struct ExprMulDiv;
  struct ExprPower;
  struct ExprAtTime;
  struct ExprBefore;
  struct ExprAgo;
  struct ExprDurationMatches;
  struct ExprFind;
  struct ExprFunction;
  struct ExprFunctionCore;
  struct ExprSeqto;
  struct ExprCast;
  struct ExprFactor;
  struct ExprAtom;
  struct IdentifierOrObjectRef;
  struct ParenExpr;
  struct FuzzySetPhrase;
  struct NewObjectExprRule;
  struct CallExprRule;

  struct ParenExpr
      : rule<lit<"("> +
                 (ref<ExprList> | ref<FuzzySetPhrase> | create<ast::ExprList>) +
                 lit<")">> {};

  struct ExprAtom
      : rule<create<ast::IdentExpr>(assign<&ast::IdentExpr::name>(ref<ID>))
                 //
                 | assign<&ast::NumberLit::value>(ref<NUMBER>)
                 //
                 | (create<ast::NullLit> + ikw<"null">)
                 //
                 | (create<ast::ItExpr> + (ikw<"it"> | ikw<"they">))
                 //
                 | assign<&ast::StringLit::value>(ref<STRING>)
                 //
                 | create<ast::TimeLit>(assign<&ast::TimeLit::text>(
                       ikw<"now"> | ref<ISO_DATE_TIME> | ref<ISO_DATE> |
                       ikw<"eventtime"> | ikw<"triggertime"> | ikw<"currenttime"> |
                       ref<TIME_OF_DAY> | ikw<"today"> | ikw<"tomorrow"> |
                       ikw<"sunday"> | ikw<"monday"> | ikw<"tuesday"> |
                       ikw<"wednesday"> | ikw<"thursday"> | ikw<"friday"> |
                       ikw<"saturday">))
                 //
                 | create<ast::BoolLit>(
                       option(ikw<"truth"> + ikw<"value">) +
                       (flag<&ast::BoolLit::value>(ikw<"true">) | ikw<"false">))
                 //
                 | create<ast::TruthValueLit>(
                       ikw<"truth"> + ikw<"value"> +
                       assign<&ast::TruthValueLit::value>(ref<NUMBER>))
                 //
                 | ref<ParenExpr>> {};

  static constexpr auto PostfixOps =
      many(nest<ast::IndexExpr, &ast::IndexExpr::base>(
               lit<"["> + assign<&ast::IndexExpr::index>(ref<ExprList>) + lit<"]">)  //
           | nest<ast::FieldExpr, &ast::FieldExpr::base>(
                 lit<"."> + assign<&ast::FieldExpr::field>(ref<ID>)));

  // Tight atom+postfix chain for argument slots that are not part of the
  // general ladder (a CASE label, a FHIR path, READ's count): A4 does not
  // rank these, so they keep the pre-A4 merged `.`/`[...]` shape.
  struct ExprFactor : rule<ref<ExprAtom> + PostfixOps> {};

  // Returns Expr: the postfix steps wrap the IdentExpr the name builds.
  struct IdentifierOrObjectRef
      : rule<create<ast::IdentExpr>(assign<&ast::IdentExpr::name>(ref<ID>)) +
             PostfixOps> {};

  // A4 group "... as number/time/string/truth value" -- tightest of all.
  struct ExprCast
      : rule<create<ast::CastExpr>(
                 assign<&ast::CastExpr::value>(ref<ExprAtom>) + ikw<"as"> +
                 assign<&ast::CastExpr::asType>(
                     ikw<"number"> | ikw<"time"> | ikw<"string"> |
                     ref<TruthValueTypeOp>))
                 //
                 | ref<ExprAtom>> {};

  // A4 group "... seqto ...".
  struct ExprSeqto
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::operand>(ref<ExprCast>) +
                              assign<&ast::FuncApply::func>(ikw<"seqto">) +
                 assign<&ast::FuncApply::from>(ref<ExprCast>))
                 //
                 | ref<ExprCast>> {};

  // A4 group "count [of].../.../.../attribute .../clone ..." -- the named
  // functions, `attribute ... from`, `clone`, and `.` object access, one
  // right-associative group. A dozen named-operation shapes collapsed onto
  // FuncApply; its `func` selects the operation. A named operator's own
  // secondary argument (an amount, a replacement, a search start) widens to
  // ExprList, past what A4 ranks this group at: A4 ranks the operator
  // family, not each argument slot, and the corpus writes a signed number
  // (`REPLACE ... WITH -10`) and an indexed name (`attribute x[1] from y`)
  // in these slots with no parentheses.
  // `first x from l` is the function, never `(first x) from l`: without `of`
  // the bare `first x` and `index minimum x` refuse a following `from`. A
  // `using` after the function is its own, not the enclosing sort's.
  struct ExprFunctionCore
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::func>(OfNoReadFuncOp | ref<OfReadFuncOp>) +
                 option(ikw<"of">) +
                 assign<&ast::FuncApply::operand>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ref<FromOfFuncOp>) +
                       ((assign<&ast::FuncApply::operand>(ref<ExprList>) +
                         ikw<"from"> +
                         assign<&ast::FuncApply::from>(ref<ExprFunction>)) |
                        (ikw<"of"> +
                         assign<&ast::FuncApply::operand>(ref<ExprFunction>)) |
                        (assign<&ast::FuncApply::operand>(ref<ExprFunction>) +
                         !ikw<"from">)) +
                       (ikw<"using"> +
                            assign<&ast::FuncApply::extra>(ref<ExprFunction>) |
                        !ikw<"using">))
                 //
                 | create<ast::FuncApply>(
                   assign<&ast::FuncApply::func>(ikw<"replace">) + assign<&ast::FuncApply::part>(ref<TimePart>) +
                       option(ikw<"of">) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"with"> +
                       assign<&ast::FuncApply::extra>(ref<ExprList>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ikw<"extract">) +
                       assign<&ast::FuncApply::part>(
                           ref<TimePart> | ikw<"characters"> | ref<AttrNamesOp>) +
                       option(ikw<"of">) +
                       assign<&ast::FuncApply::operand>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ref<NearestOp>) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       flag<&ast::FuncApply::index>(ikw<"index">) +
                       assign<&ast::FuncApply::func>(ref<IndexFromOfFuncOp>) +
                       ((ikw<"of"> +
                         assign<&ast::FuncApply::from>(ref<ExprFunction>)) |
                        (assign<&ast::FuncApply::from>(ref<ExprFunction>) +
                         !ikw<"from">)))
                 //
                 | create<ast::FuncApply>(
                       flag<&ast::FuncApply::index>(ikw<"index">) +
                       assign<&ast::FuncApply::func>(ref<FromOfFuncOp>) +
                       option(ikw<"of">) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ref<AtLeastOp> | ref<AtMostOp>) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       option(ikw<"of">) +
                       option(option(flag<&ast::FuncApply::flag>(
                                  ikw<"istrue"> | ikw<"aretrue">)) +
                              ikw<"from">) +
                       assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       flag<&ast::FuncApply::index>(ikw<"index">) +
                       option(ikw<"of">) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       flag<&ast::FuncApply::index>(ikw<"index">) +
                       assign<&ast::FuncApply::func>(ref<NearestOp>) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ikw<"attribute">) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(ikw<"sublist">) +
                       assign<&ast::FuncApply::operand>(ref<ExprList>) +
                       ikw<"elements"> +
                       option(ikw<"starting"> + ikw<"at"> +
                              assign<&ast::FuncApply::extra>(ref<ExprList>)) +
                       ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprFunction>))
                 //
                 | ref<ExprSeqto>> {};

  struct ExprFunction
      : rule<ref<ExprFunctionCore> +
                 many(nest<ast::FieldExpr, &ast::FieldExpr::base>(
                     lit<"."> + assign<&ast::FieldExpr::field>(ref<ID>)))> {};

  // A4 group "find … [in] … [starting at …]".
  struct ExprFind
      : rule<create<ast::FindInString>(
                 ikw<"find"> +
                 assign<&ast::FindInString::needle>(ref<ExprStringPrefix>) +
                 option(ikw<"in">) + ikw<"string"> +
                 assign<&ast::FindInString::haystack>(ref<ExprStringPrefix>) +
                 option(ikw<"starting"> + ikw<"at"> +
                        assign<&ast::FindInString::startingAt>(ref<ExprStringPrefix>)))
                 //
                 | ref<ExprFunction>> {};

  // A4 group "... year/.../second ... matches pattern ...".
  struct ExprDurationMatches
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::operand>(ref<ExprFind>) +
                 assign<&ast::FuncApply::func>(ref<DurationOp>))
                 //
                 | create<ast::MatchesPattern>(
                       assign<&ast::MatchesPattern::value>(ref<ExprStringPrefix>) +
                       ikw<"matches"> + ikw<"pattern"> +
                       assign<&ast::MatchesPattern::pattern>(ref<ExprStringPrefix>))
                 //
                 | ref<ExprFind>> {};

  // A4 group "... ago".
  struct ExprAgo
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::operand>(ref<ExprDurationMatches>) +
                 assign<&ast::FuncApply::func>(ikw<"ago">))
                 //
                 | ref<ExprDurationMatches>> {};

  // A4 group "... before ... / ... after ... º from ...".
  struct ExprBefore
      : rule<create<ast::BinaryComparison>(
                 assign<&ast::BinaryComparison::left>(ref<ExprAgo>) +
                 assign<&ast::BinaryComparison::op>(
                     ref<BeforeRelOp> | ref<AfterRelOp> | ref<FromRelOp>) +
                 assign<&ast::BinaryComparison::right>(ref<ExprAgo>))
                 //
                 | ref<ExprAgo>> {};

  struct ExprAtTime
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::operand>(ref<ExprBefore>) +
                 assign<&ast::FuncApply::func>(ikw<"attime">) +
                 assign<&ast::FuncApply::from>(ref<ExprAtTime>))
                 //
                 | ref<ExprBefore>> {};

  struct ExprPower
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ExprAtTime>) +
                 assign<&ast::BinaryExpr::op>(ref<POWER_OP>) +
                 assign<&ast::BinaryExpr::right>(ref<ExprAtTime>))
                 //
                 | ref<ExprAtTime>> {};

  struct MUL_OP : terminal<lit<"*">, cursiv::value<ast::BinOp::multiply>> {};
  struct DIV_OP : terminal<lit<"/">, cursiv::value<ast::BinOp::divide>> {};
  struct ADD_OP : terminal<lit<"+">, cursiv::value<ast::BinOp::add>> {};
  struct SUB_OP : terminal<lit<"-">, cursiv::value<ast::BinOp::subtract>> {};

  // A4 group "... * ... / ... / ...".
  struct ExprMulDiv
      : infix<ExprPower, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<MUL_OP> | ref<DIV_OP>)> {};

  // A4 group "... + ... / ... - ...": binary, tighter than unary +/- below.
  struct ExprAddSub
      : infix<ExprMulDiv, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<ADD_OP> | ref<SUB_OP>)> {};

  // A4 group "+ ... / - ...": unary, looser than binary +/- above -- `-a+b`
  // reads `-(a+b)`, matching HL7's own errata ("... has higher precedence
  // than unary minus" for seqto, int, ceiling, truncate, round).
  struct ExprNegation
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::func>(lit<"+"> | lit<"-">) +
                 assign<&ast::FuncApply::operand>(ref<ExprAddSub>))
                 //
                 | ref<ExprAddSub>> {};

  // A4 group "localized ...": the bare form, with no `by`.
  struct ExprLocalizedBare
      : rule<create<ast::LocalizedExpr>(
                 ikw<"localized"> + assign<&ast::LocalizedExpr::term>(ref<TERM>))
                 //
                 | ref<ExprNegation>> {};

  // A4 group "uppercase/lowercase/trim/trim left/trim right/substring .../
  // localized ... by ...".
  struct ExprStringPrefix
      : rule<create<ast::TrimExpr>(
                 ikw<"trim"> +
                 option(assign<&ast::TrimExpr::side>(ikw<"left"> | ikw<"right">)) +
                 assign<&ast::TrimExpr::value>(ref<ExprStringPrefix>))
                 //
                 | create<ast::CaseConvExpr>(
                       (flag<&ast::CaseConvExpr::upper>(ikw<"uppercase">) |
                        ikw<"lowercase">) +
                       assign<&ast::CaseConvExpr::value>(ref<ExprStringPrefix>))
                 //
                 | create<ast::SubstringExpr>(
                       ikw<"substring"> +
                       assign<&ast::SubstringExpr::length>(ref<ExprNegation>) +
                       ikw<"characters"> +
                       option(ikw<"starting"> + ikw<"at"> +
                              assign<&ast::SubstringExpr::startingAt>(
                                  ref<ExprNegation>)) +
                       ikw<"from"> +
                       assign<&ast::SubstringExpr::source>(ref<ExprStringPrefix>))
                 //
                 | create<ast::LocalizedExpr>(
                       ikw<"localized"> + assign<&ast::LocalizedExpr::term>(ref<TERM>) +
                       ikw<"by"> +
                       assign<&ast::LocalizedExpr::by>(ref<ExprStringPrefix>))
                 //
                 | ref<ExprLocalizedBare>> {};

  struct CONCAT_OP : terminal<lit<"||">, cursiv::value<ast::BinOp::concat>> {};
  struct FORMATTED_WITH_OP
      : rule<ikw<"formatted"> + ikw<"with">,
             cursiv::value<ast::BinOp::formatted_with>> {};

  // A4 group "... formatted with ...": non-associative, one step tighter
  // than `||` below.
  struct ExprFormatted
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<ExprStringPrefix>) +
                 assign<&ast::BinaryExpr::op>(ref<FORMATTED_WITH_OP>) +
                 assign<&ast::BinaryExpr::right>(ref<ExprStringPrefix>))
                 //
                 | ref<ExprStringPrefix>> {};

  // A4 group "... || ...".
  struct ExprConcat
      : infix<ExprFormatted, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<CONCAT_OP>)> {};

  // A4 group "... = / <> / < / <= / > / >= ... / is within .../ is before...
  // / occur .../ is in ... / is present / is <type> / is <object-name>":
  // one flat, non-associative family -- `find`/`matches pattern` moved out
  // to their own, tighter A4 groups above.
  struct ExprComparison
      : rule<create<ast::TypeCheckExpr>(
                 assign<&ast::TypeCheckExpr::operand>(ref<ExprConcat>) +
                 IsKw +
                 option(flag<&ast::TypeCheckExpr::negated>(ikw<"not">)) +
                 assign<&ast::TypeCheckExpr::check>(TypeCheckOp))
                 //
                 | create<ast::TypeNameCheckExpr>(
                       assign<&ast::TypeNameCheckExpr::operand>(ref<ExprConcat>) +
                       IsKw +
                       option(
                           flag<&ast::TypeNameCheckExpr::negated>(ikw<"not">)) +
                       assign<&ast::TypeNameCheckExpr::typeName>(ref<ID>))
                 //
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       IsKw +
                       option(flag<&ast::BinaryComparison::negated>(ikw<"not">)) +
                       assign<&ast::BinaryComparison::op>(BinaryCompOp) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 // `x is equal y`, distinct from `x occur equal y` below.
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       IsKw +
                       option(flag<&ast::BinaryComparison::negated>(ikw<"not">)) +
                       assign<&ast::BinaryComparison::op>(OccurBinaryOp) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 | create<ast::RangeComparison>(
                       assign<&ast::RangeComparison::subject>(ref<ExprConcat>) +
                       IsKw +
                       option(flag<&ast::RangeComparison::negated>(ikw<"not">)) +
                       ikw<"within"> +
                       assign<&ast::RangeComparison::lo>(ref<ExprConcat>) +
                       assign<&ast::RangeComparison::edge>(RangeEdgeOp) +
                       assign<&ast::RangeComparison::hi>(ref<ExprConcat>))
                 //
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       option(flag<&ast::BinaryComparison::negated>(ikw<"not">)) +
                       assign<&ast::BinaryComparison::op>(ref<CoIn>) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 | create<ast::RangeComparison>(
                       assign<&ast::RangeComparison::subject>(ref<ExprConcat>) +
                       flag<&ast::RangeComparison::occurs>(OccurKw) +
                       option(
                           flag<&ast::RangeComparison::negated>(ikw<"not">)) +
                       ikw<"within"> +
                       assign<&ast::RangeComparison::lo>(ref<ExprConcat>) +
                       assign<&ast::RangeComparison::edge>(RangeEdgeOp) +
                       assign<&ast::RangeComparison::hi>(ref<ExprConcat>))
                 //
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       OccurKw +
                       option(flag<&ast::BinaryComparison::negated>(ikw<"not">)) +
                       assign<&ast::BinaryComparison::op>(OccurBinaryOp) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       OccurKw +
                       option(flag<&ast::BinaryComparison::negated>(ikw<"not">)) +
                       assign<&ast::BinaryComparison::op>(ref<OccAt>) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 | create<ast::BinaryComparison>(
                       assign<&ast::BinaryComparison::left>(ref<ExprConcat>) +
                       assign<&ast::BinaryComparison::op>(SimpleCompOp) +
                       assign<&ast::BinaryComparison::right>(ref<ExprConcat>))
                 //
                 | ref<ExprConcat>> {};

  struct ExprNot
      : rule<create<ast::UnaryNot>(
                 ikw<"not"> + assign<&ast::UnaryNot::operand>(ref<ExprComparison>))
                 //
                 | ref<ExprComparison>> {};

  struct AND_OP : terminal<ikw<"and">, cursiv::value<ast::BinOp::logical_and>> {};
  struct OR_OP : terminal<ikw<"or">, cursiv::value<ast::BinOp::logical_or>> {};

  // A4 groups "... and ..." / "... or ...": `seqto` no longer sits between
  // this pair and `not` -- it moved to the tight cluster near `as`.
  struct ExprOrAnd
      : infix<ExprNot, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(ref<AND_OP>) >> left(ref<OR_OP>))> {};

  struct ExprWhere
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::operand>(ref<ExprOrAnd>) +
                              assign<&ast::FuncApply::func>(ikw<"where">) +
                 assign<&ast::FuncApply::extra>(ref<ExprOrAnd>))
                 //
                 | ref<ExprOrAnd>> {};

  struct ExprRemoveList
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::func>(ikw<"remove">) + assign<&ast::FuncApply::operand>(ref<ExprWhere>) +
                 ikw<"from"> + assign<&ast::FuncApply::from>(ref<ExprWhere>))
                 //
                 | ref<ExprWhere>> {};

  struct ExprAddList
      : rule<create<ast::FuncApply>(
                 option(ikw<"add">) +
                 assign<&ast::FuncApply::operand>(ref<ExprRemoveList>) +
                 assign<&ast::FuncApply::func>(ikw<"to">) + assign<&ast::FuncApply::from>(ref<ExprRemoveList>) +
                 option(ikw<"at"> +
                        assign<&ast::FuncApply::extra>(ref<ExprRemoveList>)))
                 //
                 | ref<ExprRemoveList>> {};

  struct ExprSort
      : rule<create<ast::FuncApply>(
                 assign<&ast::FuncApply::func>(ikw<"sort">) +
                 option(assign<&ast::FuncApply::part>(
                     ikw<"data"> | ikw<"time"> | ikw<"applicability">)) +
                 assign<&ast::FuncApply::operand>(ref<ExprAddList>) +
                 option(ikw<"using"> +
                        assign<&ast::FuncApply::extra>(ref<ExprFunction>)))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::operand>(ref<ExprAddList>) +
                   assign<&ast::FuncApply::func>(ikw<"using">) +
                       assign<&ast::FuncApply::extra>(ref<ExprFunction>))
                 //
                 | ref<ExprAddList>> {};

  struct MERGE_OP : terminal<ikw<"merge">, cursiv::value<ast::BinOp::merge>> {};

  struct ExprMerge
      : infix<ExprSort, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<MERGE_OP>)> {};

  struct ExprCommaList
      : rule<create<ast::ExprList>(
                 option(lit<",">) + append<&ast::ExprList::items>(ref<ExprMerge>) +
                 many(lit<","> + append<&ast::ExprList::items>(ref<ExprMerge>)))> {
  };

  // A4 group "... [...]": looser than merge/sort/where -- `a where b[1]`
  // reads `(a where b)[1]`, not `a where (b[1])`. A trailing `.field` after
  // a `[...]` step stays reachable (`namelist[1].FirstName`,
  // `my_obj.message_list[n].msg` in the corpus): `.` on its own still
  // resolves inside the tight function cluster above, never here.
  struct ExprList
      : rule<ref<ExprCommaList> +
                 many(nest<ast::IndexExpr, &ast::IndexExpr::base>(
                          lit<"["> +
                          assign<&ast::IndexExpr::index>(ref<ExprCommaList>) +
                          lit<"]">) +
                      many(nest<ast::FieldExpr, &ast::FieldExpr::base>(
                          lit<"."> + assign<&ast::FieldExpr::field>(ref<ID>))))> {};

  // ==== Object / call / fuzzy-set phrases ====

  struct ObjectInitElementRule
      : rule<(assign<&ast::ObjectInitElement::name>(ref<ID>) + lit<":="> |
              ikw<"let"> + assign<&ast::ObjectInitElement::name>(ref<ID>) +
                  ikw<"be">) +
                 assign<&ast::ObjectInitElement::value>(ref<ExprList>)> {};

  struct NewObjectExprRule
      : rule<create<ast::NewObjectExpr>(
                 ikw<"new"> +
                 assign<&ast::NewObjectExpr::typeName>(ref<ID>) +
                 option(ikw<"with"> +
                        ((lit<"["> +
                          some(append<&ast::NewObjectExpr::fields>(
                                   ref<ObjectInitElementRule>),
                               lit<",">) +
                          lit<"]">) |
                         (assign<&ast::NewObjectExpr::args>(ref<ExprList>) +
                          option(ikw<"with"> + lit<"["> +
                                 some(append<&ast::NewObjectExpr::fields>(
                                          ref<ObjectInitElementRule>),
                                      lit<",">) +
                                 lit<"]">)))))> {};

  struct CallExprRule
      : rule<create<ast::CallExpr>(
                 ikw<"call"> + assign<&ast::CallExpr::name>(ref<ID>) +
                 option(ikw<"with"> + assign<&ast::CallExpr::args>(ref<ExprList>)))> {
  };

  struct FuzzySetPointRule
      : rule<lit<"("> + assign<&ast::FuzzySetPoint::x>(ref<ExprNegation>) + lit<","> +
                 assign<&ast::FuzzySetPoint::y>(ref<ExprNegation>) + lit<")">> {};

  struct FuzzySetPhrase
      : rule<create<ast::FuzzySetLit>(
                 ikw<"fuzzy"> + ikw<"set"> +
                 some(append<&ast::FuzzySetLit::points>(ref<FuzzySetPointRule>),
                      lit<",">))
                 //
                 | create<ast::FuzzifiedExpr>(
                       assign<&ast::FuzzifiedExpr::value>(ref<ExprNegation>) +
                       ikw<"fuzzified"> + ikw<"by"> +
                       assign<&ast::FuzzifiedExpr::width>(ref<ExprNegation>))> {};

  // ==== data-slot read / mapping / object / FHIR forms ====

  struct ReadExprRule
      : rule<create<ast::ReadExpr>(
            ikw<"read"> +
            option(ikw<"as"> + assign<&ast::ReadExpr::resultName>(ref<ID>)) +
            ((option(assign<&ast::ReadExpr::aggregator>(ref<OfReadFuncOp>) +
                     option(ikw<"of">)) +
              assign<&ast::ReadExpr::mapping>(ref<DataMapping>) +
              option(ikw<"where"> +
                     assign<&ast::ReadExpr::where>(ref<ExprOrAnd>)))
             | (assign<&ast::ReadExpr::aggregator>(ref<FromOfFuncOp>) +
                option(ikw<"of">) +
                assign<&ast::ReadExpr::mapping>(ref<DataMapping>) +
                option(ikw<"where"> +
                       assign<&ast::ReadExpr::where>(ref<ExprOrAnd>)))
             | (assign<&ast::ReadExpr::aggregator>(ref<FromOfFuncOp>) +
                assign<&ast::ReadExpr::count>(ref<ExprFactor>) + ikw<"from"> +
                assign<&ast::ReadExpr::mapping>(ref<DataMapping>) +
                option(ikw<"where"> +
                       assign<&ast::ReadExpr::where>(ref<ExprOrAnd>)))))> {};

  struct MlmRefExprRule
      : rule<create<ast::MlmRefExpr>(
            ikw<"mlm"> +
            ((assign<&ast::MlmRefExpr::name>(ref<TERM>) +
              option(ikw<"from"> + ikw<"institution"> +
                     assign<&ast::MlmRefExpr::institution>(ref<STRING>)))
             | flag<&ast::MlmRefExpr::self>(ref<MlmSelf>)))> {};

  struct MappingExprRule
      : rule<create<ast::MappingExpr>(
            assign<&ast::MappingExpr::kind>(ikw<"interface"> | ikw<"event"> |
                                            ikw<"message"> | ikw<"destination">) +
            assign<&ast::MappingExpr::mapping>(ref<DataMapping>))
                 //
                 | create<ast::MappingExpr>(
                       assign<&ast::MappingExpr::kind>(ikw<"message"> |
                                                       ikw<"destination">) +
                       ikw<"as"> +
                       assign<&ast::MappingExpr::resultName>(ref<ID>) +
                       option(assign<&ast::MappingExpr::mapping>(
                           ref<DataMapping>)))> {};

  struct ObjectDefRule
      : rule<create<ast::ObjectDefExpr>(
            (flag<&ast::ObjectDefExpr::linguistic>(ikw<"linguistic"> +
                                                        ikw<"variable">) |
             ikw<"object">) +
            lit<"["> +
            some(append<&ast::ObjectDefExpr::fields>(ref<ID>), lit<",">) +
            lit<"]">)> {};

  struct FhirObjectRefRule
      : rule<assign<&ast::FhirObjectRef::name>(ref<ID>) +
            option(lit<"["> +
                   some(append<&ast::FhirObjectRef::vars>(ref<ID>), lit<",">) +
                   lit<"]">)> {};

  struct FhirEqualOp : terminal<ikw<"equal">, cursiv::value<ast::CompareOp::eq>> {};

  static constexpr auto FhirCompOp =
      assign<&ast::FhirAccessExpr::op>(SimpleCompOp)
      | (option(flag<&ast::FhirAccessExpr::negated>(ikw<"not">)) +
         assign<&ast::FhirAccessExpr::op>(ref<FhirEqualOp>))
      | assign<&ast::FhirAccessExpr::op>(ref<CoLessOrEqual> | ref<CoLessThan> |
                                         ref<CoGreaterOrEqual> | ref<CoGreaterThan>)
      | (IsKw + assign<&ast::FhirAccessExpr::op>(ref<CoIn>))
      | (IsKw + flag<&ast::FhirAccessExpr::negated>(ikw<"not">) +
         assign<&ast::FhirAccessExpr::op>(ref<FhirEqualOp>));

  struct FhirAccessExprRule
      : rule<create<ast::FhirAccessExpr>(
            flag<&ast::FhirAccessExpr::isFind>(ikw<"find">) +
            assign<&ast::FhirAccessExpr::value>(ref<ExprFactor>) + ikw<"in"> +
            ikw<"it"> +
            some(lit<"."> + append<&ast::FhirAccessExpr::path>(ref<ID>)))
                 //
                 | create<ast::FhirAccessExpr>(
                       ikw<"it"> +
                       some(lit<"."> +
                            append<&ast::FhirAccessExpr::path>(ref<ID>)) +
                       FhirCompOp +
                       assign<&ast::FhirAccessExpr::value>(ref<ExprFactor>))> {
  };

  static constexpr auto DataIdentAssignValue =
      ref<ReadExprRule> | ref<MlmRefExprRule> | ref<MappingExprRule> |
      create<ast::ArgumentExpr>(ikw<"argument">) | ref<ObjectDefRule>;

  // ==== Shared statement fragments ====

  static constexpr auto IdentTarget =
      (assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
       lit<":=">)
      | (ikw<"let"> +
         assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
         ikw<"be">);

  static constexpr auto TimeTarget =
      (flag<&ast::AssignStmt::isTime>(ikw<"time">) + option(ikw<"of">) +
       assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
       lit<":=">)
      | (ikw<"let"> + flag<&ast::AssignStmt::isTime>(ikw<"time">) +
         option(ikw<"of">) +
         assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
         ikw<"be">);

  static constexpr auto ApplicabilityTarget =
      (flag<&ast::AssignStmt::isApplicability>(ikw<"applicability">) +
       option(ikw<"of">) +
       assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
       lit<":=">)
      | (ikw<"let"> +
         flag<&ast::AssignStmt::isApplicability>(ikw<"applicability">) +
         option(ikw<"of">) +
         assign<&ast::AssignStmt::target>(ref<IdentifierOrObjectRef>) +
         ikw<"be">);

  // Assigning to a name, a time or an applicability all take the same value
  // shapes, and data and logic statements agree on them.
  static constexpr auto DataLogicAssign =
      create<ast::AssignStmt>(
          IdentTarget +
          assign<&ast::AssignStmt::value>(ref<ExprList> | ref<NewObjectExprRule> |
                                          ref<CallExprRule> | ref<FuzzySetPhrase>))
      | create<ast::AssignStmt>(TimeTarget +
                                assign<&ast::AssignStmt::value>(ref<ExprList>))
      | create<ast::AssignStmt>(
            ApplicabilityTarget + assign<&ast::AssignStmt::value>(ref<ExprList>));

  static constexpr auto DestructureTarget =
      (lit<"("> +
       some(append<&ast::DestructureAssignStmt::targets>(
                ref<IdentifierOrObjectRef>),
            lit<",">) +
       lit<")"> + lit<":=">)
      | (ikw<"let"> + lit<"("> +
         some(append<&ast::DestructureAssignStmt::targets>(
                  ref<IdentifierOrObjectRef>),
              lit<",">) +
         lit<")"> + ikw<"be">);

  static constexpr auto DataDestructureAssign = create<ast::DestructureAssignStmt>(
      DestructureTarget +
      assign<&ast::DestructureAssignStmt::value>(
          ref<ReadExprRule> | create<ast::ArgumentExpr>(ikw<"argument">) |
          ref<CallExprRule>));

  static constexpr auto LogicDestructureAssign = create<ast::DestructureAssignStmt>(
      DestructureTarget +
      assign<&ast::DestructureAssignStmt::value>(ref<CallExprRule>));

  static constexpr auto IdentReadTarget =
      (assign<&ast::IdentReadAssignStmt::target>(ref<ID>) + lit<":=">)
      | (ikw<"let"> + assign<&ast::IdentReadAssignStmt::target>(ref<ID>) +
         ikw<"be">);

  static constexpr auto DataIdentReadAssign = create<ast::IdentReadAssignStmt>(
      IdentReadTarget +
      assign<&ast::IdentReadAssignStmt::value>(DataIdentAssignValue));

  static constexpr auto FhirTarget =
      (assign<&ast::FhirAssignStmt::target>(ref<FhirObjectRefRule>) + lit<":=">)
      | (ikw<"let"> +
         assign<&ast::FhirAssignStmt::target>(ref<FhirObjectRefRule>) +
         ikw<"be">);

  static constexpr auto DataFhirAssign = create<ast::FhirAssignStmt>(
      FhirTarget + ikw<"read"> + ikw<"as"> +
      option(flag<&ast::FhirAssignStmt::latest>(ikw<"latest">)) +
      assign<&ast::FhirAssignStmt::resource>(ref<FhirObjectRefRule>) +
      option(ikw<"where"> +
             some(append<&ast::FhirAssignStmt::conditions>(
                      ref<FhirAccessExprRule>),
                  ikw<"and">)));

  // Control syntax is shared; only the enclosing slot's statement family
  // changes. This keeps context-specific leaves without copying the loops.
  template <class Statement>
  static constexpr auto ElseIfClauseBody = create<ast::ElseIfClause>(
            ikw<"elseif"> + assign<&ast::ElseIfClause::cond>(ref<ExprList>) +
            ikw<"then"> +
            many(append<&ast::ElseIfClause::body>(ref<Statement>) + lit<";">));

  template <class Statement>
  static constexpr auto CaseClauseBody = create<ast::CaseClause>(
            ikw<"case"> + assign<&ast::CaseClause::value>(ref<ExprFactor>) +
            many(append<&ast::CaseClause::body>(ref<Statement>) + lit<";">));

  template <class Statement, class ElseIf>
  static constexpr auto IfStmtBody = create<ast::IfStmt>(
                       ikw<"if"> + assign<&ast::IfStmt::cond>(ref<ExprList>) +
                       ikw<"then"> +
                       many(append<&ast::IfStmt::thenBody>(ref<Statement>) +
                            lit<";">) +
                       many(append<&ast::IfStmt::elseIfs>(ref<ElseIf>)) +
                       option(flag<&ast::IfStmt::hasElse>(ikw<"else">) +
                              many(append<&ast::IfStmt::elseBody>(ref<Statement>) +
                                   lit<";">)) +
                       ikw<"endif"> +
                       option(flag<&ast::IfStmt::aggregate>(
                           ikw<"aggregate">)));

  template <class Statement>
  static constexpr auto ForStmtBody = create<ast::ForStmt>(
                       ikw<"for"> +
                       assign<&ast::ForStmt::var>(ref<IdentifierOrObjectRef>) +
                       ikw<"in"> + assign<&ast::ForStmt::iterable>(ref<ExprList>) +
                       ikw<"do"> +
                       many(append<&ast::ForStmt::body>(ref<Statement>) +
                            lit<";">) +
                       ikw<"enddo">);

  template <class Statement>
  static constexpr auto WhileStmtBody = create<ast::WhileStmt>(
                       ikw<"while"> + assign<&ast::WhileStmt::cond>(ref<ExprList>) +
                       ikw<"do"> +
                       many(append<&ast::WhileStmt::body>(ref<Statement>) +
                            lit<";">) +
                       ikw<"enddo">);

  template <class Statement, class Case>
  static constexpr auto SwitchStmtBody = create<ast::SwitchStmt>(
                       ikw<"switch"> +
                       assign<&ast::SwitchStmt::subject>(
                           ref<IdentifierOrObjectRef>) +
                       many(append<&ast::SwitchStmt::cases>(ref<Case>)) +
                       option(flag<&ast::SwitchStmt::hasDefault>(
                                  ikw<"default">) +
                              many(append<&ast::SwitchStmt::defaultBody>(
                                       ref<Statement>) +
                                   lit<";">)) +
                       ikw<"endswitch"> +
                       option(flag<&ast::SwitchStmt::aggregate>(
                           ikw<"aggregate">)));

  // ==== Data statement ====

  struct DataStmt;
  struct DataElseIfRule;
  struct DataCaseRule;

  struct DataElseIfRule : rule<ElseIfClauseBody<DataStmt>> {};

  struct DataCaseRule : rule<CaseClauseBody<DataStmt>> {};

  struct DataStmt
      : rule<DataLogicAssign
             //
             | DataDestructureAssign
             //
             | DataIdentReadAssign
             //
             | DataFhirAssign
             //
             | IfStmtBody<DataStmt, DataElseIfRule>
             //
             | ForStmtBody<DataStmt>
             //
             | WhileStmtBody<DataStmt>
             //
             | SwitchStmtBody<DataStmt, DataCaseRule>
             //
                 | create<ast::BreakStmt>(ikw<"breakloop">)
                 //
                 | create<ast::IncludeStmt>(
                       ikw<"include"> +
                       assign<&ast::IncludeStmt::name>(ref<ID>))> {};

  // ==== Logic statement ====

  struct LogicStmt;
  struct LogicElseIfRule;
  struct LogicCaseRule;

  struct LogicElseIfRule
      : rule<ElseIfClauseBody<LogicStmt>> {
  };

  struct LogicCaseRule
      : rule<CaseClauseBody<LogicStmt>> {};

  struct LogicStmt
      : rule<DataLogicAssign
                 //
                 | LogicDestructureAssign
                 //
                 | IfStmtBody<LogicStmt, LogicElseIfRule>
             //
             | ForStmtBody<LogicStmt>
             //
             | WhileStmtBody<LogicStmt>
             //
                 | SwitchStmtBody<LogicStmt, LogicCaseRule>
             //
                 | create<ast::BreakStmt>(ikw<"breakloop">)
                 //
                 | create<ast::ConcludeStmt>(
                       ikw<"conclude"> +
                       assign<&ast::ConcludeStmt::value>(ref<ExprList>))> {};

  // ==== Action statement ====

  struct ActionStmt;
  struct ActionElseIfRule;
  struct ActionCaseRule;

  struct ActionElseIfRule
      : rule<ElseIfClauseBody<ActionStmt>> {
  };

  struct ActionCaseRule
      : rule<CaseClauseBody<ActionStmt>> {};

  struct ActionStmt
      : rule<create<ast::AssignStmt>(
                 IdentTarget +
                 (assign<&ast::AssignStmt::value>(ref<ExprList>) |
                  flag<&ast::AssignStmt::isConclude>(ikw<"conclude">)))
                 //
                 | create<ast::AssignStmt>(
                       ApplicabilityTarget +
                       (assign<&ast::AssignStmt::value>(ref<ExprList>) |
                        flag<&ast::AssignStmt::isConclude>(ikw<"conclude">)))
                 //
                 | create<ast::AssignStmt>(
                       TimeTarget + assign<&ast::AssignStmt::value>(ref<ExprList>))
                 //
                 | create<ast::AssignStmt>(
                       IdentTarget +
                       assign<&ast::AssignStmt::value>(ref<NewObjectExprRule>))
                 //
                 | IfStmtBody<ActionStmt, ActionElseIfRule>
             //
             | ForStmtBody<ActionStmt>
             //
             | WhileStmtBody<ActionStmt>
             //
                 | SwitchStmtBody<ActionStmt, ActionCaseRule>
             //
                 | create<ast::CallStmt>(
                       assign<&ast::CallStmt::call>(ref<CallExprRule>) +
                       option(ikw<"delay"> +
                              assign<&ast::CallStmt::delay>(ref<ExprList>)))
                 //
                 | create<ast::WriteStmt>(
                       ikw<"write"> + assign<&ast::WriteStmt::value>(ref<ExprList>) +
                       option(ikw<"at"> +
                              assign<&ast::WriteStmt::target>(ref<ID>)))
                 //
                 | create<ast::ReturnStmt>(
                       ikw<"return"> +
                       assign<&ast::ReturnStmt::value>(ref<ExprList>))
                 //
                 | create<ast::BreakStmt>(ikw<"breakloop">)> {};

  // ==== Evoke slot ====

  struct EvokeTimeRule
      : rule<create<ast::TimeLit>(
            assign<&ast::TimeLit::text>(ref<ISO_DATE_TIME> | ref<ISO_DATE>))
                 //
                 | create<ast::FuncApply>(
                       assign<&ast::FuncApply::func>(
                           ikw<"today"> | ikw<"tomorrow"> | ikw<"sunday"> |
                           ikw<"monday"> | ikw<"tuesday"> | ikw<"wednesday"> |
                           ikw<"thursday"> | ikw<"friday"> | ikw<"saturday">) +
                       ikw<"attime"> +
                       assign<&ast::FuncApply::operand>(create<ast::TimeLit>(
                           assign<&ast::TimeLit::text>(ref<TIME_OF_DAY>))))> {};

  struct EvokeTimeExprRule : rule<ref<EvokeTimeRule> | ref<ExprDurationMatches>> {};

  struct EvokeTimeOrRule
      : rule<create<ast::TimeOr>(
            append<&ast::TimeOr::times>(ref<EvokeTimeExprRule>) +
            many(ikw<"or"> + append<&ast::TimeOr::times>(ref<EvokeTimeExprRule>)))> {
  };

  struct SimpleTriggerRule
      : rule<create<ast::EventTrigger>(
            append<&ast::EventTrigger::events>(ref<ID>) +
            many(ikw<"or"> + append<&ast::EventTrigger::events>(ref<ID>)))
                 //
                 | create<ast::EventTrigger>(
                       flag<&ast::EventTrigger::any>(ikw<"any">) +
                       option(ikw<"of">) + lit<"("> +
                       some(append<&ast::EventTrigger::events>(ref<ID>),
                            lit<",">) +
                       lit<")">)> {};

  struct DelayedEventTriggerRule
      : rule<create<ast::DelayedEventTrigger>(
            assign<&ast::DelayedEventTrigger::delay>(ref<EvokeTimeOrRule>) +
            ikw<"after"> + ikw<"time"> + option(ikw<"of">) +
            assign<&ast::DelayedEventTrigger::event>(ref<ID>))> {};

  struct ConstantTimeTriggerRule
      : rule<create<ast::ConstantTimeTrigger>(
            assign<&ast::ConstantTimeTrigger::times>(ref<EvokeTimeOrRule>))
                 //
                 | create<ast::ConstantTimeTrigger>(
                       assign<&ast::ConstantTimeTrigger::afterDuration>(
                           ref<ExprDurationMatches>) +
                       ikw<"after"> +
                       assign<&ast::ConstantTimeTrigger::afterTime>(
                           ref<EvokeTimeRule>))> {};

  struct TimeOfEventRule
      : rule<ikw<"time"> + option(ikw<"of">) +
            assign<&ast::TimeOfEvent::event>(ref<ID>)> {};

  struct EvokeStmtRule
      : rule<ref<SimpleTriggerRule>
                 //
                 | ref<DelayedEventTriggerRule>
                 //
                 | ref<ConstantTimeTriggerRule>
                 //
                 | create<ast::EveryTrigger>(
                       ikw<"every"> +
                       assign<&ast::EveryTrigger::interval>(ref<ExprDurationMatches>) +
                       ikw<"for"> +
                       assign<&ast::EveryTrigger::forDuration>(ref<ExprDurationMatches>) +
                       ikw<"starting"> +
                       assign<&ast::EveryTrigger::starting>(
                           ref<DelayedEventTriggerRule> | ref<TimeOfEventRule> |
                           ref<ConstantTimeTriggerRule>) +
                       option(ikw<"until"> +
                              assign<&ast::EveryTrigger::until>(ref<ExprList>)))> {
  };

  // ==== MLM structure ====

  struct MaintenanceRule
      : rule<create<ast::Maintenance>(
            ikw<"maintenance"> + lit<":"> + ikw<"title"> +
            assign<&ast::Maintenance::title>(ref<SlotText>) +
            (ikw<"mlmname"> | ikw<"filename">) + lit<":"> +
            assign<&ast::Maintenance::mlmName>(ref<MlmId>) + lit<";;"> +
            ikw<"arden"> + lit<":"> + ikw<"version 3"> + lit<";;"> + ikw<"version"> +
            assign<&ast::Maintenance::version>(ref<SlotText>) +
            ikw<"institution"> +
            assign<&ast::Maintenance::institution>(ref<SlotText>) +
            ikw<"author"> + assign<&ast::Maintenance::author>(ref<SlotText>) +
            ikw<"specialist"> +
            assign<&ast::Maintenance::specialist>(ref<SlotText>) + ikw<"date"> +
            lit<":"> +
            assign<&ast::Maintenance::date>(ref<ISO_DATE_TIME> | ref<ISO_DATE>) +
            lit<";;"> + ikw<"validation"> + lit<":"> +
            assign<&ast::Maintenance::validation>(ref<ValidationCode>) +
            lit<";;">)> {};

  struct LibraryRule
      : rule<create<ast::Library>(
            ikw<"library"> + lit<":"> + ikw<"purpose"> +
            assign<&ast::Library::purpose>(ref<SlotText>) + ikw<"explanation"> +
            assign<&ast::Library::explanation>(ref<SlotText>) + ikw<"keywords"> +
            assign<&ast::Library::keywords>(ref<SlotText>) +
            option(ikw<"citations"> +
                   assign<&ast::Library::citations>(ref<SlotText>)) +
            option(ikw<"links"> + assign<&ast::Library::links>(ref<SlotText>)))> {
  };

  struct UrgencyValueRule
      : rule<assign<&ast::NumberLit::value>(ref<NUMBER>) |
            ref<IdentifierOrObjectRef>> {};

  struct KnowledgeRule
      : rule<create<ast::Knowledge>(
            ikw<"knowledge"> + lit<":"> + ikw<"type"> + lit<":"> +
            assign<&ast::Knowledge::type>(ref<TypeCode>) + lit<";;"> +
            ikw<"data"> + lit<":"> +
            many(append<&ast::Knowledge::dataStatements>(ref<DataStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> +
            option(ikw<"priority"> + lit<":"> +
                   assign<&ast::Knowledge::priority>(ref<NUMBER>) + lit<";;">) +
            ikw<"evoke"> + lit<":"> +
            many(append<&ast::Knowledge::evokeStatements>(ref<EvokeStmtRule>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> + ikw<"logic"> + lit<":"> +
            many(append<&ast::Knowledge::logicStatements>(ref<LogicStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> + ikw<"action"> + lit<":"> +
            many(append<&ast::Knowledge::actionStatements>(ref<ActionStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> +
            option(ikw<"urgency"> + lit<":"> +
                   assign<&ast::Knowledge::urgency>(ref<UrgencyValueRule>) +
                   lit<";;">))> {};

  struct LangMessageRule
      : rule<assign<&ast::LangMessage::key>(ref<TERM>) + lit<":"> +
            assign<&ast::LangMessage::text>(ref<STRING>)> {};

  struct LanguageSlotRule
      : rule<create<ast::LanguageSlot>(
            ikw<"language"> + lit<":"> +
            assign<&ast::LanguageSlot::code>(ref<TwoCharCode>) +
            many(append<&ast::LanguageSlot::messages>(ref<LangMessageRule>) +
                 lit<";">) +
            option(lit<";">) + lit<";;">)> {};

  struct ResourcesRule
      : rule<create<ast::Resources>(
            ikw<"resources"> + lit<":"> + ikw<"default"> + lit<":"> +
            assign<&ast::Resources::defaultLanguage>(ref<TwoCharCode>) +
            lit<";;"> +
            some(append<&ast::Resources::languages>(ref<LanguageSlotRule>)))> {
  };

  struct MlmRule
      : rule<create<ast::Mlm>(
            assign<&ast::Mlm::maintenance>(ref<MaintenanceRule>) +
            assign<&ast::Mlm::library>(ref<LibraryRule>) +
            assign<&ast::Mlm::knowledge>(ref<KnowledgeRule>) +
            assign<&ast::Mlm::resources>(ref<ResourcesRule>) + ikw<"end"> +
            lit<":">)> {};

  using entry = MlmRule;
  using ignore = list<WS, THE>;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace arden
