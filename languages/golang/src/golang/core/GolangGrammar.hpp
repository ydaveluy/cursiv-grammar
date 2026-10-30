// The golang language: its grammar.
#pragma once

#include <charconv>
#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "golang/core/ast.hpp"

namespace golang {

inline constexpr auto checked_literal = [](std::string_view text)
    -> cursiv::ConversionResult<std::string_view> {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.size() < 2 || (text.front() != '\'' && text.front() != '"') ||
      text.back() != text.front())
    return Result::err("Incomplete rune or string literal.");
  for (std::size_t i = 1; i + 1 < text.size(); ++i) {
    if (text[i] != '\\') continue;
    if (i + 1 >= text.size() - 1)
      return Result::err("Incomplete literal escape.");
    const char escape = text[++i];
    if (escape == 'u' || escape == 'U') {
      const std::size_t count = escape == 'u' ? 4 : 8;
      // A recovery fragment need not contain the terminal's complete escape.
      if (count > text.size() - i - 2)
        return Result::err("Incomplete Unicode escape.");
      std::uint32_t value = 0;
      const auto converted = std::from_chars(text.data() + i + 1,
                                             text.data() + i + 1 + count,
                                             value, 16);
      if (converted.ec != std::errc{} ||
          converted.ptr != text.data() + i + 1 + count || value > 0x10ffff ||
          (value >= 0xd800 && value <= 0xdfff))
        return cursiv::ConversionResult<std::string_view>::err(
            "Unicode escapes must encode a Unicode scalar value.");
      i += count;
    } else if (escape == 'x' || (escape >= '0' && escape <= '3')) {
      if (i + 2 >= text.size() - 1)
        return Result::err("Incomplete byte escape.");
      i += 2;
    }
  }
  return cursiv::ConversionResult<std::string_view>::ok(text);
};

struct Golang : cursiv::GrammarSpec {
  // -- Trivia ---- a newline is ordinary trivia everywhere; automatic
  // semicolon insertion (`Eos`, below) is what gives it meaning at a
  // statement boundary.
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto ReservedWords =
      kw<"break"> | kw<"default"> | kw<"func"> | kw<"interface"> | kw<"select"> |
      kw<"case"> | kw<"defer"> | kw<"go"> | kw<"map"> | kw<"struct"> |
      kw<"chan"> | kw<"else"> | kw<"goto"> | kw<"package"> | kw<"switch"> |
      kw<"const"> | kw<"fallthrough"> | kw<"if"> | kw<"range"> | kw<"type"> |
      kw<"continue"> | kw<"for"> | kw<"import"> | kw<"return"> | kw<"var">;
  struct ID : terminal<!ReservedWords + (uc::L | cr<"_">) +
                       many(uc::L | uc::Nd | cr<"_">)> {};

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
      ((lit<"u"> + repeat<4>(cr<"0-9a-fA-F">)) |
       (lit<"U"> + repeat<8>(cr<"0-9a-fA-F">)) | cr<R"(abfnrtv\\)">  //
       | (cr<"0-3"> + repeat<2>(cr<"0-7">)) |
       (lit<"x"> + repeat<2>(cr<"0-9a-fA-F">)));

  struct RUNE_LIT
      : terminal<as<checked_literal>(
            lit<"'"> + (EscapedValue | lit<"\\'"> | cr<R"(^\n'\\)">) +
            lit<"'">)> {};

  struct RAW_STRING : terminal<lit<"`"> + many(cr<"^`">) + lit<"`">> {};
  struct INTERPRETED_STRING
      : terminal<as<checked_literal>(lit<"\""> +
          many(EscapedValue | lit<"\\\""> | cr<R"(^"\\\n)">) + lit<"\"">)> {
  };

  // -- Forward declarations of the mutually-recursive rules ----
  struct Type_;
  struct Expression;
  struct Statement;
  struct Block;

  // -- Shared list helpers ----
  template <auto Ptr>
  static constexpr auto IdentListInto =
      some(append<Ptr>(ref<ID>), lit<",">);
  template <auto Ptr, class Expr = Expression>
  static constexpr auto ExprListInto =
      some(append<Ptr>(ref<Expr>), lit<",">);

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

  struct EmptyStmt : rule<create<ast::EmptyStmt>(lit<";">)> {};

  template <auto Ptr>
  static constexpr auto StatementsInto =
      many(append<Ptr>(ref<Statement>));

  // ======================================================================
  // Types
  // ======================================================================

  static constexpr auto TypeArgsBody =
      option(lit<"["> + some(append<&ast::NamedType::typeArgs>(ref<Type_>), lit<",">) +
             option(lit<",">) + lit<"]">);

  struct QualifiedOrNamedType
      : rule<create<ast::NamedType>(
                 assign<&ast::NamedType::pkg>(ref<ID>) + lit<"."> +
                 assign<&ast::NamedType::name>(ref<ID>) + TypeArgsBody)  //
             | create<ast::NamedType>(
                   assign<&ast::NamedType::name>(ref<ID>) + TypeArgsBody)> {};

  struct PointerType
      : rule<create<ast::PointerType>(
            lit<"*"> + assign<&ast::PointerType::base>(ref<Type_>))> {};

  struct ArrayOrSliceType
      : rule<create<ast::ArrayType>(
                 lit<"["> + assign<&ast::ArrayType::length>(ref<Expression>) +
                 lit<"]"> + assign<&ast::ArrayType::elem>(ref<Type_>))  //
             | create<ast::SliceType>(
                   lit<"["> + lit<"]"> +
                   assign<&ast::SliceType::elem>(ref<Type_>))> {};

  struct MapType
      : rule<create<ast::MapType>(
            kw<"map"> + lit<"["> + assign<&ast::MapType::key>(ref<Type_>) +
            lit<"]"> + assign<&ast::MapType::elem>(ref<Type_>))> {};

  // `chan<-` is tried before bare `chan`, so `<-` binds to the leftmost
  // `chan` -- the language spec's own tie-break for `chan<- chan T`.
  struct ChannelType
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

  struct ParamDecl
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
      option(some(append<Ptr>(ref<ParamDecl>), lit<",">) +
             option(lit<",">)) +
      lit<")">;

  struct UnnamedResult
      : rule<create<ast::Param>(assign<&ast::Param::type>(ref<Type_>))> {};

  struct Signature
      : rule<create<ast::Signature>(
            ParamsInto<&ast::Signature::params> +
            option(SameLine +
                   (ParamsInto<&ast::Signature::results>  //
                    | append<&ast::Signature::results>(ref<UnnamedResult>))))> {
  };

  struct FuncType
      : rule<create<ast::FuncType>(
            kw<"func"> + assign<&ast::FuncType::sig>(ref<Signature>))> {};

  struct MethodSpec
      : rule<create<ast::MethodSpec>(
            assign<&ast::MethodSpec::name>(ref<ID>) +
            assign<&ast::MethodSpec::sig>(ref<Signature>))> {};

  struct TypeTerm
      : rule<create<ast::TypeTerm>(
            option(flag<&ast::TypeTerm::underlying>(lit<"~">)) +
            assign<&ast::TypeTerm::type>(ref<Type_>))> {};

  struct TypeElement
      : rule<create<ast::TypeElement>(
            some(append<&ast::TypeElement::terms>(ref<TypeTerm>), lit<"|">))> {};

  struct InterfaceType
      : rule<create<ast::InterfaceType>(
            kw<"interface"> + lit<"{"> +
            many((append<&ast::InterfaceType::methods>(ref<MethodSpec>)  //
                  | append<&ast::InterfaceType::embeds>(ref<TypeElement>)) +
                 Eos) +
            lit<"}">)> {};

  struct FieldDecl
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

  struct StructType
      : rule<create<ast::StructType>(
            kw<"struct"> + lit<"{"> +
            many(append<&ast::StructType::fields>(ref<FieldDecl>) + Eos) +
            lit<"}">)> {};

  struct Type_
      : rule<ref<QualifiedOrNamedType>  //
             | ref<PointerType>     //
             | ref<ArrayOrSliceType>    //
             | ref<StructType>      //
             | ref<FuncType>        //
             | ref<InterfaceType>   //
             | ref<MapType>         //
             | ref<ChannelType>     //
             | (lit<"("> + ref<Type_> + lit<")">)> {};

  struct TypeParam
      : rule<create<ast::TypeParam>(
            IdentListInto<&ast::TypeParam::names> +
            assign<&ast::TypeParam::constraint>(ref<TypeElement>))> {};

  template <auto Ptr>
  static constexpr auto TypeParamsInto =
      option(lit<"["> + some(append<Ptr>(ref<TypeParam>), lit<",">) +
             option(lit<",">) + lit<"]">);

  // ======================================================================
  // Expressions
  // ======================================================================

  struct BasicLit
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

  struct IdentExpr
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))> {};

  struct FuncLit
      : rule<create<ast::FuncLit>(
            kw<"func"> + assign<&ast::FuncLit::sig>(ref<Signature>) +
            assign<&ast::FuncLit::body>(ref<Block>))> {};

  struct LiteralValue;

  struct KeyedElement
      : rule<create<ast::KeyedElement>(
            option(assign<&ast::KeyedElement::key>(
                       ref<Expression> | ref<LiteralValue>) +
                   lit<":">) +
            assign<&ast::KeyedElement::value>(ref<Expression> |
                                              ref<LiteralValue>))> {};

  struct LiteralValue
      : rule<create<ast::LiteralValue>(
            lit<"{"> +
            option(append<&ast::LiteralValue::elements>(ref<KeyedElement>) +
                   many(lit<","> + append<&ast::LiteralValue::elements>(
                                       ref<KeyedElement>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  // A type literal's own head, for a composite literal: like `Type_` but
  // without the parenthesised form (never useful as a literal's type) and
  // with the array form's ellipsis-length alternative.
  static constexpr auto AnonymousLiteralType =
      ref<StructType> |
      create<ast::ArrayType>(
          lit<"["> + flag<&ast::ArrayType::ellipsisLen>(lit<"...">) +
          lit<"]"> + assign<&ast::ArrayType::elem>(ref<Type_>)) |
      ref<ArrayOrSliceType> | ref<MapType>;

  struct LiteralType
      : rule<AnonymousLiteralType | ref<QualifiedOrNamedType>> {};

  struct CompositeLit
      : rule<create<ast::CompositeLit>(
            assign<&ast::CompositeLit::type>(ref<LiteralType>) +
            assign<&ast::CompositeLit::value>(ref<LiteralValue>))> {};

  // A named type followed by { starts the control statement's block at
  // header depth zero (Go1.24 Composite literals; parser.go exprLev < 0).
  // Nested (), [], call arguments, literal values and function bodies use
  // the ordinary expression rules, so named composites remain valid there.
  struct HeaderCompositeLit
      : rule<create<ast::CompositeLit>(
            assign<&ast::CompositeLit::type>(AnonymousLiteralType) +
            assign<&ast::CompositeLit::value>(ref<LiteralValue>))> {};

  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + assign<&ast::ParenExpr::inner>(ref<Expression>) +
            lit<")">)> {};

  struct MethodExpr
      : rule<create<ast::MethodExpr>(
            assign<&ast::MethodExpr::type>(
                ref<ArrayOrSliceType> | ref<StructType> | ref<FuncType> |
                ref<InterfaceType> | ref<MapType> | ref<ChannelType>) +
            lit<"."> + assign<&ast::MethodExpr::method>(ref<ID>))> {};

  static constexpr auto NonCompositeOperand =
      ref<BasicLit> | ref<FuncLit> | ref<MethodExpr> |
      ref<ParenExpr> | ref<IdentExpr>;
  struct Operand : rule<ref<CompositeLit> | NonCompositeOperand> {};
  struct HeaderOperand : rule<ref<HeaderCompositeLit> | NonCompositeOperand> {};

  // `T(x)`: a conversion, only reachable for a type literal or a
  // parenthesised type -- a plain named type is already a call once
  // `Operand` reads it.
  struct Conversion
      : rule<create<ast::ConversionExpr>(
            assign<&ast::ConversionExpr::type>(
                ref<PointerType> | ref<ArrayOrSliceType> |
                ref<StructType> | ref<FuncType> |
                ref<InterfaceType> | ref<MapType> |
                ref<ChannelType> | (lit<"("> + ref<Type_> + lit<")">)) +
            lit<"("> + assign<&ast::ConversionExpr::operand>(ref<Expression>) +
            option(lit<",">) + lit<")">)> {};

  // Go's parser represents types as expressions in call/type-argument
  // lists. Keep our distinct Type hierarchy behind an explicit wrapper;
  // names and pointer expressions keep their existing expression reading.
  struct TypeExpr
      : rule<create<ast::TypeExpr>(assign<&ast::TypeExpr::type>(ref<Type_>))> {};

  struct Argument : rule<ref<Expression> | ref<TypeExpr>> {};

  static constexpr auto PrimaryExprTail =
      nest<ast::SelectorExpr, &ast::SelectorExpr::operand>(
          lit<"."> + assign<&ast::SelectorExpr::field>(ref<ID>)) |
      nest<ast::IndexExpr, &ast::IndexExpr::operand>(
          lit<"["> + some(append<&ast::IndexExpr::indices>(ref<Argument>), lit<",">) +
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
          option(some(append<&ast::CallExpr::args>(ref<Argument>), lit<",">) +
                 option(flag<&ast::CallExpr::ellipsis>(lit<"...">)) +
                 option(lit<",">)) +
          lit<")">);

  template <class OperandRule>
  static constexpr auto PrimaryBody =
      (ref<OperandRule> | ref<Conversion>) + many(SameLine + PrimaryExprTail);
  struct PrimaryExpr : rule<PrimaryBody<Operand>> {};
  struct HeaderPrimaryExpr : rule<PrimaryBody<HeaderOperand>> {};

  struct UnaryExpr;
  struct HeaderUnaryExpr;
  template <class Self, class Primary>
  static constexpr auto UnaryBody =
      create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-"> | lit<"!"> |
                                             lit<"^"> | lit<"&"> | lit<"*"> |
                                             lit<"<-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Self>)) | ref<Primary>;
  struct UnaryExpr : rule<UnaryBody<UnaryExpr, PrimaryExpr>> {};
  struct HeaderUnaryExpr : rule<UnaryBody<HeaderUnaryExpr, HeaderPrimaryExpr>> {};

  struct MulOp : rule<SameLine + (lit<"*"> | lit<"/"> | lit<"%"> |
                                  lit<"<<"> | lit<">>"> | lit<"&^"> | lit<"&">)> {};
  struct AddOp : rule<SameLine + (lit<"+"> | lit<"-"> | lit<"|"> | lit<"^">)> {};
  struct CompareOp : rule<SameLine + (lit<"=="> | lit<"!="> | lit<"<="> |
                                      lit<"<"> | lit<">="> | lit<">">)> {};
  struct AndOp : rule<SameLine + lit<"&&">> {};
  struct OrOp : rule<SameLine + lit<"||">> {};

  static constexpr auto BinaryLevels =
      levels(left(ref<MulOp>), left(ref<AddOp>), left(ref<CompareOp>),
             left(ref<AndOp>), left(ref<OrOp>));
  struct BinaryExpr : infix<UnaryExpr, ast::BinaryExpr, BinaryLevels> {};
  struct HeaderExpression : infix<HeaderUnaryExpr, ast::BinaryExpr, BinaryLevels> {};

  struct Expression : rule<ref<BinaryExpr>> {};

  // ======================================================================
  // Statements
  // ======================================================================

  static constexpr auto AssignOp =
      lit<"+="> | lit<"-="> | lit<"|="> | lit<"^="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"<<="> | lit<">>="> | lit<"&^="> | lit<"&="> | lit<"=">;

  template <class Expr>
  static constexpr auto SimpleStmtBody =
      create<ast::SendStmt>(
                 assign<&ast::SendStmt::channel>(ref<Expr>) + SameLine + lit<"<-"> +
                 assign<&ast::SendStmt::value>(ref<Expr>))  //
             | create<ast::IncDecStmt>(
                   assign<&ast::IncDecStmt::operand>(ref<Expr>) +
                   assign<&ast::IncDecStmt::op>(lit<"++"> | lit<"--">))  //
             | create<ast::ShortVarDecl>(
                   IdentListInto<&ast::ShortVarDecl::names> + lit<":="> +
                   ExprListInto<&ast::ShortVarDecl::values, Expr>)  //
             | create<ast::AssignStmt>(
                   ExprListInto<&ast::AssignStmt::lhs, Expr> +
                   assign<&ast::AssignStmt::op>(AssignOp) +
                   ExprListInto<&ast::AssignStmt::rhs, Expr>)  //
             | create<ast::ExprStmt>(
                   assign<&ast::ExprStmt::value>(ref<Expr>));
  struct SimpleStmt : rule<SimpleStmtBody<Expression>> {};
  struct HeaderSimpleStmt : rule<SimpleStmtBody<HeaderExpression>> {};

  struct Block
      : rule<create<ast::Block>(
            lit<"{"> + StatementsInto<&ast::Block::stmts> +
            lit<"}">)> {};

  struct LabeledStmt
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<ID>) + lit<":"> +
            option(assign<&ast::LabeledStmt::stmt>(ref<Statement>)))> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(SameLine + ExprListInto<&ast::ReturnStmt::values>))> {};

  struct BreakStmt
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(SameLine + assign<&ast::BreakStmt::label>(ref<ID>)))> {};

  struct ContinueStmt
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(SameLine + assign<&ast::ContinueStmt::label>(ref<ID>)))> {};

  struct GotoStmt
      : rule<create<ast::GotoStmt>(
            kw<"goto"> + assign<&ast::GotoStmt::label>(ref<ID>))> {};

  struct FallthroughStmt
      : rule<create<ast::FallthroughStmt>(kw<"fallthrough">)> {};

  struct GoStmt
      : rule<create<ast::GoStmt>(
            kw<"go"> + assign<&ast::GoStmt::call>(ref<Expression>))> {};

  struct DeferStmt
      : rule<create<ast::DeferStmt>(
            kw<"defer"> + assign<&ast::DeferStmt::call>(ref<Expression>))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> +
            ((option(assign<&ast::IfStmt::init>(ref<HeaderSimpleStmt>)) + HeaderSemi +
              assign<&ast::IfStmt::cond>(ref<HeaderExpression>))  //
             | assign<&ast::IfStmt::cond>(ref<HeaderExpression>)) +
            assign<&ast::IfStmt::then_>(ref<Block>) +
            option(commit(SameLine + kw<"else">, assign<&ast::IfStmt::else_>(ref<IfStmt> |
                                                 ref<Block>))))> {};

  static constexpr auto ForClauseBody =
      option(assign<&ast::ForStmt::init>(ref<HeaderSimpleStmt>)) + HeaderSemi +
      option(assign<&ast::ForStmt::cond>(ref<HeaderExpression>)) + HeaderSemi +
      option(assign<&ast::ForStmt::post>(ref<HeaderSimpleStmt>));

  static constexpr auto RangeClauseBody =
      option((ExprListInto<&ast::RangeStmt::lhs, HeaderExpression> + lit<"=">)  //
             | (IdentListInto<&ast::RangeStmt::declNames> + lit<":=">)) +
      kw<"range"> + assign<&ast::RangeStmt::range>(ref<HeaderExpression>);

  struct ForStmt
      : rule<create<ast::RangeStmt>(
                 kw<"for"> + RangeClauseBody +
                 assign<&ast::RangeStmt::body>(ref<Block>))  //
             | create<ast::ForStmt>(
                   kw<"for"> + ForClauseBody +
                   assign<&ast::ForStmt::body>(ref<Block>))  //
             | create<ast::ForStmt>(
                   kw<"for"> +
                   option(assign<&ast::ForStmt::cond>(ref<HeaderExpression>)) +
                   assign<&ast::ForStmt::body>(ref<Block>))> {};

  struct ExprCaseClause
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
      assign<&ast::TypeSwitchStmt::subject>(ref<HeaderExpression>) + lit<"."> +
      lit<"("> + kw<"type"> + lit<")">;

  struct TypeCaseClause
      : rule<create<ast::TypeCaseClause>(
                 kw<"case"> +
                 some(append<&ast::TypeCaseClause::types>(ref<Type_>), lit<",">) +
                 lit<":"> +
                 StatementsInto<&ast::TypeCaseClause::body>)  //
             | create<ast::TypeCaseClause>(
                   flag<&ast::TypeCaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   StatementsInto<&ast::TypeCaseClause::body>)> {};

  struct SwitchStmt
      : rule<create<ast::TypeSwitchStmt>(
                 kw<"switch"> +
                 ((option(assign<&ast::TypeSwitchStmt::init>(ref<HeaderSimpleStmt>)) +
                   HeaderSemi + TypeSwitchGuardBody)  //
                  | TypeSwitchGuardBody) +
                 lit<"{"> +
                 many(append<&ast::TypeSwitchStmt::cases>(
                     ref<TypeCaseClause>)) +
                 lit<"}">)  //
             | create<ast::ExprSwitchStmt>(
                   kw<"switch"> +
                   option((option(assign<&ast::ExprSwitchStmt::init>(
                               ref<HeaderSimpleStmt>)) +
                           HeaderSemi +
                           option(assign<&ast::ExprSwitchStmt::tag>(
                               ref<HeaderExpression>)))  //
                          | assign<&ast::ExprSwitchStmt::tag>(ref<HeaderExpression>)) +
                   lit<"{"> +
                   many(append<&ast::ExprSwitchStmt::cases>(
                       ref<ExprCaseClause>)) +
                   lit<"}">)> {};

  struct SendStmtOnly
      : rule<create<ast::SendStmt>(
            assign<&ast::SendStmt::channel>(ref<Expression>) + lit<"<-"> +
            assign<&ast::SendStmt::value>(ref<Expression>))> {};

  struct RecvStmt
      : rule<create<ast::RecvStmt>(
            option((ExprListInto<&ast::RecvStmt::lhs> + lit<"=">)  //
                   | (IdentListInto<&ast::RecvStmt::declNames> + lit<":=">)) +
            assign<&ast::RecvStmt::value>(ref<Expression>))> {};

  struct CommClause
      : rule<create<ast::CommClause>(
                 kw<"case"> +
                 assign<&ast::CommClause::comm>(ref<SendStmtOnly> |
                                                ref<RecvStmt>) +
                 lit<":"> +
                 StatementsInto<&ast::CommClause::body>)  //
             | create<ast::CommClause>(
                   flag<&ast::CommClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   StatementsInto<&ast::CommClause::body>)> {};

  struct SelectStmt
      : rule<create<ast::SelectStmt>(
            kw<"select"> + lit<"{"> +
            many(append<&ast::SelectStmt::cases>(ref<CommClause>)) +
            lit<"}">)> {};

  // ======================================================================
  // Declarations
  // ======================================================================

  struct ConstSpec
      : rule<create<ast::ConstSpec>(
            IdentListInto<&ast::ConstSpec::names> +
            option(SameLine + option(assign<&ast::ConstSpec::type>(ref<Type_>)) +
                   lit<"="> + ExprListInto<&ast::ConstSpec::values>))> {};

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            kw<"const"> +
            (append<&ast::ConstDecl::specs>(ref<ConstSpec>)  //
             | (lit<"("> +
                many(append<&ast::ConstDecl::specs>(ref<ConstSpec>) + Eos) +
                lit<")">)))> {};

  struct TypeSpec
      : rule<create<ast::TypeSpec>(
            assign<&ast::TypeSpec::name>(ref<ID>) +
            TypeParamsInto<&ast::TypeSpec::typeParams> +
            ((flag<&ast::TypeSpec::isAlias>(lit<"=">) +
              assign<&ast::TypeSpec::type>(ref<Type_>))  //
             | assign<&ast::TypeSpec::type>(ref<Type_>)))> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
            kw<"type"> +
            (append<&ast::TypeDecl::specs>(ref<TypeSpec>)  //
             | (lit<"("> +
                many(append<&ast::TypeDecl::specs>(ref<TypeSpec>) + Eos) +
                lit<")">)))> {};

  struct VarSpec
      : rule<create<ast::VarSpec>(
            IdentListInto<&ast::VarSpec::names> +
            ((assign<&ast::VarSpec::type>(ref<Type_>) +
              option(lit<"="> + ExprListInto<&ast::VarSpec::values>))  //
             | (lit<"="> + ExprListInto<&ast::VarSpec::values>)))> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
            kw<"var"> +
            (append<&ast::VarDecl::specs>(ref<VarSpec>)  //
             | (lit<"("> +
                many(append<&ast::VarDecl::specs>(ref<VarSpec>) + Eos) +
                lit<")">)))> {};

  struct Declaration
      : rule<ref<ConstDecl> | ref<TypeDecl> | ref<VarDecl>> {};

  struct NonEmptyStmt
      : rule<ref<Declaration>        //
             | ref<GoStmt>           //
             | ref<ReturnStmt>       //
             | ref<BreakStmt>        //
             | ref<ContinueStmt>     //
             | ref<GotoStmt>         //
             | ref<FallthroughStmt>  //
             | ref<DeferStmt>        //
             | ref<IfStmt>           //
             | ref<SwitchStmt>       //
             | ref<SelectStmt>       //
             | ref<ForStmt>          //
             | ref<Block>            //
             | ref<SimpleStmt>> {};

  // Each statement consumes its own terminator. In particular, a label's
  // explicit empty statement owns one ';', rather than needing a second.
  struct Statement
      : rule<ref<EmptyStmt> | ref<LabeledStmt> |
             (ref<NonEmptyStmt> + Eos)> {};

  // ======================================================================
  // Top level
  // ======================================================================

  static constexpr auto ReceiverBody =
      lit<"("> +
      option(append<&ast::MethodDecl::receiver>(ref<ParamDecl>)) +
      lit<")">;

  struct FunctionDecl
      : rule<create<ast::FuncDecl>(
            kw<"func"> + assign<&ast::FuncDecl::name>(ref<ID>) +
            TypeParamsInto<&ast::FuncDecl::typeParams> +
            assign<&ast::FuncDecl::sig>(ref<Signature>) +
            option(assign<&ast::FuncDecl::body>(ref<Block>)))> {};

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
            kw<"func"> + ReceiverBody +
            assign<&ast::MethodDecl::name>(ref<ID>) +
            assign<&ast::MethodDecl::sig>(ref<Signature>) +
            option(assign<&ast::MethodDecl::body>(ref<Block>)))> {};

  struct ImportSpec
      : rule<create<ast::ImportSpec>(
            (flag<&ast::ImportSpec::dotImport>(lit<".">)  //
             | option(assign<&ast::ImportSpec::alias>(ref<ID>))) +
            assign<&ast::ImportSpec::path>(ref<RAW_STRING> |
                                           ref<INTERPRETED_STRING>))> {};

  static constexpr auto ImportDeclBody =
      kw<"import"> +
      (append<&ast::SourceFile::imports>(ref<ImportSpec>)  //
       | (lit<"("> +
          many(append<&ast::SourceFile::imports>(ref<ImportSpec>) + Eos) +
          lit<")">));

  struct SourceFile
      : rule<create<ast::SourceFile>(
            option(lit<"\xEF\xBB\xBF">) + kw<"package"> +
            assign<&ast::SourceFile::packageName>(ref<ID>) +
            Eos + many(ImportDeclBody + Eos) +
            many(append<&ast::SourceFile::decls>(
                     ref<FunctionDecl> | ref<MethodDecl> |
                     ref<Declaration>) +
                 Eos))> {};

  using entry = SourceFile;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
  static constexpr auto word_boundary = uc::L | uc::Nd | cr<"_">;
};

}  // namespace golang
