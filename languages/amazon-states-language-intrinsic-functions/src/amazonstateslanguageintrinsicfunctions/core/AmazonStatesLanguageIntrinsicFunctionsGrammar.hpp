// The Amazon States Language intrinsic-function grammar: one call,
// `States.<Name>(args...)`, args being scalars, `$`/`$$` paths, or nested
// calls.
#pragma once

#include "amazonstateslanguageintrinsicfunctions/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace amazonstateslanguageintrinsicfunctions {

struct AmazonStatesLanguageIntrinsicFunctions : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\n)">)> {};

  struct STRING
      : terminal<lit<"'"> +
                     many((lit<"\\"> +
                           (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">) | any)) |
                          cr<R"(^'\\\x00-\x1f)">) +
                     lit<"'">,
                 text_conv<[](std::string_view t) {
                   return t.substr(1, t.size() - 2);
                 }>> {};

  // The leading '-' is spelled inline into EXP and NUMBER too: a run of
  // digits an optional sign may precede, everywhere a sign is read.
  static constexpr auto int_body =
      option(lit<"-">) + (lit<"0"> | (cr<"1-9"> + many(d)));
  struct INT : terminal<int_body> {};

  static constexpr auto exp_body = cr<"Ee"> + option(cr<"+-">) + some(d);
  // NUMBER matches everywhere INT does too; only a fraction or an exponent
  // makes a lexeme strictly longer than INT's, which is what wins it the
  // token -- so the fraction/exponent here is not optional the way the
  // source EXP-less NUMBER reads, or INT would never win any lexeme.
  struct NUMBER : terminal<int_body + ((lit<"."> + some(d) + option(exp_body)) |
                                       exp_body),
                           std::string_view> {};

  struct BOOL : terminal<kw<"true"> | kw<"false">, bool> {};

  // A `[...]` span in a path: brackets nest, closed by the one that
  // balances them.
  struct JSON_PATH_BRACKET
      : terminal<lit<"["> +
                 many(ref<JSON_PATH_BRACKET> | (!lit<"["> + !lit<"]"> + any)) +
                 lit<"]">> {};
  static constexpr auto json_path_bracket = ref<JSON_PATH_BRACKET>;
  static constexpr auto json_path_body =
      option(json_path_bracket) +
      many(lit<"."> + many(cr<"0-9a-zA-Z_">) + option(json_path_bracket));

  struct JSON_PATH_STRING : terminal<lit<"$"> + json_path_body> {};
  struct CONTEXT_PATH_STRING : terminal<lit<"$$"> + json_path_body> {};
  struct VARIABLE : terminal<lit<"$"> + some(w) + json_path_body> {};

  struct FORMAT : terminal<kw<"Format">, value<ast::Function::format>> {};
  struct STRING_TO_JSON
      : terminal<kw<"StringToJson">, value<ast::Function::string_to_json>> {};
  struct JSON_TO_STRING
      : terminal<kw<"JsonToString">, value<ast::Function::json_to_string>> {};
  struct ARRAY_PARTITION
      : terminal<kw<"ArrayPartition">, value<ast::Function::array_partition>> {
  };
  struct ARRAY_CONTAINS
      : terminal<kw<"ArrayContains">, value<ast::Function::array_contains>> {};
  struct ARRAY_RANGE
      : terminal<kw<"ArrayRange">, value<ast::Function::array_range>> {};
  struct ARRAY_GET_ITEM
      : terminal<kw<"ArrayGetItem">, value<ast::Function::array_get_item>> {};
  struct ARRAY_LENGTH
      : terminal<kw<"ArrayLength">, value<ast::Function::array_length>> {};
  struct ARRAY_UNIQUE
      : terminal<kw<"ArrayUnique">, value<ast::Function::array_unique>> {};
  struct ARRAY : terminal<kw<"Array">, value<ast::Function::array>> {};
  struct BASE64_ENCODE
      : terminal<kw<"Base64Encode">, value<ast::Function::base64_encode>> {};
  struct BASE64_DECODE
      : terminal<kw<"Base64Decode">, value<ast::Function::base64_decode>> {};
  struct HASH : terminal<kw<"Hash">, value<ast::Function::hash>> {};
  struct JSON_MERGE
      : terminal<kw<"JsonMerge">, value<ast::Function::json_merge>> {};
  struct MATH_RANDOM
      : terminal<kw<"MathRandom">, value<ast::Function::math_random>> {};
  struct MATH_ADD : terminal<kw<"MathAdd">, value<ast::Function::math_add>> {};
  struct STRING_SPLIT
      : terminal<kw<"StringSplit">, value<ast::Function::string_split>> {};
  struct UUID : terminal<kw<"UUID">, value<ast::Function::uuid>> {};

  static constexpr auto state_fun_name =
      ref<FORMAT> | ref<STRING_TO_JSON> | ref<JSON_TO_STRING> |
      ref<ARRAY_PARTITION> | ref<ARRAY_CONTAINS> | ref<ARRAY_RANGE> |
      ref<ARRAY_GET_ITEM> | ref<ARRAY_LENGTH> | ref<ARRAY_UNIQUE> | ref<ARRAY> |
      ref<BASE64_ENCODE> | ref<BASE64_DECODE> | ref<HASH> | ref<JSON_MERGE> |
      ref<MATH_RANDOM> | ref<MATH_ADD> | ref<STRING_SPLIT> | ref<UUID>;

  struct FuncCallRule;

  struct FuncArgRule
      : rule<create<ast::StringArg>(
                 assign<&ast::StringArg::value>(ref<STRING>))             //
             | create<ast::IntArg>(assign<&ast::IntArg::value>(ref<INT>)) //
             | create<ast::FloatArg>(
                   assign<&ast::FloatArg::value>(ref<NUMBER>))               //
             | create<ast::BoolArg>(assign<&ast::BoolArg::value>(ref<BOOL>)) //
             | create<ast::ContextPathArg>(assign<&ast::ContextPathArg::path>(
                   ref<CONTEXT_PATH_STRING>)) //
             | create<ast::JsonPathArg>(
                   assign<&ast::JsonPathArg::path>(ref<JSON_PATH_STRING>)) //
             | create<ast::VariableArg>(
                   assign<&ast::VariableArg::expression>(ref<VARIABLE>)) //
             | ref<FuncCallRule>> {};

  struct FuncCallRule
      : rule<kw<"States"> + lit<"."> +
             assign<&ast::FuncCall::function>(state_fun_name) + lit<"("> +
             many(append<&ast::FuncCall::args>(ref<FuncArgRule>), lit<",">) +
             lit<")">> {};

  using entry = FuncCallRule;
  using ignore = WS;
};

} // namespace amazonstateslanguageintrinsicfunctions
