// The SPL language: its grammar.
#pragma once

#include "cursiv/grammar/GrammarSpec.hpp"
#include "spl/ast.hpp"

namespace spl {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Spl : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Za-z0-9_.:">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};



  struct LIKE_KW : terminal<ikw<"LIKE">> {};

  // Commands with their own argument syntax have a dedicated terminal.
  struct INIT_COMMAND
      : terminal<ikw<"datamodel"> | ikw<"datamodelsimple"> | ikw<"dbinspect"> | ikw<"eventcount"> | ikw<"from"> |
          ikw<"history"> | ikw<"inputcsv"> | ikw<"inputlookup"> | ikw<"makeresults"> | ikw<"metadata"> |
          ikw<"metasearch"> | ikw<"msearch"> | ikw<"savedsearch">> {};

  struct GENTIMES : terminal<ikw<"gentimes">> {};
  struct TSTATS : terminal<ikw<"tstats">> {};
  struct REST : terminal<ikw<"rest">> {};
  struct INPUTINTELLIGENCE : terminal<ikw<"inputintelligence">> {};
  static constexpr auto GeneratingCommand = ref<GENTIMES> | ref<REST> | ref<INPUTINTELLIGENCE> | ref<TSTATS>;

  struct SEARCH_COMMAND : terminal<ikw<"search">> {};

  struct FIELDS_KW : terminal<ikw<"fields">> {};
  struct LOOKUP_KW : terminal<ikw<"lookup">> {};
  struct STD_COMMAND_AND_FUNCTION
      : terminal<ikw<"replace"> | ikw<"spath">> {};

  struct STD_COMMAND
      : terminal<ikw<"abstract"> | ikw<"accum"> | ikw<"addcoltotals"> | ikw<"addinfo"> | ikw<"addtotals"> | ikw<"analyzefields"> |
          ikw<"anomalies"> | ikw<"anomalousvalue"> | ikw<"anomalydetection"> | ikw<"append"> | ikw<"appendcols"> | ikw<"appendpipe"> |
          ikw<"arules"> | ikw<"associate"> | ikw<"autoregress"> | ikw<"awssnsalert"> | ikw<"bin"> | ikw<"bucket"> |
          ikw<"bucketdir"> | ikw<"chart"> | ikw<"cluster"> | ikw<"cofilter"> | ikw<"collect"> | ikw<"concurrency"> |
          ikw<"contingency"> | ikw<"convert"> | ikw<"correlate"> | ikw<"ctable"> | ikw<"dbxquery"> | ikw<"dedup"> |
          ikw<"delete"> | ikw<"delta"> | ikw<"diff"> | ikw<"entitymerge"> | ikw<"erex"> | ikw<"eval"> |
          ikw<"eventstats"> | ikw<"extract"> | ikw<"fieldformat"> | ikw<"fieldsummary"> | ikw<"filldown"> |
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

  static constexpr auto CountWord = ikw<"count"> | ikw<"c">;
  static constexpr auto perc = cr<"Pp"> + cr<"Ee"> + cr<"Rr"> + cr<"Cc">;
  static constexpr auto percentilePrefix =
      (cr<"Ee"> + cr<"Xx"> + cr<"Aa"> + cr<"Cc"> + cr<"Tt">) |
      (cr<"Uu"> + cr<"Pp"> + cr<"Pp"> + cr<"Ee"> + cr<"Rr">);
  static constexpr auto percentile = ((percentilePrefix + perc) |
      (perc + option(cr<"Ee"> + cr<"Nn"> + cr<"Tt"> + cr<"Ii"> + cr<"Ll"> + cr<"Ee">)) |
      cr<"Pp">) + some(d);
  static constexpr auto StatWords = CountWord | ikw<"avg"> | ikw<"mean"> | ikw<"dc"> |
      ikw<"distinct_count"> | ikw<"estdc"> | ikw<"max"> | ikw<"median"> | ikw<"min"> |
      ikw<"mode"> | ikw<"range"> | ikw<"stdev"> | ikw<"stdevp"> | ikw<"sum"> |
      ikw<"sumsq"> | ikw<"var"> | ikw<"varp"> | ikw<"first"> | ikw<"last"> |
      ikw<"values"> | ikw<"earliest_time"> | ikw<"latest_time"> | ikw<"rate"> |
      percentile;
  struct StatFunction : terminal<StatWords | ikw<"earliest"> | ikw<"latest">> {};

  struct FUNCTION
      : terminal<StatWords |
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
          ikw<"validate"> | ikw<"estdc_error"> | ikw<"list"> | ikw<"per_day"> | ikw<"per_hour"> |
          ikw<"per_minute"> | ikw<"per_second"> | ikw<"rate_avg"> | ikw<"rate_sum">> {};

  struct AS_KW : terminal<ikw<"AS">> {};
  struct BY_KW : terminal<ikw<"BY">> {};
  struct OUTPUT_KW : terminal<ikw<"OUTPUT">> {};
  struct OUTPUTNEW_KW : terminal<ikw<"OUTPUTNEW">> {};
  struct IN_KW : terminal<ikw<"IN">> {};
  struct NOT_KW : terminal<kw<"NOT">> {};

  // SPL1 time aliases share the same structural unit, without letter-by-letter spelling.
  static constexpr auto SpanUnit =
      ikw<"seconds"> | ikw<"second"> | ikw<"secs"> | ikw<"sec"> | ikw<"s"> |
      ikw<"minutes"> | ikw<"minute"> | ikw<"mins"> | ikw<"min"> | ikw<"m"> |
      ikw<"hours"> | ikw<"hour"> | ikw<"hrs"> | ikw<"hr"> | ikw<"h"> |
      ikw<"days"> | ikw<"day"> | ikw<"d"> |
      ikw<"months"> | ikw<"month"> | ikw<"mon">;
  static constexpr auto TimeUnit = SpanUnit |
      ikw<"weeks"> | ikw<"week"> | ikw<"w"> |
      ikw<"quarters"> | ikw<"quarter"> | ikw<"qtrs"> | ikw<"qtr"> | ikw<"q"> |
      ikw<"years"> | ikw<"year"> | ikw<"yrs"> | ikw<"yr"> | ikw<"y"> |
      ikw<"w0"> | ikw<"w1"> | ikw<"w2"> | ikw<"w3"> | ikw<"w4"> |
      ikw<"w5"> | ikw<"w6"> | ikw<"w7">;
  static constexpr auto TimeBody =
      (cr<"+-"> + many(d) + TimeUnit + option(lit<"@"> + TimeUnit)) |
      (lit<"@"> + TimeUnit) |
      (repeat<2>(d) + lit<"/"> + repeat<2>(d) + lit<"/"> + repeat<4>(d) +
       lit<":"> + repeat<2>(d) + lit<":"> + repeat<2>(d) + lit<":"> + repeat<2>(d));
  struct TIME : terminal<TimeBody | (lit<"\""> + TimeBody + lit<"\"">)> {};

  struct NUMBER : terminal<as<double>(some(d) + option(lit<"."> + some(d)))> {};
  struct STRING : terminal<as<unquote>(
                      lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) +
                      lit<"\"">)> {};
  struct IDENTIFIER : terminal<cr<"A-Za-z_.:"> + many(cr<"A-Za-z_.:0-9">)> {};

  // Command and function keywords are ordinary names in value positions.
  static constexpr auto GenericCommand = ref<INIT_COMMAND> | ref<STD_COMMAND> | ref<STD_COMMAND_AND_FUNCTION>;
  static constexpr auto PipelineWord = ref<LOOKUP_KW> | ref<FIELDS_KW>;
  static constexpr auto CommandWord = GenericCommand | GeneratingCommand | PipelineWord | ref<SEARCH_COMMAND>;
  static constexpr auto FunctionHead = ref<FUNCTION> | ref<LOOKUP_KW> | ref<STD_COMMAND_AND_FUNCTION> |
                                       ref<MODIFIER_AND_FUNCTION> | ref<TIME_AND_FUNCTION> |
                                       ref<LIKE_KW>;
  static constexpr auto IdWord = ref<IDENTIFIER> | CommandWord | FunctionHead;

  struct Expr;
  struct EvalExpr;

  struct FuncCall
      : rule<create<ast::FuncCall>(
            assign<&ast::FuncCall::name>(FunctionHead) + lit<"("> +
            option(some(append<&ast::FuncCall::args>(ref<EvalExpr>), lit<",">)) +
            lit<")">)> {};

  struct PathExpr;
  struct DateLit;
  struct StringLit;
  struct NameExpr;
  struct NumberLit;

  static constexpr auto glob_operand =
      ref<FuncCall> | (lit<"("> + ref<Expr> + lit<")">) | ref<PathExpr> |
      ref<DateLit> | ref<StringLit> | ref<NameExpr> | ref<NumberLit>;

  // A glob: an opening star, a closing star, or both, wrapping an optional
  // expression -- either star alone stands for the whole thing. The trailing
  // form wraps an operand, not an expression: it must not be a turn of
  // `Primary`'s own loop, which would take the `*` of `2/3*4`.
  struct Glob
      : rule<create<ast::Glob>(
                 flag<&ast::Glob::leadingStar>(lit<"*">) +
                 option(assign<&ast::Glob::inner>(ref<Expr>)) +
                 option(flag<&ast::Glob::trailingStar>(lit<"*">))) |
             create<ast::Glob>(assign<&ast::Glob::inner>(glob_operand) +
                               flag<&ast::Glob::trailingStar>(lit<"*">))> {};

  // One segment: a further `/` is a division.
  struct PathExpr
      : rule<create<ast::PathExpr>(
            lit<"/"> + append<&ast::PathExpr::segments>(IdWord))> {};

  struct DateLit
      : rule<assign<&ast::DateLit::text>(ref<TIME_AND_FUNCTION>) |
             (lit<"\""> + assign<&ast::DateLit::text>(ref<TIME_AND_FUNCTION>) + lit<"\"">) |
             assign<&ast::DateLit::text>(ref<TIME>)> {};

  struct StringLit : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};
  struct NameExpr : rule<assign<&ast::NameExpr::name>(IdWord)> {};
  struct NumberLit
      : rule<option(flag<&ast::NumberLit::negative>(as<ast::ArithOp::subtract>(lit<"-">)) | as<ast::ArithOp::add>(lit<"+">)) +
             assign<&ast::NumberLit::value>(ref<NUMBER>)> {};

  struct Primary
      : rule<glob_operand | ref<Glob>> {};

  struct Expr : infix<Primary, ast::ArithExpr,
                      levels(right(as<ast::ArithOp::power>(lit<"^">)),                        //
                             left(as<ast::ArithOp::multiply>(lit<"*">) | as<ast::ArithOp::divide>(lit<"/">) | as<ast::ArithOp::modulo>(lit<"%">)),  //
                             left(as<ast::ArithOp::add>(lit<"+">) | as<ast::ArithOp::subtract>(lit<"-">)))> {};

  static constexpr auto CompareOperator =
      as<ast::CompareOp::eq>(lit<"=="> | lit<"=">) |
      as<ast::CompareOp::ne>(lit<"!=">) | as<ast::CompareOp::ge>(lit<">=">) |
      as<ast::CompareOp::le>(lit<"<=">) | as<ast::CompareOp::gt>(lit<">">) |
      as<ast::CompareOp::lt>(lit<"<">);
  struct EvalComparison
      : rule<create<ast::ValueComparison>(
                 assign<&ast::ValueComparison::left>(ref<Expr>) +
                 assign<&ast::ValueComparison::op>(CompareOperator) +
                 assign<&ast::ValueComparison::right>(ref<Expr>)) |
             (lit<"("> + ref<EvalExpr> + lit<")">) | ref<Expr>> {};
  struct EvalNot
      : rule<create<ast::ValueNegation>(kw<"NOT"> +
                 assign<&ast::ValueNegation::operand>(ref<EvalNot>)) |
             ref<EvalComparison>> {};
  struct EvalExpr
      : infix<EvalNot, ast::ValueBoolean,
              levels(left(as<ast::BoolOp::and_>(kw<"AND">)),
                     left(as<ast::BoolOp::or_>(kw<"OR">)),
                     left(as<ast::BoolOp::xor_>(kw<"XOR">)))> {};

  struct Operation;
  struct SearchOperation;

  struct LikeTest
      : rule<create<ast::LikeTest>(assign<&ast::LikeTest::subject>(ref<Expr>) + ref<LIKE_KW> +
                                   assign<&ast::LikeTest::pattern>(ref<Expr>))> {};

  struct InTest
      : rule<create<ast::InTest>(
            assign<&ast::InTest::subject>(ref<Expr>) + ref<IN_KW> + lit<"("> +
            option(some(append<&ast::InTest::options>(ref<Expr>), lit<",">)) + lit<")">)> {};

  static constexpr auto LookupChar = uc::L | uc::M | uc::N | cr<"_.:-"> | lit<"/", !cr<"/*">>;
  struct LookupWord : terminal<some(LookupChar)> {};
  struct FieldWord : terminal<!((ikw<"AS"> | ikw<"OUTPUT"> | ikw<"OUTPUTNEW">) + !LookupChar) +
      some(LookupChar)> {};
  static constexpr auto LookupName = ref<STRING> | ref<FieldWord>;
  struct LookupField : rule<assign<&ast::LookupField::tableField>(LookupName) +
      option(!gap::none + ref<AS_KW> + !gap::none + assign<&ast::LookupField::eventField>(LookupName))> {};

  template <auto Fields>
  static constexpr auto lookup_fields =
      some(append<Fields>(ref<LookupField>), lit<","> | !gap::none);
  template <auto Value>
  static constexpr auto scalar = Value | (lit<"\""> + gap::none + Value + gap::none + lit<"\"">);
  static constexpr auto TrueWord = ikw<"true"> | ikw<"t"> | ikw<"yes"> | kw<"1">;
  static constexpr auto FalseWord = ikw<"false"> | ikw<"f"> | ikw<"no"> | kw<"0">;
  template <auto Word>
  static constexpr auto quotedWord = Word | (lit<"\""> + Word + lit<"\"">);
  struct TrueOption : terminal<quotedWord<TrueWord>> {};
  struct FalseOption : terminal<quotedWord<FalseWord>> {};
  static constexpr auto LookupBool = as<true>(ref<TrueOption>) | as<false>(ref<FalseOption>);

  struct Lookup : rule<unordered(
      option(ikw<"local"> + lit<"="> + assign<&ast::Lookup::local>(LookupBool) + !gap::none),
      option(ikw<"update"> + lit<"="> + assign<&ast::Lookup::update>(LookupBool) + !gap::none)) +
      assign<&ast::Lookup::table>(ref<LookupWord> | ref<STRING>) + !gap::none +
      lookup_fields<&ast::Lookup::inputs> +
      option(!gap::none + (ref<OUTPUT_KW> | flag<&ast::Lookup::outputNew>(ref<OUTPUTNEW_KW>)) + !gap::none +
             lookup_fields<&ast::Lookup::outputs>)> {};

  template <class Command>
  static constexpr auto lookup_command =
      assign<&Command::commandName>(ref<LOOKUP_KW>) + !gap::none + append<&Command::operations>(ref<Lookup>);

  template <auto Key, auto Value>
  struct Parameter : rule<create<ast::Comparison>(assign<&ast::Comparison::field>(Key) +
      assign<&ast::Comparison::op>(as<ast::CompareOp::eq>(lit<"=">)) +
      assign<&ast::Comparison::value>(Value))> {};

  static constexpr auto dayMonth = d + option(d);
  struct Calendar : terminal<dayMonth + lit<"/"> + dayMonth + lit<"/"> +
      repeat<2>(d) + option(repeat<2>(d)) +
      option(lit<":"> + dayMonth + lit<":"> + repeat<2>(d) + lit<":"> + repeat<2>(d))> {};
  struct Interval : terminal<option(cr<"+-">) + some(d) + cr<"smhd">> {};
  struct Integer : terminal<as<double>(some(d))> {};
  struct IntegerArg : rule<create<ast::NumberLit>(
      option((flag<&ast::NumberLit::negative>(lit<"-">) | lit<"+">) + gap::none) +
      assign<&ast::NumberLit::value>(ref<Integer>))> {};
  static constexpr auto timestampArg = ref<IntegerArg> |
      create<ast::DateLit>(assign<&ast::DateLit::text>(ref<Calendar>));
  static constexpr auto intervalArg = create<ast::DateLit>(assign<&ast::DateLit::text>(ref<Interval>));
  template <class Command, auto Key, auto Value>
  static constexpr auto parameter = !gap::none + append<&Command::operations>(ref<Parameter<Key, Value>>);
  template <class Command>
  static constexpr auto gentimes = assign<&Command::commandName>(ref<GENTIMES>) + unordered(
      parameter<Command, ikw<"start">, scalar<timestampArg>>,
      option(parameter<Command, ikw<"end">, scalar<timestampArg>>),
      option(parameter<Command, ikw<"increment">, scalar<intervalArg>>));

  static constexpr auto argEscape = lit<"\\"> + any;
  struct ArgWord : terminal<some(cr<R"(^ \t\r\n\[\]|"\\)"> | argEscape |
      (lit<"["> + many(cr<R"(^\]\\\r\n)"> | argEscape) + lit<"]">))> {};
  static constexpr auto textArg = ref<StringLit> |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<ArgWord>));
  struct ArgBool : rule<create<ast::NameExpr>(assign<&ast::NameExpr::name>(TrueWord | FalseWord))> {};
  struct ResourcePart : terminal<some(cr<R"(^/ \t\r\n\[\]|=")">)> {};
  struct ResourcePath : rule<create<ast::PathExpr>(lit<"/"> + option(gap::none + some(
          append<&ast::PathExpr::segments>(ref<ResourcePart>), gap::none + lit<"/"> + gap::none) +
          option(gap::none + lit<"/">)))> {};
  struct Resource : rule<create<ast::BareExpr>(assign<&ast::BareExpr::value>(scalar<ref<ResourcePath>>))> {};
  struct RestOption : rule<
      ref<Parameter<ikw<"count"> | ikw<"timeout">, scalar<ref<IntegerArg>>>> |
      ref<Parameter<ikw<"strict">, scalar<ref<ArgBool>>>> |
      ref<Parameter<!(ikw<"count"> | ikw<"timeout"> | ikw<"strict">) + ref<IDENTIFIER>, textArg>>> {};
  template <class Command>
  static constexpr auto rest = assign<&Command::commandName>(ref<REST>) + !gap::none + many(append<&Command::operations>(ref<RestOption>) + !gap::none) +
      append<&Command::operations>(ref<Resource>) + many(!gap::none + append<&Command::operations>(ref<RestOption>));
  struct IntelligenceOption : rule<
      ref<Parameter<ikw<"fields"> | ikw<"delim_regex"> | ikw<"extract_regex"> | ikw<"ignore_regex">, textArg>> |
      ref<Parameter<ikw<"skip_header_lines">, scalar<ref<IntegerArg>>>> |
      ref<Parameter<ikw<"include_raw"> | ikw<"append"> | ikw<"no_parse">, scalar<ref<ArgBool>>>>> {};
  template <class Command>
  static constexpr auto intelligence = assign<&Command::commandName>(ref<INPUTINTELLIGENCE>) + !gap::none +
      append<&Command::operations>(create<ast::BareExpr>(assign<&ast::BareExpr::value>(ref<StringLit> |
          create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<LookupWord>))))) +
      many(!gap::none + append<&Command::operations>(ref<IntelligenceOption>));
  struct ByClause
      : rule<create<ast::ByClause>(ref<BY_KW> +
                                   some(append<&ast::ByClause::fields>(IdWord)))> {};

  struct Alias
      : rule<create<ast::Alias>(assign<&ast::Alias::value>(ref<Expr>) + ref<AS_KW> +
                                assign<&ast::Alias::name>(IdWord))> {};

  struct Comparison
      : rule<create<ast::Comparison>(
            assign<&ast::Comparison::field>(IdWord) +
            assign<&ast::Comparison::op>(CompareOperator) +
            assign<&ast::Comparison::value>(ref<Expr>))> {};

  struct BareExpr
      : rule<assign<&ast::BareExpr::value>(ref<Expr>)> {};

  static constexpr auto SimpleOperation =
      ref<LikeTest> | ref<InTest> | ref<ByClause> |
      ref<Alias> | ref<Comparison> | ref<BareExpr>;
  struct AtomOp : rule<SimpleOperation |
             (lit<"("> + ref<Operation> + lit<")">)> {};
  struct SearchAtom : rule<SimpleOperation | (lit<"("> + ref<SearchOperation> +
      many(nest<ast::BoolCombine, &ast::BoolCombine::left>(
          assign<&ast::BoolCombine::op>(as<ast::BoolOp::and_>(!gap::none)) +
          assign<&ast::BoolCombine::right>(ref<SearchOperation>))) + lit<")">)> {};
  struct Not
      : rule<create<ast::Negation>(ref<NOT_KW> +
                 assign<&ast::Negation::operand>(ref<Not>)) | ref<AtomOp>> {};
  struct SearchNot
      : rule<create<ast::Negation>(ref<NOT_KW> +
                 assign<&ast::Negation::operand>(ref<SearchNot>)) | ref<SearchAtom>> {};
  struct Operation
      : infix<Not, ast::BoolCombine,
              levels(left(as<ast::BoolOp::and_>(kw<"AND">)),
                     left(as<ast::BoolOp::or_>(kw<"OR">)),
                     left(as<ast::BoolOp::xor_>(kw<"XOR">)))> {};
  // Search evaluates OR before AND. Juxtaposed vector entries remain implicit AND.
  struct SearchOperation
      : infix<SearchNot, ast::BoolCombine,
              levels(left(as<ast::BoolOp::or_>(kw<"OR">)),
                     left(as<ast::BoolOp::and_>(kw<"AND">)))> {};

  struct PrefixText : terminal<some(cr<R"(^() \t\r\n|")">)> {};
  static constexpr auto statField = ref<StringLit> |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<LookupWord>));
  struct Prefix : rule<create<ast::FuncCall>(assign<&ast::FuncCall::name>(ikw<"prefix">) + lit<"("> +
      append<&ast::FuncCall::args>(ref<StringLit> |
          create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<PrefixText>))) + lit<")">)> {};
  struct AggregateValue : rule<create<ast::FuncCall>(
      assign<&ast::FuncCall::name>(CountWord) +
      ((lit<"("> + lit<")">) | (!gap::none + !(ikw<"from"> | ikw<"where"> | ref<BY_KW> | ref<AS_KW>) +
       append<&ast::FuncCall::args>(statField)))) | create<ast::FuncCall>(
      assign<&ast::FuncCall::name>(ref<StatFunction>) + lit<"("> +
      append<&ast::FuncCall::args>(ref<Prefix> | statField) + lit<")">) |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(CountWord))> {};
  struct Aggregate : rule<create<ast::Alias>(assign<&ast::Alias::value>(ref<AggregateValue>) +
      ref<AS_KW> + assign<&ast::Alias::name>(ref<LookupWord> | ref<STRING>)) |
      create<ast::BareExpr>(assign<&ast::BareExpr::value>(ref<AggregateValue>))> {};
  template <auto K>
  struct ClauseWord : rule<create<ast::BareExpr>(assign<&ast::BareExpr::value>(
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(K))))> {};
  struct GroupPrefix : terminal<ikw<"prefix"> + lit<"("> +
      (ref<PrefixText> | ref<STRING>) + lit<")">> {};
  struct StatBy : rule<create<ast::ByClause>(ref<BY_KW> +
      some(append<&ast::ByClause::fields>(ref<GroupPrefix> | ref<LookupWord> | ref<STRING>),
           lit<","> | !gap::none))> {};
  struct Span : terminal<ikw<"auto"> | (some(d) + SpanUnit)> {};
  struct StatOption : rule<
      ref<Parameter<ikw<"prestats"> | ikw<"local"> | ikw<"append"> | ikw<"summariesonly"> |
          ikw<"include_reduced_buckets"> | ikw<"allow_old_summaries">, scalar<ref<ArgBool>>>> |
      ref<Parameter<ikw<"chunk_size">, scalar<ref<IntegerArg>>>> |
      ref<Parameter<ikw<"fillnull_value">, textArg>>> {};
  template <class Command>
  static constexpr auto tstats = assign<&Command::commandName>(ref<TSTATS>) + many(!gap::none + append<&Command::operations>(ref<StatOption>)) +
      !gap::none + some(append<&Command::operations>(ref<Aggregate>), lit<","> | !gap::none) +
      option(!gap::none + append<&Command::operations>(ref<ClauseWord<ikw<"from">>>) +
             parameter<Command, ikw<"datamodel">, statField>) +
      option(!gap::none + append<&Command::operations>(ref<ClauseWord<ikw<"where">>>) + !gap::none +
             some(!ref<BY_KW> + append<&Command::operations>(ref<SearchOperation>))) +
      option(!gap::none + append<&Command::operations>(ref<StatBy>) +
             option(parameter<Command, ikw<"span">, create<ast::DateLit>(assign<&ast::DateLit::text>(ref<Span>))>)) +
      many(!gap::none + append<&Command::operations>(ref<StatOption>));
  struct InteriorStars : terminal<some(LookupChar) + some(some(lit<"*">) + some(LookupChar))> {};
  static constexpr auto fieldName = create<ast::NameExpr>(
      assign<&ast::NameExpr::name>(ref<LookupWord> | ref<InteriorStars>));
  struct PlainField : terminal<!lit<"-", !LookupChar> + some(LookupChar)> {};
  static constexpr auto fieldPattern = ref<StringLit> |
      create<ast::Glob>(flag<&ast::Glob::leadingStar>(some(lit<"*">)) +
          option(gap::none + assign<&ast::Glob::inner>(fieldName)) +
          option(gap::none + flag<&ast::Glob::trailingStar>(some(lit<"*">)))) |
      create<ast::Glob>(assign<&ast::Glob::inner>(fieldName) + gap::none +
          flag<&ast::Glob::trailingStar>(some(lit<"*">))) |
      create<ast::Glob>(assign<&ast::Glob::inner>(create<ast::NameExpr>(
          assign<&ast::NameExpr::name>(ref<InteriorStars>)))) |
      create<ast::NameExpr>(assign<&ast::NameExpr::name>(ref<PlainField>));
  template <class Command>
  static constexpr auto fields = assign<&Command::commandName>(ref<FIELDS_KW>) + !gap::none +
      option(append<&Command::operations>(ref<ClauseWord<lit<"+"> | lit<"-">>>) + !gap::none) +
      some(append<&Command::operations>(create<ast::BareExpr>(assign<&ast::BareExpr::value>(fieldPattern))),
           lit<","> | !gap::none);
  template <class Command>
  static constexpr auto generating = gentimes<Command> | rest<Command> | intelligence<Command> | tstats<Command>;

  struct InitCommand;
  struct NextCommand;

  struct Subquery
      : rule<lit<"["> + assign<&ast::Subquery::init>(ref<InitCommand>) +
             many(lit<"|"> + append<&ast::Subquery::pipeline>(ref<NextCommand>)) + lit<"]">> {
  };

  struct InitCommand
      : rule<(lit<"|"> + (lookup_command<ast::InitCommand> | fields<ast::InitCommand>)) |
             (option(lit<"|">) + generating<ast::InitCommand>) |
             (option(lit<"|"> + !PipelineWord) +
             ((!(ref<INIT_COMMAND> | GeneratingCommand) + option(assign<&ast::InitCommand::commandName>(ref<SEARCH_COMMAND>)) +
               some(append<&ast::InitCommand::operations>(ref<SearchOperation>))) |
              (assign<&ast::InitCommand::commandName>(ref<INIT_COMMAND>) +
               many(append<&ast::InitCommand::operations>(ref<Operation>)))) +
             option(assign<&ast::InitCommand::subquery>(ref<Subquery>)))> {};
  struct NextCommand
      : rule<lookup_command<ast::NextCommand> | fields<ast::NextCommand> | generating<ast::NextCommand> |
             (((assign<&ast::NextCommand::commandName>(ref<SEARCH_COMMAND>) +
               some(append<&ast::NextCommand::operations>(ref<SearchOperation>))) |
              (assign<&ast::NextCommand::commandName>(GenericCommand) +
               many(append<&ast::NextCommand::operations>(ref<Operation>)))) +
             option(assign<&ast::NextCommand::subquery>(ref<Subquery>)))> {};

  struct Query
      : rule<assign<&ast::Query::init>(ref<InitCommand>) +
             many(lit<"|"> + append<&ast::Query::pipeline>(ref<NextCommand>))> {};

  using entry = Query;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace spl

