// The matlab language: its grammar.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "matlab/core/ast.hpp"

namespace matlab {

inline constexpr auto undouble_string = [](std::string_view t,
                                           std::pmr::memory_resource *mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\'') == std::string_view::npos)
    return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '\'')
      ++i;
  }
  return std::string_view(room.data(), n);
};

struct Matlab : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  // Not ignored: an eostmt (`,` `;` a line break) is significant grammar,
  // never skipped as trivia.
  struct CR : terminal<some(cr<R"(\r\n)">)> {};

  struct LINE_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct CONTINUATION : terminal<lit<"..."> + many(cr<R"(^\r\n)">) + eol> {};
  static constexpr auto reserved = kw<"break"> | kw<"case"> | kw<"catch"> | kw<"classdef"> |
      kw<"continue"> | kw<"else"> | kw<"elseif"> | kw<"end"> | kw<"for"> | kw<"function"> |
      kw<"global"> | kw<"if"> | kw<"otherwise"> | kw<"parfor"> | kw<"persistent"> |
      kw<"return"> | kw<"spmd"> | kw<"switch"> | kw<"try"> | kw<"while">;
  struct IDENTIFIER : terminal<!reserved + cr<"a-zA-Z"> + many(w)> {};

  static constexpr auto NumberBody =
      (some(d) + option(lit<".", !cr<R"(*/\^')">> + many(d))) |
      (lit<"."> + some(d));
  struct CONSTANT
      : terminal<as<double>(
            NumberBody +
            option(cr<"eE"> + option(cr<"+-">) + some(d)))> {};

  // A single-quoted character vector ends on its physical line. Keeping
  // line breaks out also distinguishes adjacent transpose operators.
  struct STRING_LITERAL
      : terminal<as<undouble_string>(
            lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) + lit<"'">)> {};

  static constexpr auto Eostmt = lit<","> | lit<";"> | ref<CR>;
  static constexpr auto EndStatement = tight(Eostmt) | eof;

  // Every operator token hands its own constant over: the ladder below reads
  // `op`, never the spelling.

  static constexpr auto OpPow = as<ast::Operator::power>(lit<"^">) | as<ast::Operator::ewise_power>(lit<".^">);
  static constexpr auto OpMul = as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">) | as<ast::Operator::ldivide>(lit<"\\">) |
                                as<ast::Operator::ewise_multiply>(lit<".*">) | as<ast::Operator::ewise_ldivide>(lit<".\\">) |
                                as<ast::Operator::ewise_rdivide>(lit<"./">);
  static constexpr auto OpAdd = as<ast::Operator::add>(lit<"+">) | as<ast::Operator::subtract>(lit<"-">);
  static constexpr auto OpRange = as<ast::Operator::range>(lit<":">);
  static constexpr auto OpRel = as<ast::Operator::less_equal>(lit<"<=">) | as<ast::Operator::greater_equal>(lit<">=">) | as<ast::Operator::less>(lit<"<">) |
                                as<ast::Operator::greater>(lit<">">) | as<ast::Operator::equal>(lit<"==">) | as<ast::Operator::not_equal>(lit<"~=">);
  static constexpr auto OpAnd = as<ast::Operator::logical_and>(lit<"&">);
  static constexpr auto OpOr = as<ast::Operator::logical_or>(lit<"|">);
  static constexpr auto OpShortAnd = as<ast::Operator::short_and>(lit<"&&">);
  static constexpr auto OpShortOr = as<ast::Operator::short_or>(lit<"||">);

  struct Expression;
  struct UnaryExpression;
  struct PostfixExpression;
  struct IndexArg;
  struct IndexExpr;
  struct Statement;

  // `:` alone (the whole range along one dimension) or a full expression --
  // which may itself use the `:` ladder level, as in `a(1:5, :)`.
  struct IndexArg : rule<flag<&ast::IndexArg::all>(lit<":">) |
                         assign<&ast::IndexArg::value>(ref<Expression>)> {};

  // `name(args)`: the same syntax MATLAB uses for indexing and for a call.
  struct IndexExpr
      : rule<assign<&ast::IndexExpr::name>(ref<IDENTIFIER>) + lit<"("> +
             option(some(append<&ast::IndexExpr::args>(ref<IndexArg>), lit<",">)) +
             lit<")">> {};

  // `'[' ']'` and `'[' array_list ']'` collapse into one alternative here: an
  // empty element list already spells the same two brackets. Inside, the
  // commas and horizontal whitespace separate columns; semicolons and line
  // breaks delimit row nodes, so the AST retains the matrix dimensions.
  struct ArrayRow
      : rule<some(append<&ast::ArrayRow::elements>(ref<Expression>) +
                  option(lit<",">))> {};
  static constexpr auto RowSeparator = lit<";"> | ref<CR>;

  struct PrimaryExpression
      : rule<(create<ast::ParenExpr>(
                  lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                  lit<")">) //
              | create<ast::ArrayLit>(
                    lit<"["> + many(ref<CR>) +
                    many(append<&ast::ArrayLit::rows>(ref<ArrayRow>) +
                         some(RowSeparator)) +
                    option(append<&ast::ArrayLit::rows>(ref<ArrayRow>)) +
                    lit<"]">)      //
              | ref<IndexExpr> //
              |
              create<ast::Ident>(assign<&ast::Ident::name>(ref<IDENTIFIER>)) //
              | create<ast::NumberLit>(
                    assign<&ast::NumberLit::value>(ref<CONSTANT>) +
                    option(gap::none + flag<&ast::NumberLit::imaginary>(lit<"i", !w> | lit<"j", !w>))) //
              | create<ast::StringLit>(
                    assign<&ast::StringLit::value>(ref<STRING_LITERAL>)))> {};

  struct PostfixExpression : rule<ref<PrimaryExpression> +
             many(nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                 assign<&ast::PostfixOpExpr::op>(lit<"'"> | lit<".'">)))> {};

  struct PrefixExpression : rule<create<ast::UnaryExpr>(
      assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"~">) +
      assign<&ast::UnaryExpr::operand>(ref<UnaryExpression>))> {};
  // Power and transpose share one left-associative tier. A signed exponent
  // starts a unary expression without changing pure a^b^c left association.
  struct PowerExpression : rule<ref<PrimaryExpression> + many(
      nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
          assign<&ast::PostfixOpExpr::op>(lit<"'"> | lit<".'">)) |
      nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(OpPow) +
          assign<&ast::BinaryExpr::right>(ref<PrefixExpression> | ref<PrimaryExpression>)))> {};
  struct UnaryExpression : rule<ref<PrefixExpression> | ref<PowerExpression>> {};

  // MathWorks' operator-precedence table, tightest to loosest: transpose and
  // power; unary; multiplication/division; addition/subtraction; colon;
  // relational (one tier, all six); element-wise AND; element-wise OR.
  struct Expression : infix<UnaryExpression, ast::BinaryExpr,
                            levels(left(OpMul),    //
                                   left(OpAdd),    //
                                   left(OpRange),  //
                                   left(OpRel),    //
                                   left(OpAnd),    //
                                   left(OpOr),
                                   left(OpShortAnd), left(OpShortOr))> {};

  struct GlobalStmt
      : rule<kw<"global"> +
             some(append<&ast::GlobalStmt::names>(ref<IDENTIFIER>)) +
             EndStatement> {};

  struct ClearStmt
      : rule<kw<"clear"> +
             some(append<&ast::ClearStmt::names>(ref<IDENTIFIER>)) +
             EndStatement> {};

  // The target is a `postfix_expression`, not a full `expression`: `(x) = 1`
  // and `x(1) = 2` are targets this grammar accepts, `x + 1 = 2` is not.
  struct AssignStmt
      : rule<assign<&ast::AssignStmt::target>(ref<PostfixExpression>) +
             lit<"="> + assign<&ast::AssignStmt::value>(ref<Expression>) +
             EndStatement> {};

  // A bare separator alone is a statement with nothing in it.
  struct ExprStmt
      : rule<create<ast::ExprStmt> +
             ((option(assign<&ast::ExprStmt::value>(ref<Expression>)) + tight(Eostmt)) |
              (assign<&ast::ExprStmt::value>(ref<Expression>) + eof))> {};

  struct ElseIfClause
      : rule<create<ast::ElseIfClause> + kw<"elseif"> +
             assign<&ast::ElseIfClause::cond>(ref<Expression>) +
             some(append<&ast::ElseIfClause::body>(ref<Statement>))> {};

  struct IfStmt
      : rule<create<ast::IfStmt> + kw<"if"> +
             assign<&ast::IfStmt::cond>(ref<Expression>) +
             some(append<&ast::IfStmt::thenBody>(ref<Statement>)) +
             many(append<&ast::IfStmt::elseIfs>(ref<ElseIfClause>)) +
             option(kw<"else"> +
                    some(append<&ast::IfStmt::elseBody>(ref<Statement>))) +
             kw<"end"> + EndStatement> {};

  struct WhileStmt : rule<create<ast::WhileStmt> + kw<"while"> +
                          assign<&ast::WhileStmt::cond>(ref<Expression>) +
                          some(append<&ast::WhileStmt::body>(ref<Statement>)) +
                          kw<"end"> + EndStatement> {};

  // The header's parentheses are optional and change nothing else: both
  // forms build the same node.
  struct ForStmt
      : rule<create<ast::ForStmt> + kw<"for"> +
             ((assign<&ast::ForStmt::var>(ref<IDENTIFIER>) + lit<"="> +
               assign<&ast::ForStmt::range>(ref<Expression>)) //
              | (lit<"("> + assign<&ast::ForStmt::var>(ref<IDENTIFIER>) +
                 lit<"="> + assign<&ast::ForStmt::range>(ref<Expression>) +
                 lit<")">)) +
             some(append<&ast::ForStmt::body>(ref<Statement>)) + kw<"end"> +
             EndStatement> {};

  struct BreakStmt : rule<create<ast::BreakStmt> + kw<"break"> +
                          EndStatement> {};
  struct ReturnStmt : rule<create<ast::ReturnStmt> + kw<"return"> +
                           EndStatement> {};

  struct ContinueStmt : rule<create<ast::ContinueStmt> + kw<"continue"> + EndStatement> {};
  static constexpr auto OutputNames = some(append<&ast::FunctionDef::outputs>(ref<IDENTIFIER>) + option(lit<",">));
  struct FunctionDef : rule<kw<"function"> +
      option(((lit<"["> + option(OutputNames) + lit<"]">) |
              append<&ast::FunctionDef::outputs>(ref<IDENTIFIER>)) + lit<"=">) +
      assign<&ast::FunctionDef::name>(ref<IDENTIFIER>) +
      option(lit<"("> + option(some(append<&ast::FunctionDef::parameters>(ref<IDENTIFIER> | lit<"~">), lit<",">)) + lit<")">) +
      Eostmt + many(append<&ast::FunctionDef::body>(ref<Statement>)) +
      ((kw<"end"> + EndStatement) | eof)> {};

  struct Statement : rule<ref<GlobalStmt> | ref<ClearStmt> | ref<AssignStmt> |
                          ref<ExprStmt> | ref<IfStmt> | ref<WhileStmt> |
                          ref<ForStmt> | ref<BreakStmt> | ref<ReturnStmt> | ref<ContinueStmt> | ref<FunctionDef>> {};

  struct File : rule<create<ast::File> +
                         many(append<&ast::File::statements>(ref<Statement>))> {
  };

  using entry = File;
  using ignored = WS;
  using hidden = list<CONTINUATION, LINE_COMMENT>;
};

} // namespace matlab
