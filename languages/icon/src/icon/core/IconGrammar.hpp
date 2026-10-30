// The Icon language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "icon/core/ast.hpp"

namespace icon {

inline auto integer_spelling(std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  const auto marker = text.find_first_of("rR");
  if (marker == text.npos) return Result::ok(text);
  unsigned base = 0;
  for (std::size_t i = 0; i < marker; ++i) {
    base = base * 10 + unsigned(text[i] - '0');
    if (base > 36) return Result::err("An integer radix must be between 2 and 36.");
  }
  if (base < 2) return Result::err("An integer radix must be between 2 and 36.");
  for (char c : text.substr(marker + 1)) {
    const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
                           c >= 'a' && c <= 'z' ? c - 'a' + 10 : c - 'A' + 10;
    if (digit >= base) return Result::err("The digit is outside the integer radix.");
  }
  return Result::ok(text);
}

struct Icon : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct INTEGER_LITERAL
      : terminal<as<integer_spelling>(some(d) + option(cr<"rR"> + some(cr<"0-9a-zA-Z">)))> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct REAL_LITERAL
      : terminal<(some(d) + lit<"."> + many(d) + option(Exponent)) |
                 (lit<"."> + some(d) + option(Exponent)) |
                 (some(d) + Exponent)> {};
  static constexpr auto ContinuedString =
      lit<"_"> + many(cr<R"( \t\r\v\f)">) + lit<"\n"> + many(s);
  static constexpr auto StringEscape = lit<"\\"> + any;
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(ContinuedString | StringEscape | cr<R"(^"\\\r\n)">) + lit<"\"">> {};
  struct CSET_LITERAL
      : terminal<lit<"'"> + many(ContinuedString | StringEscape | cr<R"(^'\\\r\n)">) + lit<"'">> {};

  static constexpr auto idChar = cr<"a-zA-Z0-9_">;
  static constexpr auto word_boundary = idChar;
  static constexpr auto ReservedWord =
      kw<"break"> | kw<"by"> | kw<"case"> | kw<"create"> | kw<"default"> |
      kw<"do"> | kw<"else"> | kw<"end"> | kw<"every"> | kw<"fail"> |
      kw<"global"> | kw<"if"> | kw<"initial"> | kw<"invocable"> | kw<"link"> |
      kw<"local"> | kw<"next"> | kw<"not"> | kw<"of"> | kw<"procedure"> |
      kw<"record"> | kw<"repeat"> | kw<"return"> | kw<"static"> | kw<"suspend"> |
      kw<"then"> | kw<"to"> | kw<"until"> | kw<"while">;
  struct ID : terminal<!ReservedWord + cr<"a-zA-Z_"> + many(idChar)> {};
  static constexpr auto LBrace = lit<"{"> | lit<"$(">;
  static constexpr auto RBrace = lit<"}"> | lit<"$)">;
  static constexpr auto LBracket = lit<"["> | lit<"$<">;
  static constexpr auto RBracket = lit<"]"> | lit<"$>">;
  static constexpr auto StatementBreak = some(lit<";">) | gap::has(eol);
  // Native yylex inserts a semicolon at a newline between an Ender and a
  // Beginner. Every complete expression ends with an Ender; operators marked
  // Beginner must therefore start the next expression across such a gap.
  static constexpr auto SameLine = !gap::has(eol);

  // Operator sets, tightest level first -- one token opens on one step.
  static constexpr auto OpBackslashAtBang = SameLine + (lit<"\\"> | lit<"@"> | lit<"!">);
  static constexpr auto OpCaret = SameLine + lit<"^">;
  static constexpr auto OpMulDivModPow =
      (SameLine + (lit<"**"> | lit<"*"> | lit<"/">)) | lit<"%">;
  static constexpr auto OpAddSub = SameLine + (lit<"++"> | lit<"--"> | lit<"+"> | lit<"-">);
  static constexpr auto OpConcat = SameLine + (lit<"|||"> | lit<"||">);
  static constexpr auto OpCompare =
      lit<"<<="> | lit<"<<"> | lit<"<="> | lit<"<"> | lit<">>="> |
      lit<">>"> | lit<">="> | lit<">"> |
      (SameLine + (lit<"~==="> | lit<"==="> | lit<"~=="> | lit<"=="> | lit<"~="> | lit<"=">));
  static constexpr auto OpPipe = SameLine + lit<"|">;
  static constexpr auto OpAssign = lit<":=:"> | lit<":="> | lit<"<->"> | lit<"<-"> |
      lit<"%:="> | lit<"&:="> | lit<"*:="> | lit<"**:="> | lit<"+:="> | lit<"++:="> | lit<"-:="> | lit<"--:="> | lit<"/:="> | lit<"<:="> | lit<"<<:="> | lit<"<<=:="> | lit<"<=:="> | lit<"=:="> | lit<"==:="> | lit<"===:="> | lit<">:="> | lit<">=:="> | lit<">>:="> | lit<">>=:="> | lit<"?:="> | lit<"@:="> | lit<"^:="> | lit<"||:="> | lit<"|||:="> | lit<"~=:="> | lit<"~==:="> | lit<"~===:=">;
  static constexpr auto OpQuestion = SameLine + lit<"?">;
  static constexpr auto OpAmp = SameLine + lit<"&">;

  static constexpr auto OpPrefix =
      kw<"not"> | lit<"|||"> | lit<"||"> | lit<"++"> | lit<"--"> |
      lit<"**"> | lit<"~==="> | lit<"~=="> | lit<"~="> | lit<"==="> | lit<"=="> |
      lit<"|"> | lit<"!"> | lit<"*"> | lit<"+"> | lit<"-"> |
      lit<"."> | lit<"/"> | lit<"\\"> | lit<"="> | lit<"?"> | lit<"~"> |
      lit<"@"> | lit<"^">;

  struct Expression;
  struct ControlExpr;
  struct MaybeExpression;
  struct MaybeSubscript;
  struct OuterLadder;
  struct ToExpr;
  struct InnerLadder;
  struct Unary;
  struct Postfix;
  struct Primary;
  struct Subscript;
  struct CaseClause;
  struct Parameter;
  struct LocalDecl;
  struct Procedure;
  struct Declaration;

  // Empty slots are explicit null nodes only when a comma was written.
  // Empty () and [] keep zero items.
  template<auto Member, class Nullable>
  static constexpr auto ExpressionItems = option(
      append<Member>(ref<Expression>) + many(lit<","> + append<Member>(ref<Nullable>)) |
      (&lit<","> + append<Member>(create<ast::NullExpr>) +
       some(lit<","> + append<Member>(ref<Nullable>))));
  template<auto Member>
  static constexpr auto ExpressionSequence = many(lit<";">) +
      many(append<Member>(ref<Expression>) + StatementBreak) +
      option(append<Member>(ref<Expression>)) + many(lit<";">);

  struct ExprList
      : rule<create<ast::ExprList>(lit<"("> +
              ExpressionItems<&ast::ExprList::items, MaybeExpression> + lit<")">)> {};
  struct Block
      : rule<create<ast::Block>(LBrace + ExpressionSequence<&ast::Block::body> + RBrace)> {};
  struct ListLit
      : rule<create<ast::ListLit>(LBracket +
              ExpressionItems<&ast::ListLit::items, MaybeExpression> + RBracket)> {};

  struct KeywordRef
      : rule<create<ast::KeywordRef>(
             lit<"&"> + assign<&ast::KeywordRef::name>(ref<ID> | kw<"fail">))> {};

  struct NameRef
      : rule<create<ast::NameRef>(assign<&ast::NameRef::name>(ref<ID>))> {};

  struct StringLit
      : rule<create<ast::StringLit>(
             assign<&ast::StringLit::text>(ref<STRING_LITERAL>))> {};

  struct CsetLit
      : rule<create<ast::CsetLit>(assign<&ast::CsetLit::text>(ref<CSET_LITERAL>))> {};

  struct IntLit
      : rule<create<ast::IntLit>(
             assign<&ast::IntLit::text>(ref<INTEGER_LITERAL>))> {};

  struct RealLit
      : rule<create<ast::RealLit>(
             assign<&ast::RealLit::text>(ref<REAL_LITERAL>))> {};

  struct Primary
      : rule<ref<ExprList> | ref<Block> | ref<ListLit> |
             ref<ControlExpr> | ref<KeywordRef> | ref<NameRef> | ref<StringLit> | ref<CsetLit> |
             ref<IntLit> | ref<RealLit>> {};

  struct Subscript
      : rule<create<ast::Subscript>(
             assign<&ast::Subscript::index>(ref<Expression>) +
             option(assign<&ast::Subscript::op>(lit<"+:"> | lit<"-:"> |
                                                 lit<":">) +
                    assign<&ast::Subscript::extent>(ref<Expression>)))> {};

  struct MaybeExpression : rule<ref<Expression> | create<ast::NullExpr>> {};
  struct MaybeSubscript : rule<ref<Subscript> |
      create<ast::Subscript>(assign<&ast::Subscript::index>(create<ast::NullExpr>))> {};
  static constexpr auto SubscriptItems = option(
      append<&ast::SubscriptExpr::subscripts>(ref<Subscript>) +
      many(lit<","> + append<&ast::SubscriptExpr::subscripts>(ref<MaybeSubscript>)) |
      (&lit<","> + append<&ast::SubscriptExpr::subscripts>(
          create<ast::Subscript>(assign<&ast::Subscript::index>(create<ast::NullExpr>))) +
       some(lit<","> + append<&ast::SubscriptExpr::subscripts>(ref<MaybeSubscript>))));

  // expr13: a primary, then any number of postfixes -- each wraps what came
  // before it, so the chain folds left.
  struct Postfix
      : rule<ref<Primary> +
             many(nest<ast::FieldAccess, &ast::FieldAccess::object>(
                      SameLine + lit<"."> + assign<&ast::FieldAccess::field>(ref<ID>))  //
                  | nest<ast::CallExpr, &ast::CallExpr::callee>(
                        SameLine + lit<"("> +
                        ExpressionItems<&ast::CallExpr::args, MaybeExpression> +
                        lit<")">)  //
                  | nest<ast::BraceCallExpr, &ast::BraceCallExpr::callee>(
                        SameLine + LBrace +
                        ExpressionItems<&ast::BraceCallExpr::args, MaybeExpression> +
                        RBrace)  //
                  | nest<ast::SubscriptExpr, &ast::SubscriptExpr::object>(
                        SameLine + LBracket +
                        SubscriptItems +
                        RBracket))> {};

  // expr12: the prefix chain.
  struct Unary
      : rule<create<ast::PrefixExpr>(
             assign<&ast::PrefixExpr::op>(OpPrefix) +
             assign<&ast::PrefixExpr::operand>(ref<Unary>))  //
             | ref<Postfix>> {};

  // expr5..expr11 folded into one table over the prefix chain.
  struct InnerLadder
      : infix<Unary, ast::BinaryExpr,
              levels(left(OpBackslashAtBang),  //
                     right(OpCaret),           //
                     left(OpMulDivModPow),     //
                     left(OpAddSub),           //
                     left(OpConcat),           //
                     left(OpCompare),          //
                     right(OpPipe))> {};

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
      : infix<ToExpr, ast::BinaryExpr,
              levels(right(OpAssign),  //
                     left(OpQuestion),  //
                     left(OpAmp))> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
             assign<&ast::CaseClause::test>(ref<Expression>) + lit<":"> +
             assign<&ast::CaseClause::value>(ref<Expression>))  //
             | create<ast::CaseClause>(
                   flag<&ast::CaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   assign<&ast::CaseClause::value>(ref<Expression>))> {};

  struct ControlExpr
      : rule<create<ast::BreakExpr>(
                 kw<"break"> +
                 option(SameLine + assign<&ast::BreakExpr::value>(ref<Expression>)))  //
             | create<ast::CreateExpr>(
                   kw<"create"> +
                   assign<&ast::CreateExpr::value>(ref<Expression>))  //
             | create<ast::ReturnExpr>(
                   kw<"return"> +
                   option(SameLine + assign<&ast::ReturnExpr::value>(ref<Expression>)))  //
             | create<ast::SuspendExpr>(
                   kw<"suspend"> +
                   option(SameLine + assign<&ast::SuspendExpr::value>(ref<Expression>)) +
                   option(kw<"do"> + assign<&ast::SuspendExpr::doClause>(ref<Expression>)))  //
             | create<ast::FailExpr>(kw<"fail">)              //
             | create<ast::NextExpr>(kw<"next">)               //
             | create<ast::CaseExpr>(
                   kw<"case"> +
                   assign<&ast::CaseExpr::selector>(ref<Expression>) +
                   kw<"of"> + LBrace +
                   some(append<&ast::CaseExpr::clauses>(ref<CaseClause>),
                        StatementBreak) +
                   RBrace)  //
             | create<ast::IfExpr>(
                   kw<"if"> + assign<&ast::IfExpr::cond>(ref<Expression>) +
                   kw<"then"> +
                   assign<&ast::IfExpr::thenValue>(ref<Expression>) +
                   option(kw<"else"> + assign<&ast::IfExpr::elseValue>(ref<Expression>)))  //
             | create<ast::RepeatExpr>(
                   kw<"repeat"> +
                   assign<&ast::RepeatExpr::body>(ref<Expression>))  //
             | create<ast::WhileExpr>(
                   kw<"while"> +
                   assign<&ast::WhileExpr::cond>(ref<Expression>) +
                   option(kw<"do"> + assign<&ast::WhileExpr::doClause>(ref<Expression>)))  //
             | create<ast::UntilExpr>(
                   kw<"until"> +
                   assign<&ast::UntilExpr::cond>(ref<Expression>) +
                   option(kw<"do"> + assign<&ast::UntilExpr::doClause>(ref<Expression>)))  //
             | create<ast::EveryExpr>(
                   kw<"every"> +
                   assign<&ast::EveryExpr::generator>(ref<Expression>) +
                   option(kw<"do"> + assign<&ast::EveryExpr::doClause>(ref<Expression>)))  //
             > {};

  struct Expression : rule<ref<OuterLadder>> {};

  struct Parameter
      : rule<create<ast::Parameter>(
             assign<&ast::Parameter::name>(ref<ID>) +
             option(flag<&ast::Parameter::vararg>(LBracket +
                                                        RBracket)))> {};

  struct LocalDecl
      : rule<create<ast::LocalDecl>(
             (kw<"local"> |
              flag<&ast::LocalDecl::isStatic>(kw<"static">)) +
             some(append<&ast::LocalDecl::names>(ref<ID>), lit<",">))> {};

  struct Procedure
      : rule<kw<"procedure"> + assign<&ast::Procedure::name>(ref<ID>) +
             lit<"("> +
             many(append<&ast::Procedure::params>(ref<Parameter>), lit<",">) +
             lit<")"> +
             many(lit<";">) +
             many(append<&ast::Procedure::locals>(ref<LocalDecl>) + StatementBreak) +
             option(kw<"initial"> +
                    assign<&ast::Procedure::initial>(ref<Expression>) +
                    StatementBreak) +
             ExpressionSequence<&ast::Procedure::body> +
             kw<"end">> {};

  struct LinkDecl
      : rule<kw<"link"> +
             some(append<&ast::LinkDeclaration::files>(
                      ref<ID> | ref<STRING_LITERAL>),
                  lit<",">)> {};

  struct GlobalDecl
      : rule<kw<"global"> +
             some(append<&ast::GlobalDeclaration::names>(ref<ID>),
                  lit<",">)> {};

  struct RecordDecl
      : rule<kw<"record"> + assign<&ast::RecordDeclaration::name>(ref<ID>) +
             lit<"("> +
             many(append<&ast::RecordDeclaration::fields>(ref<ID>),
                  lit<",">) +
             lit<")">> {};

  struct InvocableItem
      : rule<assign<&ast::InvocableItem::name>(ref<ID> | ref<STRING_LITERAL>) +
             option(lit<":"> + assign<&ast::InvocableItem::arity>(ref<INTEGER_LITERAL>))> {};
  struct InvocableDecl
      : rule<kw<"invocable"> + some(append<&ast::InvocableDeclaration::items>(ref<InvocableItem>), lit<",">)> {};

  struct Declaration
      : rule<ref<LinkDecl> | ref<GlobalDecl> | ref<RecordDecl> |
             ref<Procedure> | ref<InvocableDecl>> {};

  struct Program
      : rule<create<ast::Program> +
             many(append<&ast::Program::declarations>(ref<Declaration>))> {
  };

  using entry = Program;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace icon
