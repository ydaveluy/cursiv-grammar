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
using Unescaped = cursiv::text_conv<undouble_string>;

struct Matlab : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  // Not ignored: an eostmt (`,` `;` a line break) is significant grammar,
  // never skipped as trivia.
  struct CR : terminal<some(cr<R"(\r\n)">)> {};

  struct IDENTIFIER : terminal<cr<"a-zA-Z"> + many(w)> {};

  static constexpr auto NumberBody = some(d) + option(lit<"."> + some(d));
  struct CONSTANT
      : terminal<NumberBody + option((lit<"E"> | lit<"e">)+option(cr<"+-">) +
                                     NumberBody),
                 double> {};

  // A single-quoted character vector ends on its physical line. Keeping
  // line breaks out also distinguishes adjacent transpose operators.
  struct STRING_LITERAL
      : terminal<lit<"'"> + many(lit<"''"> | cr<R"(^'\r\n)">) + lit<"'">,
                 Unescaped> {};

  static constexpr auto Eostmt = lit<","> | lit<";"> | ref<CR>;

  // Every operator token hands its own constant over: the ladder below reads
  // `op`, never the spelling.
  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};
  struct LDIVIDE : terminal<lit<"\\">, value<ast::Operator::ldivide>> {};
  struct POWER : terminal<lit<"^">, value<ast::Operator::power>> {};
  struct EWISE_MULTIPLY
      : terminal<lit<".*">, value<ast::Operator::ewise_multiply>> {};
  struct EWISE_LDIVIDE
      : terminal<lit<".\\">, value<ast::Operator::ewise_ldivide>> {};
  struct EWISE_RDIVIDE
      : terminal<lit<"./">, value<ast::Operator::ewise_rdivide>> {};
  struct EWISE_POWER : terminal<lit<".^">, value<ast::Operator::ewise_power>> {
  };
  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct LE_OP : terminal<lit<"<=">, value<ast::Operator::less_equal>> {};
  struct GE_OP : terminal<lit<">=">, value<ast::Operator::greater_equal>> {};
  struct LESS : terminal<lit<"<">, value<ast::Operator::less>> {};
  struct GREATER : terminal<lit<">">, value<ast::Operator::greater>> {};
  struct EQ_OP : terminal<lit<"==">, value<ast::Operator::equal>> {};
  struct NE_OP : terminal<lit<"~=">, value<ast::Operator::not_equal>> {};
  struct AND_OP : terminal<lit<"&">, value<ast::Operator::logical_and>> {};
  struct OR_OP : terminal<lit<"|">, value<ast::Operator::logical_or>> {};
  struct RANGE_OP : terminal<lit<":">, value<ast::Operator::range>> {};

  static constexpr auto OpPow = ref<POWER> | ref<EWISE_POWER>;
  static constexpr auto OpMul = ref<MULTIPLY> | ref<DIVIDE> | ref<LDIVIDE> |
                                ref<EWISE_MULTIPLY> | ref<EWISE_LDIVIDE> |
                                ref<EWISE_RDIVIDE>;
  static constexpr auto OpAdd = ref<ADD> | ref<SUBTRACT>;
  static constexpr auto OpRange = ref<RANGE_OP>;
  static constexpr auto OpRel = ref<LE_OP> | ref<GE_OP> | ref<LESS> |
                                ref<GREATER> | ref<EQ_OP> | ref<NE_OP>;
  static constexpr auto OpAnd = ref<AND_OP>;
  static constexpr auto OpOr = ref<OR_OP>;

  struct Expression;
  struct UnaryExpression;
  struct PostfixExpression;
  struct IndexArg;
  struct IndexExprRule;
  struct Statement;

  // `:` alone (the whole range along one dimension) or a full expression --
  // which may itself use the `:` ladder level, as in `a(1:5, :)`.
  struct IndexArg : rule<flag<&ast::IndexArg::all>(lit<":">) |
                         assign<&ast::IndexArg::value>(ref<Expression>)> {};

  // `name(args)`: the same syntax MATLAB uses for indexing and for a call.
  struct IndexExprRule
      : rule<assign<&ast::IndexExpr::name>(ref<IDENTIFIER>) + lit<"("> +
             some(append<&ast::IndexExpr::args>(ref<IndexArg>), lit<",">) +
             lit<")">> {};

  // `'[' ']'` and `'[' array_list ']'` collapse into one alternative here: an
  // empty element list already spells the same two brackets. Inside, the
  // commas and horizontal whitespace separate columns; semicolons and line
  // breaks delimit row nodes, so the AST retains the matrix dimensions.
  struct ArrayRowRule
      : rule<some(append<&ast::ArrayRow::elements>(ref<Expression>) +
                  option(lit<",">))> {};
  static constexpr auto RowSeparator = lit<";"> | ref<CR>;

  struct PostfixExpression
      : rule<(create<ast::ParenExpr>(
                  lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
                  lit<")">) //
              | create<ast::ArrayLit>(
                    lit<"["> + many(ref<CR>) +
                    many(append<&ast::ArrayLit::rows>(ref<ArrayRowRule>) +
                         some(RowSeparator)) +
                    option(append<&ast::ArrayLit::rows>(ref<ArrayRowRule>)) +
                    lit<"]">)      //
              | ref<IndexExprRule> //
              |
              create<ast::Ident>(assign<&ast::Ident::name>(ref<IDENTIFIER>)) //
              | create<ast::NumberLit>(
                    assign<&ast::NumberLit::value>(ref<CONSTANT>)) //
              | create<ast::StringLit>(
                    assign<&ast::StringLit::value>(ref<STRING_LITERAL>))) +
             many(nest<ast::PostfixOpExpr, &ast::PostfixOpExpr::operand>(
                 assign<&ast::PostfixOpExpr::op>(lit<"'"> | lit<".'">)))> {};

  struct UnaryExpression
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"~">) +
                 assign<&ast::UnaryExpr::operand>(ref<PostfixExpression>)) //
             | ref<PostfixExpression>> {};

  // MathWorks' operator-precedence table, tightest to loosest: transpose and
  // power; unary; multiplication/division; addition/subtraction; colon;
  // relational (one tier, all six); element-wise AND; element-wise OR.
  struct Expression : infix<UnaryExpression, &ast::BinaryExpr::left,
                            &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                            (left(OpPow) >>   //
                             left(OpMul) >>   //
                             left(OpAdd) >>   //
                             left(OpRange) >> //
                             left(OpRel) >>   //
                             left(OpAnd) >>   //
                             left(OpOr))> {};

  struct GlobalStmt
      : rule<kw<"global"> +
             some(append<&ast::GlobalStmt::names>(ref<IDENTIFIER>)) +
             tight(Eostmt)> {};

  struct ClearStmt
      : rule<kw<"clear"> +
             some(append<&ast::ClearStmt::names>(ref<IDENTIFIER>)) +
             tight(Eostmt)> {};

  // The target is a `postfix_expression`, not a full `expression`: `(x) = 1`
  // and `x(1) = 2` are targets this grammar accepts, `x + 1 = 2` is not.
  struct AssignStmt
      : rule<assign<&ast::AssignStmt::target>(ref<PostfixExpression>) +
             lit<"="> + assign<&ast::AssignStmt::value>(ref<Expression>) +
             tight(Eostmt)> {};

  // A bare separator alone is a statement with nothing in it.
  struct ExprStmt
      : rule<create<ast::ExprStmt> +
             option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
             tight(Eostmt)> {};

  struct ElseIfClauseRule
      : rule<create<ast::ElseIfClause> + kw<"elseif"> +
             assign<&ast::ElseIfClause::cond>(ref<Expression>) +
             some(append<&ast::ElseIfClause::body>(ref<Statement>))> {};

  struct IfStmt
      : rule<create<ast::IfStmt> + kw<"if"> +
             assign<&ast::IfStmt::cond>(ref<Expression>) +
             some(append<&ast::IfStmt::thenBody>(ref<Statement>)) +
             many(append<&ast::IfStmt::elseIfs>(ref<ElseIfClauseRule>)) +
             option(kw<"else"> +
                    some(append<&ast::IfStmt::elseBody>(ref<Statement>))) +
             kw<"end"> + tight(Eostmt)> {};

  struct WhileStmt : rule<create<ast::WhileStmt> + kw<"while"> +
                          assign<&ast::WhileStmt::cond>(ref<Expression>) +
                          some(append<&ast::WhileStmt::body>(ref<Statement>)) +
                          kw<"end"> + tight(Eostmt)> {};

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
             tight(Eostmt)> {};

  struct BreakStmt : rule<create<ast::BreakStmt> + kw<"break"> +
                          tight(Eostmt)> {};
  struct ReturnStmt : rule<create<ast::ReturnStmt> + kw<"return"> +
                           tight(Eostmt)> {};

  struct Statement : rule<ref<GlobalStmt> | ref<ClearStmt> | ref<AssignStmt> |
                          ref<ExprStmt> | ref<IfStmt> | ref<WhileStmt> |
                          ref<ForStmt> | ref<BreakStmt> | ref<ReturnStmt>> {};

  struct FileRule : rule<create<ast::File> +
                         many(append<&ast::File::statements>(ref<Statement>))> {
  };

  using entry = FileRule;
  using ignore = WS;
};

} // namespace matlab
