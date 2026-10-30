// The V language: its grammar. A handful of shared-prefix rewrites keep
// two syntactically identical spellings (a selector and
// a package-qualified name, a call and a conversion) from forking the parse
// on every occurrence.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "v/core/ast.hpp"

namespace v {

struct V : cursiv::GrammarSpec {
  struct Type;
  struct Expression;
  struct Block;
  struct Statement;
  struct LabelStmt;
  struct Signature;
  struct Result_;
  struct Param;
  struct NamedType;
  struct QualifiedType;
  struct ArrayType;
  struct FunctionType;
  struct MethodSpec;
  struct InterfaceType;
  struct MapType;
  struct TypeLit;
  struct KeyedElement;
  struct LiteralValue;
  struct LiteralType;
  struct CompositeLit;
  struct ArrayLit;
  struct FunctionLit;
  struct Operand;
  struct PrimaryExpr;
  struct UnaryExpr;
  struct ExprStmt;
  struct IncDecStmt;
  struct AssignStmt;
  struct SimpleStmt;
  struct ReturnStmt;
  struct BreakStmt;
  struct ContinueStmt;
  struct IfStmt;
  struct ForStmt;
  struct ConstSpec;
  struct ConstDecl;
  struct VarDecl;
  struct PlainVarDecl;
  struct FieldDecl;
  struct StructDecl;
  struct FunctionDecl;
  struct TopLevelDecl;
  struct ModuleClause;
  struct ImportSpec;
  struct SourceFile;

  // V 0.4.12 uses ASCII identifiers.
  static constexpr auto word_boundary = cr<"_a-zA-Z0-9">;

  // -- Trivia ---- WS and TERMINATOR are one hidden class: `gap::line`
  // reads the raw gap bytes directly, so no separate newline token is
  // needed to tell ASI's boundary condition apart from plain spacing.
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<lit<"/*"> +
      many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) + lit<"*/">> {};

  // A statement ends at a line break, right before `}`, or at EOF --
  // A written semicolon is also a statement terminator.
  static constexpr auto Eos = lit<";"> | gap::line | &lit<"}"> | eof;

  struct IdentifierToken : terminal<cr<"_a-zA-Z"> + many(cr<"_a-zA-Z0-9">)> {};
  static constexpr auto Reserved =
      kw<"assert"> | kw<"struct"> | kw<"if"> |
      kw<"else"> | kw<"asm"> | kw<"return"> | kw<"module"> |
      kw<"sizeof"> | kw<"isreftype"> | kw<"_likely_"> | kw<"_unlikely_"> |
      kw<"go"> | kw<"goto"> | kw<"const"> | kw<"mut"> |
      kw<"shared"> | kw<"lock"> | kw<"rlock"> | kw<"type"> |
      kw<"for"> | kw<"fn"> | kw<"true"> | kw<"false"> |
      kw<"continue"> | kw<"break"> | kw<"import"> | kw<"unsafe"> |
      kw<"typeof"> | kw<"dump"> | kw<"enum"> | kw<"interface"> |
      kw<"pub"> | kw<"in"> | kw<"atomic"> | kw<"or"> |
      kw<"__global"> | kw<"union"> | kw<"static"> | kw<"volatile"> |
      kw<"as"> | kw<"defer"> | kw<"match"> | kw<"select"> |
      kw<"none"> | kw<"nil"> | kw<"__offsetof"> | kw<"is"> |
      kw<"spawn"> | kw<"implements">;
  struct ID : rule<as<std::string_view>(!Reserved + ref<IdentifierToken>)> {};
  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto OctalDigit = cr<"0-7">;
  template <auto Digit>
  static constexpr auto DigitRun = Digit + many(option(lit<"_">) + Digit);
  static constexpr auto DecimalRun = DigitRun<d>;
  struct INT_LIT : terminal<
      (lit<"0"> + cr<"xX"> + DigitRun<HexDigit>) |
      (lit<"0"> + cr<"bB"> + DigitRun<cr<"01">>) |
      (lit<"0"> + cr<"oO"> + DigitRun<OctalDigit>) | DecimalRun> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};
  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + DecimalRun;
  struct FLOAT_LIT : terminal<
      (DecimalRun + lit<".", !lit<"."> + !(cr<"a-df-zA-DF-Z_">)> + option(DecimalRun) + option(Exponent)) |
      (DecimalRun + Exponent) | (lit<"."> + DecimalRun + option(Exponent))> {};
  struct FloatingLiteral : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {};
  static constexpr auto StrEscape =
      (lit<"\\u"> + repeat<4>(HexDigit)) |
      (lit<"\\U"> + repeat<8>(HexDigit)) |
      (lit<"\\x"> + repeat<2>(HexDigit)) |
      (lit<"\\"> + repeat<3>(OctalDigit)) |
      (lit<"\\"> + cr<R"(abfnrtv\\'"$`)">);
  struct STRING_LIT : terminal<
      (lit<"r'"> + many(cr<"^'">) + lit<"'">) |
      (lit<"r\""> + many(cr<R"(^")">) + lit<"\"">) |
      (lit<"'"> + many(StrEscape | cr<R"(^'\\)">) + lit<"'">) |
      (lit<"\""> + many(StrEscape | cr<R"(^"\\)">) + lit<"\"">)> {};
  struct CHAR_LIT : terminal<lit<"`"> + (StrEscape | cr<R"(^`\\\r\n)">) + lit<"`">> {};
  struct StringLiteral : rule<assign<&ast::StringLit::text>(ref<STRING_LIT>)> {};
  struct RuneLiteral : rule<assign<&ast::RuneLit::text>(ref<CHAR_LIT>)> {};
  struct BoolLiteral : rule<create<ast::BoolLit>(flag<&ast::BoolLit::value>(kw<"true">) | kw<"false">)> {};
  struct NoneLiteral : rule<create<ast::NoneLit>(kw<"none">)> {};
  struct NilLiteral : rule<create<ast::NilLit>(kw<"nil">)> {};

  // -- Reusable list tails, parameterised on the member they fill ----
  template <auto Ptr>
  static constexpr auto IdentifierListFor =
      some(append<Ptr>(ref<ID>), lit<",">);

  template <auto Ptr>
  static constexpr auto ExpressionListFor =
      some(append<Ptr>(ref<Expression>), lit<",">);

  template <auto Ptr>
  static constexpr auto ParametersFor =
      lit<"("> +
      option(append<Ptr>(ref<v::V::Param>) +
             many(lit<","> + append<Ptr>(ref<v::V::Param>)) + option(lit<",">)) +
      lit<")">;

  // -- Types ----

  // `'&'? 'mut'? IDENTIFIER`.
  struct NamedType
      : rule<create<ast::NamedType>(
            option(flag<&ast::NamedType::pointer>(lit<"&">)) +
            option(flag<&ast::NamedType::mut>(kw<"mut">)) +
            assign<&ast::NamedType::name>(ref<ID>))> {};

  // `name '.' name`, a type position only: an expression-level qualified
  // name is folded into SelectorExpr instead.
  struct QualifiedType
      : rule<create<ast::QualifiedType>(
            assign<&ast::QualifiedType::pkg>(ref<ID>) + lit<"."> +
            assign<&ast::QualifiedType::name>(ref<ID>))> {};

  struct ArrayType
      : rule<create<ast::ArrayType>(
            lit<"["> + option(assign<&ast::ArrayType::length>(ref<Expression>)) +
            lit<"]"> + assign<&ast::ArrayType::element>(ref<Type>))> {};

  struct FunctionType
      : rule<create<ast::FunctionType>(
            kw<"fn"> + assign<&ast::FunctionType::signature>(ref<Signature>))> {
  };

  struct Param : rule<create<ast::Param>(option(flag<&ast::Param::mut>(kw<"mut">)) +
      option(append<&ast::Param::names>(ref<ID>)) + assign<&ast::Param::type>(ref<Type>))> {};
  struct NamedParam : rule<create<ast::Param>(option(flag<&ast::Param::mut>(kw<"mut">)) +
      append<&ast::Param::names>(ref<ID>) + assign<&ast::Param::type>(ref<Type>))> {};

  struct MethodSpec
      : rule<create<ast::MethodSpec>(
                 assign<&ast::MethodSpec::name>(ref<ID>) +
                 ParametersFor<&ast::MethodSpec::params> +
                 option(!gap::line +
                        assign<&ast::MethodSpec::result>(ref<Result_>)))  //
             | create<ast::MethodSpec>(assign<&ast::MethodSpec::name>(ref<ID>) +
                   assign<&ast::MethodSpec::fieldType>(ref<Type>))
             | create<ast::MethodSpec>(assign<&ast::MethodSpec::embedded>(
                   ref<NamedType> | ref<QualifiedType>))> {};

  struct InterfaceType
      : rule<create<ast::InterfaceType>(
            kw<"interface"> + assign<&ast::InterfaceType::name>(ref<ID>) +
            lit<"{"> +
            many(append<&ast::InterfaceType::methods>(ref<MethodSpec>) + Eos) +
            lit<"}">)> {};

  struct MapType
      : rule<create<ast::MapType>(
            kw<"map"> + lit<"["> + assign<&ast::MapType::key>(ref<Type>) +
            lit<"]"> + assign<&ast::MapType::element>(ref<Type>))> {};

  // Array/function/interface/map types: disjoint from an expression's own
  // start (except `fn`, resolved by FunctionLit's mandatory trailing block),
  // so it is safe to lead a conversion or a type-led call argument with one
  // -- unlike a bare name, which a call already covers.
  struct TypeLit
      : rule<ref<ArrayType> | ref<FunctionType> | ref<MapType>> {};

  struct Type
      : rule<ref<NamedType> | ref<QualifiedType> | ref<ArrayType> |
            ref<FunctionType> | ref<MapType>> {};

  struct Result_
      : rule<create<ast::Result_>(
                 flag<&ast::Result_::isParams>(lit<"(">) +
                 option(some(append<&ast::Result_::params>(ref<Param>), lit<",">) +
                        option(lit<",">)) +
                 lit<")">)  //
             | create<ast::Result_>(assign<&ast::Result_::type>(ref<Type>))> {};

  struct Signature
      : rule<create<ast::Signature>(
            ParametersFor<&ast::Signature::params> +
            option(!gap::line + assign<&ast::Signature::result>(ref<Result_>)))> {
  };

  struct NamedSignature : rule<create<ast::Signature>(lit<"("> +
      option(some(append<&ast::Signature::params>(ref<NamedParam>), lit<",">) + option(lit<",">)) +
      lit<")"> + option(!gap::line + assign<&ast::Signature::result>(ref<Result_>)))> {};

  // -- Expressions ----

  struct KeyedElement
      : rule<create<ast::KeyedElement>(
            option(assign<&ast::KeyedElement::key>(ref<Expression> |
                                                    ref<v::V::LiteralValue>) +
                   lit<":">) +
            assign<&ast::KeyedElement::value>(ref<Expression> |
                                              ref<v::V::LiteralValue>))> {};

  // Struct fields may also be separated by a line break.
  struct LiteralValue
      : rule<lit<"{"> +
            option(some(append<&ast::LiteralValue::elements>(ref<KeyedElement>), lit<","> | gap::line) +
                   option(lit<",">)) +
            lit<"}">> {};

  struct LiteralType
      : rule<ref<ArrayType> | ref<NamedType> | ref<QualifiedType>> {
  };

  struct EmptyLiteralValue : rule<create<ast::LiteralValue>(lit<"{"> + lit<"}">)> {};
  struct CompositeLit : rule<create<ast::CompositeLit>(
      (assign<&ast::CompositeLit::type>(ref<MapType>) + assign<&ast::CompositeLit::value>(ref<EmptyLiteralValue>)) |
      (assign<&ast::CompositeLit::type>(ref<LiteralType>) + assign<&ast::CompositeLit::value>(ref<LiteralValue>)))> {};

  struct ArrayLit
      : rule<create<ast::ArrayLit>(
            lit<"["> + ExpressionListFor<&ast::ArrayLit::elements> + option(lit<",">) + lit<"]">)> {
  };

  struct FunctionLit
      : rule<create<ast::FunctionLit>(
            kw<"fn"> + assign<&ast::FunctionLit::signature>(ref<NamedSignature>) +
            assign<&ast::FunctionLit::body>(ref<Block>))> {};

  struct MatchArm;
  struct MatchExpr
      : rule<kw<"match"> + assign<&ast::MatchExpr::subject>(ref<Expression>) +
             lit<"{"> +
             many(append<&ast::MatchExpr::arms>(ref<MatchArm>) + option(lit<",">)) +
             lit<"}">> {};
  struct MatchArm
      : rule<(flag<&ast::MatchArm::isElse>(kw<"else">) |
              some(append<&ast::MatchArm::patterns>(ref<Expression>), lit<",">)) +
             assign<&ast::MatchArm::body>(ref<Block>)> {};
  struct EnumValue : rule<lit<"."> + assign<&ast::EnumValue::name>(ref<ID>)> {};
  struct Operand : rule<ref<MatchExpr> | ref<EnumValue> | ref<IntegerLiteral> |
                        ref<FloatingLiteral> | ref<StringLiteral> | ref<RuneLiteral> | ref<BoolLiteral> | ref<NoneLiteral> | ref<NilLiteral> | ref<CompositeLit> |
                        ref<FunctionLit> | ref<ArrayLit> | ref<LiteralValue> | ref<Identifier> |
                        (lit<"("> + ref<Expression> + lit<")">)> {};

  static constexpr auto Arguments =
      lit<"("> +
      option((assign<&ast::CallExpr::typeArg>(ref<TypeLit>) +
              option(lit<","> + ExpressionListFor<&ast::CallExpr::args>))  //
             | ExpressionListFor<&ast::CallExpr::args>) +
      option(lit<",">) + lit<")">;

  // Postfix chain, `nest<>` growing it around what came before: a selector
  // reading `.name` also covers a package-qualified name and a
  // method-expression, and a call reading `Arguments` also covers a type
  // conversion for a named type.
  struct PrimaryExpr
      : rule<ref<Operand> + many(
            nest<ast::OrExpr, &ast::OrExpr::operand>(
                kw<"or"> + assign<&ast::OrExpr::handler>(ref<Block>)) |
            nest<ast::SelectorExpr, &ast::SelectorExpr::object>(
                lit<"."> + assign<&ast::SelectorExpr::field>(ref<ID>))  //
            | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                  lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
                  lit<"]">)  //
            | nest<ast::CallExpr, &ast::CallExpr::callee>(Arguments))> {};

  static constexpr auto OpUnary =
      lit<"-"> | lit<"!"> | lit<"~"> | lit<"*"> | lit<"&">;
  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(OpUnary) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>))  //
             | ref<PrimaryExpr>> {};

  // Native V precedence tiers for the supported operator subset.
  static constexpr auto OpMul =
      lit<"<<"> | lit<">>>"> | lit<">>"> | lit<"&"> | lit<"*"> | lit<"/"> |
      lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-"> | lit<"|"> | lit<"^">;
  static constexpr auto OpRel =
      lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  // The native scanner distinguishes logical && from & followed by prefix &.
  static constexpr auto OpLogAnd = lit<"&&", s | lit<"!"> | lit<"\x85"> | lit<"\xA0">>;
  static constexpr auto OpMembership = kw<"in"> | kw<"!in">;
  static constexpr auto OpLogOr = lit<"||">;

  struct Expression
      : infix<UnaryExpr, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpRel), left(OpMembership),
                     left(OpLogAnd | OpLogOr))> {};

  // -- Statements ----

  struct ExprStmt
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>))> {
  };

  struct IncDecStmt
      : rule<create<ast::IncDecStmt>(
            assign<&ast::IncDecStmt::operand>(ref<Expression>) +
            assign<&ast::IncDecStmt::op>(lit<"++"> | lit<"--">))> {};

  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>>="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> |
      lit<"/="> | lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> |
      lit<"=">;
  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            ExpressionListFor<&ast::AssignStmt::targets> +
            assign<&ast::AssignStmt::op>(AssignOp) +
            ExpressionListFor<&ast::AssignStmt::values>)> {};

  struct SimpleStmt
      : rule<ref<AssignStmt> | ref<IncDecStmt> | ref<ExprStmt>> {};

  struct ReturnStmt
      : rule<create<ast::ReturnStmt>(
            kw<"return"> + (ExpressionListFor<&ast::ReturnStmt::values> | &(lit<"}"> | (ref<ID> + lit<":", !lit<"=">>))))> {
  };
  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break"> + option(!gap::line + assign<&ast::BreakStmt::label>(ref<ID>)))> {};
  struct ContinueStmt : rule<create<ast::ContinueStmt>(kw<"continue"> + option(!gap::line + assign<&ast::ContinueStmt::label>(ref<ID>)))> {};

  struct IfStmt : rule<create<ast::IfStmt>(kw<"if"> +
      assign<&ast::IfStmt::cond>(ref<Expression>) + assign<&ast::IfStmt::thenBranch>(ref<Block>) +
      option(kw<"else"> + assign<&ast::IfStmt::elseBranch>(ref<IfStmt> | ref<Block>)))> {};

  struct Iterable : rule<ref<Expression> + option(nest<ast::RangeExpr, &ast::RangeExpr::low>(
      lit<".."> + assign<&ast::RangeExpr::high>(ref<Expression>)))> {};
  // The classic three-part form's init also admits a `:=` declaration:
  // the idiomatic `for i := 0; i < n; i++` needs it.
  struct ForStmt
      : rule<create<ast::ForStmt>(
                 kw<"for"> + ExpressionListFor<&ast::ForStmt::inTargets> +
                 kw<"in"> + assign<&ast::ForStmt::inIterable>(ref<Iterable>) +
                 assign<&ast::ForStmt::body>(ref<Block>))  //
             | create<ast::ForStmt>(
                   kw<"for"> +
                   option(assign<&ast::ForStmt::init>(ref<PlainVarDecl> |
                                                       ref<SimpleStmt>)) +
                   flag<&ast::ForStmt::isForClause>(lit<";">) +
                   option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                   lit<";"> +
                   option(assign<&ast::ForStmt::post>(ref<SimpleStmt>)) +
                   assign<&ast::ForStmt::body>(ref<Block>))  //
             | create<ast::ForStmt>(
                   kw<"for"> +
                   option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                   assign<&ast::ForStmt::body>(ref<Block>))> {};

  // -- Declarations ----

  struct ConstSpec
      : rule<create<ast::ConstSpec>(
            assign<&ast::ConstSpec::name>(ref<ID>) + lit<"="> +
            assign<&ast::ConstSpec::value>(ref<Expression>))> {};

  struct ConstDecl : rule<create<ast::ConstDecl>(
      option(flag<&ast::ConstDecl::isPub>(kw<"pub">)) + kw<"const"> +
      ((lit<"("> + many(append<&ast::ConstDecl::specs>(ref<ConstSpec>) + (Eos | &lit<")">)) + lit<")">) |
       append<&ast::ConstDecl::specs>(ref<ConstSpec>)))> {};
  static constexpr auto VariableBody = IdentifierListFor<&ast::VarDecl::names> + lit<":="> +
      ExpressionListFor<&ast::VarDecl::values>;
  struct VarDecl : rule<create<ast::VarDecl>(option(flag<&ast::VarDecl::mut>(kw<"mut">)) + VariableBody)> {};
  struct PlainVarDecl : rule<create<ast::VarDecl>(VariableBody)> {};

  // A field (named, or anonymous -- an embedded type alone), or a `pub:` /
  // `mut:` / `pub mut:` visibility marker: `type == nullptr` tells the
  // marker forms apart from a real field.
  struct FieldDecl
      : rule<create<ast::FieldDecl>(
                 append<&ast::FieldDecl::names>(ref<ID>) + !gap::line +
                 assign<&ast::FieldDecl::type>(ref<Type>) +
                 option(lit<"="> + assign<&ast::FieldDecl::defaultValue>(ref<Expression>)))  //
             | create<ast::FieldDecl>(
                   assign<&ast::FieldDecl::type>(ref<NamedType> |
                                                 ref<QualifiedType>))  //
             | create<ast::FieldDecl>(
                   flag<&ast::FieldDecl::pub>(kw<"pub">) +
                   option(flag<&ast::FieldDecl::mut>(kw<"mut">)) +
                   flag<&ast::FieldDecl::isSection>(lit<":">))  //
             | create<ast::FieldDecl>(flag<&ast::FieldDecl::mut>(kw<"mut">) +
                                      flag<&ast::FieldDecl::isSection>(lit<":">))> {};

  struct StructDecl
      : rule<create<ast::StructDecl>(
            option(flag<&ast::StructDecl::isPub>(kw<"pub">)) + kw<"struct"> + assign<&ast::StructDecl::name>(ref<ID>) + lit<"{"> +
            many(append<&ast::StructDecl::fields>(ref<FieldDecl>) + Eos) +
            lit<"}">)> {};

  struct EnumMember
      : rule<assign<&ast::EnumMember::name>(ref<ID>) +
             option(lit<"="> + assign<&ast::EnumMember::value>(ref<Expression>))> {};
  struct EnumDecl
      : rule<option(flag<&ast::EnumDecl::isPub>(kw<"pub">)) + kw<"enum"> +
             assign<&ast::EnumDecl::name>(ref<ID>) + lit<"{"> +
             many(append<&ast::EnumDecl::members>(ref<EnumMember>) + option(lit<",">)) +
             lit<"}">> {};
  struct InterfaceDecl : rule<create<ast::InterfaceDecl>(
      option(flag<&ast::InterfaceDecl::isPub>(kw<"pub">)) +
      assign<&ast::InterfaceDecl::type>(ref<InterfaceType>))> {};

  static constexpr auto FunctionEnd =
      (assign<&ast::FunctionDecl::signature>(ref<NamedSignature>) + assign<&ast::FunctionDecl::body>(ref<Block>)) |
      (assign<&ast::FunctionDecl::signature>(ref<Signature>) + !lit<"{">);
  struct FunctionDecl : rule<create<ast::FunctionDecl>(
      option(flag<&ast::FunctionDecl::isPub>(kw<"pub">)) + kw<"fn"> +
      option(lit<"("> + append<&ast::FunctionDecl::receiver>(ref<NamedParam>) + lit<")">) +
      assign<&ast::FunctionDecl::name>(ref<ID>) + FunctionEnd)> {};

  struct TopLevelDecl : rule<ref<EnumDecl> | ref<ConstDecl> | ref<StructDecl> | ref<InterfaceDecl> | ref<FunctionDecl>> {};

  struct Statement
      : rule<ref<EnumDecl> | ref<VarDecl> | ref<StructDecl> | ref<SimpleStmt> | ref<ReturnStmt> |
            ref<BreakStmt> | ref<ContinueStmt> | ref<Block> | ref<IfStmt> |
            ref<ForStmt> | ref<LabelStmt>> {};

  struct LabelStmt : rule<create<ast::LabelStmt>(assign<&ast::LabelStmt::name>(ref<ID>) +
      lit<":"> + assign<&ast::LabelStmt::body>(ref<ForStmt>))> {};
  struct Block
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::stmts>(ref<Statement>) + Eos) +
            lit<"}">)> {};

  struct ModuleClause
      : rule<create<ast::ModuleClause>(
            kw<"module"> + assign<&ast::ModuleClause::name>(ref<ID>))> {};

  struct ImportPath : rule<as<std::string_view>(some(ref<ID>, lit<".">) | ref<STRING_LIT>)> {};
  struct ImportSpec : rule<create<ast::ImportSpec>(kw<"import"> +
      assign<&ast::ImportSpec::path>(ref<ImportPath>) +
      option(kw<"as"> + assign<&ast::ImportSpec::alias>(ref<ID>)))> {};

  struct SourceFile
      : rule<create<ast::SourceFile>(
            option(assign<&ast::SourceFile::module>(ref<ModuleClause>) + Eos) +
            many(append<&ast::SourceFile::imports>(ref<ImportSpec>) + Eos) +
            many(append<&ast::SourceFile::decls>(ref<TopLevelDecl>) + Eos))> {
  };

  using entry = SourceFile;
  using ignored = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace v
