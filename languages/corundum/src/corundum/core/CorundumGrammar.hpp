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


  static constexpr auto AnyAssignOp =
      as<ast::AssignOp::assign>(lit<"=">) | as<ast::AssignOp::add>(lit<"+=">) | as<ast::AssignOp::sub>(lit<"-=">) |
      as<ast::AssignOp::mul>(lit<"*=">) | as<ast::AssignOp::div>(lit<"/=">) | as<ast::AssignOp::mod>(lit<"%=">) |
      as<ast::AssignOp::exp>(lit<"**=">);

  struct MulOp : terminal<as<ast::BinOp::mul>(lit<"*">)> {};
  struct AddOp : terminal<as<ast::BinOp::add>(lit<"+">)> {};
  struct AndOp : terminal<as<ast::BinOp::logical_and>(kw<"and"> | lit<"&&">)> {
  };
  struct OrOp : terminal<as<ast::BinOp::logical_or>(kw<"or"> | lit<"||">)> {};

  struct NotTok : terminal<as<ast::UnOp::logical_not>(kw<"not"> | lit<"!">)> {};

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
                 assign<&ast::NamedArg::name>(ref<ID>) + as<ast::AssignOp::assign>(lit<"=">) +
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
            assign<&ast::BoolLit::value>(as<true>(kw<"true">) | as<false>(kw<"false">)))> {};

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

  // Value is the narrower arithmetic domain used by the source grammar.

  struct Value : infix<ValuePrimary, ast::BinaryExpr,
                       levels(left(as<ast::BinOp::mul>(lit<"*">) | as<ast::BinOp::div>(lit<"/">) |
                                   as<ast::BinOp::mod>(lit<"%">)),  //
                              left(as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::sub>(lit<"-">)))> {};

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

  struct Power : infix<ExprPrimary, ast::BinaryExpr,
                       left(as<ast::BinOp::exp>(lit<"**">))> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ref<NotTok> | as<ast::UnOp::bit_not>(lit<"~">)) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | ref<Power>> {};

  struct Expression
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(ref<MulOp> | as<ast::BinOp::div>(lit<"/">) | as<ast::BinOp::mod>(lit<"%">)),          //
                     left(ref<AddOp> | as<ast::BinOp::sub>(lit<"-">)),                       //
                     left(as<ast::BinOp::shl>(lit<"<<">) | as<ast::BinOp::shr>(lit<">>">)),                       //
                     left(as<ast::BinOp::bit_and>(lit<"&">)),                                 //
                     left(as<ast::BinOp::bit_or>(lit<"|">) | as<ast::BinOp::bit_xor>(lit<"^">)),                  //
                     left(as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::gt>(lit<">">) | as<ast::BinOp::le>(lit<"<=">) |  //
                          as<ast::BinOp::ge>(lit<">=">)),                              //
                     left(as<ast::BinOp::eq>(lit<"==">) | as<ast::BinOp::ne>(lit<"!=">)),                      //
                     left(ref<OrOp> | ref<AndOp>))> {};

  struct Condition;
  struct Comparison
      : rule<create<ast::BinaryExpr>(
            assign<&ast::BinaryExpr::left>(ref<Value>) +
            assign<&ast::BinaryExpr::op>(
                as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::gt>(lit<">">) |
                as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::ge>(lit<">=">) |
                as<ast::BinOp::eq>(lit<"==">) | as<ast::BinOp::ne>(lit<"!=">)) +
            assign<&ast::BinaryExpr::right>(ref<Value>))> {};
  struct ConditionAtom
      : rule<ref<Comparison> | (lit<"("> + ref<Condition> + lit<")">)> {};
  // The author's comparison_list is right-recursive, with all four
  // boolean/bitwise spellings on one level. This differs from rvalue.
  struct Condition
      : infix<ConditionAtom, ast::BinaryExpr,
              right(ref<AndOp> | ref<OrOp> |
                    as<ast::BinOp::logical_and>(lit<"&">) |
                    as<ast::BinOp::logical_or>(lit<"|">))> {};
  static constexpr auto ConditionalValue = ref<Condition> | ref<Expression>;

  static constexpr auto Terminator = some(lit<";"> | ref<CRLF>);

  struct Block
      : rule<create<ast::Block> +
             some(option(append<&ast::Block::statements>(ref<Stmt>)) +
                  (lit<";"> | ref<CRLF>))> {};

  struct Retry : rule<create<ast::Retry>(kw<"retry">)> {};
  struct Break : rule<create<ast::Break>(kw<"break">)> {};

  struct LoopBlock
      : rule<create<ast::Block> +
             some((append<&ast::Block::statements>(ref<Stmt>) + Terminator) |
                  (append<&ast::Block::statements>(ref<Retry>) +
                   Terminator) |
                  (append<&ast::Block::statements>(ref<Break>) +
                   Terminator))> {};

  struct ElsIf
      : rule<kw<"elsif"> +
             create<ast::ElsIf>(assign<&ast::ElsIf::cond>(ConditionalValue) +
                                ref<CRLF> +
                                assign<&ast::ElsIf::body>(ref<LoopBlock>))> {
  };

  static constexpr auto ConditionalTail =
      assign<&ast::Conditional::cond>(ConditionalValue) + ref<CRLF> +
      assign<&ast::Conditional::thenBody>(ref<LoopBlock>) +
      many(append<&ast::Conditional::elifs>(ref<ElsIf>)) +
      option(kw<"else"> + ref<CRLF> +
             assign<&ast::Conditional::elseBody>(ref<LoopBlock>));

  struct If
      : rule<kw<"if"> + create<ast::Conditional>(ConditionalTail) +
             kw<"end">> {};

  struct Unless
      : rule<create<ast::Conditional>(
                 flag<&ast::Conditional::negated>(kw<"unless">) +
                 ConditionalTail) +
             kw<"end">> {};

  struct While
      : rule<kw<"while"> +
             create<ast::While>(assign<&ast::While::cond>(ConditionalValue) +
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
      assign<&ast::For::cond>(ConditionalValue) + lit<";"> +
      some(append<&ast::For::step>(ref<AssignItem>), lit<",">);

  struct For
      : rule<kw<"for"> +
             create<ast::For>(
                 ((lit<"("> + ForClauses + lit<")">) | ForClauses) +
                 ref<CRLF> + assign<&ast::For::body>(ref<LoopBlock>)) +
             kw<"end">> {};

  struct Return
      : rule<kw<"return"> +
             create<ast::Return>(assign<&ast::Return::value>(ref<Value>))> {
  };

  struct Require
      : rule<kw<"require"> +
             create<ast::Require>(
                 assign<&ast::Require::value>(ref<StringLitAtom>))> {};

  struct Pir
      : rule<kw<"pir"> +
             create<ast::Pir>(ref<CRLF> + assign<&ast::Pir::body>(ref<Block>)) +
             kw<"end">> {};

  static constexpr auto ParamList =
      some(append<&ast::FuncDef::params>(ref<ID>), lit<",">);

  struct FuncDef
      : rule<kw<"def"> +
             create<ast::FuncDef>(
                 assign<&ast::FuncDef::name>(ref<ID_FUNCTION> | ref<ID>) +
                 option((lit<"("> + option(ParamList) + lit<")">) |
                        ParamList) +
                 ref<CRLF> + assign<&ast::FuncDef::body>(ref<Block>)) +
             kw<"end">> {};

  struct Stmt : rule<ref<If>        //
                     | ref<Unless>  //
                     | ref<While>   //
                     | ref<For>     //
                     | ref<Return>  //
                     | ref<Require> //
                     | ref<Pir>     //
                     | ref<FuncDef> //
                     | ref<Expression>> {};

  using entry = Block;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace corundum
