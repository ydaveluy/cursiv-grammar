// The Amazon States Language grammar: a JSON document whose object keys are
// a fixed vocabulary of quoted field names -- each its own terminal, so the
// parser dispatches on the key directly; per-state lexing offers only the
// keys legal at that position. Fixed names also accept JSON escapes.
#pragma once

#include <cstdint>

#include "amazonstateslanguage/core/ast.hpp"
#include "amazonstateslanguage/core/JsonText.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace amazonstateslanguage {

inline constexpr auto unquote = json_string_value;

struct AmazonStatesLanguage : cursiv::GrammarSpec {
  template <cursiv::str Name>
  struct Field : terminal<json_name<Name>> {};
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};

  static constexpr auto escape =
      lit<"\\"> + (cr<R"("\\/bfnrt)"> | (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)));
  static constexpr auto safe_codepoint =
      cr<R"(\x20-\x21\x23-\x5b\x5d-\u{10FFFF})">;

  struct STRING : terminal<as<unquote>(
                      lit<"\""> + many(escape | safe_codepoint) + lit<"\"">)> {
  };

  static constexpr auto int_body = lit<"0"> | (cr<"1-9"> + many(d));
  struct INT : terminal<as<std::int64_t>(int_body)> {};

  static constexpr auto frac_body = lit<"."> + some(d);
  static constexpr auto exp_body = cr<"Ee"> + option(cr<"+-">) + some(d);
  // NUMBER overlaps INT on a bare unsigned integer (both match "5"); a
  // leading '-' or a fraction/exponent makes it strictly longer, which is
  // what wins it the token wherever both are legal (`INT | NUMBER`), so a
  // bare integer still lexes as INT.
  struct NUMBER
      : terminal<as<double>(
            (lit<"-"> + int_body + option(frac_body) + option(exp_body)) |
            (int_body + ((frac_body + option(exp_body)) | exp_body)))> {};

  struct TRUEK : terminal<as<true>(kw<"true">)> {};
  struct FALSEK : terminal<as<false>(kw<"false">)> {};
  struct NULLK : terminal<as<[](std::string_view) {
    return std::monostate{};
  }>(kw<"null">)> {};

  // The field-name vocabulary: every quoted key ASL defines, one terminal
  // per key, unquoted the same way a plain STRING is.
  struct COMMENT : terminal<json_name<"Comment">> {};
  struct STATES : terminal<json_name<"States">> {};
  struct STARTAT : terminal<json_name<"StartAt">> {};
  struct VERSION : terminal<json_name<"Version">> {};
  struct TYPE : terminal<json_name<"Type">> {};
  struct TASK : terminal<json_name<"Task">> {};
  struct CHOICE : terminal<json_name<"Choice">> {};
  struct FAIL : terminal<json_name<"Fail">> {};
  struct SUCCEED : terminal<json_name<"Succeed">> {};
  struct PASS : terminal<json_name<"Pass">> {};
  struct WAIT : terminal<json_name<"Wait">> {};
  struct PARALLEL : terminal<json_name<"Parallel">> {};
  struct MAP : terminal<json_name<"Map">> {};
  struct CHOICES : terminal<json_name<"Choices">> {};
  struct VARIABLE : terminal<json_name<"Variable">> {};
  struct DEFAULT : terminal<json_name<"Default">> {};
  struct BRANCHES : terminal<json_name<"Branches">> {};
  struct AND : terminal<json_name<"And">> {};
  struct BOOLEANEQUALS : terminal<json_name<"BooleanEquals">> {};
  struct BOOLEANQUALSPATH
      : terminal<json_name<"BooleanEqualsPath">> {};
  struct ISBOOLEAN : terminal<json_name<"IsBoolean">> {};
  struct ISNULL : terminal<json_name<"IsNull">> {};
  struct ISNUMERIC : terminal<json_name<"IsNumeric">> {};
  struct ISPRESENT : terminal<json_name<"IsPresent">> {};
  struct ISSTRING : terminal<json_name<"IsString">> {};
  struct ISTIMESTAMP : terminal<json_name<"IsTimestamp">> {};
  struct NOT : terminal<json_name<"Not">> {};
  struct NUMERICEQUALS : terminal<json_name<"NumericEquals">> {};
  struct NUMERICEQUALSPATH
      : terminal<json_name<"NumericEqualsPath">> {};
  struct NUMERICGREATERTHAN
      : terminal<json_name<"NumericGreaterThan">> {};
  struct NUMERICGREATERTHANPATH
      : terminal<json_name<"NumericGreaterThanPath">> {};
  struct NUMERICGREATERTHANEQUALS
      : terminal<json_name<"NumericGreaterThanEquals">> {};
  struct NUMERICGREATERTHANEQUALSPATH
      : terminal<json_name<"NumericGreaterThanEqualsPath">> {};
  struct NUMERICLESSTHAN : terminal<json_name<"NumericLessThan">> {};
  struct NUMERICLESSTHANPATH
      : terminal<json_name<"NumericLessThanPath">> {};
  struct NUMERICLESSTHANEQUALS
      : terminal<json_name<"NumericLessThanEquals">> {};
  struct NUMERICLESSTHANEQUALSPATH
      : terminal<json_name<"NumericLessThanEqualsPath">> {};
  struct OR : terminal<json_name<"Or">> {};
  struct STRINGEQUALS : terminal<json_name<"StringEquals">> {};
  struct STRINGEQUALSPATH : terminal<json_name<"StringEqualsPath">> {
  };
  struct STRINGGREATERTHAN
      : terminal<json_name<"StringGreaterThan">> {};
  struct STRINGGREATERTHANPATH
      : terminal<json_name<"StringGreaterThanPath">> {};
  struct STRINGGREATERTHANEQUALS
      : terminal<json_name<"StringGreaterThanEquals">> {};
  struct STRINGGREATERTHANEQUALSPATH
      : terminal<json_name<"StringGreaterThanEqualsPath">> {};
  struct STRINGLESSTHAN : terminal<json_name<"StringLessThan">> {};
  struct STRINGLESSTHANPATH
      : terminal<json_name<"StringLessThanPath">> {};
  struct STRINGLESSTHANEQUALS
      : terminal<json_name<"StringLessThanEquals">> {};
  struct STRINGLESSTHANEQUALSPATH
      : terminal<json_name<"StringLessThanEqualsPath">> {};
  struct STRINGMATCHES : terminal<json_name<"StringMatches">> {};
  struct TIMESTAMPEQUALS : terminal<json_name<"TimestampEquals">> {};
  struct TIMESTAMPEQUALSPATH
      : terminal<json_name<"TimestampEqualsPath">> {};
  struct TIMESTAMPGREATERTHAN
      : terminal<json_name<"TimestampGreaterThan">> {};
  struct TIMESTAMPGREATERTHANPATH
      : terminal<json_name<"TimestampGreaterThanPath">> {};
  struct TIMESTAMPGREATERTHANEQUALS
      : terminal<json_name<"TimestampGreaterThanEquals">> {};
  struct TIMESTAMPGREATERTHANEQUALSPATH
      : terminal<json_name<"TimestampGreaterThanEqualsPath">> {};
  struct TIMESTAMPLESSTHAN
      : terminal<json_name<"TimestampLessThan">> {};
  struct TIMESTAMPLESSTHANPATH
      : terminal<json_name<"TimestampLessThanPath">> {};
  struct TIMESTAMPLESSTHANEQUALS
      : terminal<json_name<"TimestampLessThanEquals">> {};
  struct TIMESTAMPLESSTHANEQUALSPATH
      : terminal<json_name<"TimestampLessThanEqualsPath">> {};
  struct SECONDSPATH : terminal<json_name<"SecondsPath">> {};
  struct SECONDS : terminal<json_name<"Seconds">> {};
  struct TIMESTAMPPATH : terminal<json_name<"TimestampPath">> {};
  struct TIMESTAMP : terminal<json_name<"Timestamp">> {};
  struct TIMEOUTSECONDS : terminal<json_name<"TimeoutSeconds">> {};
  struct TIMEOUTSECONDSPATH
      : terminal<json_name<"TimeoutSecondsPath">> {};
  struct HEARTBEATSECONDS : terminal<json_name<"HeartbeatSeconds">> {
  };
  struct HEARTBEATSECONDSPATH
      : terminal<json_name<"HeartbeatSecondsPath">> {};
  struct PROCESSORCONFIG : terminal<json_name<"ProcessorConfig">> {};
  struct MODE : terminal<json_name<"Mode">> {};
  struct INLINE : terminal<json_name<"INLINE">> {};
  struct DISTRIBUTED : terminal<json_name<"DISTRIBUTED">> {};
  struct EXECUTIONTYPE : terminal<json_name<"ExecutionType">> {};
  struct STANDARD : terminal<json_name<"STANDARD">> {};
  struct ITEMPROCESSOR : terminal<json_name<"ItemProcessor">> {};
  struct ITERATOR : terminal<json_name<"Iterator">> {};
  struct ITEMSELECTOR : terminal<json_name<"ItemSelector">> {};
  struct MAXCONCURRENCY : terminal<json_name<"MaxConcurrency">> {};
  struct RESOURCE : terminal<json_name<"Resource">> {};
  struct INPUTPATH : terminal<json_name<"InputPath">> {};
  struct OUTPUTPATH : terminal<json_name<"OutputPath">> {};
  struct ITEMSPATH : terminal<json_name<"ItemsPath">> {};
  struct RESULTPATH : terminal<json_name<"ResultPath">> {};
  struct RESULT : terminal<json_name<"Result">> {};
  struct PARAMETERS : terminal<json_name<"Parameters">> {};
  struct RESULTSELECTOR : terminal<json_name<"ResultSelector">> {};
  struct ITEMREADER : terminal<json_name<"ItemReader">> {};
  struct READERCONFIG : terminal<json_name<"ReaderConfig">> {};
  struct INPUTTYPE : terminal<json_name<"InputType">> {};
  struct CSVHEADERLOCATION
      : terminal<json_name<"CSVHeaderLocation">> {};
  struct CSVHEADERS : terminal<json_name<"CSVHeaders">> {};
  struct MAXITEMS : terminal<json_name<"MaxItems">> {};
  struct MAXITEMSPATH : terminal<json_name<"MaxItemsPath">> {};
  struct NEXT : terminal<json_name<"Next">> {};
  struct END : terminal<json_name<"End">> {};
  struct CAUSE : terminal<json_name<"Cause">> {};
  struct CAUSEPATH : terminal<json_name<"CausePath">> {};
  struct ERROR : terminal<json_name<"Error">> {};
  struct ERRORPATH : terminal<json_name<"ErrorPath">> {};
  struct RETRY : terminal<json_name<"Retry">> {};
  struct ERROREQUALS : terminal<json_name<"ErrorEquals">> {};
  struct INTERVALSECONDS : terminal<json_name<"IntervalSeconds">> {};
  struct MAXATTEMPTS : terminal<json_name<"MaxAttempts">> {};
  struct BACKOFFRATE : terminal<json_name<"BackoffRate">> {};
  struct MAXDELAYSECONDS : terminal<json_name<"MaxDelaySeconds">> {};
  struct JITTERSTRATEGY : terminal<json_name<"JitterStrategy">> {};
  struct FULL : terminal<json_name<"FULL">> {};
  struct NONE : terminal<json_name<"NONE">> {};
  struct CATCH : terminal<json_name<"Catch">> {};
  struct ERRORNAMEStatesALL : terminal<json_name<"States.ALL">> {};
  struct ERRORNAMEStatesHeartbeatTimeout
      : terminal<json_name<"States.HeartbeatTimeout">> {};
  struct ERRORNAMEStatesTimeout
      : terminal<json_name<"States.Timeout">> {};
  struct ERRORNAMEStatesTaskFailed
      : terminal<json_name<"States.TaskFailed">> {};
  struct ERRORNAMEStatesPermissions
      : terminal<json_name<"States.Permissions">> {};
  struct ERRORNAMEStatesResultPathMatchFailure
      : terminal<json_name<"States.ResultPathMatchFailure">> {};
  struct ERRORNAMEStatesParameterPathFailure
      : terminal<json_name<"States.ParameterPathFailure">> {};
  struct ERRORNAMEStatesBranchFailed
      : terminal<json_name<"States.BranchFailed">> {};
  struct ERRORNAMEStatesNoChoiceMatched
      : terminal<json_name<"States.NoChoiceMatched">> {};
  struct ERRORNAMEStatesIntrinsicFailure
      : terminal<json_name<"States.IntrinsicFailure">> {};
  struct ERRORNAMEStatesExceedToleratedFailureThreshold
      : terminal<as<unquote>(
            lit<"\"States.ExceedToleratedFailureThreshold\"">)> {};
  struct ERRORNAMEStatesItemReaderFailed
      : terminal<json_name<"States.ItemReaderFailed">> {};
  struct ERRORNAMEStatesResultWriterFailed
      : terminal<json_name<"States.ResultWriterFailed">> {};
  struct ERRORNAMEStatesRuntime
      : terminal<json_name<"States.Runtime">> {};

  // All JSON string positions use one terminal. The fixed-name terminals
  // match its text too; Cursiv keeps these equal-category alternatives.
  static constexpr auto keyword_or_string = ref<STRING>;

  struct ComparisonOp
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
  static constexpr auto comparison_op = ref<ComparisonOp>;

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
  struct JsonObj;
  struct JsonArr;
  struct JsonBinding;
  static constexpr auto json_non_object =
      ref<JsonArr> | ref<NUMBER> | ref<INT> |
      ref<TRUEK> | ref<FALSEK> | ref<NULLK> |
      keyword_or_string;
  static constexpr auto json_value = ref<JsonObj> | json_non_object;

  struct JsonBinding
      : rule<assign<&ast::Binding::key>(keyword_or_string) + lit<":"> +
             assign<&ast::Binding::value>(json_value)> {};

  struct JsonObj
      : rule<create<ast::Object> + lit<"{"> +
             many(append<&ast::Object::members>(ref<JsonBinding>),
                  lit<",">) +
             lit<"}">> {};

  struct JsonArr
      : rule<create<ast::Array> + lit<"["> +
             many(append<&ast::Array::items>(json_value), lit<",">) +
             lit<"]">> {};

  // Payload templates are ordinary JSON objects; their `.$` keys remain
  // available to semantic validation in the shared Binding nodes.

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
      append<&ast::ReaderConfig::max_items_path>(ref<STRING>);

  struct ReaderConfigRule
      : rule<create<ast::ReaderConfig>(
                                lit<"{"> +
                                many((reader_csv_headers | reader_input_type |
                                      reader_csv_header_location |
                                      reader_max_items | reader_max_items_path |
                                      append<&ast::ReaderConfig::extensions>(
                                          ref<JsonBinding>)),
                                     lit<",">) +
             lit<"}">)> {};

  static constexpr auto item_reader_resource =
      append<&ast::ItemReader::members>(ref<RESOURCE>) + lit<":"> +
      append<&ast::ItemReader::resource>(keyword_or_string);
  static constexpr auto item_reader_parameters =
      append<&ast::ItemReader::members>(ref<PARAMETERS>) + lit<":"> +
      append<&ast::ItemReader::parameters>(ref<JsonObj>);
  static constexpr auto item_reader_config =
      append<&ast::ItemReader::members>(ref<READERCONFIG>) + lit<":"> +
      append<&ast::ItemReader::reader_config>(ref<ReaderConfigRule>);
  static constexpr auto item_reader_arguments =
      append<&ast::ItemReader::members>(ref<Field<"Arguments">>) + lit<":"> +
      append<&ast::ItemReader::arguments>(json_value);

  struct ItemReaderRule
      : rule<create<ast::ItemReader>(
            lit<"{"> +
            many((item_reader_resource | item_reader_parameters |
                  item_reader_config | item_reader_arguments),
                 lit<",">) +
             lit<"}">)> {};

  static constexpr auto processor_mode =
      append<&ast::ProcessorConfig::members>(ref<MODE>) + lit<":"> +
      append<&ast::ProcessorConfig::mode>(keyword_or_string);
  static constexpr auto processor_execution_type =
      append<&ast::ProcessorConfig::members>(ref<EXECUTIONTYPE>) + lit<":"> +
      append<&ast::ProcessorConfig::execution_type>(keyword_or_string);

  struct ProcessorConfigRule
      : rule<create<ast::ProcessorConfig>(
            lit<"{"> +
            many((processor_mode | processor_execution_type |
                  append<&ast::ProcessorConfig::extensions>(ref<JsonBinding>)),
                 lit<",">) +
             lit<"}">)> {};

  struct Retrier
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
  static constexpr auto catcher_assign =
      append<&ast::Catcher::members>(ref<Field<"Assign">>) + lit<":"> +
      append<&ast::Catcher::assignments>(ref<JsonObj>);
  static constexpr auto catcher_output =
      append<&ast::Catcher::members>(ref<Field<"Output">>) + lit<":"> +
      append<&ast::Catcher::outputs>(json_value);

  struct Catcher
      : rule<create<ast::Catcher>(
                           lit<"{"> +
                           many((catcher_error_equals | catcher_result_path |
                                 catcher_next | catcher_comment |
                                 catcher_assign | catcher_output),
                                lit<",">) +
             lit<"}">)> {};

  struct ComparisonRule;
  struct CompositeRule;
  struct JsonataChoiceRule;
  static constexpr auto choice_rule =
      ref<ComparisonRule> | ref<CompositeRule> | ref<JsonataChoiceRule>;

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
  static constexpr auto choice_assign =
      append<&ast::ChoiceRule::members>(ref<Field<"Assign">>) + lit<":"> +
      append<&ast::ChoiceRule::assignments>(ref<JsonObj>);
  static constexpr auto choice_output =
      append<&ast::ChoiceRule::members>(ref<Field<"Output">>) + lit<":"> +
      append<&ast::ChoiceRule::outputs>(json_value);
  static constexpr auto choice_common =
      comparison_comment | choice_next | choice_assign | choice_output;
  struct ComparisonRule
      : rule<create<ast::ComparisonRule>(
                                  lit<"{"> +
                                  many((comparison_variable | comparison_test |
                                        choice_common),
                                       lit<",">) +
             lit<"}">)> {};

  static constexpr auto composite_operator =
      append<&ast::CompositeRule::logical_op>(choice_operator) + lit<":"> +
      (append<&ast::CompositeRule::operands>(choice_rule) |
       (lit<"["> +
        some(append<&ast::CompositeRule::operands>(choice_rule), lit<",">) +
        lit<"]">));

  struct CompositeRule
      : rule<lit<"{"> + many(choice_common + lit<",">) +
             composite_operator +
             many(lit<","> +
                  (composite_operator | choice_common)) + lit<"}">> {};

  struct JsonataChoiceRule
      : rule<create<ast::JsonataChoiceRule>(
            lit<"{"> +
            some((append<&ast::ChoiceRule::members>(ref<Field<"Condition">>) +
                  lit<":"> +
                  append<&ast::JsonataChoiceRule::conditions>(json_value)) |
                     choice_common,
                 lit<",">) + lit<"}">)> {};

  struct StateDecl;
  struct ProgramDecl;
  struct ItemProcessorBody;
  struct IteratorBody;

  struct MapOptionName
      : rule<ref<Field<"MaxConcurrencyPath">> | ref<Field<"ItemBatcher">> |
             ref<Field<"ResultWriter">> | ref<Field<"ToleratedFailureCount">> |
             ref<Field<"ToleratedFailureCountPath">> |
             ref<Field<"ToleratedFailurePercentage">> |
             ref<Field<"ToleratedFailurePercentagePath">> |
             ref<Field<"Label">> | ref<Field<"Items">>> {};
  struct MapOption
      : rule<assign<&ast::Binding::key>(ref<MapOptionName>) + lit<":"> +
             assign<&ast::Binding::value>(json_value)> {};

  struct NumericExpressionName
      : rule<ref<SECONDS> | ref<TIMEOUTSECONDS> | ref<HEARTBEATSECONDS> |
             ref<MAXCONCURRENCY>> {};
  struct NumericExpression
      : rule<assign<&ast::Binding::key>(ref<NumericExpressionName>) + lit<":"> +
             assign<&ast::Binding::value>(keyword_or_string)> {};

  // Every field a state may carry. Each alternative opens on its own
  // keyword, so the choice below never has to look past one token.
  static constexpr auto state_stmt =
      (ref<Field<"QueryLanguage">> + lit<":"> +
       append<&ast::State::query_languages>(keyword_or_string)) |
      (ref<Field<"Assign">> + lit<":"> +
       append<&ast::State::assignments>(ref<JsonObj>)) |
      (ref<Field<"Arguments">> + lit<":"> +
       append<&ast::State::arguments>(json_value)) |
      (ref<Field<"Output">> + lit<":"> +
       append<&ast::State::outputs>(json_value)) |
      (ref<Field<"Credentials">> + lit<":"> +
       append<&ast::State::credentials>(ref<JsonObj>)) |
      append<&ast::State::map_options>(ref<MapOption>) |
      append<&ast::State::numeric_expressions>(ref<NumericExpression>) |
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
       append<&ast::State::error_paths>(ref<STRING>)) |
      (ref<CAUSE> + lit<":"> + append<&ast::State::causes>(keyword_or_string)) |
      (ref<CAUSEPATH> + lit<":"> +
       append<&ast::State::cause_paths>(ref<STRING>)) |
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
       append<&ast::State::item_processors>(ref<ItemProcessorBody>)) |
      (ref<ITERATOR> + lit<":"> +
       append<&ast::State::iterators>(ref<IteratorBody>)) |
      (ref<ITEMSELECTOR> + lit<":"> +
       (append<&ast::State::item_selectors>(ref<JsonObj>) |
        append<&ast::State::item_selector_values>(json_non_object))) |
      (ref<ITEMREADER> + lit<":"> +
       append<&ast::State::item_readers>(ref<ItemReaderRule>)) |
      (ref<MAXCONCURRENCY> + lit<":"> +
       append<&ast::State::max_concurrencies>(ref<INT>)) |
      (ref<TIMEOUTSECONDS> + lit<":"> +
       append<&ast::State::timeout_seconds>(ref<INT>)) |
      (ref<TIMEOUTSECONDSPATH> + lit<":"> +
       append<&ast::State::timeout_seconds_paths>(ref<STRING>)) |
      (ref<HEARTBEATSECONDS> + lit<":"> +
       append<&ast::State::heartbeat_seconds>(ref<INT>)) |
      (ref<HEARTBEATSECONDSPATH> + lit<":"> +
       append<&ast::State::heartbeat_seconds_paths>(ref<STRING>)) |
      (ref<BRANCHES> + lit<":"> + lit<"["> +
       some(append<&ast::State::branches>(ref<ProgramDecl>), lit<",">) +
       lit<"]">) |
      (ref<PARAMETERS> + lit<":"> +
       append<&ast::State::parameters>(ref<JsonObj>)) |
      (ref<RETRY> + lit<":"> + lit<"["> +
       many(append<&ast::State::retry>(ref<Retrier>), lit<",">) +
       lit<"]">) |
      (ref<CATCH> + lit<":"> + lit<"["> +
       many(append<&ast::State::catch_>(ref<Catcher>), lit<",">) +
       lit<"]">) |
      (ref<RESULTSELECTOR> + lit<":"> +
       append<&ast::State::result_selectors>(ref<JsonObj>));

  struct StateDecl
      : rule<assign<&ast::State::name>(keyword_or_string) + lit<":"> +
             lit<"{"> + some(state_stmt, lit<",">) + lit<"}">> {};

  static constexpr auto body_states =
      append<&ast::Program::members>(ref<STATES>) + lit<":"> + lit<"{"> +
      some(append<&ast::Program::states>(ref<StateDecl>), lit<",">) +
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
  static constexpr auto body_query_language =
      append<&ast::Program::members>(ref<Field<"QueryLanguage">>) + lit<":"> +
      append<&ast::Program::query_languages>(keyword_or_string);

  struct ItemProcessorBody
      : rule<create<ast::Program>(lit<"{"> +
                                  many((body_states | body_processor_config |
                                        body_start_at | body_comment |
                                        body_query_language),
                                       lit<",">) +
             lit<"}">)> {};

  struct IteratorBody
      : rule<create<ast::Program>(
            lit<"{"> +
            many((body_states | body_start_at | body_comment |
                  body_query_language), lit<",">) +
             lit<"}">)> {};

  static constexpr auto program_version =
      append<&ast::Program::members>(ref<VERSION>) + lit<":"> +
      append<&ast::Program::version>(keyword_or_string);
  static constexpr auto program_timeout_seconds =
      append<&ast::Program::members>(ref<TIMEOUTSECONDS>) + lit<":"> +
      append<&ast::Program::timeout_seconds>(ref<INT>);

  struct ProgramDecl
      : rule<create<ast::Program>(
            lit<"{"> +
            many((body_states | body_comment | program_version | body_start_at |
                  program_timeout_seconds | body_query_language),
                 lit<",">) +
             lit<"}">)> {};

  using entry = ProgramDecl;
  using ignored = WS;
};

}  // namespace amazonstateslanguage
