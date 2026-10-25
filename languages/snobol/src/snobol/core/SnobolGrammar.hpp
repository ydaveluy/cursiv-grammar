// The snobol language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "snobol/core/ast.hpp"

namespace snobol {

struct Snobol : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<"*"> + many(cr<R"(^\r\n)">)> {};

  struct END_KW : terminal<kw<"END">> {};

  struct STRING : terminal<cr<"a-zA-Z"> + many(cr<"0-9a-zA-Z">)> {};
  struct INTEGER
      : terminal<option(cr<"+-">) + some(d), std::int64_t> {};

  inline static constexpr auto unquote = [](std::string_view t) {
    return t.substr(1, t.size() - 2);
  };
  using Unquoted = cursiv::text_conv<unquote>;

  struct STRINGLITERAL1
      : terminal<lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">, Unquoted> {};
  struct STRINGLITERAL2
      : terminal<lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">, Unquoted> {};

  struct TRANSFERPRE : terminal<cr<"fFsS">> {};

  struct PLUS : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct MINUS : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct TIMES : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct POW : terminal<lit<"^">, value<ast::Operator::pow>> {};

  struct IDENT_KW : terminal<kw<"ident">, value<ast::Builtin::ident>> {};
  struct DIFFER_KW : terminal<kw<"differ">, value<ast::Builtin::differ>> {};
  struct EQ_KW : terminal<kw<"eq">, value<ast::Builtin::eq>> {};
  struct NE_KW : terminal<kw<"ne">, value<ast::Builtin::ne>> {};
  struct GE_KW : terminal<kw<"ge">, value<ast::Builtin::ge>> {};
  struct LE_KW : terminal<kw<"le">, value<ast::Builtin::le>> {};
  struct LT_KW : terminal<kw<"lt">, value<ast::Builtin::lt>> {};
  struct INTEGER_KW
      : terminal<kw<"integer">, value<ast::Builtin::integer>> {};
  struct LGT_KW : terminal<kw<"lgt">, value<ast::Builtin::lgt>> {};
  struct ATAN_KW : terminal<kw<"atan">, value<ast::Builtin::atan>> {};
  struct CHOP_KW : terminal<kw<"chop">, value<ast::Builtin::chop>> {};
  struct COS_KW : terminal<kw<"cos">, value<ast::Builtin::cos>> {};
  struct EXP_KW : terminal<kw<"exp">, value<ast::Builtin::exp>> {};
  struct LN_KW : terminal<kw<"ln">, value<ast::Builtin::ln>> {};
  struct REMDR_KW : terminal<kw<"remdr">, value<ast::Builtin::remdr>> {};
  struct SIN_KW : terminal<kw<"sin">, value<ast::Builtin::sin>> {};
  struct TAN_KW : terminal<kw<"tan">, value<ast::Builtin::tan>> {};
  struct DATE_KW : terminal<kw<"date">, value<ast::Builtin::date>> {};
  struct DUPL_KW : terminal<kw<"dupl">, value<ast::Builtin::dupl>> {};
  struct REVERSE_KW
      : terminal<kw<"reverse">, value<ast::Builtin::reverse>> {};
  struct REPLACE_KW
      : terminal<kw<"replace">, value<ast::Builtin::replace>> {};
  struct SIZE_KW : terminal<kw<"size">, value<ast::Builtin::size>> {};
  struct TRIM_KW : terminal<kw<"trim">, value<ast::Builtin::trim>> {};
  struct ARRAY_KW : terminal<kw<"array">, value<ast::Builtin::array_>> {};
  struct SORT_KW : terminal<kw<"sort">, value<ast::Builtin::sort>> {};
  struct TABLE_KW : terminal<kw<"table">, value<ast::Builtin::table>> {};
  struct BREAK_KW : terminal<kw<"break">, value<ast::Builtin::break_>> {};

  struct Expression;

  struct SubjectRule
      : rule<create<ast::Subject>(
            option(flag<&ast::Subject::indirect>(lit<"&">)) +
            assign<&ast::Subject::name>(ref<STRING>) +
            option(lit<"["> +
                   some(append<&ast::Subject::subscript>(ref<STRING>),
                        lit<",">) +
                   lit<"]">))> {};

  struct StringLit
      : rule<create<ast::StringLiteral>(assign<&ast::StringLiteral::value>(
            ref<STRINGLITERAL1> | ref<STRINGLITERAL2>))> {};

  // Every builtin is `name(arg[, arg[, arg]])`: `fn` is the token, filled by
  // its own `value<>` converter, and each rule differs only in its argument
  // count (0, 1, 2 or 3).
  static constexpr auto BuiltinName = ref<IDENT_KW> | ref<DIFFER_KW> | ref<EQ_KW> | ref<NE_KW> | ref<GE_KW> | ref<LE_KW> | ref<LT_KW> | ref<INTEGER_KW> | ref<LGT_KW> | ref<ATAN_KW> | ref<CHOP_KW> | ref<COS_KW> | ref<EXP_KW> | ref<LN_KW> | ref<REMDR_KW> | ref<SIN_KW> | ref<TAN_KW> | ref<DATE_KW> | ref<DUPL_KW> | ref<REVERSE_KW> | ref<REPLACE_KW> | ref<SIZE_KW> | ref<TRIM_KW> | ref<ARRAY_KW> | ref<SORT_KW> | ref<TABLE_KW> | ref<BREAK_KW>;
  struct CommandRule : rule<create<ast::FunctionCall>(
      assign<&ast::FunctionCall::fn>(BuiltinName) + lit<"("> +
      option(some(append<&ast::FunctionCall::args>(ref<Expression>), lit<",">)) + lit<")">)> {};

  struct Atom
      : rule<ref<StringLit>  //
             | create<ast::IntegerLiteral>(
                   assign<&ast::IntegerLiteral::value>(ref<INTEGER>))  //
             | ref<SubjectRule>                                       //
             | ref<CommandRule>                                       //
             | create<ast::ListExpr>(
                   lit<"["> +
                   some(append<&ast::ListExpr::items>(ref<Expression>),
                        lit<",">) +
                   lit<"]">)  //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  // Tightest, right-associative (SNOBOL4 gives `**`/`^` the highest
  // precedence of the arithmetic operators): `a^b^c` is `a^(b^c)`, and
  // `a^b+c` is `(a^b)+c`, not `a^(b+c)`.
  struct PowExpr
      : rule<create<ast::BinaryExpr>(
            assign<&ast::BinaryExpr::left>(ref<Atom>) +
            assign<&ast::BinaryExpr::op>(ref<POW>) +
            assign<&ast::BinaryExpr::right>(ref<PowExpr>))  //
             | ref<Atom>> {};

  struct Expression
      : infix<PowExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<TIMES> | ref<DIVIDE>) >>
               left(ref<PLUS> | ref<MINUS>))> {};

  struct TransferRule
      : rule<create<ast::Transfer>(
            option(option(assign<&ast::Transfer::mode>(ref<TRANSFERPRE>)) +
                   lit<"("> +
                   (flag<&ast::Transfer::toEnd>(ref<END_KW>) |
                    assign<&ast::Transfer::label>(ref<STRING>)) +
                   lit<")">))> {};

  struct StatementLine
      : rule<create<ast::Statement>(
            option(assign<&ast::Statement::label>(ref<STRING>)) +
            assign<&ast::Statement::subject>(ref<SubjectRule>) +
            option(assign<&ast::Statement::pattern>(ref<StringLit>)) +
            option(lit<"="> +
                   some(append<&ast::Statement::replacement>(
                       ref<Expression>))) +
            option(lit<":"> +
                   assign<&ast::Statement::transfer>(ref<TransferRule>)))> {};

  struct GotoLineRule
      : rule<create<ast::GotoLine>(
            lit<":"> + assign<&ast::GotoLine::transfer>(ref<TransferRule>))> {
  };

  struct CommentLineRule
      : rule<create<ast::CommentLine>(
            assign<&ast::CommentLine::text>(ref<COMMENT>))> {};

  struct EndLineRule : rule<create<ast::EndLine> + ref<END_KW>> {};

  struct LineRule
      : rule<ref<StatementLine> | ref<GotoLineRule> | ref<CommentLineRule> |
             ref<EndLineRule>> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             some(option(append<&ast::Program::lines>(ref<LineRule>)) +
                  ref<EOL>)> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace snobol
