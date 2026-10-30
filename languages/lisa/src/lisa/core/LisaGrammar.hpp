// The LISA language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lisa/core/ast.hpp"

namespace lisa {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Lisa : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};

  struct INTEGER : terminal<cr<"1-9"> + many(d)> {};
  struct SignedInteger : rule<option(lit<"+"> | lit<"-">) + ref<INTEGER>> {};
  struct STRINGLITERAL : terminal<as<unquote>(through(lit<"\"">, lit<"\"">))> {
  };

  static constexpr auto TypeWord = as<ast::TypeKind::Int>(kw<"int">) | as<ast::TypeKind::Dfa>(kw<"dfa">) | as<ast::TypeKind::Nfa>(kw<"nfa">) |
                                   as<ast::TypeKind::Regex>(kw<"regex">) | as<ast::TypeKind::Bool>(kw<"bool">) | as<ast::TypeKind::String>(kw<"string">);

  static constexpr auto GeneratorWord = as<ast::GeneratorKind::Random>(kw<"random">) | as<ast::GeneratorKind::Enumerate>(kw<"enumerate">);

  // A terminal guard uses pure token patterns; enum conversions stay in rules.
  static constexpr auto reserved = kw<"int"> | kw<"dfa"> | kw<"nfa"> | kw<"regex"> |
      kw<"bool"> | kw<"string"> | kw<"random"> | kw<"enumerate"> |
      kw<"declare"> | kw<"program"> | kw<"if"> | kw<"generate"> |
      kw<"break"> | kw<"continue"> | kw<"TRUE"> | kw<"FALSE"> |
      kw<"WHILE"> | kw<"next"> | kw<"hasnext">;
  struct ID : terminal<!reserved + cr<"a-zA-Z"> + many(w)> {};

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

  struct Declaration
      : rule<create<ast::Declaration>(
                 assign<&ast::Declaration::type>(TypeWord) +
                 assign<&ast::Declaration::name>(ref<ID>) + lit<";">)> {};

  struct VariableRef
      : rule<assign<&ast::VariableRef::target>(ref<ID>)> {};

  struct FunctionCall
      : rule<create<ast::FunctionCall>(
                 assign<&ast::FunctionCall::name>(ref<ID>) + lit<"("> +
                 some(append<&ast::FunctionCall::args>(ref<LogicalExpr>),
                      lit<",">) +
                 lit<")">)> {};

  struct IntegerLiteral
      : rule<assign<&ast::IntegerLiteral::value>(ref<SignedInteger>)> {};
  struct BoolLiteral
      : rule<assign<&ast::BoolLiteral::value>(as<true>(kw<"TRUE">) | as<false>(kw<"FALSE">))> {};
  struct StringLiteral
      : rule<assign<&ast::StringLiteral::value>(ref<STRINGLITERAL>)> {};
  struct TypeLiteral
      : rule<assign<&ast::TypeLiteral::value>(TypeWord)> {};

  struct Constant
      : rule<ref<IntegerLiteral>                                  //
             | ref<BoolLiteral>                                   //
             | create<ast::NextExpr>(kw<"next">)                      //
             | create<ast::HasNextExpr>(kw<"hasnext">)                //
             | ref<VariableRef>                                   //
             | ref<StringLiteral>                                 //
             | ref<FunctionCall>                                  //
             | ref<TypeLiteral>> {};

  struct Factor
      : rule<create<ast::GroupedExpr>(
                 lit<"("> +
                 assign<&ast::GroupedExpr::expression>(ref<LogicalExpr>) +
                 lit<")">)  //
             | ref<Constant>> {};

  struct RelationExpr
      : infix<Factor, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpRel))> {};

  // At most one `!`: optional, not repeatable.
  struct UnaryRelation
      : rule<create<ast::NotExpr>(
                 lit<"!"> +
                 assign<&ast::NotExpr::operand>(ref<RelationExpr>))  //
             | ref<RelationExpr>> {};

  struct LogicalExpr
      : infix<UnaryRelation, ast::BinaryExpr, levels(left(OpAnd), left(OpOr))> {
  };

  struct AssignmentPart
      : rule<assign<&ast::AssignmentPart::op>(OpAssign) +
             assign<&ast::AssignmentPart::value>(ref<LogicalExpr>)> {};
  // The defining parser permits several pairs even when a value is not an
  // assignable variable (a = 2 = 3). Preserve that syntax without evaluation.
  struct Expression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<ID>) +
                 assign<&ast::AssignExpr::op>(OpAssign) +
                 assign<&ast::AssignExpr::value>(ref<LogicalExpr>) +
                 many(append<&ast::AssignExpr::continuations>(ref<AssignmentPart>))) |
             ref<LogicalExpr>> {};

  struct Statement;

  struct ExpressionStatement
      : rule<create<ast::ExpressionStatement>(
                 assign<&ast::ExpressionStatement::expression>(
                     ref<Expression>) +
                 lit<";">)> {};

  struct IfStatement
      : rule<create<ast::IfStatement>(
                 kw<"if"> + lit<"("> +
                 assign<&ast::IfStatement::condition>(ref<Expression>) +
                 lit<")"> +
                 assign<&ast::IfStatement::body>(ref<Statement>))> {};

  struct WhileStatement
      : rule<create<ast::WhileStatement>(
                 kw<"WHILE"> + lit<"("> +
                 assign<&ast::WhileStatement::condition>(ref<Expression>) +
                 lit<")"> +
                 assign<&ast::WhileStatement::body>(ref<Statement>))> {};

  struct GeneratingStatement
      : rule<create<ast::GeneratingStatement>(
                 kw<"generate"> + lit<"("> +
                 assign<&ast::GeneratingStatement::generator>(
                     GeneratorWord) +
                 lit<","> + kw<"int"> + lit<","> + kw<"int"> + lit<")"> +
                 assign<&ast::GeneratingStatement::body>(
                     ref<Statement>))> {};

  struct Statement
      : rule<ref<ExpressionStatement>       //
             | ref<IfStatement>             //
             | ref<WhileStatement>          //
             | ref<GeneratingStatement>     //
             | create<ast::BreakStatement>(kw<"break"> + lit<";">)     //
             | create<ast::ContinueStatement>(kw<"continue"> +
                                              lit<";">)> {};

  struct Program
      : rule<create<ast::Program>(
                 kw<"declare"> + lit<"{"> +
                 some(append<&ast::Program::declarations>(
                     ref<Declaration>)) +
                 lit<"}"> +  //
                 kw<"program"> + lit<"{"> +
                 some(append<&ast::Program::statements>(
                     ref<Statement>)) +
                 lit<"}">)> {};

  using entry = Program;
  using ignored = list<WS, COMMENT>;
};

}  // namespace lisa
