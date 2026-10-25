// The Icon language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "icon/core/ast.hpp"

namespace icon {

struct Icon : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct INTEGER_LITERAL : terminal<some(d), std::int64_t> {};
  struct REAL_LITERAL : terminal<many(d) + lit<"."> + some(d) +
                                  many((lit<"e"> | lit<"E">) + some(d)),
                                  double> {};
  struct STRING_LITERAL : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {
  };

  static constexpr auto idChar = cr<"a-zA-Z0-9">;
  // A keyword ends where an identifier does.
  static constexpr auto word_boundary = idChar;

  struct ID : terminal<cr<"a-zA-Z"> + many(idChar)> {};

  // Operator sets, tightest level first -- one token opens on one step.
  static constexpr auto OpBackslashAtBang = lit<"\\"> | lit<"@"> | lit<"!">;
  static constexpr auto OpCaret = lit<"^">;
  static constexpr auto OpMulDivModPow =
      lit<"**"> | lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAddSub = lit<"++"> | lit<"--"> | lit<"+"> | lit<"-">;
  static constexpr auto OpConcat = lit<"|||"> | lit<"||">;
  static constexpr auto OpCompare =
      lit<"<<="> | lit<"<<"> | lit<"<="> | lit<"<"> | lit<">>="> |
      lit<">>"> | lit<">="> | lit<">"> | lit<"~==="> | lit<"==="> |
      lit<"~=="> | lit<"=="> | lit<"~="> | lit<"=">;
  static constexpr auto OpPipe = lit<"|">;
  static constexpr auto OpAssign = lit<":=:"> | lit<":="> | lit<"<->"> | lit<"<-">;
  static constexpr auto OpQuestion = lit<"?">;
  static constexpr auto OpAmp = lit<"&">;

  static constexpr auto OpPrefix =
      kw<"not"> | lit<"|"> | lit<"!"> | lit<"*"> | lit<"+"> | lit<"-"> |
      lit<"."> | lit<"/"> | lit<"\\"> | lit<"="> | lit<"?"> | lit<"~"> |
      lit<"@"> | lit<"^">;

  struct Expression;
  struct OuterLadder;
  struct ToExpr;
  struct InnerLadder;
  struct Unary;
  struct Postfix;
  struct Primary;
  struct Subscript;
  struct CaseClauseRule;
  struct Parameter;
  struct LocalDecl;
  struct ProcedureRule;
  struct DeclarationRule;

  struct ExprListRule
      : rule<create<ast::ExprList>(
             lit<"("> +
             some(append<&ast::ExprList::items>(ref<Expression>), lit<",">) +
             lit<")">)> {};

  struct BlockRule
      : rule<create<ast::Block>(
             lit<"{"> +
             some(append<&ast::Block::body>(ref<Expression>), lit<";">) +
             lit<"}">)> {};

  struct ListLitRule
      : rule<create<ast::ListLit>(
             lit<"["> +
             some(append<&ast::ListLit::items>(ref<Expression>), lit<",">) +
             lit<"]">)> {};

  struct KeywordRefRule
      : rule<create<ast::KeywordRef>(
             lit<"&"> + assign<&ast::KeywordRef::name>(ref<ID> | kw<"fail">))> {};

  struct NameRefRule
      : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<ID>))> {};

  struct StringLitRule
      : rule<create<ast::StringLit>(
             assign<&ast::StringLit::text>(ref<STRING_LITERAL>))> {};

  struct IntLitRule
      : rule<create<ast::IntLit>(
             assign<&ast::IntLit::value>(ref<INTEGER_LITERAL>))> {};

  struct RealLitRule
      : rule<create<ast::RealLit>(
             assign<&ast::RealLit::value>(ref<REAL_LITERAL>))> {};

  struct Primary
      : rule<ref<ExprListRule> | ref<BlockRule> | ref<ListLitRule> |
             ref<KeywordRefRule> | ref<NameRefRule> | ref<StringLitRule> |
             ref<IntLitRule> | ref<RealLitRule>> {};

  struct Subscript
      : rule<create<ast::Subscript>(
             assign<&ast::Subscript::index>(ref<Expression>) +
             option(assign<&ast::Subscript::op>(lit<"+:"> | lit<"-:"> |
                                                 lit<":">) +
                    assign<&ast::Subscript::extent>(ref<Expression>)))> {};

  // expr13: a primary, then any number of postfixes -- each wraps what came
  // before it, so the chain folds left.
  struct Postfix
      : rule<ref<Primary> +
             many(nest<ast::FieldAccess, &ast::FieldAccess::object>(
                      lit<"."> + assign<&ast::FieldAccess::field>(ref<ID>))  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        lit<"("> +
                        many(append<&ast::CallExpr::args>(ref<Expression>),
                             lit<",">) +
                        lit<")">)  //
                  | nest<ast::BraceCallExpr, &ast::BraceCallExpr::callee>(
                        lit<"{"> +
                        many(append<&ast::BraceCallExpr::args>(ref<Expression>),
                             lit<",">) +
                        lit<"}">)  //
                  | nest<ast::SubscriptExpr, &ast::SubscriptExpr::object>(
                        lit<"["> +
                        some(append<&ast::SubscriptExpr::subscripts>(
                                 ref<Subscript>),
                             lit<",">) +
                        lit<"]">))> {};

  // expr12: the prefix chain.
  struct Unary
      : rule<create<ast::PrefixExpr>(
             assign<&ast::PrefixExpr::op>(OpPrefix) +
             assign<&ast::PrefixExpr::operand>(ref<Unary>))  //
             | ref<Postfix>> {};

  // expr5..expr11 folded into one table over the prefix chain.
  struct InnerLadder
      : infix<Unary, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpBackslashAtBang) >>  //
               right(OpCaret) >>           //
               left(OpMulDivModPow) >>     //
               left(OpAddSub) >>           //
               left(OpConcat) >>           //
               left(OpCompare) >>          //
               left(OpPipe))> {};

  // expr4: `a to b`, `a to b by c` -- ternary, so not a level of a ladder;
  // left-associative chaining folds the same way a postfix does.
  struct ToExpr
      : rule<ref<InnerLadder> +
             many(nest<ast::ToRangeExpr, &ast::ToRangeExpr::from>(
                 kw<"to"> + assign<&ast::ToRangeExpr::to>(ref<InnerLadder>) +
                 option(kw<"by"> +
                        assign<&ast::ToRangeExpr::by>(ref<InnerLadder>))))> {
  };

  // expr1..expr3 folded into one table over the to-range level.
  struct OuterLadder
      : infix<ToExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (right(OpAssign) >>  //
               left(OpQuestion) >>  //
               left(OpAmp))> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
             assign<&ast::CaseClause::test>(ref<Expression>) + lit<":"> +
             assign<&ast::CaseClause::value>(ref<Expression>))  //
             | create<ast::CaseClause>(
                   flag<&ast::CaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   assign<&ast::CaseClause::value>(ref<Expression>))> {};

  struct Expression
      : rule<create<ast::BreakExpr>(
                 kw<"break"> +
                 assign<&ast::BreakExpr::value>(ref<Expression>))  //
             | create<ast::CreateExpr>(
                   kw<"create"> +
                   assign<&ast::CreateExpr::value>(ref<Expression>))  //
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   assign<&ast::ReturnExpr::value>(ref<Expression>))  //
             // The source names these clauses "_opt" but never marks them
             // optional at any call site or with an empty alternative: `do`
             // and `else` are mandatory here, as the grammar actually reads.
             | create<ast::SuspendExpr>(
                   kw<"suspend"> +
                   assign<&ast::SuspendExpr::value>(ref<Expression>) +
                   kw<"do"> +
                   assign<&ast::SuspendExpr::doClause>(ref<Expression>))  //
             | create<ast::FailExpr>(kw<"fail">)              //
             | create<ast::NextExpr>(kw<"next">)               //
             | create<ast::CaseExpr>(
                   kw<"case"> +
                   assign<&ast::CaseExpr::selector>(ref<Expression>) +
                   kw<"of"> + lit<"{"> +
                   some(append<&ast::CaseExpr::clauses>(ref<CaseClauseRule>),
                        lit<";">) +
                   lit<"}">)  //
             | create<ast::IfExpr>(
                   kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
                   kw<"then"> +
                   assign<&ast::IfExpr::thenValue>(ref<Expression>) +
                   kw<"else"> +
                   assign<&ast::IfExpr::elseValue>(ref<Expression>))  //
             | create<ast::RepeatExpr>(
                   kw<"repeat"> +
                   assign<&ast::RepeatExpr::body>(ref<Expression>))  //
             | create<ast::WhileExpr>(
                   kw<"while"> +
                   assign<&ast::WhileExpr::cond>(ref<Expression>) +
                   kw<"do"> +
                   assign<&ast::WhileExpr::doClause>(ref<Expression>))  //
             | create<ast::UntilExpr>(
                   kw<"until"> +
                   assign<&ast::UntilExpr::cond>(ref<Expression>) +
                   kw<"do"> +
                   assign<&ast::UntilExpr::doClause>(ref<Expression>))  //
             | create<ast::EveryExpr>(
                   kw<"every"> +
                   assign<&ast::EveryExpr::generator>(ref<Expression>) +
                   kw<"do"> +
                   assign<&ast::EveryExpr::doClause>(ref<Expression>))  //
             | ref<OuterLadder>> {};

  struct Parameter
      : rule<create<ast::Parameter>(
             assign<&ast::Parameter::name>(ref<ID>) +
             option(flag<&ast::Parameter::vararg>(lit<"["> +
                                                        lit<"]">)))> {};

  struct LocalDecl
      : rule<create<ast::LocalDecl>(
             (kw<"local"> |
              flag<&ast::LocalDecl::isStatic>(kw<"static">)) +
             some(append<&ast::LocalDecl::names>(ref<ID>), lit<",">))> {};

  struct ProcedureRule
      : rule<kw<"procedure"> + assign<&ast::Procedure::name>(ref<ID>) +
             lit<"("> +
             many(append<&ast::Procedure::params>(ref<Parameter>), lit<",">) +
             lit<")"> +
             many(append<&ast::Procedure::locals>(ref<LocalDecl>), lit<";">) +
             option(kw<"initial"> +
                    assign<&ast::Procedure::initial>(ref<Expression>) +
                    lit<";">) +
             some(append<&ast::Procedure::body>(ref<Expression>), lit<";">) +
             kw<"end">> {};

  struct LinkDeclRule
      : rule<kw<"link"> +
             some(append<&ast::LinkDeclaration::files>(
                      ref<ID> | ref<STRING_LITERAL>),
                  lit<",">)> {};

  struct GlobalDeclRule
      : rule<kw<"global"> +
             some(append<&ast::GlobalDeclaration::names>(ref<ID>),
                  lit<",">)> {};

  struct RecordDeclRule
      : rule<kw<"record"> + assign<&ast::RecordDeclaration::name>(ref<ID>) +
             lit<"("> +
             some(append<&ast::RecordDeclaration::fields>(ref<ID>),
                  lit<",">) +
             lit<")">> {};

  struct DeclarationRule
      : rule<ref<LinkDeclRule> | ref<GlobalDeclRule> | ref<RecordDeclRule> |
             ref<ProcedureRule>> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             some(append<&ast::Program::declarations>(ref<DeclarationRule>))> {
  };

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace icon
