// The Amazon States Language grammar: a JSON document whose object keys are
// a fixed vocabulary of quoted field names -- each its own terminal, so the
// parser dispatches on the key directly; per-state lexing offers only the
// keys legal at that position, and ties resolve to the earliest
// declaration among those.
#pragma once

#include <cstdint>

#include "amazonstateslanguage/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace amazonstateslanguage {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct AmazonStatesLanguage : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};

  static constexpr auto escape =
      lit<"\\"> + (cr<R"("\\/bfnrt)"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  static constexpr auto safe_codepoint = cr<R"(^"\\\x00-\x1f)">;

  // STRINGDOLLAR, STRINGPATHCONTEXTOBJ, STRINGPATH and STRING can tie on
  // the same input; every position that admits more than one of them also
  // admits `keyword_or_string` as a whole, so which one a tie resolves to
  // never changes the value read.
  struct STRINGDOLLAR
      : terminal<lit<"\""> + many(escape | safe_codepoint) + lit<".$\"">,
                 Unquoted> {};
  struct STRINGPATHCONTEXTOBJ
      : terminal<lit<"\"$$"> + many(escape | safe_codepoint) + lit<"\"">,
                 Unquoted> {};
  struct STRINGPATH
      : terminal<lit<"\"$"> + many(escape | safe_codepoint) + lit<"\"">,
                 Unquoted> {};
  struct STRING
      : terminal<lit<"\""> + many(escape | safe_codepoint) + lit<"\"">,
                 Unquoted> {};

  static constexpr auto int_body = lit<"0"> | (cr<"1-9"> + many(d));
  struct INT : terminal<int_body, std::int64_t> {};

  static constexpr auto frac_body = lit<"."> + some(d);
  static constexpr auto exp_body = cr<"Ee"> + option(cr<"+-">) + some(d);
  // NUMBER overlaps INT on a bare unsigned integer (both match "5"); a
  // leading '-' or a fraction/exponent makes it strictly longer, which is
  // what wins it the token wherever both are legal (`INT | NUMBER`), so a
  // bare integer still lexes as INT.
  struct NUMBER
      : terminal<(lit<"-"> + int_body + option(frac_body) + option(exp_body)) |
                     (int_body + ((frac_body + option(exp_body)) | exp_body)),
                 double> {};

  struct TRUEK : terminal<kw<"true">, value<true>> {};
  struct FALSEK : terminal<kw<"false">, value<false>> {};
  struct NULLK : terminal<kw<"null">> {};

  // The field-name vocabulary: every quoted key ASL defines, one terminal
  // per key, unquoted the same way a plain STRING is.
  struct COMMENT : terminal<lit<"\"Comment\"">, Unquoted> {};
  struct STATES : terminal<lit<"\"States\"">, Unquoted> {};
  struct STARTAT : terminal<lit<"\"StartAt\"">, Unquoted> {};
  struct VERSION : terminal<lit<"\"Version\"">, Unquoted> {};
  struct TYPE : terminal<lit<"\"Type\"">, Unquoted> {};
  struct TASK : terminal<lit<"\"Task\"">, Unquoted> {};
  struct CHOICE : terminal<lit<"\"Choice\"">, Unquoted> {};
  struct FAIL : terminal<lit<"\"Fail\"">, Unquoted> {};
  struct SUCCEED : terminal<lit<"\"Succeed\"">, Unquoted> {};
  struct PASS : terminal<lit<"\"Pass\"">, Unquoted> {};
  struct WAIT : terminal<lit<"\"Wait\"">, Unquoted> {};
  struct PARALLEL : terminal<lit<"\"Parallel\"">, Unquoted> {};
  struct MAP : terminal<lit<"\"Map\"">, Unquoted> {};
  struct CHOICES : terminal<lit<"\"Choices\"">, Unquoted> {};
  struct VARIABLE : terminal<lit<"\"Variable\"">, Unquoted> {};
  struct DEFAULT : terminal<lit<"\"Default\"">, Unquoted> {};
  struct BRANCHES : terminal<lit<"\"Branches\"">, Unquoted> {};
  struct AND : terminal<lit<"\"And\"">, Unquoted> {};
  struct BOOLEANEQUALS : terminal<lit<"\"BooleanEquals\"">, Unquoted> {};
  struct BOOLEANQUALSPATH : terminal<lit<"\"BooleanEqualsPath\"">, Unquoted> {};
  struct ISBOOLEAN : terminal<lit<"\"IsBoolean\"">, Unquoted> {};
  struct ISNULL : terminal<lit<"\"IsNull\"">, Unquoted> {};
  struct ISNUMERIC : terminal<lit<"\"IsNumeric\"">, Unquoted> {};
  struct ISPRESENT : terminal<lit<"\"IsPresent\"">, Unquoted> {};
  struct ISSTRING : terminal<lit<"\"IsString\"">, Unquoted> {};
  struct ISTIMESTAMP : terminal<lit<"\"IsTimestamp\"">, Unquoted> {};
  struct NOT : terminal<lit<"\"Not\"">, Unquoted> {};
  struct NUMERICEQUALS : terminal<lit<"\"NumericEquals\"">, Unquoted> {};
  struct NUMERICEQUALSPATH : terminal<lit<"\"NumericEqualsPath\"">, Unquoted> {};
  struct NUMERICGREATERTHAN : terminal<lit<"\"NumericGreaterThan\"">, Unquoted> {};
  struct NUMERICGREATERTHANPATH
      : terminal<lit<"\"NumericGreaterThanPath\"">, Unquoted> {};
  struct NUMERICGREATERTHANEQUALS
      : terminal<lit<"\"NumericGreaterThanEquals\"">, Unquoted> {};
  struct NUMERICGREATERTHANEQUALSPATH
      : terminal<lit<"\"NumericGreaterThanEqualsPath\"">, Unquoted> {};
  struct NUMERICLESSTHAN : terminal<lit<"\"NumericLessThan\"">, Unquoted> {};
  struct NUMERICLESSTHANPATH
      : terminal<lit<"\"NumericLessThanPath\"">, Unquoted> {};
  struct NUMERICLESSTHANEQUALS
      : terminal<lit<"\"NumericLessThanEquals\"">, Unquoted> {};
  struct NUMERICLESSTHANEQUALSPATH
      : terminal<lit<"\"NumericLessThanEqualsPath\"">, Unquoted> {};
  struct OR : terminal<lit<"\"Or\"">, Unquoted> {};
  struct STRINGEQUALS : terminal<lit<"\"StringEquals\"">, Unquoted> {};
  struct STRINGEQUALSPATH : terminal<lit<"\"StringEqualsPath\"">, Unquoted> {};
  struct STRINGGREATERTHAN : terminal<lit<"\"StringGreaterThan\"">, Unquoted> {};
  struct STRINGGREATERTHANPATH
      : terminal<lit<"\"StringGreaterThanPath\"">, Unquoted> {};
  struct STRINGGREATERTHANEQUALS
      : terminal<lit<"\"StringGreaterThanEquals\"">, Unquoted> {};
  struct STRINGGREATERTHANEQUALSPATH
      : terminal<lit<"\"StringGreaterThanEqualsPath\"">, Unquoted> {};
  struct STRINGLESSTHAN : terminal<lit<"\"StringLessThan\"">, Unquoted> {};
  struct STRINGLESSTHANPATH
      : terminal<lit<"\"StringLessThanPath\"">, Unquoted> {};
  struct STRINGLESSTHANEQUALS
      : terminal<lit<"\"StringLessThanEquals\"">, Unquoted> {};
  struct STRINGLESSTHANEQUALSPATH
      : terminal<lit<"\"StringLessThanEqualsPath\"">, Unquoted> {};
  struct STRINGMATCHES : terminal<lit<"\"StringMatches\"">, Unquoted> {};
  struct TIMESTAMPEQUALS : terminal<lit<"\"TimestampEquals\"">, Unquoted> {};
  struct TIMESTAMPEQUALSPATH
      : terminal<lit<"\"TimestampEqualsPath\"">, Unquoted> {};
  struct TIMESTAMPGREATERTHAN
      : terminal<lit<"\"TimestampGreaterThan\"">, Unquoted> {};
  struct TIMESTAMPGREATERTHANPATH
      : terminal<lit<"\"TimestampGreaterThanPath\"">, Unquoted> {};
  struct TIMESTAMPGREATERTHANEQUALS
      : terminal<lit<"\"TimestampGreaterThanEquals\"">, Unquoted> {};
  struct TIMESTAMPGREATERTHANEQUALSPATH
      : terminal<lit<"\"TimestampGreaterThanEqualsPath\"">, Unquoted> {};
  struct TIMESTAMPLESSTHAN : terminal<lit<"\"TimestampLessThan\"">, Unquoted> {};
  struct TIMESTAMPLESSTHANPATH
      : terminal<lit<"\"TimestampLessThanPath\"">, Unquoted> {};
  struct TIMESTAMPLESSTHANEQUALS
      : terminal<lit<"\"TimestampLessThanEquals\"">, Unquoted> {};
  struct TIMESTAMPLESSTHANEQUALSPATH
      : terminal<lit<"\"TimestampLessThanEqualsPath\"">, Unquoted> {};
  struct SECONDSPATH : terminal<lit<"\"SecondsPath\"">, Unquoted> {};
  struct SECONDS : terminal<lit<"\"Seconds\"">, Unquoted> {};
  struct TIMESTAMPPATH : terminal<lit<"\"TimestampPath\"">, Unquoted> {};
  struct TIMESTAMP : terminal<lit<"\"Timestamp\"">, Unquoted> {};
  struct TIMEOUTSECONDS : terminal<lit<"\"TimeoutSeconds\"">, Unquoted> {};
  struct TIMEOUTSECONDSPATH
      : terminal<lit<"\"TimeoutSecondsPath\"">, Unquoted> {};
  struct HEARTBEATSECONDS : terminal<lit<"\"HeartbeatSeconds\"">, Unquoted> {};
  struct HEARTBEATSECONDSPATH
      : terminal<lit<"\"HeartbeatSecondsPath\"">, Unquoted> {};
  struct PROCESSORCONFIG : terminal<lit<"\"ProcessorConfig\"">, Unquoted> {};
  struct MODE : terminal<lit<"\"Mode\"">, Unquoted> {};
  struct INLINE : terminal<lit<"\"INLINE\"">, Unquoted> {};
  struct DISTRIBUTED : terminal<lit<"\"DISTRIBUTED\"">, Unquoted> {};
  struct EXECUTIONTYPE : terminal<lit<"\"ExecutionType\"">, Unquoted> {};
  struct STANDARD : terminal<lit<"\"STANDARD\"">, Unquoted> {};
  struct ITEMPROCESSOR : terminal<lit<"\"ItemProcessor\"">, Unquoted> {};
  struct ITERATOR : terminal<lit<"\"Iterator\"">, Unquoted> {};
  struct ITEMSELECTOR : terminal<lit<"\"ItemSelector\"">, Unquoted> {};
  struct MAXCONCURRENCY : terminal<lit<"\"MaxConcurrency\"">, Unquoted> {};
  struct RESOURCE : terminal<lit<"\"Resource\"">, Unquoted> {};
  struct INPUTPATH : terminal<lit<"\"InputPath\"">, Unquoted> {};
  struct OUTPUTPATH : terminal<lit<"\"OutputPath\"">, Unquoted> {};
  struct ITEMSPATH : terminal<lit<"\"ItemsPath\"">, Unquoted> {};
  struct RESULTPATH : terminal<lit<"\"ResultPath\"">, Unquoted> {};
  struct RESULT : terminal<lit<"\"Result\"">, Unquoted> {};
  struct PARAMETERS : terminal<lit<"\"Parameters\"">, Unquoted> {};
  struct RESULTSELECTOR : terminal<lit<"\"ResultSelector\"">, Unquoted> {};
  struct ITEMREADER : terminal<lit<"\"ItemReader\"">, Unquoted> {};
  struct READERCONFIG : terminal<lit<"\"ReaderConfig\"">, Unquoted> {};
  struct INPUTTYPE : terminal<lit<"\"InputType\"">, Unquoted> {};
  struct CSVHEADERLOCATION
      : terminal<lit<"\"CSVHeaderLocation\"">, Unquoted> {};
  struct CSVHEADERS : terminal<lit<"\"CSVHeaders\"">, Unquoted> {};
  struct MAXITEMS : terminal<lit<"\"MaxItems\"">, Unquoted> {};
  struct MAXITEMSPATH : terminal<lit<"\"MaxItemsPath\"">, Unquoted> {};
  struct NEXT : terminal<lit<"\"Next\"">, Unquoted> {};
  struct END : terminal<lit<"\"End\"">, Unquoted> {};
  struct CAUSE : terminal<lit<"\"Cause\"">, Unquoted> {};
  struct CAUSEPATH : terminal<lit<"\"CausePath\"">, Unquoted> {};
  struct ERROR : terminal<lit<"\"Error\"">, Unquoted> {};
  struct ERRORPATH : terminal<lit<"\"ErrorPath\"">, Unquoted> {};
  struct RETRY : terminal<lit<"\"Retry\"">, Unquoted> {};
  struct ERROREQUALS : terminal<lit<"\"ErrorEquals\"">, Unquoted> {};
  struct INTERVALSECONDS : terminal<lit<"\"IntervalSeconds\"">, Unquoted> {};
  struct MAXATTEMPTS : terminal<lit<"\"MaxAttempts\"">, Unquoted> {};
  struct BACKOFFRATE : terminal<lit<"\"BackoffRate\"">, Unquoted> {};
  struct MAXDELAYSECONDS : terminal<lit<"\"MaxDelaySeconds\"">, Unquoted> {};
  struct JITTERSTRATEGY : terminal<lit<"\"JitterStrategy\"">, Unquoted> {};
  struct FULL : terminal<lit<"\"FULL\"">, Unquoted> {};
  struct NONE : terminal<lit<"\"NONE\"">, Unquoted> {};
  struct CATCH : terminal<lit<"\"Catch\"">, Unquoted> {};
  struct ERRORNAMEStatesALL : terminal<lit<"\"States.ALL\"">, Unquoted> {};
  struct ERRORNAMEStatesHeartbeatTimeout
      : terminal<lit<"\"States.HeartbeatTimeout\"">, Unquoted> {};
  struct ERRORNAMEStatesTimeout
      : terminal<lit<"\"States.Timeout\"">, Unquoted> {};
  struct ERRORNAMEStatesTaskFailed
      : terminal<lit<"\"States.TaskFailed\"">, Unquoted> {};
  struct ERRORNAMEStatesPermissions
      : terminal<lit<"\"States.Permissions\"">, Unquoted> {};
  struct ERRORNAMEStatesResultPathMatchFailure
      : terminal<lit<"\"States.ResultPathMatchFailure\"">, Unquoted> {};
  struct ERRORNAMEStatesParameterPathFailure
      : terminal<lit<"\"States.ParameterPathFailure\"">, Unquoted> {};
  struct ERRORNAMEStatesBranchFailed
      : terminal<lit<"\"States.BranchFailed\"">, Unquoted> {};
  struct ERRORNAMEStatesNoChoiceMatched
      : terminal<lit<"\"States.NoChoiceMatched\"">, Unquoted> {};
  struct ERRORNAMEStatesIntrinsicFailure
      : terminal<lit<"\"States.IntrinsicFailure\"">, Unquoted> {};
  struct ERRORNAMEStatesExceedToleratedFailureThreshold
      : terminal<lit<"\"States.ExceedToleratedFailureThreshold\"">, Unquoted> {
  };
  struct ERRORNAMEStatesItemReaderFailed
      : terminal<lit<"\"States.ItemReaderFailed\"">, Unquoted> {};
  struct ERRORNAMEStatesResultWriterFailed
      : terminal<lit<"\"States.ResultWriterFailed\"">, Unquoted> {};
  struct ERRORNAMEStatesRuntime
      : terminal<lit<"\"States.Runtime\"">, Unquoted> {};

  // Any quoted string: a general STRING, a path/context/binding-key
  // spelling, or one of the field-name keywords, which lex to their own
  // terminal even where a plain string is expected instead.
  // A named rule, not a bare expression: spliced at ~30 call sites, the
  // ~120-branch choice must compile once, not once per splice.
  struct KeywordOrStringRule
      : rule<ref<STRINGDOLLAR> | ref<STRINGPATHCONTEXTOBJ> | ref<STRINGPATH> |
      ref<STRING> | ref<COMMENT> | ref<STATES> | ref<STARTAT> | ref<TYPE> |
      ref<TASK> | ref<CHOICE> | ref<FAIL> | ref<SUCCEED> | ref<PASS> |
      ref<WAIT> | ref<PARALLEL> | ref<MAP> | ref<CHOICES> | ref<VARIABLE> |
      ref<DEFAULT> | ref<BRANCHES> | ref<AND> | ref<BOOLEANEQUALS> |
      ref<BOOLEANQUALSPATH> | ref<ISBOOLEAN> | ref<ISNULL> | ref<ISNUMERIC> |
      ref<ISPRESENT> | ref<ISSTRING> | ref<ISTIMESTAMP> | ref<NOT> |
      ref<NUMERICEQUALS> | ref<NUMERICEQUALSPATH> | ref<NUMERICGREATERTHAN> |
      ref<NUMERICGREATERTHANPATH> | ref<NUMERICGREATERTHANEQUALS> |
      ref<NUMERICGREATERTHANEQUALSPATH> | ref<NUMERICLESSTHAN> |
      ref<NUMERICLESSTHANPATH> | ref<NUMERICLESSTHANEQUALS> |
      ref<NUMERICLESSTHANEQUALSPATH> | ref<OR> | ref<STRINGEQUALS> |
      ref<STRINGEQUALSPATH> | ref<STRINGGREATERTHAN> |
      ref<STRINGGREATERTHANPATH> | ref<STRINGGREATERTHANEQUALS> |
      ref<STRINGGREATERTHANEQUALSPATH> | ref<STRINGLESSTHAN> |
      ref<STRINGLESSTHANPATH> | ref<STRINGLESSTHANEQUALS> |
      ref<STRINGLESSTHANEQUALSPATH> | ref<STRINGMATCHES> |
      ref<TIMESTAMPEQUALS> | ref<TIMESTAMPEQUALSPATH> |
      ref<TIMESTAMPGREATERTHAN> | ref<TIMESTAMPGREATERTHANPATH> |
      ref<TIMESTAMPGREATERTHANEQUALS> | ref<TIMESTAMPGREATERTHANEQUALSPATH> |
      ref<TIMESTAMPLESSTHAN> | ref<TIMESTAMPLESSTHANPATH> |
      ref<TIMESTAMPLESSTHANEQUALS> | ref<TIMESTAMPLESSTHANEQUALSPATH> |
      ref<SECONDSPATH> | ref<SECONDS> | ref<TIMESTAMPPATH> | ref<TIMESTAMP> |
      ref<TIMEOUTSECONDS> | ref<TIMEOUTSECONDSPATH> | ref<HEARTBEATSECONDS> |
      ref<HEARTBEATSECONDSPATH> | ref<PROCESSORCONFIG> | ref<MODE> |
      ref<INLINE> | ref<DISTRIBUTED> | ref<EXECUTIONTYPE> | ref<STANDARD> |
      ref<ITEMPROCESSOR> | ref<ITERATOR> | ref<ITEMSELECTOR> |
      ref<MAXCONCURRENCY> | ref<RESOURCE> | ref<INPUTPATH> | ref<OUTPUTPATH> |
      ref<ITEMSPATH> | ref<RESULTPATH> | ref<RESULT> | ref<PARAMETERS> |
      ref<RESULTSELECTOR> | ref<ITEMREADER> | ref<READERCONFIG> |
      ref<INPUTTYPE> | ref<CSVHEADERLOCATION> | ref<CSVHEADERS> |
      ref<MAXITEMS> | ref<MAXITEMSPATH> | ref<NEXT> | ref<END> | ref<CAUSE> |
      ref<ERROR> | ref<RETRY> | ref<ERROREQUALS> | ref<INTERVALSECONDS> |
      ref<MAXATTEMPTS> | ref<BACKOFFRATE> | ref<MAXDELAYSECONDS> |
      ref<JITTERSTRATEGY> | ref<FULL> | ref<NONE> | ref<CATCH> |
      ref<ERRORNAMEStatesALL> | ref<ERRORNAMEStatesHeartbeatTimeout> |
      ref<ERRORNAMEStatesTimeout> | ref<ERRORNAMEStatesTaskFailed> |
      ref<ERRORNAMEStatesPermissions> |
      ref<ERRORNAMEStatesResultPathMatchFailure> |
      ref<ERRORNAMEStatesParameterPathFailure> |
      ref<ERRORNAMEStatesBranchFailed> | ref<ERRORNAMEStatesNoChoiceMatched> |
      ref<ERRORNAMEStatesIntrinsicFailure> |
      ref<ERRORNAMEStatesExceedToleratedFailureThreshold> |
      ref<ERRORNAMEStatesItemReaderFailed> |
      ref<ERRORNAMEStatesResultWriterFailed> | ref<ERRORNAMEStatesRuntime>> {
  };
  static constexpr auto keyword_or_string = ref<KeywordOrStringRule>;

  struct ComparisonOpRule
      : rule<ref<BOOLEANEQUALS> | ref<BOOLEANQUALSPATH> | ref<ISBOOLEAN> |
      ref<ISNULL> | ref<ISNUMERIC> | ref<ISPRESENT> | ref<ISSTRING> |
      ref<ISTIMESTAMP> | ref<NUMERICEQUALS> | ref<NUMERICEQUALSPATH> |
      ref<NUMERICGREATERTHAN> | ref<NUMERICGREATERTHANPATH> |
      ref<NUMERICGREATERTHANEQUALS> | ref<NUMERICGREATERTHANEQUALSPATH> |
      ref<NUMERICLESSTHAN> | ref<NUMERICLESSTHANPATH> |
      ref<NUMERICLESSTHANEQUALS> | ref<NUMERICLESSTHANEQUALSPATH> |
      ref<STRINGEQUALS> | ref<STRINGEQUALSPATH> | ref<STRINGGREATERTHAN> |
      ref<STRINGGREATERTHANPATH> | ref<STRINGGREATERTHANEQUALS> |
      ref<STRINGGREATERTHANEQUALSPATH> | ref<STRINGLESSTHAN> |
      ref<STRINGLESSTHANPATH> | ref<STRINGLESSTHANEQUALS> |
      ref<STRINGLESSTHANEQUALSPATH> | ref<STRINGMATCHES> |
      ref<TIMESTAMPEQUALS> | ref<TIMESTAMPEQUALSPATH> |
      ref<TIMESTAMPGREATERTHAN> | ref<TIMESTAMPGREATERTHANPATH> |
      ref<TIMESTAMPGREATERTHANEQUALS> | ref<TIMESTAMPGREATERTHANEQUALSPATH> |
      ref<TIMESTAMPLESSTHAN> | ref<TIMESTAMPLESSTHANPATH> |
      ref<TIMESTAMPLESSTHANEQUALS> | ref<TIMESTAMPLESSTHANEQUALSPATH>> {};
  static constexpr auto comparison_op = ref<ComparisonOpRule>;

  static constexpr auto choice_operator = ref<NOT> | ref<AND> | ref<OR>;

  static constexpr auto states_error_name =
      ref<ERRORNAMEStatesALL> | ref<ERRORNAMEStatesHeartbeatTimeout> |
      ref<ERRORNAMEStatesTimeout> | ref<ERRORNAMEStatesTaskFailed> |
      ref<ERRORNAMEStatesPermissions> |
      ref<ERRORNAMEStatesResultPathMatchFailure> |
      ref<ERRORNAMEStatesParameterPathFailure> |
      ref<ERRORNAMEStatesBranchFailed> | ref<ERRORNAMEStatesNoChoiceMatched> |
      ref<ERRORNAMEStatesIntrinsicFailure> |
      ref<ERRORNAMEStatesExceedToleratedFailureThreshold> |
      ref<ERRORNAMEStatesItemReaderFailed> |
      ref<ERRORNAMEStatesResultWriterFailed> | ref<ERRORNAMEStatesRuntime>;
  static constexpr auto error_name = states_error_name | keyword_or_string;

  static constexpr auto state_type = ref<TASK> | ref<PASS> | ref<CHOICE> |
                                     ref<FAIL> | ref<SUCCEED> | ref<WAIT> |
                                     ref<MAP> | ref<PARALLEL>;

  // The generic JSON value tree: `Result` and a Choices comparison's operand
  // may be any JSON shape, unlike a payload template (always an object).
  struct JsonObjRule;
  struct JsonArrRule;
  struct JsonBindingRule;
  static constexpr auto json_value =
      ref<JsonObjRule> | ref<JsonArrRule> | ref<NUMBER> | ref<INT> |
      ref<TRUEK> | ref<FALSEK> | ref<NULLK> | ref<JsonBindingRule> |
      keyword_or_string;

  struct JsonBindingRule
      : rule<assign<&ast::Binding::key>(keyword_or_string) + lit<":"> +
             assign<&ast::Binding::value>(json_value)> {};

  struct JsonObjRule
      : rule<create<ast::Object> + lit<"{"> +
             many(append<&ast::Object::members>(ref<JsonBindingRule>),
                  lit<",">) +
             lit<"}">> {};

  struct JsonArrRule
      : rule<create<ast::Array> + lit<"["> +
             many(append<&ast::Array::items>(json_value), lit<",">) +
             lit<"]">> {};

  // A payload template: an object whose members may bind a literal value or,
  // through a `"key.$"` name, a JSONPath / context-object path / intrinsic
  // call -- the `.$` stays part of the key text (see ast::Binding).
  struct PayloadTmplRule;
  struct PayloadArrRule;
  struct PayloadBindingRule;
  static constexpr auto payload_value =
      ref<PayloadBindingRule> | ref<PayloadArrRule> | ref<PayloadTmplRule> |
      ref<NUMBER> | ref<INT> | ref<TRUEK> | ref<FALSEK> | ref<NULLK> |
      keyword_or_string;

  struct PayloadBindingRule
      : rule<(assign<&ast::Binding::key>(ref<STRINGDOLLAR>) + lit<":"> +
              assign<&ast::Binding::value>(ref<STRINGPATH> |
                                           ref<STRINGPATHCONTEXTOBJ> |
                                           ref<STRING>)) |
             (assign<&ast::Binding::key>(keyword_or_string) + lit<":"> +
              assign<&ast::Binding::value>(payload_value))> {};

  struct PayloadTmplRule
      : rule<create<ast::Object> + lit<"{"> +
             many(append<&ast::Object::members>(ref<PayloadBindingRule>),
                  lit<",">) +
             lit<"}">> {};

  struct PayloadArrRule
      : rule<create<ast::Array> + lit<"["> +
             many(append<&ast::Array::items>(payload_value), lit<",">) +
             lit<"]">> {};

  // Objects retain every member name, including duplicates. Ordering and
  // uniqueness are runtime concerns, so each shape is a simple field list.
  static constexpr auto reader_csv_headers =
      append<&ast::ReaderConfig::members>(ref<CSVHEADERS>) + lit<":"> + lit<"["> +
      some(append<&ast::ReaderConfig::csv_headers>(keyword_or_string),
           lit<",">) +
      lit<"]">;
  static constexpr auto reader_input_type =
      append<&ast::ReaderConfig::members>(ref<INPUTTYPE>) + lit<":"> +
      append<&ast::ReaderConfig::input_type>(keyword_or_string);
  static constexpr auto reader_csv_header_location =
      append<&ast::ReaderConfig::members>(ref<CSVHEADERLOCATION>) + lit<":"> +
      append<&ast::ReaderConfig::csv_header_location>(keyword_or_string);
  static constexpr auto reader_max_items =
      append<&ast::ReaderConfig::members>(ref<MAXITEMS>) + lit<":"> +
      append<&ast::ReaderConfig::max_items>(ref<INT>);
  static constexpr auto reader_max_items_path =
      append<&ast::ReaderConfig::members>(ref<MAXITEMSPATH>) + lit<":"> +
      append<&ast::ReaderConfig::max_items_path>(ref<STRINGPATH>);

  struct ReaderConfigRule
      : rule<create<ast::ReaderConfig>(
                                lit<"{"> +
                                many((reader_csv_headers | reader_input_type |
                                      reader_csv_header_location |
                                      reader_max_items | reader_max_items_path),
                                     lit<",">) +
             lit<"}">)> {};

  static constexpr auto item_reader_resource =
      append<&ast::ItemReader::members>(ref<RESOURCE>) + lit<":"> +
      append<&ast::ItemReader::resource>(keyword_or_string);
  static constexpr auto item_reader_parameters =
      append<&ast::ItemReader::members>(ref<PARAMETERS>) + lit<":"> +
      append<&ast::ItemReader::parameters>(ref<PayloadTmplRule>);
  static constexpr auto item_reader_config =
      append<&ast::ItemReader::members>(ref<READERCONFIG>) + lit<":"> +
      append<&ast::ItemReader::reader_config>(ref<ReaderConfigRule>);

  struct ItemReaderRule
      : rule<create<ast::ItemReader>(
            lit<"{"> +
            many((item_reader_resource | item_reader_parameters |
                  item_reader_config),
                 lit<",">) +
             lit<"}">)> {};

  static constexpr auto processor_mode =
      append<&ast::ProcessorConfig::members>(ref<MODE>) + lit<":"> +
      append<&ast::ProcessorConfig::mode>(ref<INLINE> | ref<DISTRIBUTED>);
  static constexpr auto processor_execution_type =
      append<&ast::ProcessorConfig::members>(ref<EXECUTIONTYPE>) + lit<":"> +
      append<&ast::ProcessorConfig::execution_type>(ref<STANDARD>);

  struct ProcessorConfigRule
      : rule<create<ast::ProcessorConfig>(
            lit<"{"> +
            many((processor_mode | processor_execution_type), lit<",">) +
             lit<"}">)> {};

  struct RetrierRule
      : rule<lit<"{"> +
             some((ref<ERROREQUALS> + lit<":"> + lit<"["> +
                   some(append<&ast::Retrier::error_equals>(error_name),
                        lit<",">) +
                   lit<"]">) |
                      (ref<INTERVALSECONDS> + lit<":"> +
                       append<&ast::Retrier::interval_seconds>(ref<INT>)) |
                      (ref<MAXATTEMPTS> + lit<":"> +
                       append<&ast::Retrier::max_attempts>(ref<INT>)) |
                      (ref<BACKOFFRATE> + lit<":"> +
                       append<&ast::Retrier::backoff_rates>(ref<INT> |
                                                             ref<NUMBER>)) |
                      (ref<MAXDELAYSECONDS> + lit<":"> +
                       append<&ast::Retrier::max_delay_seconds>(ref<INT>)) |
                      (ref<JITTERSTRATEGY> + lit<":"> +
                       append<&ast::Retrier::jitter_strategies>(ref<FULL> |
                                                                 ref<NONE>)) |
                      (ref<COMMENT> + lit<":"> +
                       append<&ast::Retrier::comments>(keyword_or_string)),
                  lit<",">) +
             lit<"}">> {};

  static constexpr auto catcher_error_equals =
      append<&ast::Catcher::members>(ref<ERROREQUALS>) + lit<":"> + lit<"["> +
      some(append<&ast::Catcher::error_equals>(error_name), lit<",">) +
      lit<"]">;
  static constexpr auto catcher_result_path =
      append<&ast::Catcher::members>(ref<RESULTPATH>) + lit<":"> +
      (append<&ast::Catcher::result_path>(keyword_or_string) |
       flag<&ast::Catcher::result_path_null>(ref<NULLK>));
  static constexpr auto catcher_next =
      append<&ast::Catcher::members>(ref<NEXT>) + lit<":"> + append<&ast::Catcher::next>(keyword_or_string);
  static constexpr auto catcher_comment =
      append<&ast::Catcher::members>(ref<COMMENT>) + lit<":"> +
      append<&ast::Catcher::comment>(keyword_or_string);

  struct CatcherRule
      : rule<create<ast::Catcher>(
                           lit<"{"> +
                           many((catcher_error_equals | catcher_result_path |
                                 catcher_next | catcher_comment),
                                lit<",">) +
             lit<"}">)> {};

  struct ComparisonRuleRule;
  struct CompositeRuleRule;
  static constexpr auto choice_rule =
      ref<ComparisonRuleRule> | ref<CompositeRuleRule>;

  static constexpr auto choice_next =
      append<&ast::ChoiceRule::members>(ref<NEXT>) + lit<":"> + append<&ast::ChoiceRule::next>(keyword_or_string);

  static constexpr auto comparison_variable =
      append<&ast::ChoiceRule::members>(ref<VARIABLE>) + lit<":"> +
      append<&ast::ComparisonRule::variable>(keyword_or_string);
  static constexpr auto comparison_test =
      append<&ast::ComparisonRule::comparison_op>(comparison_op) + lit<":"> +
      append<&ast::ComparisonRule::comparison_value>(json_value);
  static constexpr auto comparison_comment =
      append<&ast::ChoiceRule::members>(ref<COMMENT>) + lit<":"> +
      append<&ast::ChoiceRule::comment>(keyword_or_string);
  struct ComparisonRuleRule
      : rule<create<ast::ComparisonRule>(
                                  lit<"{"> +
                                  many((comparison_variable | comparison_test |
                                        comparison_comment | choice_next),
                                       lit<",">) +
             lit<"}">)> {};

  static constexpr auto composite_operator =
      append<&ast::CompositeRule::logical_op>(choice_operator) + lit<":"> +
      (append<&ast::CompositeRule::operands>(choice_rule) |
       (lit<"["> +
        some(append<&ast::CompositeRule::operands>(choice_rule), lit<",">) +
        lit<"]">));

  struct CompositeRuleRule
      : rule<lit<"{"> + many((choice_next | comparison_comment) + lit<",">) +
             composite_operator +
             many(lit<","> +
                  (composite_operator | choice_next | comparison_comment)) + lit<"}">> {};

  struct StateDeclRule;
  struct ProgramDeclRule;
  struct ItemProcessorBodyRule;
  struct IteratorBodyRule;

  // Every field a state may carry. Each alternative opens on its own
  // keyword, so the choice below never has to look past one token.
  static constexpr auto state_stmt =
      (ref<COMMENT> + lit<":"> +
       append<&ast::State::comments>(keyword_or_string)) |
      (ref<TYPE> + lit<":"> + append<&ast::State::types>(state_type)) |
      (ref<INPUTPATH> + lit<":"> +
       (append<&ast::State::input_paths>(keyword_or_string) |
        flag<&ast::State::input_path_null>(ref<NULLK>))) |
      (ref<RESOURCE> + lit<":"> +
       append<&ast::State::resources>(keyword_or_string)) |
      (ref<NEXT> + lit<":"> + append<&ast::State::next>(keyword_or_string)) |
      (ref<RESULT> + lit<":"> + append<&ast::State::results>(json_value)) |
      (ref<RESULTPATH> + lit<":"> +
       (append<&ast::State::result_paths>(keyword_or_string) |
        flag<&ast::State::result_path_null>(ref<NULLK>))) |
      (ref<OUTPUTPATH> + lit<":"> +
       (append<&ast::State::output_paths>(keyword_or_string) |
        flag<&ast::State::output_path_null>(ref<NULLK>))) |
      (ref<END> + lit<":"> + append<&ast::State::end>(ref<TRUEK> |
                                                       ref<FALSEK>)) |
      (ref<DEFAULT> + lit<":"> +
       append<&ast::State::default_states>(keyword_or_string)) |
      (ref<CHOICES> + lit<":"> + lit<"["> +
       some(append<&ast::State::choices>(choice_rule), lit<",">) +
       lit<"]">) |
      (ref<ERROR> + lit<":"> + append<&ast::State::errors>(keyword_or_string)) |
      (ref<ERRORPATH> + lit<":"> +
       append<&ast::State::error_paths>(ref<STRINGPATH> | ref<STRING>)) |
      (ref<CAUSE> + lit<":"> + append<&ast::State::causes>(keyword_or_string)) |
      (ref<CAUSEPATH> + lit<":"> +
       append<&ast::State::cause_paths>(ref<STRINGPATH> | ref<STRING>)) |
      (ref<SECONDS> + lit<":"> + append<&ast::State::seconds>(ref<INT>)) |
      (ref<SECONDSPATH> + lit<":"> +
       append<&ast::State::seconds_paths>(keyword_or_string)) |
      (ref<TIMESTAMP> + lit<":"> +
       append<&ast::State::timestamps>(keyword_or_string)) |
      (ref<TIMESTAMPPATH> + lit<":"> +
       append<&ast::State::timestamp_paths>(keyword_or_string)) |
      (ref<ITEMSPATH> + lit<":"> +
       append<&ast::State::items_paths>(keyword_or_string)) |
      (ref<ITEMPROCESSOR> + lit<":"> +
       append<&ast::State::item_processors>(ref<ItemProcessorBodyRule>)) |
      (ref<ITERATOR> + lit<":"> +
       append<&ast::State::iterators>(ref<IteratorBodyRule>)) |
      (ref<ITEMSELECTOR> + lit<":"> +
       append<&ast::State::item_selectors>(ref<PayloadTmplRule>)) |
      (ref<ITEMREADER> + lit<":"> +
       append<&ast::State::item_readers>(ref<ItemReaderRule>)) |
      (ref<MAXCONCURRENCY> + lit<":"> +
       append<&ast::State::max_concurrencies>(ref<INT>)) |
      (ref<TIMEOUTSECONDS> + lit<":"> +
       append<&ast::State::timeout_seconds>(ref<INT>)) |
      (ref<TIMEOUTSECONDSPATH> + lit<":"> +
       append<&ast::State::timeout_seconds_paths>(ref<STRINGPATH>)) |
      (ref<HEARTBEATSECONDS> + lit<":"> +
       append<&ast::State::heartbeat_seconds>(ref<INT>)) |
      (ref<HEARTBEATSECONDSPATH> + lit<":"> +
       append<&ast::State::heartbeat_seconds_paths>(ref<STRINGPATH>)) |
      (ref<BRANCHES> + lit<":"> + lit<"["> +
       some(append<&ast::State::branches>(ref<ProgramDeclRule>), lit<",">) +
       lit<"]">) |
      (ref<PARAMETERS> + lit<":"> +
       append<&ast::State::parameters>(ref<PayloadTmplRule>)) |
      (ref<RETRY> + lit<":"> + lit<"["> +
       many(append<&ast::State::retry>(ref<RetrierRule>), lit<",">) +
       lit<"]">) |
      (ref<CATCH> + lit<":"> + lit<"["> +
       many(append<&ast::State::catch_>(ref<CatcherRule>), lit<",">) +
       lit<"]">) |
      (ref<RESULTSELECTOR> + lit<":"> +
       append<&ast::State::result_selectors>(ref<PayloadTmplRule>));

  struct StateDeclRule
      : rule<assign<&ast::State::name>(keyword_or_string) + lit<":"> +
             lit<"{"> + some(state_stmt, lit<",">) + lit<"}">> {};

  static constexpr auto body_states =
      append<&ast::Program::members>(ref<STATES>) + lit<":"> + lit<"{"> +
      some(append<&ast::Program::states>(ref<StateDeclRule>), lit<",">) +
      lit<"}">;
  static constexpr auto body_start_at =
      append<&ast::Program::members>(ref<STARTAT>) + lit<":"> +
      append<&ast::Program::start_at>(keyword_or_string);
  static constexpr auto body_comment =
      append<&ast::Program::members>(ref<COMMENT>) + lit<":"> +
      append<&ast::Program::comment>(keyword_or_string);
  static constexpr auto body_processor_config =
      append<&ast::Program::members>(ref<PROCESSORCONFIG>) + lit<":"> +
      append<&ast::Program::processor_config>(ref<ProcessorConfigRule>);

  struct ItemProcessorBodyRule
      : rule<create<ast::Program>(lit<"{"> +
                                  many((body_states | body_processor_config |
                                        body_start_at | body_comment),
                                       lit<",">) +
             lit<"}">)> {};

  struct IteratorBodyRule
      : rule<create<ast::Program>(
            lit<"{"> +
            many((body_states | body_start_at | body_comment), lit<",">) +
             lit<"}">)> {};

  static constexpr auto program_version =
      append<&ast::Program::members>(ref<VERSION>) + lit<":"> +
      append<&ast::Program::version>(keyword_or_string);
  static constexpr auto program_timeout_seconds =
      append<&ast::Program::members>(ref<TIMEOUTSECONDS>) + lit<":"> +
      append<&ast::Program::timeout_seconds>(ref<INT>);

  struct ProgramDeclRule
      : rule<create<ast::Program>(
            lit<"{"> +
            many((body_states | body_comment | program_version | body_start_at |
                  program_timeout_seconds),
                 lit<",">) +
             lit<"}">)> {};

  using entry = ProgramDeclRule;
  using ignore = WS;
};

}  // namespace amazonstateslanguage
