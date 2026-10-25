// The SPL language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "spl/core/ast.hpp"

namespace spl {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Spl : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Za-z0-9_.:">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct POW : terminal<lit<"^">, value<ast::ArithOp::power>> {};
  struct MULT : terminal<lit<"*">, value<ast::ArithOp::multiply>> {};
  struct MOD : terminal<lit<"%">, value<ast::ArithOp::modulo>> {};
  struct DIV : terminal<lit<"/">, value<ast::ArithOp::divide>> {};
  struct ADD : terminal<lit<"+">, value<ast::ArithOp::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::ArithOp::subtract>> {};

  struct NE : terminal<lit<"!=">, value<ast::CompareOp::ne>> {};
  struct GE : terminal<lit<">=">, value<ast::CompareOp::ge>> {};
  struct LE : terminal<lit<"<=">, value<ast::CompareOp::le>> {};
  struct EQ : terminal<lit<"=">, value<ast::CompareOp::eq>> {};
  struct GT : terminal<lit<">">, value<ast::CompareOp::gt>> {};
  struct LT : terminal<lit<"<">, value<ast::CompareOp::lt>> {};

  struct AND_KW : terminal<ikw<"AND">, value<ast::BoolOp::and_>> {};
  struct OR_KW : terminal<ikw<"OR">, value<ast::BoolOp::or_>> {};
  struct LIKE_KW : terminal<ikw<"LIKE">> {};

  // Commands and functions: one terminal per word category, so a word
  // classifies the same way cursiv's per-state lexer chooses between a
  // keyword and a plain name; `in` and `like` also serve as functions.
  struct INIT_COMMAND
      : terminal<ikw<"datamodel"> | ikw<"datamodelsimple"> | ikw<"dbinspect"> | ikw<"eventcount"> | ikw<"from"> | ikw<"gentimes"> |
          ikw<"history"> | ikw<"inputcsv"> | ikw<"inputintelligence"> | ikw<"inputlookup"> | ikw<"makeresults"> | ikw<"metadata"> |
          ikw<"metasearch"> | ikw<"msearch"> | ikw<"rest"> | ikw<"savedsearch"> | ikw<"search"> | ikw<"tstats">> {};

  struct STD_COMMAND_AND_FUNCTION
      : terminal<ikw<"lookup"> | ikw<"replace"> | ikw<"spath">> {};

  struct STD_COMMAND
      : terminal<ikw<"abstract"> | ikw<"accum"> | ikw<"addcoltotals"> | ikw<"addinfo"> | ikw<"addtotals"> | ikw<"analyzefields"> |
          ikw<"anomalies"> | ikw<"anomalousvalue"> | ikw<"anomalydetection"> | ikw<"append"> | ikw<"appendcols"> | ikw<"appendpipe"> |
          ikw<"arules"> | ikw<"associate"> | ikw<"autoregress"> | ikw<"awssnsalert"> | ikw<"bin"> | ikw<"bucket"> |
          ikw<"bucketdir"> | ikw<"chart"> | ikw<"cluster"> | ikw<"cofilter"> | ikw<"collect"> | ikw<"concurrency"> |
          ikw<"contingency"> | ikw<"convert"> | ikw<"correlate"> | ikw<"ctable"> | ikw<"dbxquery"> | ikw<"dedup"> |
          ikw<"delete"> | ikw<"delta"> | ikw<"diff"> | ikw<"entitymerge"> | ikw<"erex"> | ikw<"eval"> |
          ikw<"eventstats"> | ikw<"extract"> | ikw<"fieldformat"> | ikw<"fields"> | ikw<"fieldsummary"> | ikw<"filldown"> |
          ikw<"fillnull"> | ikw<"findtypes"> | ikw<"folderize"> | ikw<"foreach"> | ikw<"format"> | ikw<"fromjson"> |
          ikw<"gauge"> | ikw<"geom"> | ikw<"geomfilter"> | ikw<"geostats"> | ikw<"head"> | ikw<"highlight"> |
          ikw<"iconify"> | ikw<"iplocation"> | ikw<"join"> | ikw<"kmeans"> | ikw<"kvform"> | ikw<"loadjob"> |
          ikw<"localize"> | ikw<"localop"> | ikw<"makecontinuous"> | ikw<"makemv"> | ikw<"map"> | ikw<"mcollect"> |
          ikw<"meventcollect"> | ikw<"mpreview"> | ikw<"mstats"> | ikw<"multikv"> | ikw<"multisearch"> | ikw<"mvcombine"> |
          ikw<"mvexpand"> | ikw<"nomv"> | ikw<"outlier"> | ikw<"outputcsv"> | ikw<"outputlookup"> | ikw<"outputtext"> |
          ikw<"overlap"> | ikw<"pivot"> | ikw<"predict"> | ikw<"rangemap"> | ikw<"rare"> | ikw<"regex"> |
          ikw<"reltime"> | ikw<"rename"> | ikw<"require"> | ikw<"return"> | ikw<"reverse"> | ikw<"rex"> |
          ikw<"rtorder"> | ikw<"run"> | ikw<"script"> | ikw<"scrub"> | ikw<"searchtxn"> | ikw<"selfjoin"> |
          ikw<"sendalert"> | ikw<"sendemail"> | ikw<"set"> | ikw<"setfields"> | ikw<"sichart"> | ikw<"sirare"> |
          ikw<"sistats"> | ikw<"sitimechart"> | ikw<"sitop"> | ikw<"snowincident"> | ikw<"snowincidentstream"> | ikw<"snowevent"> |
          ikw<"snoweventstream"> | ikw<"sort"> | ikw<"stats"> | ikw<"strcat"> | ikw<"streamstats"> | ikw<"table"> |
          ikw<"tags"> | ikw<"tail"> | ikw<"timechart"> | ikw<"timewrap"> | ikw<"tojson"> | ikw<"top"> |
          ikw<"transaction"> | ikw<"transpose"> | ikw<"trendline"> | ikw<"tscollect"> | ikw<"typeahead"> | ikw<"typelearner"> |
          ikw<"typer"> | ikw<"union"> | ikw<"uniq"> | ikw<"untable"> | ikw<"walklex"> | ikw<"where"> |
          ikw<"x11"> | ikw<"xmlkv"> | ikw<"xmlunescape"> | ikw<"xpath"> | ikw<"xyseries">> {};

  struct MODIFIER_AND_FUNCTION : terminal<ikw<"earliest"> | ikw<"latest">> {};
  struct TIME_AND_FUNCTION : terminal<ikw<"now">> {};

  // The three percentile functions take an embedded number: `exactperc95`,
  // `percentile50`, `upperperc90`.
  struct FUNCTION
      : terminal<
          (lit<"exactperc"> + some(d)) | (lit<"percentile"> + some(d)) | (lit<"upperperc"> + some(d)) |
          ikw<"abs"> | ikw<"acos"> | ikw<"acosh"> | ikw<"asin"> | ikw<"asinh"> | ikw<"atan"> |
          ikw<"atan2"> | ikw<"atanh"> | ikw<"bit_and"> | ikw<"bit_or"> | ikw<"bit_not"> | ikw<"bit_xor"> |
          ikw<"bit_shift_left"> | ikw<"bit_shift_right"> | ikw<"case"> | ikw<"cidrmatch"> | ikw<"ceiling"> | ikw<"coalesce"> |
          ikw<"commands"> | ikw<"cos"> | ikw<"cosh"> | ikw<"exact"> | ikw<"exp"> | ikw<"false"> |
          ikw<"floor"> | ikw<"hypot"> | ikw<"if"> | ikw<"in"> | ikw<"ipmask"> | ikw<"isbool"> |
          ikw<"isint"> | ikw<"isnotnull"> | ikw<"isnull"> | ikw<"isnum"> | ikw<"isstr"> | ikw<"json_append"> |
          ikw<"json_array"> | ikw<"json_array_to_mv"> | ikw<"json_extend"> | ikw<"json_extract"> | ikw<"json_extract_exact"> | ikw<"json_keys"> |
          ikw<"json_object"> | ikw<"json_set"> | ikw<"json_set_exact"> | ikw<"json_valid"> | ikw<"len"> | ikw<"ln"> |
          ikw<"log"> | ikw<"lower"> | ikw<"ltrim"> | ikw<"match"> | ikw<"md5"> | ikw<"mvappend"> |
          ikw<"mvcount"> | ikw<"mvdedup"> | ikw<"mvfilter"> | ikw<"mvfind"> | ikw<"mvindex"> | ikw<"mvjoin"> |
          ikw<"mvmap"> | ikw<"mvrange"> | ikw<"mvsort"> | ikw<"mvzip"> | ikw<"mv_to_json_array"> | ikw<"null"> |
          ikw<"nullif"> | ikw<"pi"> | ikw<"pow"> | ikw<"printf"> | ikw<"random"> | ikw<"relative_time"> |
          ikw<"round"> | ikw<"rtrim"> | ikw<"searchmatch"> | ikw<"sha1"> | ikw<"sha256"> | ikw<"sha512"> |
          ikw<"sigfig"> | ikw<"sin"> | ikw<"sinh"> | ikw<"split"> | ikw<"sqrt"> | ikw<"strftime"> |
          ikw<"strptime"> | ikw<"substr"> | ikw<"tan"> | ikw<"tanh"> | ikw<"time"> | ikw<"tonumber"> |
          ikw<"tostring"> | ikw<"trim"> | ikw<"true"> | ikw<"typeof"> | ikw<"upper"> | ikw<"urldecode"> |
          ikw<"validate"> | ikw<"avg"> | ikw<"count"> | ikw<"distinct_count"> | ikw<"estdc"> | ikw<"estdc_error"> |
          ikw<"max"> | ikw<"mean"> | ikw<"median"> | ikw<"min"> | ikw<"mode"> | ikw<"range"> |
          ikw<"stdev"> | ikw<"stdevp"> | ikw<"sum"> | ikw<"sumsq"> | ikw<"var"> | ikw<"varp"> |
          ikw<"first"> | ikw<"last"> | ikw<"list"> | ikw<"values"> | ikw<"earliest_time"> | ikw<"latest_time"> |
          ikw<"per_day"> | ikw<"per_hour"> | ikw<"per_minute"> | ikw<"per_second"> | ikw<"rate"> | ikw<"rate_avg"> |
          ikw<"rate_sum">> {};

  struct AS_KW : terminal<ikw<"AS">> {};
  struct BY_KW : terminal<ikw<"BY">> {};
  struct OUTPUT_KW : terminal<ikw<"OUTPUT">> {};
  struct OUTPUTNEW_KW : terminal<ikw<"OUTPUTNEW">> {};
  struct IN_KW : terminal<ikw<"IN">> {};
  struct NOT_KW : terminal<ikw<"NOT">> {};

  // A relative time unit, case-insensitive: longest spellings first, so
  // `week`/`quarter` are not cut short by the single-letter forms.
  static constexpr auto TimeUnit =
      (cr<"Qq"> + cr<"Uu"> + cr<"Aa"> + cr<"Rr"> + cr<"Tt"> + cr<"Ee"> + cr<"Rr">) |
      (cr<"Ww"> + cr<"Ee"> + cr<"Ee"> + cr<"Kk">) | (cr<"Yy"> + cr<"Ee"> + cr<"Aa"> + cr<"Rr">) |
      (cr<"Mm"> + cr<"Oo"> + cr<"Nn">) | (cr<"Qq"> + cr<"Tt"> + cr<"Rr">) |
      (cr<"Ww"> + cr<"0">) | (cr<"Ww"> + cr<"1">) | (cr<"Ww"> + cr<"2">) | (cr<"Ww"> + cr<"3">) |
      (cr<"Ww"> + cr<"4">) | (cr<"Ww"> + cr<"5">) | (cr<"Ww"> + cr<"6">) | (cr<"Ww"> + cr<"7">) |
      cr<"SMHDWYQsmhdwyq">;

  // The sign is `+` or `-`.
  struct TIME
      : terminal<(option(lit<"\"">) + cr<"+-"> + some(d) + TimeUnit +
                  option(lit<"@"> + TimeUnit) + option(lit<"\"">)) |
                 (option(lit<"\"">) + lit<"@"> + TimeUnit + option(lit<"\"">)) |
                 (option(lit<"\"">) + repeat<2>(d) + lit<"/"> + repeat<2>(d) + lit<"/"> +
                  repeat<4>(d) + lit<":"> + repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> +
                  repeat<2>(d) + option(lit<"\"">))> {};

  struct NUMBER : terminal<some(d) + option(lit<"."> + some(d)), double> {};
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">, Unquoted> {};
  struct IDENTIFIER : terminal<cr<"A-Za-z_.:"> + many(cr<"A-Za-z_.:0-9">)> {};

  // A command or function word is also a plain name (`id`): cursiv's
  // per-state lexer takes the keyword reading where a command/function is
  // expected and the name reading everywhere else -- no terminal is spelled
  // twice.
  static constexpr auto CommandWord =
      ref<INIT_COMMAND> | ref<STD_COMMAND> | ref<STD_COMMAND_AND_FUNCTION>;
  static constexpr auto FunctionHead = ref<FUNCTION> | ref<STD_COMMAND_AND_FUNCTION> |
                                       ref<MODIFIER_AND_FUNCTION> | ref<TIME_AND_FUNCTION> |
                                       ref<LIKE_KW>;
  static constexpr auto IdWord = ref<IDENTIFIER> | CommandWord | FunctionHead;

  struct Expr;

  struct FuncCallRule
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(FunctionHead) + lit<"("> +
            option(some(append<&ast::FuncCall::args>(ref<Expr>), lit<",">)) +
            lit<")">)> {};

  struct PathExprRule;
  struct DateLitRule;
  struct StringLitRule;
  struct NameExprRule;
  struct NumberLitRule;

  static constexpr auto glob_operand =
      ref<FuncCallRule> | (lit<"("> + ref<Expr> + lit<")">) | ref<PathExprRule> |
      ref<DateLitRule> | ref<StringLitRule> | ref<NameExprRule> | ref<NumberLitRule>;

  // A glob: an opening star, a closing star, or both, wrapping an optional
  // expression -- either star alone stands for the whole thing. The trailing
  // form wraps an operand, not an expression: it must not be a turn of
  // `Primary`'s own loop, which would take the `*` of `2/3*4`.
  struct GlobRule
      : rule<create<ast::Glob>(
                 flag<&ast::Glob::leadingStar>(lit<"*">) +
                 option(assign<&ast::Glob::inner>(ref<Expr>)) +
                 option(flag<&ast::Glob::trailingStar>(lit<"*">))) |
             create<ast::Glob>(assign<&ast::Glob::inner>(glob_operand) +
                               flag<&ast::Glob::trailingStar>(lit<"*">))> {};

  // One segment: a further `/` is a division.
  struct PathExprRule
      : rule<create<ast::PathExpr>(
            lit<"/"> + append<&ast::PathExpr::segments>(IdWord))> {};

  struct DateLitRule
      : rule<(option(lit<"\"">) + assign<&ast::DateLit::text>(ref<TIME_AND_FUNCTION>) +
              option(lit<"\"">)) |
             assign<&ast::DateLit::text>(ref<TIME>)> {};

  struct StringLitRule : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct NameExprRule : rule<assign<&ast::NameExpr::name>(IdWord)> {};
  struct NumberLitRule
      : rule<option(flag<&ast::NumberLit::negative>(ref<SUB>) | ref<ADD>) +
             assign<&ast::NumberLit::value>(ref<NUMBER>)> {};

  struct Primary
      : rule<glob_operand | ref<GlobRule>> {};

  struct Expr : infix<Primary, &ast::ArithExpr::left, &ast::ArithExpr::op, &ast::ArithExpr::right,
                      (right(ref<POW>) >>                       //
                       left(ref<MULT> | ref<DIV> | ref<MOD>) >>  //
                       left(ref<ADD> | ref<SUB>))> {};

  struct Operation;

  struct LikeTestRule
      : rule<create<ast::LikeTest>(assign<&ast::LikeTest::subject>(ref<Expr>) + ref<LIKE_KW> +
                                   assign<&ast::LikeTest::pattern>(ref<Expr>))> {};

  struct InTestRule
      : rule<create<ast::InTest>(
            assign<&ast::InTest::subject>(ref<Expr>) + ref<IN_KW> + lit<"("> +
            option(some(append<&ast::InTest::options>(ref<Expr>), lit<",">)) + lit<")">)> {};

  struct OutputClauseRule : rule<create<ast::OutputClause>(
      some(append<&ast::OutputClause::sources>(ref<Expr>)) +
      (flag<&ast::OutputClause::isNew>(ref<OUTPUTNEW_KW>) | ref<OUTPUT_KW>) +
      some(append<&ast::OutputClause::targets>(IdWord), lit<",">))> {};

  struct ByClauseRule
      : rule<create<ast::ByClause>(ref<BY_KW> +
                                   some(append<&ast::ByClause::fields>(IdWord)))> {};

  struct AliasRule
      : rule<create<ast::Alias>(assign<&ast::Alias::value>(ref<Expr>) + ref<AS_KW> +
                                assign<&ast::Alias::name>(IdWord))> {};

  struct ComparisonRule
      : rule<create<ast::Comparison>(
            assign<&ast::Comparison::field>(IdWord) +
            assign<&ast::Comparison::op>(ref<EQ> | ref<NE> | ref<GE> | ref<LE> | ref<GT> | ref<LT>) +
            assign<&ast::Comparison::value>(ref<Expr>))> {};

  struct BareExprRule
      : rule<create<ast::BareExpr>(assign<&ast::BareExpr::value>(ref<Expr>))> {};

  struct AtomOp
      : rule<ref<LikeTestRule> | ref<InTestRule> | ref<OutputClauseRule> | ref<ByClauseRule> | ref<AliasRule> | ref<ComparisonRule> |
             (lit<"("> + ref<Operation> + lit<")">) | ref<BareExprRule>> {};

  struct NotRule
      : rule<create<ast::Negation>(ref<NOT_KW> +
                                   assign<&ast::Negation::operand>(ref<NotRule>)) |
             ref<AtomOp>> {};

  struct Operation
      : infix<NotRule, &ast::BoolCombine::left, &ast::BoolCombine::op, &ast::BoolCombine::right,
              (left(ref<AND_KW>) >> left(ref<OR_KW>))> {};

  struct InitCommandRule;
  struct NextCommandRule;

  struct SubqueryRule
      : rule<lit<"["> + assign<&ast::Subquery::init>(ref<InitCommandRule>) +
             many(lit<"|"> + append<&ast::Subquery::pipeline>(ref<NextCommandRule>)) + lit<"]">> {
  };

  struct InitCommandRule
      : rule<option(lit<"|">) +
             option(assign<&ast::InitCommand::commandName>(ref<INIT_COMMAND>)) +
             some(append<&ast::InitCommand::operations>(ref<Operation>)) +
             option(assign<&ast::InitCommand::subquery>(ref<SubqueryRule>))> {};

  struct NextCommandRule
      : rule<assign<&ast::NextCommand::commandName>(CommandWord) +
             some(append<&ast::NextCommand::operations>(ref<Operation>)) +
             option(assign<&ast::NextCommand::subquery>(ref<SubqueryRule>))> {};

  struct QueryRule
      : rule<assign<&ast::Query::init>(ref<InitCommandRule>) +
             many(lit<"|"> + append<&ast::Query::pipeline>(ref<NextCommandRule>))> {};

  using entry = QueryRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace spl

