// The LISA language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lisa/core/ast.hpp"

namespace lisa {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Lisa : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z"> + many(w)> {};
  struct INTEGER
      : terminal<option(lit<"+"> | lit<"-">) + cr<"1-9"> + many(d),
                 std::int64_t> {};
  struct STRINGLITERAL
      : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  struct TRUE_ : terminal<kw<"TRUE">, value<true>> {};
  struct FALSE_ : terminal<kw<"FALSE">, value<false>> {};

  struct IntKw : terminal<kw<"int">, value<ast::TypeKind::Int>> {};
  struct DfaKw : terminal<kw<"dfa">, value<ast::TypeKind::Dfa>> {};
  struct NfaKw : terminal<kw<"nfa">, value<ast::TypeKind::Nfa>> {};
  struct RegexKw : terminal<kw<"regex">, value<ast::TypeKind::Regex>> {};
  struct BoolKw : terminal<kw<"bool">, value<ast::TypeKind::Bool>> {};
  struct StringKw : terminal<kw<"string">, value<ast::TypeKind::String>> {};
  static constexpr auto TypeWord = ref<IntKw> | ref<DfaKw> | ref<NfaKw> |
                                   ref<RegexKw> | ref<BoolKw> | ref<StringKw>;

  struct RandomKw
      : terminal<kw<"random">, value<ast::GeneratorKind::Random>> {};
  struct EnumerateKw
      : terminal<kw<"enumerate">, value<ast::GeneratorKind::Enumerate>> {};
  static constexpr auto GeneratorWord = ref<RandomKw> | ref<EnumerateKw>;

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  // No `<`: the language has no way to spell it.
  static constexpr auto OpRel =
      lit<"<="> | lit<">="> | lit<"=="> | lit<"!="> | lit<">">;
  static constexpr auto OpAnd = lit<"&&">;
  static constexpr auto OpOr = lit<"||">;
  // `-+`, not a typo.
  static constexpr auto OpAssign =
      lit<"="> | lit<"-+"> | lit<"*="> | lit<"/="> | lit<"+=">;

  struct Expression;
  struct LogicalExpr;
  struct Factor;

  struct DeclarationRule
      : rule<create<ast::Declaration>(
                 assign<&ast::Declaration::type>(TypeWord) +
                 assign<&ast::Declaration::name>(ref<ID>) + lit<";">)> {};

  struct VariableRefRule
      : rule<assign<&ast::VariableRef::target>(ref<ID>)> {};

  struct FunctionCallRule
      : rule<create<ast::FunctionCall>(
                 assign<&ast::FunctionCall::name>(ref<ID>) + lit<"("> +
                 some(append<&ast::FunctionCall::args>(ref<LogicalExpr>),
                      lit<",">) +
                 lit<")">)> {};

  struct IntegerLiteralRule
      : rule<assign<&ast::IntegerLiteral::value>(ref<INTEGER>)> {};
  struct BoolLiteralRule
      : rule<assign<&ast::BoolLiteral::value>(ref<TRUE_> | ref<FALSE_>)> {};
  struct StringLiteralRule
      : rule<assign<&ast::StringLiteral::value>(ref<STRINGLITERAL>)> {};
  struct TypeLiteralRule
      : rule<assign<&ast::TypeLiteral::value>(TypeWord)> {};

  struct Constant
      : rule<ref<IntegerLiteralRule>                                  //
             | ref<BoolLiteralRule>                                   //
             | create<ast::NextExpr>(kw<"next">)                      //
             | create<ast::HasNextExpr>(kw<"hasnext">)                //
             | ref<VariableRefRule>                                   //
             | ref<StringLiteralRule>                                 //
             | ref<FunctionCallRule>                                  //
             | ref<TypeLiteralRule>> {};

  struct Factor
      : rule<create<ast::GroupedExpr>(
                 lit<"("> +
                 assign<&ast::GroupedExpr::expression>(ref<LogicalExpr>) +
                 lit<")">)  //
             | ref<Constant>> {};

  struct RelationExpr
      : infix<Factor, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpRel))> {};

  // At most one `!`: optional, not repeatable.
  struct UnaryRelation
      : rule<create<ast::NotExpr>(
                 lit<"!"> +
                 assign<&ast::NotExpr::operand>(ref<RelationExpr>))  //
             | ref<RelationExpr>> {};

  struct LogicalExpr
      : infix<UnaryRelation, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(OpAnd) >> left(OpOr))> {};

  // A run of assignments is folded into right recursion through `value`:
  // a second pair reads as a nested assignment, `a = b = c` as
  // `a = (b = c)`.
  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<ID>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<Expression>))  //
             | ref<LogicalExpr>> {};

  struct StatementRule;

  struct ExpressionStatementRule
      : rule<create<ast::ExpressionStatement>(
                 assign<&ast::ExpressionStatement::expression>(
                     ref<Expression>) +
                 lit<";">)> {};

  struct IfStatementRule
      : rule<create<ast::IfStatement>(
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfStatement::condition>(ref<Expression>) +
                 lit<")"> +
                 assign<&ast::IfStatement::body>(ref<StatementRule>))> {};

  struct WhileStatementRule
      : rule<create<ast::WhileStatement>(
                 kw<"WHILE"> + lit<"("> +
                 assign<&ast::WhileStatement::condition>(ref<Expression>) +
                 lit<")"> +
                 assign<&ast::WhileStatement::body>(ref<StatementRule>))> {};

  struct GeneratingStatementRule
      : rule<create<ast::GeneratingStatement>(
                 kw<"generate"> + lit<"("> +
                 assign<&ast::GeneratingStatement::generator>(
                     GeneratorWord) +
                 lit<","> + kw<"int"> + lit<","> + kw<"int"> + lit<")"> +
                 assign<&ast::GeneratingStatement::body>(
                     ref<StatementRule>))> {};

  struct StatementRule
      : rule<ref<ExpressionStatementRule>       //
             | ref<IfStatementRule>             //
             | ref<WhileStatementRule>          //
             | ref<GeneratingStatementRule>     //
             | create<ast::BreakStatement>(kw<"break"> + lit<";">)     //
             | create<ast::ContinueStatement>(kw<"continue"> +
                                              lit<";">)> {};

  struct ProgramRule
      : rule<create<ast::Program>(
                 kw<"declare"> + lit<"{"> +
                 some(append<&ast::Program::declarations>(
                     ref<DeclarationRule>)) +
                 lit<"}"> +  //
                 kw<"program"> + lit<"{"> +
                 some(append<&ast::Program::statements>(
                     ref<StatementRule>)) +
                 lit<"}">)> {};

  using entry = ProgramRule;
  using ignore = list<WS, COMMENT>;
};

}  // namespace lisa
