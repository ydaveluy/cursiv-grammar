// The golang language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "golang/core/ast.hpp"

namespace golang {

struct Golang : cursiv::GrammarSpec {
  // -- Trivia ---- a newline is ordinary trivia everywhere; automatic
  // semicolon insertion (`Eos`, below) is what gives it meaning at a
  // statement boundary.
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<(uc::L | cr<"_">) + many(uc::L | uc::Nd | cr<"_">)> {};

  // -- Literals ----
  static constexpr auto Digits = many(option(lit<"_">) + cr<"0-9">);
  static constexpr auto HexDigits = many(option(lit<"_">) + cr<"0-9a-fA-F">);
  static constexpr auto OctDigits = many(option(lit<"_">) + cr<"0-7">);
  static constexpr auto BinDigits = many(option(lit<"_">) + cr<"01">);
  static constexpr auto Exponent =
      cr<"eE"> + option(cr<"+-">) + cr<"0-9"> + Digits;
  static constexpr auto HexExponent =
      cr<"pP"> + option(cr<"+-">) + cr<"0-9"> + Digits;

  static constexpr auto IntBody =
      (lit<"0"> + cr<"xX"> + option(lit<"_">) + cr<"0-9a-fA-F"> + HexDigits) |
      (lit<"0"> + cr<"bB"> + option(lit<"_">) + cr<"01"> + BinDigits) |
      (lit<"0"> + cr<"oO"> + option(lit<"_">) + cr<"0-7"> + OctDigits) |
      (cr<"1-9"> + Digits) |  //
      (lit<"0"> + OctDigits);
  struct INT_LIT : terminal<IntBody> {};

  static constexpr auto HexMantissa =
      (some(option(lit<"_">) + cr<"0-9a-fA-F">) +
       option(lit<"."> + HexDigits)) |
      (lit<"."> + cr<"0-9a-fA-F"> + HexDigits);
  static constexpr auto FloatBody =
      (cr<"0-9"> + Digits + lit<"."> + option(cr<"0-9"> + Digits) +
       option(Exponent)) |
      (lit<"."> + cr<"0-9"> + Digits + option(Exponent)) |
      (cr<"0-9"> + Digits + Exponent) |
      (lit<"0"> + cr<"xX"> + HexMantissa + HexExponent);
  struct FLOAT_LIT : terminal<FloatBody> {};

  // A decimal imaginary coefficient may start with zero and contain 8/9:
  // unlike an integer such as 089, 089i is a decimal imaginary literal.
  struct IMAGINARY_LIT
      : terminal<(FloatBody | IntBody | (cr<"0-9"> + Digits)) + lit<"i">> {};

  static constexpr auto EscapedValue =
      lit<"\\"> +
      (lit<"u"> + repeat<4>(cr<"0-9a-fA-F">) |
       lit<"U"> + repeat<8>(cr<"0-9a-fA-F">) | cr<R"(abfnrtv\\'")">  //
       | repeat<3>(cr<"0-7">) | lit<"x"> + repeat<2>(cr<"0-9a-fA-F">));

  struct RUNE_LIT
      : terminal<lit<"'"> + (EscapedValue | ~cr<R"(\n')">) +
                 lit<"'">> {};

  struct RAW_STRING : terminal<lit<"`"> + many(cr<"^`">) + lit<"`">> {};
  struct INTERPRETED_STRING
      : terminal<lit<"\""> + many(EscapedValue | cr<R"(^"\\\n)">) + lit<"\"">> {
  };

  // -- Forward declarations of the mutually-recursive rules ----
  struct Type_;
  struct Expression;
  struct Statement;
  struct BlockRule;

  // -- Shared list helpers ----
  template <auto Ptr>
  static constexpr auto IdentListInto =
      append<Ptr>(ref<ID>) + many(lit<","> + append<Ptr>(ref<ID>));
  template <auto Ptr>
  static constexpr auto ExprListInto =
      append<Ptr>(ref<Expression>) +
      many(lit<","> + append<Ptr>(ref<Expression>));

  // Go inserts a semicolon after a line's final identifier, literal,
  // break/continue/fallthrough/return, `++`, `--`, `)`, `]` or `}` --
  // exactly where the grammar already expects `eos`. `)` closes an
  // import/const/type/var group, `}` a block/interface/struct body; both
  // are harmless to offer where neither can actually follow.
  // Go counts LF, including an LF inside a comment. A lone CR is whitespace,
  // so the engine's general `gap::line` (which also includes CR) is too broad.
  static constexpr auto LineBreak = gap::has(lit<"\n">);
  static constexpr auto Eos =
      implied(lit<";">, LineBreak | &lit<"}"> | &lit<")"> | eof);

  // The same rule also ends a statement mid-production, where the grammar
  // has no `eos` token: a signature's unnamed result, break/continue's
  // label, return's operand, else, and a spec or field's un-lined-up type
  // all stop at a newline. `implied` only fires where `eos` is written, so
  // these need the guard spelled explicitly.
  static constexpr auto SameLine = !LineBreak;
  static constexpr auto HeaderSemi = lit<";"> | LineBreak;

  struct EmptyStmtRule : rule<create<ast::EmptyStmt>(lit<";">)> {};

  template <auto Ptr>
  static constexpr auto StatementsInto =
      many(append<Ptr>(ref<Statement>));

  // ======================================================================
  // Types
  // ======================================================================

  static constexpr auto TypeArgsBody =
      option(lit<"["> + append<&ast::NamedType::typeArgs>(ref<Type_>) +
             many(lit<","> + append<&ast::NamedType::typeArgs>(ref<Type_>)) +
             option(lit<",">) + lit<"]">);

  struct QualifiedOrNamedType
      : rule<create<ast::NamedType>(
                 assign<&ast::NamedType::pkg>(ref<ID>) + lit<"."> +
                 assign<&ast::NamedType::name>(ref<ID>) + TypeArgsBody)  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>) + TypeArgsBody)> {};

  struct PointerTypeRule
      : rule<create<ast::PointerType>(
            lit<"*"> + assign<&ast::PointerType::base>(ref<Type_>))> {};

  struct ArrayOrSliceType
      : rule<create<ast::ArrayType>(
                 lit<"["> + assign<&ast::ArrayType::length>(ref<Expression>) +
                 lit<"]"> + assign<&ast::ArrayType::elem>(ref<Type_>))  //
             | create<ast::SliceType>(
                   lit<"["> + lit<"]"> +
                   assign<&ast::SliceType::elem>(ref<Type_>))> {};

  struct MapTypeRule
      : rule<create<ast::MapType>(
            kw<"map"> + lit<"["> + assign<&ast::MapType::key>(ref<Type_>) +
            lit<"]"> + assign<&ast::MapType::elem>(ref<Type_>))> {};

  // `chan<-` is tried before bare `chan`, so `<-` binds to the leftmost
  // `chan` -- the language spec's own tie-break for `chan<- chan T`.
  struct ChannelTypeRule
      : rule<create<ast::ChannelType>(
                 kw<"chan"> +
                 flag<&ast::ChannelType::sendOnly>(lit<"<-">) +
                 assign<&ast::ChannelType::elem>(ref<Type_>))  //
             | create<ast::ChannelType>(
                   kw<"chan"> +
                   assign<&ast::ChannelType::elem>(ref<Type_>))  //
             | create<ast::ChannelType>(
                   flag<&ast::ChannelType::recvOnly>(lit<"<-">) +
                   kw<"chan"> +
                   assign<&ast::ChannelType::elem>(ref<Type_>))> {};

  struct ParamDeclRule
      : rule<create<ast::Param>(
                 IdentListInto<&ast::Param::names> +
                 option(flag<&ast::Param::variadic>(lit<"...">)) +
                 assign<&ast::Param::type>(ref<Type_>))  //
             | create<ast::Param>(
                   option(flag<&ast::Param::variadic>(lit<"...">)) +
                   assign<&ast::Param::type>(ref<Type_>))> {};

  template <auto Ptr>
  static constexpr auto ParamsInto =
      lit<"("> +
      option(append<Ptr>(ref<ParamDeclRule>) +
             many(lit<","> + append<Ptr>(ref<ParamDeclRule>)) +
             option(lit<",">)) +
      lit<")">;

  struct UnnamedResultRule
      : rule<create<ast::Param>(assign<&ast::Param::type>(ref<Type_>))> {};

  struct SignatureRule
      : rule<create<ast::Signature>(
            ParamsInto<&ast::Signature::params> +
            option(SameLine +
                   (ParamsInto<&ast::Signature::results>  //
                    | append<&ast::Signature::results>(ref<UnnamedResultRule>))))> {
  };

  struct FuncTypeRule
      : rule<create<ast::FuncType>(
            kw<"func"> + assign<&ast::FuncType::sig>(ref<SignatureRule>))> {};

  struct MethodSpecRule
      : rule<create<ast::MethodSpec>(
            assign<&ast::MethodSpec::name>(ref<ID>) +
            assign<&ast::MethodSpec::sig>(ref<SignatureRule>))> {};

  struct TypeTermRule
      : rule<create<ast::TypeTerm>(
            option(flag<&ast::TypeTerm::underlying>(lit<"~">)) +
            assign<&ast::TypeTerm::type>(ref<Type_>))> {};

  struct TypeElementRule
      : rule<create<ast::TypeElement>(
            append<&ast::TypeElement::terms>(ref<TypeTermRule>) +
            many(lit<"|"> +
                 append<&ast::TypeElement::terms>(ref<TypeTermRule>)))> {};

  struct InterfaceTypeRule
      : rule<create<ast::InterfaceType>(
            kw<"interface"> + lit<"{"> +
            many((append<&ast::InterfaceType::methods>(ref<MethodSpecRule>)  //
                  | append<&ast::InterfaceType::embeds>(ref<TypeElementRule>)) +
                 Eos) +
            lit<"}">)> {};

  struct FieldDeclRule
      : rule<create<ast::FieldDecl>(
                 IdentListInto<&ast::FieldDecl::names> + SameLine +
                 assign<&ast::FieldDecl::type>(ref<Type_>) +
                 option(assign<&ast::FieldDecl::tag>(
                     ref<RAW_STRING> | ref<INTERPRETED_STRING>)))  //
             | create<ast::FieldDecl>(
                   option(flag<&ast::FieldDecl::embeddedPointer>(
                       lit<"*">)) +
                   flag<&ast::FieldDecl::embedded>(
                       assign<&ast::FieldDecl::type>(ref<QualifiedOrNamedType>)) +
                   option(assign<&ast::FieldDecl::tag>(
                       ref<RAW_STRING> | ref<INTERPRETED_STRING>)))> {};

  struct StructTypeRule
      : rule<create<ast::StructType>(
            kw<"struct"> + lit<"{"> +
            many(append<&ast::StructType::fields>(ref<FieldDeclRule>) + Eos) +
            lit<"}">)> {};

  struct Type_
      : rule<ref<QualifiedOrNamedType>  //
             | ref<PointerTypeRule>     //
             | ref<ArrayOrSliceType>    //
             | ref<StructTypeRule>      //
             | ref<FuncTypeRule>        //
             | ref<InterfaceTypeRule>   //
             | ref<MapTypeRule>         //
             | ref<ChannelTypeRule>     //
             | (lit<"("> + ref<Type_> + lit<")">)> {};

  struct TypeParamRule
      : rule<create<ast::TypeParam>(
            IdentListInto<&ast::TypeParam::names> +
            assign<&ast::TypeParam::constraint>(ref<TypeElementRule>))> {};

  template <auto Ptr>
  static constexpr auto TypeParamsInto =
      option(lit<"["> + append<Ptr>(ref<TypeParamRule>) +
             many(lit<","> + append<Ptr>(ref<TypeParamRule>)) +
             option(lit<",">) + lit<"]">);

  // ======================================================================
  // Expressions
  // ======================================================================

  struct BasicLitRule
      : rule<create<ast::ImaginaryLit>(
                   assign<&ast::ImaginaryLit::text>(ref<IMAGINARY_LIT>))  //
             | create<ast::IntLit>(
                   assign<&ast::IntLit::text>(ref<INT_LIT>))  //
             | create<ast::RuneLit>(
                   assign<&ast::RuneLit::text>(ref<RUNE_LIT>))  //
             | create<ast::StringLit>(assign<&ast::StringLit::text>(
                   ref<RAW_STRING> | ref<INTERPRETED_STRING>))  //
             | create<ast::FloatLit>(
                   assign<&ast::FloatLit::text>(ref<FLOAT_LIT>))> {};

  struct IdentExprRule
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))> {};

  struct FuncLitRule
      : rule<create<ast::FuncLit>(
            kw<"func"> + assign<&ast::FuncLit::sig>(ref<SignatureRule>) +
            assign<&ast::FuncLit::body>(ref<BlockRule>))> {};

  struct LiteralValueRule;

  struct KeyedElementRule
      : rule<create<ast::KeyedElement>(
            option(assign<&ast::KeyedElement::key>(
                       ref<Expression> | ref<LiteralValueRule>) +
                   lit<":">) +
            assign<&ast::KeyedElement::value>(ref<Expression> |
                                              ref<LiteralValueRule>))> {};

  struct LiteralValueRule
      : rule<create<ast::LiteralValue>(
            lit<"{"> +
            option(append<&ast::LiteralValue::elements>(ref<KeyedElementRule>) +
                   many(lit<","> + append<&ast::LiteralValue::elements>(
                                       ref<KeyedElementRule>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  // A type literal's own head, for a composite literal: like `Type_` but
  // without the parenthesised form (never useful as a literal's type) and
  // with the array form's ellipsis-length alternative.
  struct LiteralTypeRule
      : rule<ref<StructTypeRule>  //
             | create<ast::ArrayType>(
                   lit<"["> +
                   flag<&ast::ArrayType::ellipsisLen>(lit<"...">) +
                   lit<"]"> + assign<&ast::ArrayType::elem>(ref<Type_>))  //
             | ref<ArrayOrSliceType>  //
             | ref<MapTypeRule>       //
             | ref<QualifiedOrNamedType>> {};

  struct CompositeLitRule
      : rule<create<ast::CompositeLit>(
            assign<&ast::CompositeLit::type>(ref<LiteralTypeRule>) +
            assign<&ast::CompositeLit::value>(ref<LiteralValueRule>))> {};

  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            lit<")">)> {};

  struct MethodExprRule
      : rule<create<ast::MethodExpr>(
            assign<&ast::MethodExpr::type>(
                ref<ArrayOrSliceType> | ref<StructTypeRule> | ref<FuncTypeRule> |
                ref<InterfaceTypeRule> | ref<MapTypeRule> | ref<ChannelTypeRule>) +
            lit<"."> + assign<&ast::MethodExpr::method>(ref<ID>))> {};

  struct OperandRule
      : rule<ref<BasicLitRule>        //
             | ref<CompositeLitRule>  //
             | ref<FuncLitRule>       //
             | ref<MethodExprRule>    //
             | ref<ParenExprRule>     //
             | ref<IdentExprRule>> {};

  // `T(x)`: a conversion, only reachable for a type literal or a
  // parenthesised type -- a plain named type is already a call once
  // `OperandRule` reads it.
  struct ConversionRule
      : rule<create<ast::ConversionExpr>(
            assign<&ast::ConversionExpr::type>(
                ref<PointerTypeRule> | ref<ArrayOrSliceType> |
                ref<StructTypeRule> | ref<FuncTypeRule> |
                ref<InterfaceTypeRule> | ref<MapTypeRule> |
                ref<ChannelTypeRule> | (lit<"("> + ref<Type_> + lit<")">)) +
            lit<"("> + assign<&ast::ConversionExpr::operand>(ref<Expression>) +
            option(lit<",">) + lit<")">)> {};

  // Go's parser represents types as expressions in call/type-argument
  // lists. Keep our distinct Type hierarchy behind an explicit wrapper;
  // names and pointer expressions keep their existing expression reading.
  struct TypeExprRule
      : rule<create<ast::TypeExpr>(assign<&ast::TypeExpr::type>(ref<Type_>))> {};

  struct ArgumentRule : rule<ref<Expression> | ref<TypeExprRule>> {};

  static constexpr auto PrimaryExprTail =
      nest<ast::SelectorExpr, &ast::SelectorExpr::operand>(
          lit<"."> + assign<&ast::SelectorExpr::field>(ref<ID>)) |
      nest<ast::IndexExpr, &ast::IndexExpr::operand>(
          lit<"["> + append<&ast::IndexExpr::indices>(ref<ArgumentRule>) +
          many(lit<","> + append<&ast::IndexExpr::indices>(ref<ArgumentRule>)) +
          option(lit<",">) +
          lit<"]">) |
      nest<ast::SliceExpr, &ast::SliceExpr::operand>(
          lit<"["> + option(assign<&ast::SliceExpr::low>(ref<Expression>)) +
          lit<":"> + option(assign<&ast::SliceExpr::high>(ref<Expression>)) +
          option(lit<":"> + assign<&ast::SliceExpr::max>(ref<Expression>)) +
          lit<"]">) |
      nest<ast::TypeAssertExpr, &ast::TypeAssertExpr::operand>(
          lit<"."> + lit<"("> + assign<&ast::TypeAssertExpr::type>(ref<Type_>) +
          lit<")">) |
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          lit<"("> +
          option(append<&ast::CallExpr::args>(ref<ArgumentRule>) +
                 many(lit<","> + append<&ast::CallExpr::args>(ref<ArgumentRule>)) +
                 option(flag<&ast::CallExpr::ellipsis>(lit<"...">)) +
                 option(lit<",">)) +
          lit<")">);

  struct PrimaryExprRule
      : rule<(ref<OperandRule> | ref<ConversionRule>) +
             many(SameLine + PrimaryExprTail)> {};

  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"!"> |
                                             lit<"^"> | lit<"&"> | lit<"*"> |
                                             lit<"<-">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExprRule>))  //
             | ref<PrimaryExprRule>> {};

  struct MulOp : rule<SameLine + (lit<"*"> | lit<"/"> | lit<"%"> |
                                  lit<"<<"> | lit<">>"> | lit<"&^"> | lit<"&">)> {};
  struct AddOp : rule<SameLine + (lit<"+"> | lit<"-"> | lit<"|"> | lit<"^">)> {};
  struct CompareOp : rule<SameLine + (lit<"=="> | lit<"!="> | lit<"<="> |
                                      lit<"<"> | lit<">="> | lit<">">)> {};
  struct AndOp : rule<SameLine + lit<"&&">> {};
  struct OrOp : rule<SameLine + lit<"||">> {};

  struct BinaryExprRule
      : infix<UnaryExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MulOp>) >> left(ref<AddOp>) >> left(ref<CompareOp>) >>
               left(ref<AndOp>) >> left(ref<OrOp>))> {};

  struct Expression : rule<ref<BinaryExprRule>> {};

  // ======================================================================
  // Statements
  // ======================================================================

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"|="> | lit<"^="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"<<="> | lit<">>="> | lit<"&^="> | lit<"&="> | lit<"=">;

  struct SimpleStmt
      : rule<create<ast::SendStmt>(
                 assign<&ast::SendStmt::channel>(ref<Expression>) + SameLine + lit<"<-"> +
                 assign<&ast::SendStmt::value>(ref<Expression>))  //
             | create<ast::IncDecStmt>(
                   assign<&ast::IncDecStmt::operand>(ref<Expression>) +
                   assign<&ast::IncDecStmt::op>(lit<"++"> | lit<"--">))  //
             | create<ast::ShortVarDecl>(
                   IdentListInto<&ast::ShortVarDecl::names> + lit<":="> +
                   ExprListInto<&ast::ShortVarDecl::values>)  //
             | create<ast::AssignStmt>(
                   ExprListInto<&ast::AssignStmt::lhs> +
                   assign<&ast::AssignStmt::op>(AssignOp) +
                   ExprListInto<&ast::AssignStmt::rhs>)  //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expression>))> {};

  struct BlockRule
      : rule<create<ast::Block>(
            lit<"{"> + StatementsInto<&ast::Block::stmts> +
            lit<"}">)> {};

  struct LabeledStmtRule
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<ID>) + lit<":"> +
            option(assign<&ast::LabeledStmt::stmt>(ref<Statement>)))> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(SameLine + ExprListInto<&ast::ReturnStmt::values>))> {};

  struct BreakStmtRule
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(SameLine + assign<&ast::BreakStmt::label>(ref<ID>)))> {};

  struct ContinueStmtRule
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(SameLine + assign<&ast::ContinueStmt::label>(ref<ID>)))> {};

  struct GotoStmtRule
      : rule<create<ast::GotoStmt>(
            kw<"goto"> + assign<&ast::GotoStmt::label>(ref<ID>))> {};

  struct FallthroughStmtRule
      : rule<create<ast::FallthroughStmt>(kw<"fallthrough">)> {};

  struct GoStmtRule
      : rule<create<ast::GoStmt>(
            kw<"go"> + assign<&ast::GoStmt::call>(ref<Expression>))> {};

  struct DeferStmtRule
      : rule<create<ast::DeferStmt>(
            kw<"defer"> + assign<&ast::DeferStmt::call>(ref<Expression>))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            kw<"if"> +
            ((option(assign<&ast::IfStmt::init>(ref<SimpleStmt>)) + HeaderSemi +
              assign<&ast::IfStmt::cond>(ref<Expression>))  //
             | assign<&ast::IfStmt::cond>(ref<Expression>)) +
            assign<&ast::IfStmt::then_>(ref<BlockRule>) +
            option(commit(SameLine + kw<"else">, assign<&ast::IfStmt::else_>(ref<IfStmtRule> |
                                                 ref<BlockRule>))))> {};

  static constexpr auto ForClauseBody =
      option(assign<&ast::ForStmt::init>(ref<SimpleStmt>)) + HeaderSemi +
      option(assign<&ast::ForStmt::cond>(ref<Expression>)) + HeaderSemi +
      option(assign<&ast::ForStmt::post>(ref<SimpleStmt>));

  static constexpr auto RangeClauseBody =
      option((ExprListInto<&ast::RangeStmt::lhs> + lit<"=">)  //
             | (IdentListInto<&ast::RangeStmt::declNames> + lit<":=">)) +
      kw<"range"> + assign<&ast::RangeStmt::range>(ref<Expression>);

  struct ForStmtRule
      : rule<create<ast::RangeStmt>(
                 kw<"for"> + RangeClauseBody +
                 assign<&ast::RangeStmt::body>(ref<BlockRule>))  //
             | create<ast::ForStmt>(
                   kw<"for"> + ForClauseBody +
                   assign<&ast::ForStmt::body>(ref<BlockRule>))  //
             | create<ast::ForStmt>(
                   kw<"for"> +
                   option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                   assign<&ast::ForStmt::body>(ref<BlockRule>))> {};

  struct ExprCaseClauseRule
      : rule<create<ast::ExprCaseClause>(
                 kw<"case"> + ExprListInto<&ast::ExprCaseClause::values> +
                 lit<":"> +
                 StatementsInto<&ast::ExprCaseClause::body>)  //
             | create<ast::ExprCaseClause>(
                   flag<&ast::ExprCaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   StatementsInto<&ast::ExprCaseClause::body>)> {};

  static constexpr auto TypeSwitchGuardBody =
      option(assign<&ast::TypeSwitchStmt::bindName>(ref<ID>) + lit<":=">) +
      assign<&ast::TypeSwitchStmt::subject>(ref<Expression>) + lit<"."> +
      lit<"("> + kw<"type"> + lit<")">;

  struct TypeCaseClauseRule
      : rule<create<ast::TypeCaseClause>(
                 kw<"case"> +
                 append<&ast::TypeCaseClause::types>(ref<Type_>) +
                 many(lit<","> + append<&ast::TypeCaseClause::types>(ref<Type_>)) +
                 lit<":"> +
                 StatementsInto<&ast::TypeCaseClause::body>)  //
             | create<ast::TypeCaseClause>(
                   flag<&ast::TypeCaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   StatementsInto<&ast::TypeCaseClause::body>)> {};

  struct SwitchStmtRule
      : rule<create<ast::TypeSwitchStmt>(
                 kw<"switch"> +
                 ((option(assign<&ast::TypeSwitchStmt::init>(ref<SimpleStmt>)) +
                   HeaderSemi + TypeSwitchGuardBody)  //
                  | TypeSwitchGuardBody) +
                 lit<"{"> +
                 many(append<&ast::TypeSwitchStmt::cases>(
                     ref<TypeCaseClauseRule>)) +
                 lit<"}">)  //
             | create<ast::ExprSwitchStmt>(
                   kw<"switch"> +
                   option((option(assign<&ast::ExprSwitchStmt::init>(
                               ref<SimpleStmt>)) +
                           HeaderSemi +
                           option(assign<&ast::ExprSwitchStmt::tag>(
                               ref<Expression>)))  //
                          | assign<&ast::ExprSwitchStmt::tag>(ref<Expression>)) +
                   lit<"{"> +
                   many(append<&ast::ExprSwitchStmt::cases>(
                       ref<ExprCaseClauseRule>)) +
                   lit<"}">)> {};

  struct SendStmtOnlyRule
      : rule<create<ast::SendStmt>(
            assign<&ast::SendStmt::channel>(ref<Expression>) + lit<"<-"> +
            assign<&ast::SendStmt::value>(ref<Expression>))> {};

  struct RecvStmtRule
      : rule<create<ast::RecvStmt>(
            option((ExprListInto<&ast::RecvStmt::lhs> + lit<"=">)  //
                   | (IdentListInto<&ast::RecvStmt::declNames> + lit<":=">)) +
            assign<&ast::RecvStmt::value>(ref<Expression>))> {};

  struct CommClauseRule
      : rule<create<ast::CommClause>(
                 kw<"case"> +
                 assign<&ast::CommClause::comm>(ref<SendStmtOnlyRule> |
                                                ref<RecvStmtRule>) +
                 lit<":"> +
                 StatementsInto<&ast::CommClause::body>)  //
             | create<ast::CommClause>(
                   flag<&ast::CommClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   StatementsInto<&ast::CommClause::body>)> {};

  struct SelectStmtRule
      : rule<create<ast::SelectStmt>(
            kw<"select"> + lit<"{"> +
            many(append<&ast::SelectStmt::cases>(ref<CommClauseRule>)) +
            lit<"}">)> {};

  // ======================================================================
  // Declarations
  // ======================================================================

  struct ConstSpecRule
      : rule<create<ast::ConstSpec>(
            IdentListInto<&ast::ConstSpec::names> +
            option(SameLine + option(assign<&ast::ConstSpec::type>(ref<Type_>)) +
                   lit<"="> + ExprListInto<&ast::ConstSpec::values>))> {};

  struct ConstDeclRule
      : rule<create<ast::ConstDecl>(
            kw<"const"> +
            (append<&ast::ConstDecl::specs>(ref<ConstSpecRule>)  //
             | (lit<"("> +
                many(append<&ast::ConstDecl::specs>(ref<ConstSpecRule>) + Eos) +
                lit<")">)))> {};

  struct TypeSpecRule
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(ref<ID>) +
            TypeParamsInto<&ast::TypeSpec::typeParams> +
            ((flag<&ast::TypeSpec::isAlias>(lit<"=">) +
              assign<&ast::TypeSpec::type>(ref<Type_>))  //
             | assign<&ast::TypeSpec::type>(ref<Type_>)))> {};

  struct TypeDeclRule
      : rule<create<ast::TypeDecl>(
            kw<"type"> +
            (append<&ast::TypeDecl::specs>(ref<TypeSpecRule>)  //
             | (lit<"("> +
                many(append<&ast::TypeDecl::specs>(ref<TypeSpecRule>) + Eos) +
                lit<")">)))> {};

  struct VarSpecRule
      : rule<create<ast::VarSpec>(
            IdentListInto<&ast::VarSpec::names> +
            ((assign<&ast::VarSpec::type>(ref<Type_>) +
              option(lit<"="> + ExprListInto<&ast::VarSpec::values>))  //
             | (lit<"="> + ExprListInto<&ast::VarSpec::values>)))> {};

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            kw<"var"> +
            (append<&ast::VarDecl::specs>(ref<VarSpecRule>)  //
             | (lit<"("> +
                many(append<&ast::VarDecl::specs>(ref<VarSpecRule>) + Eos) +
                lit<")">)))> {};

  struct DeclarationRule
      : rule<ref<ConstDeclRule> | ref<TypeDeclRule> | ref<VarDeclRule>> {};

  struct NonEmptyStmt
      : rule<ref<DeclarationRule>        //
             | ref<GoStmtRule>           //
             | ref<ReturnStmtRule>       //
             | ref<BreakStmtRule>        //
             | ref<ContinueStmtRule>     //
             | ref<GotoStmtRule>         //
             | ref<FallthroughStmtRule>  //
             | ref<DeferStmtRule>        //
             | ref<IfStmtRule>           //
             | ref<SwitchStmtRule>       //
             | ref<SelectStmtRule>       //
             | ref<ForStmtRule>          //
             | ref<BlockRule>            //
             | ref<SimpleStmt>> {};

  // Each statement consumes its own terminator. In particular, a label's
  // explicit empty statement owns one ';', rather than needing a second.
  struct Statement
      : rule<ref<EmptyStmtRule> | ref<LabeledStmtRule> |
             (ref<NonEmptyStmt> + Eos)> {};

  // ======================================================================
  // Top level
  // ======================================================================

  static constexpr auto ReceiverBody =
      lit<"("> +
      option(append<&ast::MethodDecl::receiver>(ref<ParamDeclRule>)) +
      lit<")">;

  struct FunctionDeclRule
      : rule<create<ast::FuncDecl>(
            kw<"func"> + assign<&ast::FuncDecl::name>(ref<ID>) +
            TypeParamsInto<&ast::FuncDecl::typeParams> +
            assign<&ast::FuncDecl::sig>(ref<SignatureRule>) +
            option(assign<&ast::FuncDecl::body>(ref<BlockRule>)))> {};

  struct MethodDeclRule
      : rule<create<ast::MethodDecl>(
            kw<"func"> + ReceiverBody +
            assign<&ast::MethodDecl::name>(ref<ID>) +
            assign<&ast::MethodDecl::sig>(ref<SignatureRule>) +
            option(assign<&ast::MethodDecl::body>(ref<BlockRule>)))> {};

  struct ImportSpecRule
      : rule<create<ast::ImportSpec>(
            (flag<&ast::ImportSpec::dotImport>(lit<".">)  //
             | option(assign<&ast::ImportSpec::alias>(ref<ID>))) +
            assign<&ast::ImportSpec::path>(ref<RAW_STRING> |
                                           ref<INTERPRETED_STRING>))> {};

  static constexpr auto ImportDeclBody =
      kw<"import"> +
      (append<&ast::SourceFile::imports>(ref<ImportSpecRule>)  //
       | (lit<"("> +
          many(append<&ast::SourceFile::imports>(ref<ImportSpecRule>) + Eos) +
          lit<")">));

  struct SourceFileRule
      : rule<create<ast::SourceFile>(
            option(lit<"\xEF\xBB\xBF">) + kw<"package"> +
            assign<&ast::SourceFile::packageName>(ref<ID>) +
            Eos + many(ImportDeclBody + Eos) +
            many(append<&ast::SourceFile::decls>(
                     ref<FunctionDeclRule> | ref<MethodDeclRule> |
                     ref<DeclarationRule>) +
                 Eos))> {};

  using entry = SourceFileRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
  static constexpr auto word_boundary = uc::L | uc::Nd | cr<"_">;
};

}  // namespace golang
