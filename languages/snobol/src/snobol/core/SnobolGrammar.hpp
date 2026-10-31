// The snobol language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "snobol/core/ast.hpp"

namespace snobol {

struct Snobol : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"0-9a-zA-Z._">;
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<"*"> + many(cr<R"(^\r\n)">)> {};

  struct END_KW : terminal<ikw<"END">> {};

  struct STRING : terminal<cr<"a-zA-Z"> + many(cr<"0-9a-zA-Z._">)> {};
  struct INTEGER : terminal<as<std::int64_t>(option(cr<"+-">) + some(d))> {};

  inline static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };

  struct STRINGLITERAL1
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">)> {};
  struct STRINGLITERAL2
      : terminal<as<unquote>(lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">)> {};

  struct SUCCESS : terminal<cr<"sS">> {};
  struct FAILURE : terminal<cr<"fF">> {};



  struct Expression;

  struct Subject
      : rule<create<ast::Subject>(
            option(flag<&ast::Subject::keyword>(lit<"&">)) +
            assign<&ast::Subject::name>(ref<STRING>) +
            option(lit<"["> +
                   some(append<&ast::Subject::subscript>(ref<STRING>),
                        lit<",">) +
                   lit<"]">))> {};

  struct StringLit
      : rule<create<ast::StringLiteral>(assign<&ast::StringLiteral::value>(
            ref<STRINGLITERAL1> | ref<STRINGLITERAL2>))> {};

  // Every builtin is `name(arg[, arg[, arg]])`: `fn` is the token, filled by
  // its own `as<>` constant, and each rule differs only in its argument
  // count (0, 1, 2 or 3).
  static constexpr auto BuiltinName = as<ast::Builtin::ident>(ikw<"ident">) | as<ast::Builtin::differ>(ikw<"differ">) | as<ast::Builtin::eq>(ikw<"eq">) | as<ast::Builtin::ne>(ikw<"ne">) | as<ast::Builtin::ge>(ikw<"ge">) | as<ast::Builtin::le>(ikw<"le">) | as<ast::Builtin::lt>(ikw<"lt">) | as<ast::Builtin::integer>(ikw<"integer">) | as<ast::Builtin::lgt>(ikw<"lgt">) | as<ast::Builtin::atan>(ikw<"atan">) | as<ast::Builtin::chop>(ikw<"chop">) | as<ast::Builtin::cos>(ikw<"cos">) | as<ast::Builtin::exp>(ikw<"exp">) | as<ast::Builtin::ln>(ikw<"ln">) | as<ast::Builtin::remdr>(ikw<"remdr">) | as<ast::Builtin::sin>(ikw<"sin">) | as<ast::Builtin::tan>(ikw<"tan">) | as<ast::Builtin::date>(ikw<"date">) | as<ast::Builtin::dupl>(ikw<"dupl">) | as<ast::Builtin::reverse>(ikw<"reverse">) | as<ast::Builtin::replace>(ikw<"replace">) | as<ast::Builtin::size>(ikw<"size">) | as<ast::Builtin::trim>(ikw<"trim">) | as<ast::Builtin::array_>(ikw<"array">) | as<ast::Builtin::sort>(ikw<"sort">) | as<ast::Builtin::table>(ikw<"table">) | as<ast::Builtin::break_>(ikw<"break">);
  struct Command : rule<create<ast::FunctionCall>(
      assign<&ast::FunctionCall::fn>(BuiltinName) + lit<"("> +
      option(some(append<&ast::FunctionCall::args>(ref<Expression>), lit<",">)) + lit<")">)> {};

  struct Atom
      : rule<ref<StringLit>  //
             | create<ast::IntegerLiteral>(
                   assign<&ast::IntegerLiteral::value>(ref<INTEGER>))  //
             | ref<Subject>                                       //
             | ref<Command>                                       //
             | create<ast::ListExpr>(
                   lit<"["> +
                   some(append<&ast::ListExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"]">)  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // SNOBOL4 gives multiplication higher precedence than division.
  struct Expression
      : infix<Atom, ast::BinaryExpr,
              levels(right(as<ast::Operator::pow>(lit<"^"> | lit<"!"> | lit<"**">)),
                     left(as<ast::Operator::multiply>(lit<"*">)),
                     left(as<ast::Operator::divide>(lit<"/">)),
                     left(as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">)))> {};

  static constexpr auto transferTarget =
      lit<"("> + (flag<&ast::Transfer::toEnd>(ref<END_KW>) |
                  assign<&ast::Transfer::label>(ref<STRING>)) + lit<")">;
  struct SuccessTransfer
      : rule<create<ast::Transfer>(
            assign<&ast::Transfer::mode>(ref<SUCCESS>) + transferTarget)> {};
  struct FailureTransfer
      : rule<create<ast::Transfer>(
            assign<&ast::Transfer::mode>(ref<FAILURE>) + transferTarget)> {};
  struct Transfer
      : rule<create<ast::Transfer>(option(
            (assign<&ast::Transfer::mode>(ref<SUCCESS>) + transferTarget +
             option(assign<&ast::Transfer::next>(ref<FailureTransfer>))) |
            (assign<&ast::Transfer::mode>(ref<FAILURE>) + transferTarget +
             option(assign<&ast::Transfer::next>(ref<SuccessTransfer>))) |
            transferTarget))> {};

  struct StatementLine
      : rule<create<ast::Statement>(
            option(assign<&ast::Statement::label>(ref<STRING>)) +
            assign<&ast::Statement::subject>(ref<Subject>) +
            option(assign<&ast::Statement::pattern>(ref<StringLit>)) +
            option(flag<&ast::Statement::hasReplacement>(lit<"=">) +
                   many(append<&ast::Statement::replacement>(
                       ref<Expression>))) +
            option(lit<":"> +
                   assign<&ast::Statement::transfer>(ref<Transfer>)))> {};

  struct GotoLine
      : rule<create<ast::GotoLine>(
            lit<":"> + assign<&ast::GotoLine::transfer>(ref<Transfer>))> {
  };

  struct CommentLine
      : rule<assign<&ast::CommentLine::text>(ref<COMMENT>)> {};

  struct EndLine : rule<create<ast::EndLine> + ref<END_KW>> {};

  struct Line
      : rule<ref<StatementLine> | ref<GotoLine> | ref<CommentLine> |
             ref<EndLine>> {};

  struct Program
      : rule<create<ast::Program> +
             some(option(append<&ast::Program::lines>(ref<Line>)) +
                  ref<EOL>)> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace snobol
