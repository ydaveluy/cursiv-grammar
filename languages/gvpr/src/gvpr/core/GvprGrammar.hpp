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
      cr<"a-zA-Z_$#"> | UniversalCharacterName;

  struct ID : terminal<IdentifierNondigit +
                       many(IdentifierNondigit | cr<"0-9">)> {};

  static constexpr auto EscapeSequence =
      (lit<"\\"> + cr<R"('"?abfnrtv.\\)">)                                //
      | (lit<"\\"> + cr<"0-7"> + option(cr<"0-7"> + option(cr<"0-7">)))  //
      | (lit<"\\x"> + some(HexDigit))                                   //
      | UniversalCharacterName;

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
      : terminal<option(lit<"L"> | lit<"u"> | lit<"U">) + lit<"'"> +
                 some(CChar) + lit<"'">> {};

  static constexpr auto SChar =
      cr<R"(^"\\\n\r)"> | EscapeSequence | lit<"\\\n"> | lit<"\\\r\n">;
  struct StringLitTok
      : terminal<option(lit<"u8"> | lit<"u"> | lit<"U"> | lit<"L">) +
                 lit<"\""> + many(SChar) + lit<"\"">> {};

  // Keywords a value is read from: which enum member a spelling denotes.
  struct BEGIN_KW : terminal<kw<"BEGIN">, value<ast::PredKind::begin_>> {};
  struct BEG_G_KW : terminal<kw<"BEG_G">, value<ast::PredKind::beg_g>> {};
  struct END_G_KW : terminal<kw<"END_G">, value<ast::PredKind::end_g>> {};
  struct END_KW : terminal<kw<"END">, value<ast::PredKind::end_>> {};
  struct N_KW : terminal<kw<"N">, value<ast::GuardKind::node_>> {};
  struct E_KW : terminal<kw<"E">, value<ast::GuardKind::edge_>> {};

  struct AEQ_OP : terminal<lit<"=">, value<ast::AssignOp::set>> {};
  struct APEQ_OP : terminal<lit<"+=">, value<ast::AssignOp::add>> {};
  struct AMEQ_OP : terminal<lit<"-=">, value<ast::AssignOp::sub>> {};
  struct ASEQ_OP : terminal<lit<"*=">, value<ast::AssignOp::mul>> {};
  struct ASLEQ_OP : terminal<lit<"/=">, value<ast::AssignOp::div>> {};

  struct INC_OP : terminal<lit<"++">, value<ast::IncDecOp::inc>> {};
  struct DEC_OP : terminal<lit<"--">, value<ast::IncDecOp::dec>> {};

  struct NOT_OP : terminal<lit<"!">, value<ast::UnaryOp::not_>> {};
  struct BNOT_OP : terminal<lit<"~">, value<ast::UnaryOp::bnot>> {};
  struct NEG_OP : terminal<lit<"-">, value<ast::UnaryOp::neg>> {};
  struct POS_OP : terminal<lit<"+">, value<ast::UnaryOp::pos>> {};

  struct MUL_OP : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct DIV_OP : terminal<lit<"/">, value<ast::BinOp::div>> {};
  struct MOD_OP : terminal<lit<"%">, value<ast::BinOp::mod>> {};
  struct ADD_OP : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct SUB_OP : terminal<lit<"-">, value<ast::BinOp::sub>> {};
  struct IN_OP : terminal<kw<"in">, value<ast::BinOp::in_>> {};
  struct LSH_OP : terminal<kw<"lsh">, value<ast::BinOp::lsh>> {};
  struct RSH_OP : terminal<kw<"rsh">, value<ast::BinOp::rsh>> {};
  struct LE_OP : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct GE_OP : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct LT_OP : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct GT_OP : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct EQ_OP : terminal<lit<"==">, value<ast::BinOp::eq>> {};
  struct NE_OP : terminal<lit<"!=">, value<ast::BinOp::ne>> {};
  struct BAND_OP : terminal<lit<"&">, value<ast::BinOp::band>> {};
  struct BXOR_OP : terminal<lit<"^">, value<ast::BinOp::bxor>> {};
  struct BOR_OP : terminal<lit<"|">, value<ast::BinOp::bor>> {};
  struct AND_OP : terminal<lit<"&&">, value<ast::BinOp::land>> {};
  struct OR_OP : terminal<lit<"||">, value<ast::BinOp::lor>> {};
  struct COMMA_OP : terminal<lit<",">, value<ast::BinOp::comma>> {};

  struct SPLIT_KW : terminal<kw<"split">, value<false>> {};
  struct TOKENS_KW : terminal<kw<"tokens">, value<true>> {};

  struct GSUB_KW : terminal<kw<"gsub">, value<ast::BuiltinKind::gsub>> {};
  struct SUB_KW : terminal<kw<"sub">, value<ast::BuiltinKind::sub>> {};
  struct SUBSTR_KW
      : terminal<kw<"substr">, value<ast::BuiltinKind::substr>> {};
  struct EXIT_KW : terminal<kw<"exit">, value<ast::BuiltinKind::exit_>> {};
  struct RAND_KW : terminal<kw<"rand">, value<ast::BuiltinKind::rand_>> {};
  struct SRAND_KW : terminal<kw<"srand">, value<ast::BuiltinKind::srand_>> {};
  struct PROCEDURE_KW
      : terminal<kw<"procedure">, value<ast::BuiltinKind::procedure>> {};
  struct XPRINT_KW
      : terminal<kw<"print">, value<ast::BuiltinKind::print_>> {};
  struct PRINTF_KW
      : terminal<kw<"printf">, value<ast::BuiltinKind::printf_>> {};
  struct QUERY_KW : terminal<kw<"query">, value<ast::BuiltinKind::query>> {};
  struct SPRINTF_KW
      : terminal<kw<"sprintf">, value<ast::BuiltinKind::sprintf_>> {};
  struct SCANF_KW : terminal<kw<"scanf">, value<ast::BuiltinKind::scanf_>> {};
  struct SSCANF_KW
      : terminal<kw<"sscanf">, value<ast::BuiltinKind::sscanf_>> {};

  static constexpr auto TypeKw = kw<"char"> | kw<"double"> | kw<"float"> |
                                 kw<"int"> | kw<"long"> | kw<"unsigned"> |
                                 kw<"void"> | kw<"string">;
  static constexpr auto BuiltinKw =
      ref<GSUB_KW> | ref<SUB_KW> | ref<SUBSTR_KW> | ref<EXIT_KW> |
      ref<RAND_KW> | ref<SRAND_KW> | ref<PROCEDURE_KW> | ref<XPRINT_KW> |
      ref<PRINTF_KW> | ref<QUERY_KW> | ref<SPRINTF_KW> | ref<SCANF_KW> |
      ref<SSCANF_KW>;

  struct Primary;
  struct Expr9;
  struct CondExpr;
  struct CommaExpr;
  struct Statement;

  struct TypeRefRule
      : rule<create<ast::TypeRef>(
             assign<&ast::TypeRef::name>(TypeKw | ref<ID>) +
             option(flag<&ast::TypeRef::pointer>(lit<"*">)))> {};

  struct VarRule
      : rule<create<ast::Var>(
             assign<&ast::Var::name>(lit<"$"> | ref<ID>) +
             option(assign<&ast::Var::index>(lit<"["> + ref<CommaExpr> +
                                             lit<"]">)) +
             option(some(lit<"."> + append<&ast::Var::members>(
                                        ref<ID> | kw<"split">))))> {};

  struct ArrayCallRule
      : rule<create<ast::ArrayCall>(
             &kw<"ARGV"> + assign<&ast::ArrayCall::name>(ref<ID>) + lit<"["> +
             option(some(append<&ast::ArrayCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<"]">)> {};

  struct FuncCallRule
      : rule<create<ast::FuncCall>(
             assign<&ast::FuncCall::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::FuncCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<")">)> {};

  struct BuiltinCallRule
      : rule<create<ast::BuiltinCall>(
             assign<&ast::BuiltinCall::kind>(BuiltinKw) + lit<"("> +
             option(some(append<&ast::BuiltinCall::args>(ref<CondExpr>),
                        lit<",">)) +
             lit<")">)> {};

  struct SplitCallRule
      : rule<create<ast::SplitCall>(
             assign<&ast::SplitCall::tokens>(ref<SPLIT_KW> | ref<TOKENS_KW>) +
             lit<"("> + assign<&ast::SplitCall::subject>(ref<CondExpr>) +
             lit<","> + assign<&ast::SplitCall::arrayName>(ref<ID>) +
             option(lit<","> +
                    assign<&ast::SplitCall::separator>(ref<CondExpr>)) +
             lit<")">)> {};

  struct IntLitRule
      : rule<create<ast::IntLit>(
             assign<&ast::IntLit::text>(ref<IntegerConstant>))> {};
  struct FloatLitRule
      : rule<create<ast::FloatLit>(
             assign<&ast::FloatLit::text>(ref<FloatingConstant>))> {};
  struct CharLitRule
      : rule<create<ast::CharLit>(
             assign<&ast::CharLit::text>(ref<CharacterConstant>))> {};
  struct StringLitRule
      : rule<create<ast::StringLit>(
             assign<&ast::StringLit::text>(ref<StringLitTok>))> {};

  struct Constant
      : rule<ref<IntLitRule> | ref<FloatLitRule> | ref<CharLitRule> |
             ref<StringLitRule>> {};

  // The cast's operand binds at unary tightness.
  struct CastRule
      : rule<create<ast::Cast>(
             lit<"("> + assign<&ast::Cast::type>(ref<TypeRefRule>) +
             lit<")"> + assign<&ast::Cast::operand>(ref<Primary>))> {};

  struct PrefixIncDecRule
      : rule<create<ast::PrefixIncDec>(
             assign<&ast::PrefixIncDec::op>(ref<INC_OP> | ref<DEC_OP>) +
             assign<&ast::PrefixIncDec::var>(ref<VarRule>))> {};

  struct PostfixIncDecRule
      : rule<create<ast::PostfixIncDec>(
             assign<&ast::PostfixIncDec::var>(ref<VarRule>) +
             assign<&ast::PostfixIncDec::op>(ref<INC_OP> | ref<DEC_OP>))> {};

  // The field-count operator; its operand is a bare name.
  struct CardinalityRule
      : rule<create<ast::Cardinality>(
             lit<"#"> + assign<&ast::Cardinality::name>(ref<ID>))> {};

  struct UnaryOpRule
      : rule<create<ast::Unary>(
             assign<&ast::Unary::op>(ref<NOT_OP> | ref<BNOT_OP> |
                                     ref<NEG_OP> | ref<POS_OP>) +
             assign<&ast::Unary::operand>(ref<Primary>))> {};

  struct AddressOfRule
      : rule<create<ast::AddressOf>(
             lit<"&"> + assign<&ast::AddressOf::var>(ref<VarRule>))> {};

  struct AssignRule
      : rule<create<ast::Assign>(
             assign<&ast::Assign::target>(ref<VarRule>) +
             assign<&ast::Assign::op>(ref<AEQ_OP> | ref<APEQ_OP> |
                                      ref<AMEQ_OP> | ref<ASEQ_OP> |
                                      ref<ASLEQ_OP>) +
             assign<&ast::Assign::value>(ref<CondExpr>))> {};

  struct Primary
      : rule<(lit<"("> + ref<CommaExpr> + lit<")">)  //
             | ref<CastRule>                          //
             | ref<PrefixIncDecRule>                  //
             | ref<PostfixIncDecRule>                 //
             | ref<UnaryOpRule>                        //
             | ref<CardinalityRule>                    //
             | ref<AddressOfRule>                      //
             | ref<ArrayCallRule>                      //
             | ref<FuncCallRule>                        //
             | ref<BuiltinCallRule>                     //
             | ref<SplitCallRule>                       //
             | ref<AssignRule>                          //
             | ref<VarRule>                             //
             | ref<Constant>> {};

  static constexpr auto OpMul = ref<MUL_OP> | ref<DIV_OP> | ref<MOD_OP>;
  static constexpr auto OpAdd = ref<ADD_OP> | ref<SUB_OP> | ref<IN_OP>;
  static constexpr auto OpShift = ref<LSH_OP> | ref<RSH_OP>;
  static constexpr auto OpRel =
      ref<LE_OP> | ref<GE_OP> | ref<LT_OP> | ref<GT_OP>;
  static constexpr auto OpEq = ref<EQ_OP> | ref<NE_OP>;

  struct Expr9
      : infix<Primary, &ast::Binary::left, &ast::Binary::op,
              &ast::Binary::right,
              (left(OpMul) >>          //
               left(OpAdd) >>          //
               left(OpShift) >>        //
               left(OpRel) >>          //
               left(OpEq) >>           //
               left(ref<BAND_OP>) >>   //
               left(ref<BXOR_OP>) >>   //
               left(ref<BOR_OP>) >>    //
               left(ref<AND_OP>) >>    //
               left(ref<OR_OP>))> {};

  // `else_` is a full CondExpr, so the ternary nests right-associatively:
  // `a?b:c?d:e` reads `a?b:(c?d:e)`.
  struct TernaryRule
      : rule<create<ast::Ternary>(
             assign<&ast::Ternary::cond>(ref<Expr9>) + lit<"?"> +
             assign<&ast::Ternary::then_>(ref<CondExpr>) + lit<":"> +
             assign<&ast::Ternary::else_>(ref<CondExpr>))> {};

  struct CondExpr : rule<ref<TernaryRule> | ref<Expr9>> {};

  // The comma operator: excluded from comma-delimited list contexts (args,
  // declaration initializers) to keep a written comma a separator there;
  // kept at statement-level expression positions.
  struct CommaExpr
      : infix<CondExpr, &ast::Binary::left, &ast::Binary::op,
              &ast::Binary::right, left(ref<COMMA_OP>)> {};

  struct DclItemRule
      : rule<create<ast::DclItem>(
             assign<&ast::DclItem::name>(ref<ID>) +
             option(lit<"["> +
                    option(assign<&ast::DclItem::keyType>(ref<TypeRefRule>)) +
                    lit<"]">) +
             option(assign<&ast::DclItem::initOp>(ref<AEQ_OP> | ref<APEQ_OP> | ref<AMEQ_OP> |
                     ref<ASEQ_OP> | ref<ASLEQ_OP>) +
                    assign<&ast::DclItem::init>(ref<CondExpr>)))> {};

  struct FormalRule
      : rule<create<ast::Formal>(
             assign<&ast::Formal::type>(ref<TypeRefRule>) +
             option(assign<&ast::Formal::name>(ref<ID>)))> {};

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
             option(flag<&ast::VarDecl::isStatic>(kw<"static">)) +
             assign<&ast::VarDecl::type>(ref<TypeRefRule>) +
             some(append<&ast::VarDecl::items>(ref<DclItemRule>), lit<",">))> {
  };

  struct FuncDeclRule
      : rule<create<ast::FuncDecl>(
             option(flag<&ast::FuncDecl::isStatic>(kw<"static">)) +
             assign<&ast::FuncDecl::returnType>(ref<TypeRefRule>) +
             assign<&ast::FuncDecl::name>(ref<ID>) + lit<"("> +
             option(some(append<&ast::FuncDecl::formals>(ref<FormalRule>),
                        lit<",">)) +
             lit<")"> + lit<"{"> +
             option(some(append<&ast::FuncDecl::statements>(ref<Statement>) +
                        option(lit<";">))) +
             lit<"}">)> {};

  struct BlockRule
      : rule<create<ast::Block>(
             lit<"{"> +
             option(some(append<&ast::Block::statements>(ref<Statement>) +
                        option(lit<";">))) +
             lit<"}">)> {};

  struct ExprStmtRule
      : rule<create<ast::ExprStmt>(
             assign<&ast::ExprStmt::expr>(ref<CommaExpr>))> {};

  struct IfRule
      : rule<create<ast::If>(
             kw<"if"> + lit<"("> + assign<&ast::If::cond>(ref<CommaExpr>) +
             lit<")"> + assign<&ast::If::then_>(ref<Statement>) +
             option(option(lit<";">) + kw<"else"> +
                    assign<&ast::If::else_>(ref<Statement>)))> {};

  struct ForCRule
      : rule<create<ast::ForC>(
             kw<"for"> + lit<"("> +
             option(assign<&ast::ForC::init>(ref<CommaExpr>)) + lit<";"> +
             option(assign<&ast::ForC::cond>(ref<CommaExpr>)) + lit<";"> +
             option(assign<&ast::ForC::step>(ref<CommaExpr>)) + lit<")"> +
             assign<&ast::ForC::body>(ref<Statement>))> {};

  struct ForVarRule
      : rule<create<ast::ForVar>(
             kw<"for"> + lit<"("> + assign<&ast::ForVar::var>(ref<VarRule>) +
             lit<")"> + assign<&ast::ForVar::body>(ref<Statement>))> {};

  struct ForRRule
      : rule<create<ast::ForR>(
             kw<"forr"> + lit<"("> + assign<&ast::ForR::var>(ref<VarRule>) +
             lit<")"> + assign<&ast::ForR::body>(ref<Statement>))> {};

  struct UnsetRule
      : rule<create<ast::Unset>(
             kw<"unset"> + lit<"("> + assign<&ast::Unset::name>(ref<ID>) +
             option(lit<","> +
                    assign<&ast::Unset::value>(ref<CommaExpr>)) +
             lit<")">)> {};

  struct WhileRule
      : rule<create<ast::While>(
             kw<"while"> + lit<"("> +
             assign<&ast::While::cond>(ref<CommaExpr>) + lit<")"> +
             assign<&ast::While::body>(ref<Statement>))> {};

  struct CaseRule
      : rule<(kw<"case"> + assign<&ast::Case::value>(ref<Constant>) +
             lit<":">) |
             (flag<&ast::Case::isDefault>(kw<"default">) +
              lit<":">)> {};

  struct SwitchItemRule
      : rule<create<ast::SwitchItem>(
             some(append<&ast::SwitchItem::cases>(ref<CaseRule>)) +
             some(append<&ast::SwitchItem::statements>(ref<Statement>) +
                  option(lit<";">)))> {};

  struct SwitchLastItemRule
      : rule<create<ast::SwitchItem>(
             some(append<&ast::SwitchItem::cases>(ref<CaseRule>)))> {};

  struct SwitchRule
      : rule<create<ast::Switch>(
             kw<"switch"> + lit<"("> +
             assign<&ast::Switch::subject>(ref<CommaExpr>) + lit<")"> +
             lit<"{"> +
             many(append<&ast::Switch::items>(ref<SwitchItemRule>)) +
             option(append<&ast::Switch::items>(ref<SwitchLastItemRule>)) +
             lit<"}">)> {};

  struct BreakRule
      : rule<create<ast::Break>(
             kw<"break"> + option(assign<&ast::Break::value>(ref<CommaExpr>)))> {
  };
  struct ContinueRule
      : rule<create<ast::Continue>(
             kw<"continue"> +
             option(assign<&ast::Continue::value>(ref<CommaExpr>)))> {};
  struct ReturnRule
      : rule<create<ast::Return>(
             kw<"return"> +
             option(assign<&ast::Return::value>(ref<CommaExpr>)))> {};

  struct Statement
      : rule<ref<BlockRule>       //
             | ref<VarDeclRule>    //
             | ref<FuncDeclRule>   //
             | ref<IfRule>          //
             | ref<ForCRule>        //
             | ref<ForVarRule>      //
             | ref<ForRRule>        //
             | ref<UnsetRule>       //
             | ref<WhileRule>       //
             | ref<SwitchRule>      //
             | ref<BreakRule>       //
             | ref<ContinueRule>    //
             | ref<ReturnRule>      //
             | ref<ExprStmtRule>> {};

  struct ActionRule
      : rule<create<ast::Action>(
             assign<&ast::Action::label>(ref<ID>) + lit<":"> +
             option(some(append<&ast::Action::statements>(ref<Statement>) +
                        option(lit<";">))))> {};

  struct BodyRule
      : rule<create<ast::Body>(
             option(some(append<&ast::Body::statements>(ref<Statement>) +
                        option(lit<";">))) +
             option(some(append<&ast::Body::actions>(ref<ActionRule>))))> {};

  struct SimplePredRule
      : rule<create<ast::SimplePred>(
             assign<&ast::SimplePred::kind>(ref<BEGIN_KW> | ref<BEG_G_KW> |
                                            ref<END_G_KW> | ref<END_KW>) +
             option(lit<"{"> + assign<&ast::SimplePred::body>(ref<BodyRule>) +
                    lit<"}">))> {};

  struct GuardedPredRule
      : rule<create<ast::GuardedPred>(
             assign<&ast::GuardedPred::kind>(ref<N_KW> | ref<E_KW>) +
             option(lit<"["> +
                    assign<&ast::GuardedPred::guard>(ref<BodyRule>) +
                    lit<"]">) +
             option(lit<"{"> +
                    assign<&ast::GuardedPred::body>(ref<BodyRule>) +
                    lit<"}">))> {};

  struct PredRule : rule<ref<SimplePredRule> | ref<GuardedPredRule>> {};

  struct Preds
      : rule<create<ast::Preds>(
             option(ref<LeadingShellComment>) +
             many(append<&ast::Preds::items>(ref<PredRule>)))> {};

  using entry = Preds;
  using hidden = list<WS, MLCOMMENT, SLCOMMENT, SHELLCOMMENT>;
};

}  // namespace gvpr
