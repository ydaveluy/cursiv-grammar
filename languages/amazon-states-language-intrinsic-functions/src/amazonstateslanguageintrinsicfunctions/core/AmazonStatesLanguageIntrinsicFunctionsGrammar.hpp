// The Amazon States Language intrinsic-function grammar: one call,
// `States.<Name>(args...)`, args being scalars, `$`/`$$` paths, or nested
// calls.
#pragma once

#include "amazonstateslanguageintrinsicfunctions/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace amazonstateslanguageintrinsicfunctions {

struct AmazonStatesLanguageIntrinsicFunctions : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct STRING
      : terminal<as<[](std::string_view t) {
        return t.substr(1, t.size() - 2);
      }>(lit<"'"> +
         many((lit<"\\"> + ((lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)) | any)) |
              cr<R"(^'\\\x00-\x1f)">) +
         lit<"'">)> {};

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
  struct NUMBER
      : terminal<as<std::string_view>(
            int_body + ((lit<"."> + some(d) + option(exp_body)) | exp_body))> {
  };

  struct BOOL : terminal<as<bool>(kw<"true"> | kw<"false">)> {};

  // Quoted member names can themselves contain brackets. Their delimiters
  // must not participate in the surrounding path's bracket balance.
  static constexpr auto path_quoted =
      (lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">) |
      (lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">);
  struct JSON_PATH_BRACKET
      : terminal<lit<"["> +
                 many(path_quoted | ref<JSON_PATH_BRACKET> |
                      cr<R"(^[]'")">) +
                 lit<"]">> {};
  static constexpr auto json_path_bracket = ref<JSON_PATH_BRACKET>;
  static constexpr auto json_path_body =
      many(json_path_bracket | (lit<"."> + many(cr<"0-9a-zA-Z_">)));

  struct JSON_PATH_STRING : terminal<lit<"$"> + json_path_body> {};
  struct CONTEXT_PATH_STRING : terminal<lit<"$$"> + json_path_body> {};
  struct VARIABLE : terminal<lit<"$"> + some(w) + json_path_body> {};

  struct UUID : terminal<as<ast::Function::uuid>(kw<"UUID">)> {};

  static constexpr auto state_fun_name =
      as<ast::Function::format>(kw<"Format">) | as<ast::Function::string_to_json>(kw<"StringToJson">) | as<ast::Function::json_to_string>(kw<"JsonToString">) |
      as<ast::Function::array_partition>(kw<"ArrayPartition">) | as<ast::Function::array_contains>(kw<"ArrayContains">) | as<ast::Function::array_range>(kw<"ArrayRange">) |
      as<ast::Function::array_get_item>(kw<"ArrayGetItem">) | as<ast::Function::array_length>(kw<"ArrayLength">) | as<ast::Function::array_unique>(kw<"ArrayUnique">) | as<ast::Function::array>(kw<"Array">) |
      as<ast::Function::base64_encode>(kw<"Base64Encode">) | as<ast::Function::base64_decode>(kw<"Base64Decode">) | as<ast::Function::hash>(kw<"Hash">) | as<ast::Function::json_merge>(kw<"JsonMerge">) |
      as<ast::Function::math_random>(kw<"MathRandom">) | as<ast::Function::math_add>(kw<"MathAdd">) | as<ast::Function::string_split>(kw<"StringSplit">) | ref<UUID>;

  struct FuncCall;

  struct FuncArg
      : rule<create<ast::StringArg>(
                 assign<&ast::StringArg::value>(ref<STRING>))             //
             | create<ast::IntArg>(assign<&ast::IntArg::value>(ref<INT>)) //
             | create<ast::FloatArg>(
                   assign<&ast::FloatArg::value>(ref<NUMBER>))               //
             | create<ast::BoolArg>(assign<&ast::BoolArg::value>(ref<BOOL>)) //
             | create<ast::NullArg>(kw<"null">) //
             | create<ast::ContextPathArg>(assign<&ast::ContextPathArg::path>(
                   ref<CONTEXT_PATH_STRING>)) //
             | create<ast::JsonPathArg>(
                   assign<&ast::JsonPathArg::path>(ref<JSON_PATH_STRING>)) //
             | create<ast::VariableArg>(
                   assign<&ast::VariableArg::expression>(ref<VARIABLE>)) //
             | ref<FuncCall>> {};

  struct FuncCall
      : rule<kw<"States"> + lit<"."> +
             assign<&ast::FuncCall::function>(state_fun_name) + lit<"("> +
             many(append<&ast::FuncCall::args>(ref<FuncArg>), lit<",">) +
             lit<")">> {};

  using entry = FuncCall;
  using ignored = WS;
};

} // namespace amazonstateslanguageintrinsicfunctions
