// The gvpr language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gvpr/core/ast.hpp"

namespace gvpr {

struct Gvpr : cursiv::GrammarSpec {
  // Trivia. A `#` starts a comment only when nothing but a line break comes
  // before it -- column zero, which a hidden terminal cannot ask about
  // directly, so the newline is read as part of the token itself. The one
  // document whose very first byte is `#`, with no line break to carry, is
  // skipped explicitly at the front of the entry rule below.
  struct WS : terminal<some(cr<R"( \t\n\r)">)> {};
  struct MLCOMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct SLCOMMENT : terminal<lit<"//"> + many(cr<R"(^\n\r)">)> {};
  // Blank lines between the newline and `#` are pure whitespace too, so the
  // run back to it is read as part of this token: otherwise WS's own greedy
  // run across those blank lines reaches the newline first.
  struct SHELLCOMMENT
      : terminal<many(cr<R"( \t)">) + eol + many(cr<R"( \t\r\n)">) + lit<"#"> +
                many(cr<R"(^\n\r)">)> {};
  struct LeadingShellComment : terminal<lit<"#"> + many(cr<R"(^\n\r)">)> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexQuad = HexDigit + HexDigit + HexDigit + HexDigit;
  static constexpr auto UniversalCharacterName =
      (lit<"\\u"> + HexQuad) | (lit<"\\U"> + HexQuad + HexQuad);
  static constexpr auto IdentifierNondigit =
      cr<"a-zA-Z_$"> | UniversalCharacterName;

  struct ID : terminal<IdentifierNondigit +
                       many(IdentifierNondigit | cr<"0-9">)> {};

  static constexpr auto EscapeSequence =
      (lit<"\\"> + cr<R"('"?abfnrtv.\\)">)                                //
      | (lit<"\\"> + cr<"0-7"> + option(cr<"0-7"> + option(cr<"0-7">)))  //
      | (lit<"\\x"> + some(HexDigit))                                   //
      | UniversalCharacterName
      | (lit<"\\"> + cr<R"(^\r\n)">);

  struct IntegerConstant
      : terminal<((lit<"0"> + cr<"bB"> + some(cr<"0-1">))  //
                  | (lit<"0"> + cr<"xX"> + some(HexDigit))  //
                  | (cr<"1-9"> + many(cr<"0-9">))            //
                  | (lit<"0"> + many(cr<"0-7">))) +
                 option((cr<"uU"> + option(cr<"lL"> + option(cr<"lL">))) |
                        (cr<"lL"> + option(cr<"lL">) + option(cr<"uU">)))> {};

  static constexpr auto DigitSeq = some(cr<"0-9">);
  static constexpr auto FractionalConstant =
      (option(DigitSeq) + lit<"."> + DigitSeq) | (DigitSeq + lit<".">);
  static constexpr auto ExponentPart = cr<"eE"> + option(cr<"+-">) + DigitSeq;

  struct FloatingConstant
      : terminal<(FractionalConstant + option(ExponentPart) +
                  option(cr<"flFL">)) |
                 (DigitSeq + ExponentPart + option(cr<"flFL">))> {};

  static constexpr auto CChar = cr<R"(^'\\\n\r)"> | EscapeSequence;
  struct CharacterConstant
      : terminal<(lit<"L"> | lit<"u"> | lit<"U">) + lit<"'"> +
                 some(CChar) + lit<"'">> {};

  static constexpr auto SChar =
      cr<R"(^"\\\n\r)"> | EscapeSequence | lit<"\\\n"> | lit<"\\\r\n">;
  struct StringLitTok
      : terminal<option(lit<"u8"> | lit<"u"> | lit<"U"> | lit<"L">) +
                 lit<"\""> + many(SChar) + lit<"\""> |
                 (lit<"'"> + many(CChar | lit<"\\\n"> | lit<"\\\r\n">) + lit<"'">)> {};

  // Keywords a value is read from: which enum member a spelling denotes.







  static constexpr auto TypeKw = kw<"char"> | kw<"double"> | kw<"float"> |
                                 kw<"int"> | kw<"long"> | kw<"unsigned"> |
                                 kw<"void"> | kw<"string">;
  static constexpr auto BuiltinKw =
      as<ast::BuiltinKind::gsub>(kw<"gsub">) | as<ast::BuiltinKind::sub>(kw<"sub">) | as<ast::BuiltinKind::substr>(kw<"substr">) | as<ast::BuiltinKind::exit_>(kw<"exit">) |
      as<ast::BuiltinKind::rand_>(kw<"rand">) | as<ast::BuiltinKind::srand_>(kw<"srand">) | as<ast::BuiltinKind::procedure>(kw<"procedure">) | as<ast::BuiltinKind::print_>(kw<"print">) |
      as<ast::BuiltinKind::printf_>(kw<"printf">) | as<ast::BuiltinKind::query>(kw<"query">) | as<ast::BuiltinKind::sprintf_>(kw<"sprintf">) | as<ast::BuiltinKind::scanf_>(kw<"scanf">) |
      as<ast::BuiltinKind::sscanf_>(kw<"sscanf">);

  struct Primary;
  struct Expr9;
  struct CondExpr;
  struct CommaExpr;
  struct Statement;
  struct BodyStatement;
  template<auto Member>
  static constexpr auto Statements = many(lit<";">) +
      many(append<Member>(ref<Statement>) + many(lit<";">));

  struct TypeRef
      : rule<create<ast::TypeRef>(
             assign<&ast::TypeRef::name>(TypeKw | ref<ID>) +
             option(flag<&ast::TypeRef::pointer>(lit<"*">)))> {};

  struct Var
      : rule<create<ast::Var>(
             assign<&ast::Var::name>(lit<"$"> | ref<ID>) +
             option(assign<&ast::Var::index>(lit<"["> + ref<CommaExpr> +
                                             lit<"]">)) +
             option(some(lit<"."> + append<&ast::Var::members>(
                                        ref<ID> | kw<"split">))))> {};

  struct ArrayCall
      : rule<create<ast::ArrayCall>(
             &kw<"ARGV"> + assign<&ast::ArrayCall::name>(ref<ID>) + lit<"["> +
             option(some(append<&ast::ArrayCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<"]">)> {};

  struct FuncCall
      : rule<create<ast::FuncCall>(
             assign<&ast::FuncCall::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::FuncCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<")">)> {};

  struct BuiltinCall
      : rule<create<ast::BuiltinCall>(
             assign<&ast::BuiltinCall::kind>(BuiltinKw) + lit<"("> +
             option(some(append<&ast::BuiltinCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<")">)> {};

  struct SplitCall
      : rule<create<ast::SplitCall>(
             assign<&ast::SplitCall::tokens>(as<false>(kw<"split">) | as<true>(kw<"tokens">)) +
             lit<"("> + assign<&ast::SplitCall::subject>(ref<CondExpr>) +
             lit<","> + assign<&ast::SplitCall::arrayName>(ref<ID>) +
             option(lit<","> +
                    assign<&ast::SplitCall::separator>(ref<CondExpr>)) +
             lit<")">)> {};

  struct IntLit
      : rule<create<ast::IntLit>(
             assign<&ast::IntLit::text>(ref<IntegerConstant>))> {};
  struct FloatLit
      : rule<create<ast::FloatLit>(
             assign<&ast::FloatLit::text>(ref<FloatingConstant>))> {};
  struct CharLit
      : rule<create<ast::CharLit>(
             assign<&ast::CharLit::text>(ref<CharacterConstant>))> {};
  struct StringLit
      : rule<create<ast::StringLit>(
             assign<&ast::StringLit::text>(ref<StringLitTok>))> {};

  struct Constant
      : rule<ref<IntLit> | ref<FloatLit> | ref<CharLit> |
             ref<StringLit>> {};

  // The cast's operand binds at unary tightness.
  struct Cast
      : rule<create<ast::Cast>(
             lit<"("> + assign<&ast::Cast::type>(ref<TypeRef>) +
             lit<")"> + assign<&ast::Cast::operand>(ref<Primary>))> {};

  struct PrefixIncDec
      : rule<create<ast::PrefixIncDec>(
             assign<&ast::PrefixIncDec::op>(as<ast::IncDecOp::inc>(lit<"++">) | as<ast::IncDecOp::dec>(lit<"--">)) +
             assign<&ast::PrefixIncDec::var>(ref<Var>))> {};

  struct PostfixIncDec
      : rule<create<ast::PostfixIncDec>(
             assign<&ast::PostfixIncDec::var>(ref<Var>) +
             assign<&ast::PostfixIncDec::op>(as<ast::IncDecOp::inc>(lit<"++">) | as<ast::IncDecOp::dec>(lit<"--">)))> {};

  // The field-count operator; its operand is a bare name.
  struct Cardinality
      : rule<create<ast::Cardinality>(
             lit<"#"> + assign<&ast::Cardinality::name>(ref<ID>))> {};

  struct UnaryOp
      : rule<create<ast::Unary>(
             assign<&ast::Unary::op>(as<ast::UnaryOp::not_>(lit<"!">) | as<ast::UnaryOp::bnot>(lit<"~">) |
                                     as<ast::UnaryOp::neg>(lit<"-">) | as<ast::UnaryOp::pos>(lit<"+">)) +
             assign<&ast::Unary::operand>(ref<Primary>))> {};

  struct AddressOf
      : rule<create<ast::AddressOf>(
             lit<"&"> + assign<&ast::AddressOf::var>(ref<Var>))> {};

  static constexpr auto AssignOperator =
      as<ast::AssignOp::set>(lit<"=">) | as<ast::AssignOp::add>(lit<"+=">) |
      as<ast::AssignOp::sub>(lit<"-=">) | as<ast::AssignOp::mul>(lit<"*=">) |
      as<ast::AssignOp::div>(lit<"/=">) | as<ast::AssignOp::mod>(lit<"%=">) |
      as<ast::AssignOp::band>(lit<"&=">) | as<ast::AssignOp::bxor>(lit<"^=">) |
      as<ast::AssignOp::bor>(lit<"|=">) | as<ast::AssignOp::lsh>(lit<"<<=">) |
      as<ast::AssignOp::rsh>(lit<">>=">);

  struct Assign
      : rule<create<ast::Assign>(
             assign<&ast::Assign::target>(ref<Var>) +
             assign<&ast::Assign::op>(AssignOperator) +
             assign<&ast::Assign::value>(ref<CondExpr>))> {};

  struct Primary
      : rule<(lit<"("> + ref<CommaExpr> + lit<")">)  //
             | ref<Cast>                          //
             | ref<PrefixIncDec>                  //
             | ref<PostfixIncDec>                 //
             | ref<UnaryOp>                        //
             | ref<Cardinality>                    //
             | ref<AddressOf>                      //
             | ref<ArrayCall>                      //
             | ref<FuncCall>                        //
             | ref<BuiltinCall>                     //
             | ref<SplitCall>                       //
             | ref<Assign>                          //
             | ref<Var>                             //
             | ref<Constant>> {};

  static constexpr auto OpMul = as<ast::BinOp::mul>(lit<"*">) | as<ast::BinOp::div>(lit<"/">) | as<ast::BinOp::mod>(lit<"%">);
  static constexpr auto OpAdd = as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::sub>(lit<"-">) | as<ast::BinOp::in_>(kw<"in">);
  static constexpr auto OpShift = as<ast::BinOp::lsh>(lit<"<<">) | as<ast::BinOp::rsh>(lit<">>">);
  static constexpr auto OpRel =
      as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::ge>(lit<">=">) | as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::gt>(lit<">">);
  static constexpr auto OpEq = as<ast::BinOp::eq>(lit<"==">) | as<ast::BinOp::ne>(lit<"!=">);

  struct Expr9
      : infix<Primary, ast::Binary,
              levels(left(OpMul),         //
                     left(OpAdd),         //
                     left(OpShift),       //
                     left(OpRel),         //
                     left(OpEq),          //
                     left(as<ast::BinOp::band>(lit<"&">)),  //
                     left(as<ast::BinOp::bxor>(lit<"^">)),  //
                     left(as<ast::BinOp::bor>(lit<"|">)),   //
                     left(as<ast::BinOp::land>(lit<"&&">)),   //
                     left(as<ast::BinOp::lor>(lit<"||">)))> {};

  // `else_` is a full CondExpr, so the ternary nests right-associatively:
  // `a?b:c?d:e` reads `a?b:(c?d:e)`.
  struct Ternary
      : rule<create<ast::Ternary>(
             assign<&ast::Ternary::cond>(ref<Expr9>) + lit<"?"> +
             assign<&ast::Ternary::then_>(ref<CondExpr>) + lit<":"> +
             assign<&ast::Ternary::else_>(ref<CondExpr>))> {};

  struct CondExpr : rule<ref<Ternary> | ref<Expr9>> {};

  // The comma operator: excluded from comma-delimited list contexts (args,
  // declaration initializers) to keep a written comma a separator there;
  // kept at statement-level expression positions.
  struct CommaExpr
      : infix<CondExpr, ast::Binary, left(as<ast::BinOp::comma>(lit<",">))> {};

  struct DclItem
      : rule<create<ast::DclItem>(
             assign<&ast::DclItem::name>(ref<ID>) +
             option(flag<&ast::DclItem::isArray>(lit<"[">) +
                    option(assign<&ast::DclItem::keyType>(ref<TypeRef>)) +
                    lit<"]">) +
             option(assign<&ast::DclItem::initOp>(AssignOperator) +
                    assign<&ast::DclItem::init>(ref<CondExpr>)))> {};

  struct Formal
      : rule<create<ast::Formal>(
             assign<&ast::Formal::type>(ref<TypeRef>) +
             assign<&ast::Formal::name>(ref<ID>))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
             option(flag<&ast::VarDecl::isStatic>(kw<"static">)) +
             assign<&ast::VarDecl::type>(ref<TypeRef>) +
             some(append<&ast::VarDecl::items>(ref<DclItem>), lit<",">))> {
  };

  struct FuncDecl
      : rule<create<ast::FuncDecl>(
             option(flag<&ast::FuncDecl::isStatic>(kw<"static">)) +
             assign<&ast::FuncDecl::returnType>(ref<TypeRef>) +
             assign<&ast::FuncDecl::name>(ref<ID>) + lit<"("> +
             option(kw<"void"> | some(append<&ast::FuncDecl::formals>(ref<Formal>),
                                      lit<",">)) +
             lit<")"> + lit<"{"> +
             Statements<&ast::FuncDecl::statements> +
             lit<"}">)> {};

  struct Block
      : rule<create<ast::Block>(
             lit<"{"> +
             Statements<&ast::Block::statements> +
             lit<"}">)> {};

  struct ExprStmt
      : rule<create<ast::ExprStmt>(
             assign<&ast::ExprStmt::expr>(ref<CommaExpr>))> {};

  struct If
      : rule<create<ast::If>(
             kw<"if"> + lit<"("> + assign<&ast::If::cond>(ref<CommaExpr>) +
             lit<")"> + assign<&ast::If::then_>(ref<BodyStatement>) +
             option(option(lit<";">) + kw<"else"> +
                    assign<&ast::If::else_>(ref<BodyStatement>)))> {};

  struct ForC
      : rule<create<ast::ForC>(
             kw<"for"> + lit<"("> +
             option(assign<&ast::ForC::init>(ref<CommaExpr>)) + lit<";"> +
             option(assign<&ast::ForC::cond>(ref<CommaExpr>)) + lit<";"> +
             option(assign<&ast::ForC::step>(ref<CommaExpr>)) + lit<")"> +
             assign<&ast::ForC::body>(ref<BodyStatement>))> {};

  struct ForVar
      : rule<create<ast::ForVar>(
             kw<"for"> + lit<"("> + assign<&ast::ForVar::var>(ref<Var>) +
             lit<")"> + assign<&ast::ForVar::body>(ref<BodyStatement>))> {};

  struct ForR
      : rule<create<ast::ForR>(
             kw<"forr"> + lit<"("> + assign<&ast::ForR::var>(ref<Var>) +
             lit<")"> + assign<&ast::ForR::body>(ref<BodyStatement>))> {};

  struct Unset
      : rule<create<ast::Unset>(
             kw<"unset"> + lit<"("> + assign<&ast::Unset::name>(ref<ID>) +
             option(lit<","> +
                    assign<&ast::Unset::value>(ref<CommaExpr>)) +
             lit<")">)> {};

  struct While
      : rule<create<ast::While>(
             kw<"while"> + lit<"("> +
             assign<&ast::While::cond>(ref<CommaExpr>) + lit<")"> +
             assign<&ast::While::body>(ref<BodyStatement>))> {};

  struct Case
      : rule<(kw<"case"> + assign<&ast::Case::value>(ref<Constant>) +
             lit<":">) |
             (flag<&ast::Case::isDefault>(kw<"default">) +
              lit<":">)> {};

  struct SwitchItem
      : rule<create<ast::SwitchItem>(
             some(append<&ast::SwitchItem::cases>(ref<Case>)) +
             some(append<&ast::SwitchItem::statements>(ref<BodyStatement>) +
                  many(lit<";">)))> {};

  struct SwitchLastItem
      : rule<create<ast::SwitchItem>(
             some(append<&ast::SwitchItem::cases>(ref<Case>)))> {};

  struct Switch
      : rule<create<ast::Switch>(
             kw<"switch"> + lit<"("> +
             assign<&ast::Switch::subject>(ref<CommaExpr>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::Switch::items>(ref<SwitchItem>)) +
             option(append<&ast::Switch::items>(ref<SwitchLastItem>)) +
             lit<"}">)> {};

  struct Break
      : rule<create<ast::Break>(
             kw<"break"> + option(assign<&ast::Break::value>(ref<CommaExpr>)))> {
  };
  struct Continue
      : rule<create<ast::Continue>(
             kw<"continue"> +
             option(assign<&ast::Continue::value>(ref<CommaExpr>)))> {};
  struct Return
      : rule<create<ast::Return>(
             kw<"return"> +
             option(assign<&ast::Return::value>(ref<CommaExpr>)))> {};

  struct Statement
      : rule<ref<Block>       //
             | ref<VarDecl>    //
             | ref<FuncDecl>   //
             | ref<If>          //
             | ref<ForC>        //
             | ref<ForVar>      //
             | ref<ForR>        //
             | ref<Unset>       //
             | ref<While>       //
             | ref<Switch>      //
             | ref<Break>       //
             | ref<Continue>    //
             | ref<Return>      //
             | ref<ExprStmt>> {};

  struct BodyStatement : rule<create<ast::EmptyStmt>(lit<";">) | ref<Statement>> {};

  struct Action
      : rule<create<ast::Action>(
             assign<&ast::Action::label>(ref<ID>) + lit<":"> +
             Statements<&ast::Action::statements>)> {};

  struct Body
      : rule<create<ast::Body>(
             Statements<&ast::Body::statements> +
             option(some(append<&ast::Body::actions>(ref<Action>))))> {};

  struct SimplePred
      : rule<create<ast::SimplePred>(
             assign<&ast::SimplePred::kind>(as<ast::PredKind::begin_>(kw<"BEGIN">) | as<ast::PredKind::beg_g>(kw<"BEG_G">) |
                                            as<ast::PredKind::end_g>(kw<"END_G">) | as<ast::PredKind::end_>(kw<"END">)) +
             option(lit<"{"> + assign<&ast::SimplePred::body>(ref<Body>) +
                    lit<"}">))> {};

  struct GuardedPred
      : rule<create<ast::GuardedPred>(
             assign<&ast::GuardedPred::kind>(as<ast::GuardKind::node_>(kw<"N">) | as<ast::GuardKind::edge_>(kw<"E">)) +
             option(lit<"["> +
                    assign<&ast::GuardedPred::guard>(ref<Body>) +
                    lit<"]">) +
             option(lit<"{"> +
                    assign<&ast::GuardedPred::body>(ref<Body>) +
                    lit<"}">))> {};

  struct Pred : rule<ref<SimplePred> | ref<GuardedPred>> {};

  struct Preds
      : rule<create<ast::Preds>(
             option(ref<LeadingShellComment>) +
             many(append<&ast::Preds::items>(ref<Pred>)))> {};

  using entry = Preds;
  using hidden = list<WS, MLCOMMENT, SLCOMMENT, SHELLCOMMENT>;
};

}  // namespace gvpr
