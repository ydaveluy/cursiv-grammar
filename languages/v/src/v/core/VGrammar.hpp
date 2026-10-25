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
  struct ConversionExpr;
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
  struct CaseClause;
  struct SwitchStmt;
  struct ForStmt;
  struct ConstSpec;
  struct ConstDecl;
  struct VarDecl;
  struct FieldDecl;
  struct StructDecl;
  struct Declaration;
  struct FunctionDecl;
  struct TopLevelDecl;
  struct ModuleClause;
  struct ImportSpec;
  struct SourceFile;

  // Identifiers admit full Unicode letters/digits, so the boundary a
  // keyword may not run into does too.
  static constexpr auto word_boundary = cr<"_"> | uc::L | uc::Nd;

  // -- Trivia ---- WS and TERMINATOR are one hidden class: `gap::line`
  // reads the raw gap bytes directly, so no separate newline token is
  // needed to tell ASI's boundary condition apart from plain spacing.
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // A statement ends at a line break, right before `}`, or at EOF --
  // V has no `;` terminator.
  static constexpr auto Eos = gap::line | &lit<"}"> | eof;

  struct ID : terminal<(uc::L | cr<"_">) + many(uc::L | uc::Nd | cr<"_">)> {};
  struct Identifier : rule<assign<&ast::Ident::name>(ref<ID>)> {};

  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto OctalDigit = cr<"0-7">;
  struct INT_LIT : terminal<(cr<"1-9"> + many(d))  //
                            | (lit<"0"> + cr<"xX"> + some(HexDigit))  //
                            | (lit<"0"> + many(OctalDigit))> {};
  struct IntegerLiteral : rule<assign<&ast::IntLit::text>(ref<INT_LIT>)> {};

  static constexpr auto Exponent = cr<"eE"> + option(cr<"+-">) + some(d);
  struct FLOAT_LIT
      : terminal<(some(d) + lit<"."> + option(some(d)) + option(Exponent))  //
                | (some(d) + Exponent)                                     //
                | (lit<"."> + some(d) + option(Exponent))> {};
  struct FloatingLiteral : rule<assign<&ast::FloatLit::text>(ref<FLOAT_LIT>)> {
  };

  // A rune literal and a string literal in one terminal: both are
  // single-quoted, and they match the same bytes at length one.
  static constexpr auto StrEscape =
      (lit<"\\u"> + repeat<4>(HexDigit))    //
      | (lit<"\\U"> + repeat<8>(HexDigit))  //
      | (lit<"\\x"> + repeat<2>(HexDigit))  //
      | (lit<"\\"> + repeat<3>(OctalDigit))  //
      | (lit<"\\"> + cr<R"(abfnrtv\\'")">);
  struct STRING_LIT
      : terminal<lit<"'"> + many(StrEscape | (!cr<R"('\n)"> + any)) + lit<"'">> {
  };
  struct StringLiteral : rule<assign<&ast::StringLit::text>(ref<STRING_LIT>)> {
  };

  // -- Reusable list tails, parameterised on the member they fill ----
  template <auto Ptr>
  static constexpr auto IdentifierListFor =
      append<Ptr>(ref<ID>) + many(lit<","> + append<Ptr>(ref<ID>));

  template <auto Ptr>
  static constexpr auto ExpressionListFor =
      append<Ptr>(ref<Expression>) + many(lit<","> + append<Ptr>(ref<Expression>));

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

  struct Param
      : rule<create<ast::Param>(option(IdentifierListFor<&ast::Param::names>) +
                                assign<&ast::Param::type>(ref<Type>))> {};

  struct MethodSpec
      : rule<create<ast::MethodSpec>(
                 assign<&ast::MethodSpec::name>(ref<ID>) +
                 ParametersFor<&ast::MethodSpec::params> +
                 option(!gap::line +
                        assign<&ast::MethodSpec::result>(ref<Result_>)))  //
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
      : rule<ref<ArrayType> | ref<FunctionType> | ref<InterfaceType> |
            ref<MapType>> {};

  struct Type
      : rule<ref<NamedType> | ref<QualifiedType> | ref<ArrayType> |
            ref<FunctionType> | ref<InterfaceType> | ref<MapType>> {};

  struct Result_
      : rule<create<ast::Result_>(
                 flag<&ast::Result_::isParams>(lit<"(">) +
                 option(append<&ast::Result_::params>(ref<Param>) +
                        many(lit<","> + append<&ast::Result_::params>(ref<Param>)) +
                        option(lit<",">)) +
                 lit<")">)  //
             | create<ast::Result_>(assign<&ast::Result_::type>(ref<Type>))> {};

  struct Signature
      : rule<create<ast::Signature>(
            ParametersFor<&ast::Signature::params> +
            option(!gap::line + assign<&ast::Signature::result>(ref<Result_>)))> {
  };

  // -- Expressions ----

  struct KeyedElement
      : rule<create<ast::KeyedElement>(
            option(assign<&ast::KeyedElement::key>(ref<Expression> |
                                                    ref<v::V::LiteralValue>) +
                   lit<":">) +
            assign<&ast::KeyedElement::value>(ref<Expression> |
                                              ref<v::V::LiteralValue>))> {};

  // `{...}`, untyped: elements separated by commas -- V's real struct/map
  // literals need the separator.
  struct LiteralValue
      : rule<lit<"{"> +
            option(append<&ast::LiteralValue::elements>(ref<KeyedElement>) +
                   many(lit<","> +
                        append<&ast::LiteralValue::elements>(ref<KeyedElement>)) +
                   option(lit<",">)) +
            lit<"}">> {};

  struct LiteralType
      : rule<ref<ArrayType> | ref<MapType> | ref<NamedType> | ref<QualifiedType>> {
  };

  struct CompositeLit
      : rule<create<ast::CompositeLit>(
            assign<&ast::CompositeLit::type>(ref<LiteralType>) +
            assign<&ast::CompositeLit::value>(ref<LiteralValue>))> {};

  struct ArrayLit
      : rule<create<ast::ArrayLit>(
            lit<"["> + ExpressionListFor<&ast::ArrayLit::elements> + lit<"]">)> {
  };

  struct FunctionLit
      : rule<create<ast::FunctionLit>(
            kw<"fn"> + assign<&ast::FunctionLit::signature>(ref<Signature>) +
            assign<&ast::FunctionLit::body>(ref<Block>))> {};

  struct ConversionExpr
      : rule<create<ast::ConversionExpr>(
            assign<&ast::ConversionExpr::type>(ref<TypeLit>) + lit<"("> +
            assign<&ast::ConversionExpr::value>(ref<Expression>) + lit<")">)> {};

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
                        ref<FloatingLiteral> | ref<StringLiteral> | ref<CompositeLit> |
                        ref<FunctionLit> | ref<ArrayLit> | ref<Identifier> |
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
      : rule<(ref<Operand> | ref<ConversionExpr>)+many(
            nest<ast::OrExpr, &ast::OrExpr::operand>(
                kw<"or"> + assign<&ast::OrExpr::handler>(ref<Block>)) |
            nest<ast::SelectorExpr, &ast::SelectorExpr::object>(
                lit<"."> + assign<&ast::SelectorExpr::field>(ref<ID>))  //
            | nest<ast::IndexExpr, &ast::IndexExpr::array>(
                  lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) +
                  lit<"]">)  //
            | nest<ast::CallExpr, &ast::CallExpr::callee>(Arguments))> {};

  static constexpr auto OpUnary =
      lit<"+"> | lit<"-"> | lit<"!"> | lit<"^"> | lit<"*"> | lit<"&">;
  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(OpUnary) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>))  //
             | ref<PrimaryExpr>> {};

  // Precedence tiers a Go-shaped language actually uses.
  static constexpr auto OpMul =
      lit<"<<"> | lit<">>"> | lit<"&^"> | lit<"&"> | lit<"*"> | lit<"/"> |
      lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-"> | lit<"|"> | lit<"^">;
  static constexpr auto OpRel =
      lit<"=="> | lit<"!="> | lit<"<="> | lit<">="> | lit<"<"> | lit<">">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;

  struct Expression
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpRel) >> left(OpLogAnd) >>
               left(OpLogOr))> {};

  // -- Statements ----

  struct ExprStmt
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>))> {
  };

  struct IncDecStmt
      : rule<create<ast::IncDecStmt>(
            assign<&ast::IncDecStmt::operand>(ref<Expression>) +
            assign<&ast::IncDecStmt::op>(lit<"++"> | lit<"--">))> {};

  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> |
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
            kw<"return"> + option(ExpressionListFor<&ast::ReturnStmt::values>))> {
  };
  struct BreakStmt : rule<create<ast::BreakStmt>(kw<"break">)> {};
  struct ContinueStmt : rule<create<ast::ContinueStmt>(kw<"continue">)> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            kw<"if"> +
            option(assign<&ast::IfStmt::init>(ref<SimpleStmt>) + lit<";">) +
            assign<&ast::IfStmt::cond>(ref<Expression>) +
            assign<&ast::IfStmt::thenBranch>(ref<Block>) +
            option(kw<"else"> + assign<&ast::IfStmt::elseBranch>(
                                     ref<IfStmt> | ref<Block>)))> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
                 kw<"case"> + ExpressionListFor<&ast::CaseClause::values> +
                 lit<":"> +
                 many(append<&ast::CaseClause::stmts>(ref<Statement>) + Eos))  //
             | create<ast::CaseClause>(
                   flag<&ast::CaseClause::isDefault>(kw<"default">) +
                   lit<":"> +
                   many(append<&ast::CaseClause::stmts>(ref<Statement>) +
                        Eos))> {};

  struct SwitchStmt
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> +
            option(assign<&ast::SwitchStmt::init>(ref<SimpleStmt>) + lit<";">) +
            option(assign<&ast::SwitchStmt::tag>(ref<Expression>)) + lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) + lit<"}">)> {
  };

  // The classic three-part form's init also admits a `:=` declaration:
  // the idiomatic `for i := 0; i < n; i++` needs it.
  struct ForStmt
      : rule<create<ast::ForStmt>(
                 kw<"for"> + ExpressionListFor<&ast::ForStmt::inTargets> +
                 kw<"in"> + assign<&ast::ForStmt::inIterable>(ref<ID>) +
                 assign<&ast::ForStmt::body>(ref<Block>))  //
             | create<ast::ForStmt>(
                   kw<"for"> +
                   option(assign<&ast::ForStmt::init>(ref<VarDecl> |
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

  struct ConstDecl
      : rule<create<ast::ConstDecl>(
            kw<"const"> + lit<"("> +
            many(append<&ast::ConstDecl::specs>(ref<ConstSpec>) + Eos) +
            lit<")">)> {};

  struct VarDecl
      : rule<create<ast::VarDecl>(
            option(flag<&ast::VarDecl::mut>(kw<"mut">)) +
            IdentifierListFor<&ast::VarDecl::names> + lit<":="> +
            ExpressionListFor<&ast::VarDecl::values>)> {};

  // A field (named, or anonymous -- an embedded type alone), or a `pub:` /
  // `mut:` / `pub mut:` visibility marker: `type == nullptr` tells the
  // marker forms apart from a real field.
  struct FieldDecl
      : rule<create<ast::FieldDecl>(
                 IdentifierListFor<&ast::FieldDecl::names> + !gap::line +
                 assign<&ast::FieldDecl::type>(ref<Type>) +
                 option(assign<&ast::FieldDecl::tag>(ref<STRING_LIT>)))  //
             | create<ast::FieldDecl>(
                   assign<&ast::FieldDecl::type>(ref<NamedType> |
                                                 ref<QualifiedType>) +
                   option(assign<&ast::FieldDecl::tag>(ref<STRING_LIT>)))  //
             | create<ast::FieldDecl>(
                   flag<&ast::FieldDecl::pub>(kw<"pub">) +
                   option(flag<&ast::FieldDecl::mut>(kw<"mut">) +
                          option(kw<"mut">)) +
                   lit<":">)  //
             | create<ast::FieldDecl>(flag<&ast::FieldDecl::mut>(kw<"mut">) +
                                      lit<":">)> {};

  struct StructDecl
      : rule<create<ast::StructDecl>(
            kw<"struct"> + assign<&ast::StructDecl::name>(ref<ID>) + lit<"{"> +
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
  struct Declaration
      : rule<ref<EnumDecl> | ref<ConstDecl> | ref<VarDecl> | ref<StructDecl>> {};

  // A method (receiver parenthesised right after `fn`) or a plain function
  // (name right after `fn`) -- disjoint on the token following `fn`.
  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
                 kw<"fn"> + ParametersFor<&ast::FunctionDecl::receiver> +
                 assign<&ast::FunctionDecl::name>(ref<ID>) +
                 assign<&ast::FunctionDecl::signature>(ref<Signature>) +
                 option(assign<&ast::FunctionDecl::body>(ref<Block>)))  //
             | create<ast::FunctionDecl>(
                   kw<"fn"> + assign<&ast::FunctionDecl::name>(ref<ID>) +
                   assign<&ast::FunctionDecl::signature>(ref<Signature>) +
                   option(assign<&ast::FunctionDecl::body>(ref<Block>)))> {};

  struct TopLevelDecl : rule<ref<Declaration> | ref<FunctionDecl>> {};

  struct Statement
      : rule<ref<Declaration> | ref<SimpleStmt> | ref<ReturnStmt> |
            ref<BreakStmt> | ref<ContinueStmt> | ref<Block> | ref<IfStmt> |
            ref<SwitchStmt> | ref<ForStmt>> {};

  struct Block
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::stmts>(ref<Statement>) + Eos) +
            lit<"}">)> {};

  struct ModuleClause
      : rule<create<ast::ModuleClause>(
            kw<"module"> + assign<&ast::ModuleClause::name>(ref<ID>))> {};

  struct ImportSpec
      : rule<create<ast::ImportSpec>(
            kw<"import"> +
            option(flag<&ast::ImportSpec::dot>(lit<".">) |
                   assign<&ast::ImportSpec::alias>(ref<ID>)) +
            assign<&ast::ImportSpec::path>(ref<ID> | ref<STRING_LIT>))> {};

  struct SourceFile
      : rule<create<ast::SourceFile>(
            option(assign<&ast::SourceFile::module>(ref<ModuleClause>) + Eos) +
            many(append<&ast::SourceFile::imports>(ref<ImportSpec>) + Eos) +
            many(append<&ast::SourceFile::decls>(ref<TopLevelDecl>) + Eos))> {
  };

  using entry = SourceFile;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace v
