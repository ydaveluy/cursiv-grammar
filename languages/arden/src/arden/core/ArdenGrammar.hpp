// The arden language: its grammar.
#pragma once

#include <memory_resource>
#include <string_view>

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

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto string_value =
    [](std::string_view text, std::pmr::memory_resource* mem) {
  const auto body = text.substr(1, text.size() - 2);
  if (body.find_first_of("\"\r\n") == body.npos) return body;
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  const auto whitespace = [](char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
  };
  for (std::size_t i = 0; i < body.size();) {
    const char c = body[i++];
    if (c == '"') {
      ++i;  // the terminal has matched a doubled quote
      room[n++] = '"';
    } else if (c == '\r' || c == '\n') {
      while (n != 0 && whitespace(room[n - 1])) --n;
      std::size_t lineFeeds = c == '\n' ? 1 : 0;
      while (i < body.size() && whitespace(body[i]))
        lineFeeds += body[i++] == '\n';
      room[n++] = lineFeeds > 1 ? '\n' : ' ';
    } else {
      room[n++] = c;
    }
  }
  return std::string_view(room.data(), n);
};

inline constexpr auto unbrace = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

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
      : terminal<as<double>((some(d) + option(lit<".", !lit<".">> + many(d)) +
                             option(Exponent)) |
                            (lit<"."> + some(d) + option(Exponent)))> {};

  struct STRING
      : terminal<as<string_value>(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) +
                             lit<"\"">)> {};
  // A raw single-quoted term: no escape, closes on the first `'`.
  struct TERM : terminal<as<unquote>(lit<"'"> + many(cr<"^'">) + lit<"'">)> {};

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
  struct SlotText
      : terminal<as<slot_text_value>(through(lit<":">, lit<";;">))> {};

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
      : terminal<as<unbrace>(lit<"{"> + many((lit<"\\"> + any) | cr<"^}">) +
                             lit<"}">)> {};

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
      : rule<as<any_istrue_text>(ikw<"any"> + option(ikw<"istrue">))> {};
  static constexpr auto all_aretrue_text =
      [] { return std::string_view("all aretrue"); };
  struct AllAreTrue
      : rule<as<all_aretrue_text>(ikw<"all"> + option(ikw<"aretrue">))> {};
  static constexpr auto no_istrue_text = [] { return std::string_view("no istrue"); };
  struct NoIsTrue
      : rule<as<no_istrue_text>(ikw<"no"> + option(ikw<"istrue">))> {};
  static constexpr auto percent_increase_text =
      [] { return std::string_view("percent increase"); };
  struct PercentIncrease
      : rule<as<percent_increase_text>(option(ikw<"percent"> | lit<"%">) +
                                       ikw<"increase">)> {};
  static constexpr auto percent_decrease_text =
      [] { return std::string_view("percent decrease"); };
  struct PercentDecrease
      : rule<as<percent_decrease_text>(option(ikw<"percent"> | lit<"%">) +
                                       ikw<"decrease">)> {};
  static constexpr auto time_of_day_text = [] { return std::string_view("time of day"); };
  struct TimeOfDayFn : rule<as<time_of_day_text>(
                           ikw<"time"> + option(ikw<"of"> + ikw<"day">))> {};
  static constexpr auto day_of_week_text = [] { return std::string_view("day of week"); };
  struct DayOfWeekFn
      : rule<as<day_of_week_text>(ikw<"day"> + ikw<"of"> + ikw<"week">)> {};
  static constexpr auto attr_names_text =
      [] { return std::string_view("attribute names"); };
  struct AttrNamesOp
      : rule<as<attr_names_text>(ikw<"attribute"> + ikw<"names">)> {};

  static constexpr auto OfNoReadFuncOp =
      ref<OfNoReadFuncOpSimple> | ref<AnyIsTrue> | ref<AllAreTrue> | ref<NoIsTrue> |
      ref<PercentIncrease> | ref<PercentDecrease> | ref<TimeOfDayFn> | ref<DayOfWeekFn>;

  // ---- Operator vocabularies for the comparison/function ladders ----

  static constexpr auto nearest_text = [] { return std::string_view("nearest"); };
  struct NearestOp : rule<as<nearest_text>(ikw<"nearest">)> {};
  static constexpr auto at_least_text = [] { return std::string_view("at least"); };
  struct AtLeastOp : rule<as<at_least_text>(ikw<"at"> + ikw<"least">)> {};
  static constexpr auto at_most_text = [] { return std::string_view("at most"); };
  struct AtMostOp : rule<as<at_most_text>(ikw<"at"> + ikw<"most">)> {};

  struct POWER_OP : terminal<as<ast::BinOp::power>(lit<"**">)> {};

  static constexpr auto truth_value_type_text =
      [] { return std::string_view("truth value"); };
  struct TruthValueTypeOp
      : rule<as<truth_value_type_text>(ikw<"truth"> + ikw<"value">)> {};

  struct BeforeRelOp : terminal<as<ast::CompareOp::before_rel>(ikw<"before">)> {
  };
  struct AfterRelOp : terminal<as<ast::CompareOp::after_rel>(ikw<"after">)> {};
  struct FromRelOp : terminal<as<ast::CompareOp::from_rel>(ikw<"from">)> {};

  struct TcPresent : terminal<as<ast::TypeCheck::present>(ikw<"present">)> {};
  struct TcNull : terminal<as<ast::TypeCheck::is_null>(ikw<"null">)> {};
  struct TcBoolean : terminal<as<ast::TypeCheck::boolean_>(ikw<"boolean">)> {};
  struct TcTruthValue
      : rule<as<ast::TypeCheck::truth_value>(ikw<"truth"> + ikw<"value">)> {};
  struct TcNumber : terminal<as<ast::TypeCheck::number_>(ikw<"number">)> {};
  struct TcString : terminal<as<ast::TypeCheck::string_>(ikw<"string">)> {};
  struct TcTimeOfDay : rule<as<ast::TypeCheck::time_of_day>(
                           ikw<"time"> + ikw<"of"> + ikw<"day">)> {};
  struct TcTime : terminal<as<ast::TypeCheck::time_>(ikw<"time">)> {};
  struct TcDuration : terminal<as<ast::TypeCheck::duration_>(ikw<"duration">)> {
  };
  struct TcList : terminal<as<ast::TypeCheck::list_>(ikw<"list">)> {};
  struct TcObject : terminal<as<ast::TypeCheck::object_>(ikw<"object">)> {};
  struct TcFuzzy : terminal<as<ast::TypeCheck::fuzzy_>(ikw<"fuzzy">)> {};
  struct TcCrisp : terminal<as<ast::TypeCheck::crisp_>(ikw<"crisp">)> {};
  struct TcLinguisticVariable : rule<as<ast::TypeCheck::linguistic_variable>(
                                    ikw<"linguistic"> + ikw<"variable">)> {};
  static constexpr auto TypeCheckOp =
      ref<TcTruthValue> | ref<TcTimeOfDay> | ref<TcLinguisticVariable> |
      ref<TcPresent> | ref<TcNull> | ref<TcBoolean> | ref<TcNumber> | ref<TcString> |
      ref<TcTime> | ref<TcDuration> | ref<TcList> | ref<TcObject> | ref<TcFuzzy> |
      ref<TcCrisp>;

  struct CoLessOrEqual
      : rule<as<ast::CompareOp::less_or_equal>(ikw<"less"> + ikw<"than"> +
                                               ikw<"or"> + ikw<"equal">)> {};
  struct CoLessThan
      : rule<as<ast::CompareOp::less_than>(ikw<"less"> + ikw<"than">)> {};
  struct CoGreaterOrEqual
      : rule<as<ast::CompareOp::greater_or_equal>(ikw<"greater"> + ikw<"than"> +
                                                  ikw<"or"> + ikw<"equal">)> {};
  struct CoGreaterThan
      : rule<as<ast::CompareOp::greater_than>(ikw<"greater"> + ikw<"than">)> {};
  struct CoIn : terminal<as<ast::CompareOp::in_list>(ikw<"in">)> {};
  static constexpr auto BinaryCompOp = ref<CoLessOrEqual> | ref<CoLessThan> |
                                       ref<CoGreaterOrEqual> | ref<CoGreaterThan> |
                                       ref<CoIn>;

  struct ReTo : terminal<as<ast::RangeEdge::to>(ikw<"to">)> {};
  struct RePreceding
      : terminal<as<ast::RangeEdge::preceding>(ikw<"preceding">)> {};
  struct ReFollowing
      : terminal<as<ast::RangeEdge::following>(ikw<"following">)> {};
  struct ReSurrounding
      : terminal<as<ast::RangeEdge::surrounding>(ikw<"surrounding">)> {};
  static constexpr auto RangeEdgeOp =
      ref<ReTo> | ref<RePreceding> | ref<ReFollowing> | ref<ReSurrounding>;

  struct OccEqual : terminal<as<ast::CompareOp::occurs_equal>(ikw<"equal">)> {};
  struct OccBefore
      : terminal<as<ast::CompareOp::occurs_before>(ikw<"before">)> {};
  struct OccAfter : terminal<as<ast::CompareOp::occurs_after>(ikw<"after">)> {};
  struct OccWithinPast : rule<as<ast::CompareOp::occurs_within_past>(
                             ikw<"within"> + ikw<"past">)> {};
  struct OccWithinSameDay
      : rule<as<ast::CompareOp::occurs_within_sameday>(
            ikw<"within"> + ikw<"same"> + ikw<"day"> + ikw<"as">)> {};
  static constexpr auto OccurBinaryOp = ref<OccWithinSameDay> | ref<OccWithinPast> |
                                        ref<OccEqual> | ref<OccBefore> | ref<OccAfter>;
  struct OccAt : terminal<as<ast::CompareOp::occurs_at>(ikw<"at">)> {};

  struct CmpLe : terminal<as<ast::CompareOp::le>(lit<"<="> | ikw<"le">)> {};
  struct CmpGe : terminal<as<ast::CompareOp::ge>(lit<">="> | ikw<"ge">)> {};
  struct CmpNe : terminal<as<ast::CompareOp::ne>(lit<"<>"> | ikw<"ne">)> {};
  struct CmpEq : terminal<as<ast::CompareOp::eq>(lit<"="> | ikw<"eq">)> {};
  struct CmpLt : terminal<as<ast::CompareOp::lt>(lit<"<"> | ikw<"lt">)> {};
  struct CmpGt : terminal<as<ast::CompareOp::gt>(lit<">"> | ikw<"gt">)> {};
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
  struct NewObjectExpr;
  struct CallExpr;

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
                       ((ikw<"using"> +
                            assign<&ast::FuncApply::extra>(ref<ExprFunction>)) |
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
  // Consume the value before nesting a match. A head ExprStringPrefix
  // would return here through ExprAgo without consuming any input. The
  // pattern keeps its wider string-expression argument, while an `ago`
  // continuation must be followed by another match (never `x ago ago`).
  static constexpr auto MatchesPatternSuffix =
      nest<ast::MatchesPattern, &ast::MatchesPattern::value>(
          ikw<"matches"> + ikw<"pattern"> +
          assign<&ast::MatchesPattern::pattern>(ref<ExprStringPrefix>));
  static constexpr auto AgoMatchesPatternSuffix =
      nest<ast::FuncApply, &ast::FuncApply::operand>(
          assign<&ast::FuncApply::func>(ikw<"ago">)) +
      MatchesPatternSuffix;

  struct ExprDurationMatches
      : rule<(ref<ExprFind> +
              option(nest<ast::FuncApply, &ast::FuncApply::operand>(
                  assign<&ast::FuncApply::func>(ref<DurationOp>))) +
              option(MatchesPatternSuffix) + many(AgoMatchesPatternSuffix))
                 //
                 | (create<ast::LocalizedExpr>(
                        ikw<"localized"> +
                        assign<&ast::LocalizedExpr::term>(ref<TERM>)) +
                    MatchesPatternSuffix + many(AgoMatchesPatternSuffix))> {};

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

  struct MUL_OP : terminal<as<ast::BinOp::multiply>(lit<"*">)> {};
  struct DIV_OP : terminal<as<ast::BinOp::divide>(lit<"/">)> {};
  struct ADD_OP : terminal<as<ast::BinOp::add>(lit<"+">)> {};
  struct SUB_OP : terminal<as<ast::BinOp::subtract>(lit<"-">)> {};

  // A4 group "... * ... / ... / ...".
  struct ExprMulDiv
      : infix<ExprPower, ast::BinaryExpr, left(ref<MUL_OP> | ref<DIV_OP>)> {};

  // A4 group "... + ... / ... - ...": binary, tighter than unary +/- below.
  struct ExprAddSub
      : infix<ExprMulDiv, ast::BinaryExpr, left(ref<ADD_OP> | ref<SUB_OP>)> {};

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

  struct CONCAT_OP : terminal<as<ast::BinOp::concat>(lit<"||">)> {};
  struct FORMATTED_WITH_OP
      : rule<as<ast::BinOp::formatted_with>(ikw<"formatted"> + ikw<"with">)> {};

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
      : infix<ExprFormatted, ast::BinaryExpr, left(ref<CONCAT_OP>)> {};

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

  struct AND_OP : terminal<as<ast::BinOp::logical_and>(ikw<"and">)> {};
  struct OR_OP : terminal<as<ast::BinOp::logical_or>(ikw<"or">)> {};

  // A4 groups "... and ..." / "... or ...": `seqto` no longer sits between
  // this pair and `not` -- it moved to the tight cluster near `as`.
  struct ExprOrAnd
      : infix<ExprNot, ast::BinaryExpr,
              levels(left(ref<AND_OP>), left(ref<OR_OP>))> {};

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

  struct MERGE_OP : terminal<as<ast::BinOp::merge>(ikw<"merge">)> {};

  struct ExprMerge
      : infix<ExprSort, ast::BinaryExpr, left(ref<MERGE_OP>)> {};

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

  struct ObjectInitElement
      : rule<((assign<&ast::ObjectInitElement::name>(ref<ID>) + lit<":=">) |
              (ikw<"let"> + assign<&ast::ObjectInitElement::name>(ref<ID>) +
                  ikw<"be">)) +
                 assign<&ast::ObjectInitElement::value>(ref<ExprList>)> {};

  struct NewObjectExpr
      : rule<create<ast::NewObjectExpr>(
                 ikw<"new"> +
                 assign<&ast::NewObjectExpr::typeName>(ref<ID>) +
                 option(ikw<"with"> +
                        ((lit<"["> +
                          some(append<&ast::NewObjectExpr::fields>(
                                   ref<ObjectInitElement>),
                               lit<",">) +
                          lit<"]">) |
                         (assign<&ast::NewObjectExpr::args>(ref<ExprList>) +
                          option(ikw<"with"> + lit<"["> +
                                 some(append<&ast::NewObjectExpr::fields>(
                                          ref<ObjectInitElement>),
                                      lit<",">) +
                                 lit<"]">)))))> {};

  struct CallExpr
      : rule<create<ast::CallExpr>(
                 ikw<"call"> + assign<&ast::CallExpr::name>(ref<ID>) +
                 option(ikw<"with"> + assign<&ast::CallExpr::args>(ref<ExprList>)))> {
  };

  struct FuzzySetPoint
      : rule<lit<"("> + assign<&ast::FuzzySetPoint::x>(ref<ExprNegation>) + lit<","> +
                 assign<&ast::FuzzySetPoint::y>(ref<ExprNegation>) + lit<")">> {};

  struct FuzzySetPhrase
      : rule<create<ast::FuzzySetLit>(
                 ikw<"fuzzy"> + ikw<"set"> +
                 some(append<&ast::FuzzySetLit::points>(ref<FuzzySetPoint>),
                      lit<",">))
                 //
                 | create<ast::FuzzifiedExpr>(
                       assign<&ast::FuzzifiedExpr::value>(ref<ExprNegation>) +
                       ikw<"fuzzified"> + ikw<"by"> +
                       assign<&ast::FuzzifiedExpr::width>(ref<ExprNegation>))> {};

  // ==== data-slot read / mapping / object / FHIR forms ====

  struct ReadExpr
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

  struct MlmRefExpr
      : rule<create<ast::MlmRefExpr>(
            ikw<"mlm"> +
            ((assign<&ast::MlmRefExpr::name>(ref<TERM>) +
              option(ikw<"from"> + ikw<"institution"> +
                     assign<&ast::MlmRefExpr::institution>(ref<STRING>)))
             | flag<&ast::MlmRefExpr::self>(ref<MlmSelf>)))> {};

  struct MappingExpr
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

  struct ObjectDef
      : rule<create<ast::ObjectDefExpr>(
            (flag<&ast::ObjectDefExpr::linguistic>(ikw<"linguistic"> +
                                                        ikw<"variable">) |
             ikw<"object">) +
            lit<"["> +
            some(append<&ast::ObjectDefExpr::fields>(ref<ID>), lit<",">) +
            lit<"]">)> {};

  struct FhirObjectRef
      : rule<assign<&ast::FhirObjectRef::name>(ref<ID>) +
            option(lit<"["> +
                   some(append<&ast::FhirObjectRef::vars>(ref<ID>), lit<",">) +
                   lit<"]">)> {};

  struct FhirEqualOp : terminal<as<ast::CompareOp::eq>(ikw<"equal">)> {};

  static constexpr auto FhirCompOp =
      assign<&ast::FhirAccessExpr::op>(SimpleCompOp)
      | (option(flag<&ast::FhirAccessExpr::negated>(ikw<"not">)) +
         assign<&ast::FhirAccessExpr::op>(ref<FhirEqualOp>))
      | assign<&ast::FhirAccessExpr::op>(ref<CoLessOrEqual> | ref<CoLessThan> |
                                         ref<CoGreaterOrEqual> | ref<CoGreaterThan>)
      | (IsKw + assign<&ast::FhirAccessExpr::op>(ref<CoIn>))
      | (IsKw + flag<&ast::FhirAccessExpr::negated>(ikw<"not">) +
         assign<&ast::FhirAccessExpr::op>(ref<FhirEqualOp>));

  struct FhirAccessExpr
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
      ref<ReadExpr> | ref<MlmRefExpr> | ref<MappingExpr> |
      create<ast::ArgumentExpr>(ikw<"argument">) | ref<ObjectDef>;

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
          assign<&ast::AssignStmt::value>(ref<ExprList> | ref<NewObjectExpr> |
                                          ref<CallExpr> | ref<FuzzySetPhrase>))
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
          ref<ReadExpr> | create<ast::ArgumentExpr>(ikw<"argument">) |
          ref<CallExpr>));

  static constexpr auto LogicDestructureAssign = create<ast::DestructureAssignStmt>(
      DestructureTarget +
      assign<&ast::DestructureAssignStmt::value>(ref<CallExpr>));

  static constexpr auto IdentReadTarget =
      (assign<&ast::IdentReadAssignStmt::target>(ref<ID>) + lit<":=">)
      | (ikw<"let"> + assign<&ast::IdentReadAssignStmt::target>(ref<ID>) +
         ikw<"be">);

  static constexpr auto DataIdentReadAssign = create<ast::IdentReadAssignStmt>(
      IdentReadTarget +
      assign<&ast::IdentReadAssignStmt::value>(DataIdentAssignValue));

  static constexpr auto FhirTarget =
      (assign<&ast::FhirAssignStmt::target>(ref<FhirObjectRef>) + lit<":=">)
      | (ikw<"let"> +
         assign<&ast::FhirAssignStmt::target>(ref<FhirObjectRef>) +
         ikw<"be">);

  static constexpr auto DataFhirAssign = create<ast::FhirAssignStmt>(
      FhirTarget + ikw<"read"> + ikw<"as"> +
      option(flag<&ast::FhirAssignStmt::latest>(ikw<"latest">)) +
      assign<&ast::FhirAssignStmt::resource>(ref<FhirObjectRef>) +
      option(ikw<"where"> +
             some(append<&ast::FhirAssignStmt::conditions>(
                      ref<FhirAccessExpr>),
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
  struct DataElseIf;
  struct DataCase;

  struct DataElseIf : rule<ElseIfClauseBody<DataStmt>> {};

  struct DataCase : rule<CaseClauseBody<DataStmt>> {};

  struct DataStmt
      : rule<DataLogicAssign
             //
             | DataDestructureAssign
             //
             | DataIdentReadAssign
             //
             | DataFhirAssign
             //
             | IfStmtBody<DataStmt, DataElseIf>
             //
             | ForStmtBody<DataStmt>
             //
             | WhileStmtBody<DataStmt>
             //
             | SwitchStmtBody<DataStmt, DataCase>
             //
                 | create<ast::BreakStmt>(ikw<"breakloop">)
                 //
                 | create<ast::IncludeStmt>(
                       ikw<"include"> +
                       assign<&ast::IncludeStmt::name>(ref<ID>))> {};

  // ==== Logic statement ====

  struct LogicStmt;
  struct LogicElseIf;
  struct LogicCase;

  struct LogicElseIf
      : rule<ElseIfClauseBody<LogicStmt>> {
  };

  struct LogicCase
      : rule<CaseClauseBody<LogicStmt>> {};

  struct LogicStmt
      : rule<DataLogicAssign
                 //
                 | LogicDestructureAssign
                 //
                 | IfStmtBody<LogicStmt, LogicElseIf>
             //
             | ForStmtBody<LogicStmt>
             //
             | WhileStmtBody<LogicStmt>
             //
                 | SwitchStmtBody<LogicStmt, LogicCase>
             //
                 | create<ast::BreakStmt>(ikw<"breakloop">)
                 //
                 | create<ast::ConcludeStmt>(
                       ikw<"conclude"> +
                       assign<&ast::ConcludeStmt::value>(ref<ExprList>))> {};

  // ==== Action statement ====

  struct ActionStmt;
  struct ActionElseIf;
  struct ActionCase;

  struct ActionElseIf
      : rule<ElseIfClauseBody<ActionStmt>> {
  };

  struct ActionCase
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
                       assign<&ast::AssignStmt::value>(ref<NewObjectExpr>))
                 //
                 | IfStmtBody<ActionStmt, ActionElseIf>
             //
             | ForStmtBody<ActionStmt>
             //
             | WhileStmtBody<ActionStmt>
             //
                 | SwitchStmtBody<ActionStmt, ActionCase>
             //
                 | create<ast::CallStmt>(
                       assign<&ast::CallStmt::call>(ref<CallExpr>) +
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

  struct EvokeTime
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

  struct EvokeTimeExpr : rule<ref<EvokeTime> | ref<ExprDurationMatches>> {};

  struct EvokeTimeOr
      : rule<create<ast::TimeOr>(
            append<&ast::TimeOr::times>(ref<EvokeTimeExpr>) +
            many(ikw<"or"> + append<&ast::TimeOr::times>(ref<EvokeTimeExpr>)))> {
  };

  struct SimpleTrigger
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

  struct DelayedEventTrigger
      : rule<create<ast::DelayedEventTrigger>(
            assign<&ast::DelayedEventTrigger::delay>(ref<EvokeTimeOr>) +
            ikw<"after"> + ikw<"time"> + option(ikw<"of">) +
            assign<&ast::DelayedEventTrigger::event>(ref<ID>))> {};

  struct ConstantTimeTrigger
      : rule<create<ast::ConstantTimeTrigger>(
            assign<&ast::ConstantTimeTrigger::times>(ref<EvokeTimeOr>))
                 //
                 | create<ast::ConstantTimeTrigger>(
                       assign<&ast::ConstantTimeTrigger::afterDuration>(
                           ref<ExprDurationMatches>) +
                       ikw<"after"> +
                       assign<&ast::ConstantTimeTrigger::afterTime>(
                           ref<EvokeTime>))> {};

  struct TimeOfEvent
      : rule<ikw<"time"> + option(ikw<"of">) +
            assign<&ast::TimeOfEvent::event>(ref<ID>)> {};

  struct EvokeStmt
      : rule<ref<SimpleTrigger>
                 //
                 | ref<DelayedEventTrigger>
                 //
                 | ref<ConstantTimeTrigger>
                 //
                 | create<ast::EveryTrigger>(
                       ikw<"every"> +
                       assign<&ast::EveryTrigger::interval>(ref<ExprDurationMatches>) +
                       ikw<"for"> +
                       assign<&ast::EveryTrigger::forDuration>(ref<ExprDurationMatches>) +
                       ikw<"starting"> +
                       assign<&ast::EveryTrigger::starting>(
                           ref<DelayedEventTrigger> | ref<TimeOfEvent> |
                           ref<ConstantTimeTrigger>) +
                       option(ikw<"until"> +
                              assign<&ast::EveryTrigger::until>(ref<ExprList>)))> {
  };

  // ==== MLM structure ====

  struct Maintenance
      : rule<create<ast::Maintenance>(
            ikw<"maintenance"> + lit<":"> + ikw<"title"> +
            assign<&ast::Maintenance::title>(ref<SlotText>) +
            (ikw<"mlmname"> | ikw<"filename">) + lit<":"> +
            assign<&ast::Maintenance::mlmName>(ref<MlmId>) + lit<";;"> +
            ikw<"arden"> + lit<":"> + ikw<"version"> + lit<"3"> + lit<";;"> + ikw<"version"> +
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

  struct Library
      : rule<create<ast::Library>(
            ikw<"library"> + lit<":"> + ikw<"purpose"> +
            assign<&ast::Library::purpose>(ref<SlotText>) + ikw<"explanation"> +
            assign<&ast::Library::explanation>(ref<SlotText>) + ikw<"keywords"> +
            assign<&ast::Library::keywords>(ref<SlotText>) +
            option(ikw<"citations"> +
                   assign<&ast::Library::citations>(ref<SlotText>)) +
            option(ikw<"links"> + assign<&ast::Library::links>(ref<SlotText>)))> {
  };

  struct UrgencyValue
      : rule<assign<&ast::NumberLit::value>(ref<NUMBER>) |
            ref<IdentifierOrObjectRef>> {};

  struct Knowledge
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
            many(append<&ast::Knowledge::evokeStatements>(ref<EvokeStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> + ikw<"logic"> + lit<":"> +
            many(append<&ast::Knowledge::logicStatements>(ref<LogicStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> + ikw<"action"> + lit<":"> +
            many(append<&ast::Knowledge::actionStatements>(ref<ActionStmt>) +
                 lit<";">) +
            option(lit<";">) + lit<";;"> +
            option(ikw<"urgency"> + lit<":"> +
                   assign<&ast::Knowledge::urgency>(ref<UrgencyValue>) +
                   lit<";;">))> {};

  struct LangMessage
      : rule<assign<&ast::LangMessage::key>(ref<TERM>) + lit<":"> +
            assign<&ast::LangMessage::text>(ref<STRING>)> {};

  struct LanguageSlot
      : rule<create<ast::LanguageSlot>(
            ikw<"language"> + lit<":"> +
            assign<&ast::LanguageSlot::code>(ref<TwoCharCode>) +
            many(append<&ast::LanguageSlot::messages>(ref<LangMessage>) +
                 lit<";">) +
            option(lit<";">) + lit<";;">)> {};

  struct Resources
      : rule<create<ast::Resources>(
            ikw<"resources"> + lit<":"> + ikw<"default"> + lit<":"> +
            assign<&ast::Resources::defaultLanguage>(ref<TwoCharCode>) +
            lit<";;"> +
            some(append<&ast::Resources::languages>(ref<LanguageSlot>)))> {
  };

  struct Mlm
      : rule<create<ast::Mlm>(
            assign<&ast::Mlm::maintenance>(ref<Maintenance>) +
            assign<&ast::Mlm::library>(ref<Library>) +
            assign<&ast::Mlm::knowledge>(ref<Knowledge>) +
            assign<&ast::Mlm::resources>(ref<Resources>) + ikw<"end"> +
            lit<":">)> {};

  using entry = Mlm;
  using ignored = list<WS, THE>;
  using hidden = list<COMMENT, LINE_COMMENT>;
};

}  // namespace arden
