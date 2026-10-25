// The corundum language: its grammar. A Ruby-like scripting language whose
// reference threads four parallel "typed" expression hierarchies
// (int/float/string/dynamic_result) through comparisons, array elements and
// call arguments to fake static typing; since they overlap completely in
// what they accept, this grammar collapses them into one expression ladder
// (Expression) and a narrower arithmetic-only one (Value) that mirrors what
// those typed rules actually offered (mul/div/mod, add/sub, no comparisons,
// no bitwise ops, no assignment).
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "corundum/core/ast.hpp"

namespace corundum {

struct Corundum : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct SL_COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + option(eol)> {};
  struct ML_COMMENT
      : terminal<through(lit<"=begin">, lit<"=end">) + option(eol)> {};

  struct CRLF : terminal<option(lit<"\r">) + lit<"\n">> {};

  static constexpr auto IdBody = cr<"a-zA-Z_"> + many(w);
  struct ID : terminal<IdBody> {};
  struct ID_FUNCTION : terminal<IdBody + lit<"?">> {};
  struct ID_GLOBAL : terminal<lit<"$"> + IdBody> {};

  struct INT : terminal<some(d)> {};
  struct FLOAT : terminal<many(d) + lit<"."> + some(d)> {};

  static constexpr auto EscapedQuote = lit<"\\\"">;
  struct LITERAL
      : terminal<(lit<"\""> + many(EscapedQuote | (!cr<R"("\r\n)"> + any)) +
                  lit<"\"">) |
                 (lit<"'"> + many(EscapedQuote | (!cr<R"('\r\n)"> + any)) +
                  lit<"'">)> {};

  struct TrueTok : terminal<kw<"true">, value<true>> {};
  struct FalseTok : terminal<kw<"false">, value<false>> {};

  struct PlainAssignOp : terminal<lit<"=">, value<ast::AssignOp::assign>> {};
  struct AddAssignOp : terminal<lit<"+=">, value<ast::AssignOp::add>> {};
  struct SubAssignOp : terminal<lit<"-=">, value<ast::AssignOp::sub>> {};
  struct MulAssignOp : terminal<lit<"*=">, value<ast::AssignOp::mul>> {};
  struct DivAssignOp : terminal<lit<"/=">, value<ast::AssignOp::div>> {};
  struct ModAssignOp : terminal<lit<"%=">, value<ast::AssignOp::mod>> {};
  struct ExpAssignOp : terminal<lit<"**=">, value<ast::AssignOp::exp>> {};
  static constexpr auto AnyAssignOp =
      ref<PlainAssignOp> | ref<AddAssignOp> | ref<SubAssignOp> |
      ref<MulAssignOp> | ref<DivAssignOp> | ref<ModAssignOp> |
      ref<ExpAssignOp>;

  struct MulOp : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct DivOp : terminal<lit<"/">, value<ast::BinOp::div>> {};
  struct ModOp : terminal<lit<"%">, value<ast::BinOp::mod>> {};
  struct AddOp : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct SubOp : terminal<lit<"-">, value<ast::BinOp::sub>> {};
  struct ExpOp : terminal<lit<"**">, value<ast::BinOp::exp>> {};
  struct ShlOp : terminal<lit<"<<">, value<ast::BinOp::shl>> {};
  struct ShrOp : terminal<lit<">>">, value<ast::BinOp::shr>> {};
  struct BitAndOp : terminal<lit<"&">, value<ast::BinOp::bit_and>> {};
  struct BitOrOp : terminal<lit<"|">, value<ast::BinOp::bit_or>> {};
  struct BitXorOp : terminal<lit<"^">, value<ast::BinOp::bit_xor>> {};
  struct LessOp : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct GreaterOp : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct LessEqOp : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct GreaterEqOp : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct EqOp : terminal<lit<"==">, value<ast::BinOp::eq>> {};
  struct NotEqOp : terminal<lit<"!=">, value<ast::BinOp::ne>> {};
  struct AndOp
      : terminal<kw<"and"> | lit<"&&">, value<ast::BinOp::logical_and>> {};
  struct OrOp
      : terminal<kw<"or"> | lit<"||">, value<ast::BinOp::logical_or>> {};

  struct NotTok
      : terminal<kw<"not"> | lit<"!">, value<ast::UnOp::logical_not>> {};
  struct BitNotTok : terminal<lit<"~">, value<ast::UnOp::bit_not>> {};

  struct Stmt;
  struct Expression;
  struct Value;

  struct IdentAtom
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))> {};
  struct GlobalIdentAtom
      : rule<create<ast::GlobalIdent>(
            assign<&ast::GlobalIdent::name>(ref<ID_GLOBAL>))> {};

  static constexpr auto IndexSuffix = nest<ast::Index, &ast::Index::base>(
      lit<"["> + assign<&ast::Index::index>(ref<Value>) + lit<"]">);

  struct FuncArg;
  static constexpr auto CallArgList =
      some(append<&ast::Call::args>(ref<FuncArg>), lit<",">);

  struct CallAtom
      : rule<create<ast::Call>(
            assign<&ast::Call::name>(ref<ID_FUNCTION> | ref<ID>) +
            ((lit<"("> + option(CallArgList) + lit<")">) | CallArgList))> {};

  struct ReadAtom : rule<ref<CallAtom>                              //
                         | (ref<IdentAtom> + option(IndexSuffix))    //
                         | (ref<GlobalIdentAtom> + option(IndexSuffix))> {
  };

  struct FuncArg
      : rule<create<ast::NamedArg>(
                 assign<&ast::NamedArg::name>(ref<ID>) + ref<PlainAssignOp> +
                 assign<&ast::NamedArg::value>(ref<Value>))  //
             | ref<Value>> {};

  struct IntLitAtom
      : rule<create<ast::IntLit>(assign<&ast::IntLit::value>(ref<INT>))> {};
  struct FloatLitAtom
      : rule<create<ast::FloatLit>(
            assign<&ast::FloatLit::value>(ref<FLOAT>))> {};
  struct StringLitAtom
      : rule<create<ast::StringLit>(
            assign<&ast::StringLit::text>(ref<LITERAL>))> {};
  struct BoolLitAtom
      : rule<create<ast::BoolLit>(
            assign<&ast::BoolLit::value>(ref<TrueTok> | ref<FalseTok>))> {};

  struct ArrayLitAtom
      : rule<lit<"["> +
             create<ast::ArrayLit>(
                 option(some(append<&ast::ArrayLit::elements>(ref<Value>),
                             lit<",">))) +
             lit<"]">> {};

  struct ValuePrimary : rule<ref<ReadAtom>       //
                             | ref<IntLitAtom>    //
                             | ref<FloatLitAtom>  //
                             | ref<StringLitAtom> //
                             | (lit<"("> + ref<Value> + lit<")">)> {};

  // A separate operator set from `Expression`'s: an infix table's operator
  // tokens belong to one grouping in the whole grammar, so the two ladders
  // cannot share `MulOp`/`AddOp` even though they spell the same text.
  struct ValMulOp : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct ValDivOp : terminal<lit<"/">, value<ast::BinOp::div>> {};
  struct ValModOp : terminal<lit<"%">, value<ast::BinOp::mod>> {};
  struct ValAddOp : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct ValSubOp : terminal<lit<"-">, value<ast::BinOp::sub>> {};

  struct Value : infix<ValuePrimary, &ast::BinaryExpr::left,
                       &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                       (left(ref<ValMulOp> | ref<ValDivOp> | ref<ValModOp>) >>  //
                        left(ref<ValAddOp> | ref<ValSubOp>))> {};

  struct ExprPrimary
      : rule<(ref<ReadAtom> +
              option(nest<ast::Assign, &ast::Assign::target>(
                  assign<&ast::Assign::op>(AnyAssignOp) +
                  (assign<&ast::Assign::value>(ref<ArrayLitAtom>)  //
                   | assign<&ast::Assign::value>(ref<Expression>)))))  //
             | ref<IntLitAtom>                                         //
             | ref<FloatLitAtom>                                       //
             | ref<StringLitAtom>                                      //
             | ref<BoolLitAtom>                                        //
             | create<ast::NilLit>(kw<"nil">)                          //
             | (lit<"("> + ref<Expression> + lit<")">)> {};

  struct Power : infix<ExprPrimary, &ast::BinaryExpr::left,
                       &ast::BinaryExpr::op, &ast::BinaryExpr::right,
                       left(ref<ExpOp>)> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NotTok> | ref<BitNotTok>) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<Power>> {};

  struct Expression
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MulOp> | ref<DivOp> | ref<ModOp>) >>         //
               left(ref<AddOp> | ref<SubOp>) >>                     //
               left(ref<ShlOp> | ref<ShrOp>) >>                     //
               left(ref<BitAndOp>) >>                               //
               left(ref<BitOrOp> | ref<BitXorOp>) >>                //
               left(ref<LessOp> | ref<GreaterOp> | ref<LessEqOp> |  //
                    ref<GreaterEqOp>) >>                            //
               left(ref<EqOp> | ref<NotEqOp>) >>                    //
               left(ref<OrOp> | ref<AndOp>))> {};

  static constexpr auto Terminator = some(lit<";"> | ref<CRLF>);

  struct Block
      : rule<create<ast::Block> +
             some(option(append<&ast::Block::statements>(ref<Stmt>)) +
                  (lit<";"> | ref<CRLF>))> {};

  struct RetryRule : rule<create<ast::Retry>(kw<"retry">)> {};
  struct BreakRule : rule<create<ast::Break>(kw<"break">)> {};

  struct LoopBlock
      : rule<create<ast::Block> +
             some((append<&ast::Block::statements>(ref<Stmt>) + Terminator) |
                  (append<&ast::Block::statements>(ref<RetryRule>) +
                   Terminator) |
                  (append<&ast::Block::statements>(ref<BreakRule>) +
                   Terminator))> {};

  struct ElsIfRule
      : rule<kw<"elsif"> +
             create<ast::ElsIf>(assign<&ast::ElsIf::cond>(ref<Expression>) +
                                ref<CRLF> +
                                assign<&ast::ElsIf::body>(ref<LoopBlock>))> {
  };

  static constexpr auto ConditionalTail =
      assign<&ast::Conditional::cond>(ref<Expression>) + ref<CRLF> +
      assign<&ast::Conditional::thenBody>(ref<LoopBlock>) +
      many(append<&ast::Conditional::elifs>(ref<ElsIfRule>)) +
      option(kw<"else"> + ref<CRLF> +
             assign<&ast::Conditional::elseBody>(ref<LoopBlock>));

  struct IfRule
      : rule<kw<"if"> + create<ast::Conditional>(ConditionalTail) +
             kw<"end">> {};

  struct UnlessRule
      : rule<create<ast::Conditional>(
                 flag<&ast::Conditional::negated>(kw<"unless">) +
                 ConditionalTail) +
             kw<"end">> {};

  struct WhileRule
      : rule<kw<"while"> +
             create<ast::While>(assign<&ast::While::cond>(ref<Expression>) +
                                ref<CRLF> +
                                assign<&ast::While::body>(ref<LoopBlock>)) +
             kw<"end">> {};

  struct AssignItem
      : rule<create<ast::Assign>(
            assign<&ast::Assign::target>(ref<IdentAtom>) +
            assign<&ast::Assign::op>(AnyAssignOp) +
            assign<&ast::Assign::value>(ref<Value>))> {};

  static constexpr auto ForClauses =
      some(append<&ast::For::init>(ref<AssignItem>), lit<",">) + lit<";"> +
      assign<&ast::For::cond>(ref<Expression>) + lit<";"> +
      some(append<&ast::For::step>(ref<AssignItem>), lit<",">);

  struct ForRule
      : rule<kw<"for"> +
             create<ast::For>(
                 ((lit<"("> + ForClauses + lit<")">) | ForClauses) +
                 ref<CRLF> + assign<&ast::For::body>(ref<LoopBlock>)) +
             kw<"end">> {};

  struct ReturnRule
      : rule<kw<"return"> +
             create<ast::Return>(assign<&ast::Return::value>(ref<Value>))> {
  };

  struct RequireRule
      : rule<kw<"require"> +
             create<ast::Require>(
                 assign<&ast::Require::value>(ref<StringLitAtom>))> {};

  struct PirRule
      : rule<kw<"pir"> +
             create<ast::Pir>(ref<CRLF> + assign<&ast::Pir::body>(ref<Block>)) +
             kw<"end">> {};

  static constexpr auto ParamList =
      some(append<&ast::FuncDef::params>(ref<ID>), lit<",">);

  struct FuncDefRule
      : rule<kw<"def"> +
             create<ast::FuncDef>(
                 assign<&ast::FuncDef::name>(ref<ID_FUNCTION> | ref<ID>) +
                 option((lit<"("> + option(ParamList) + lit<")">) |
                        ParamList) +
                 ref<CRLF> + assign<&ast::FuncDef::body>(ref<Block>)) +
             kw<"end">> {};

  struct Stmt : rule<ref<IfRule>        //
                     | ref<UnlessRule>  //
                     | ref<WhileRule>   //
                     | ref<ForRule>     //
                     | ref<ReturnRule>  //
                     | ref<RequireRule> //
                     | ref<PirRule>     //
                     | ref<FuncDefRule> //
                     | ref<Expression>> {};

  using entry = Block;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace corundum
