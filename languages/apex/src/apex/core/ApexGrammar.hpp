// The apex language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "apex/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace apex {

struct Apex : cursiv::GrammarSpec {
  // Java letters: ASCII letters, `$`, `_`, and every scalar past ASCII --
  // one code-point class is enough since a lone surrogate cannot occur in
  // well-formed UTF-8.
  static constexpr auto JavaLetter = cr<"a-zA-Z$_"> | cr<R"(-􏿿)">;
  static constexpr auto JavaLetterOrDigit = cr<"a-zA-Z0-9$_"> | cr<R"(-􏿿)">;
  static constexpr auto word_boundary = JavaLetterOrDigit;

  // Legacy C-style array brackets (`Integer a[]`): a plain count, robust to
  // the whitespace/comments a rule's skipper allows between them.
  static constexpr auto CountBrackets = [](std::string_view t) -> std::uint8_t {
    std::uint8_t n = 0;
    for (const char c : t)
      if (c == '[') ++n;
    return n;
  };
  struct ExtraDims : rule<some(lit<"["> + lit<"]">), text_conv<CountBrackets>> {};

  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  // Covers the ApexDoc `/** ... */` shape too: a longer prefix of the same
  // run, not a rule of its own.
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<JavaLetter + many(JavaLetterOrDigit)> {};

  static constexpr auto Digits = d + option(many(d | lit<"_">) + d);
  static constexpr auto ExponentPart = cr<"eE"> + option(cr<"+-">) + Digits;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexDigits = HexDigit + option(many(HexDigit | lit<"_">) + HexDigit);

  struct DECIMAL_LITERAL
      : terminal<(lit<"0"> | (cr<"1-9"> + (option(Digits) | (some(lit<"_">) + Digits)))) +
                 option(cr<"lL">)> {};
  struct HEX_LITERAL
      : terminal<lit<"0"> + cr<"xX"> + HexDigit + option(many(HexDigit | lit<"_">) + HexDigit) +
                 option(cr<"lL">)> {};
  struct OCT_LITERAL
      : terminal<lit<"0"> + many(lit<"_">) + cr<"0-7"> +
                 option(many(cr<"0-7"> | lit<"_">) + cr<"0-7">) + option(cr<"lL">)> {};
  struct BINARY_LITERAL
      : terminal<lit<"0"> + cr<"bB"> + cr<"01"> + option(many(cr<"01"> | lit<"_">) + cr<"01">) +
                 option(cr<"lL">)> {};
  struct FLOAT_LITERAL
      : terminal<(((Digits + lit<"."> + option(Digits)) | (lit<"."> + Digits)) +
                  option(ExponentPart) + option(cr<"fFdD">)) |
                 (Digits + ((ExponentPart + option(cr<"fFdD">)) | cr<"fFdD">))> {};
  struct HEX_FLOAT_LITERAL
      : terminal<lit<"0"> + cr<"xX"> +
                 ((HexDigits + option(lit<".">)) | (option(HexDigits) + lit<"."> + HexDigits)) +
                 cr<"pP"> + option(cr<"+-">) + Digits + option(cr<"fFdD">)> {};
  // `true`/`false` are case-sensitive, unlike other keywords here:
  // `TRUE`/`True` lex as an identifier, not a boolean.
  struct BOOL_LITERAL : terminal<kw<"true"> | kw<"false">> {};

  static constexpr auto OctalDigit = cr<"0-7">;
  static constexpr auto EscapeSeq =
      lit<"\\"> + (cr<R"(btnfr"'\\)"> | (option(option(cr<"0-3">) + OctalDigit) + OctalDigit) |
                   (lit<"u"> + repeat<4>(HexDigit)));
  // Real Apex has no character type: a single-quoted literal is always a
  // string, `''` included (zero or more characters).
  struct STRING_LITERAL
      : terminal<lit<"'"> + many(EscapeSeq | (!cr<R"('\\)"> + any)) + lit<"'">> {};

  // SOQL query text, `[SELECT ...]`, one token never parsed inside. The
  // leading `SELECT` is what keeps it apart from every other `[`: without
  // it the token swallows an array type's `[]` and the rest of the line.
  struct SOQL_LITERAL
      : terminal<lit<"["> + many(s) + ikw<"select"> + many(cr<"^]">) +
                 lit<"]">> {};

  // A compound keyword has a canonical semantic spelling; source gaps
  // (including comments) are still present in its CST.
  struct SharingModifier
      : rule<(ikw<"with"> | ikw<"without"> | ikw<"inherited">)+ikw<"sharing">,
             cursiv::text_conv<[](std::string_view text) -> std::string_view {
               auto lower = [](char c) {
                 return c >= 'A' && c <= 'Z' ? char(c + 'a' - 'A') : c;
               };
               if (!text.empty() && lower(text[0]) == 'i')
                 return "inherited sharing";
               if (text.size() >= 7 && lower(text[4]) == 'o')
                 return "without sharing";
               return "with sharing";
             }>> {};

  static constexpr auto ModifierKw =
      ikw<"public"> | ikw<"protected"> | ikw<"private"> | ikw<"static"> | ikw<"abstract"> |
      ikw<"final"> | ikw<"global"> | ikw<"webservice"> | ikw<"override"> | ikw<"virtual"> |
      ikw<"testmethod"> | ikw<"native"> | ikw<"synchronized"> | ikw<"transient"> | ref<SharingModifier>;
  static constexpr auto VariableModifierKw = ikw<"final">;

  static constexpr auto PrimitiveKw =
      ikw<"char"> | ikw<"byte"> | ikw<"short"> | ikw<"int"> | ikw<"float">;

  // `<`/`>` guarded against `=` so a shift op built from two of them never
  // eats into `<=`/`>=`/`<<=`/`>>=`/`>>>=` -- those longer literals still
  // win the first token by length; the guard is a second line of defence.
  static constexpr auto Lt = lit<"<", !lit<"=">>;
  static constexpr auto Gt = lit<">", !lit<"=">>;

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | Gt | Lt;
  static constexpr auto OpEq = lit<"=="> | lit<"!="> | lit<"<>">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;
  static constexpr auto OpNullCoalesce = lit<"??">;
  static constexpr auto PrefixOp = lit<"++"> | lit<"--"> | lit<"+"> | lit<"-">;
  static constexpr auto NotOp = lit<"~"> | lit<"!">;
  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>>="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | lit<"=", !cr<"=">>;

  // A name that must spell `Set` to reach the generic collection type,
  // since `set`/`get` are reserved for property accessors: an explicit,
  // non-dotted `Set<...>` spelling avoids the clash.
  static constexpr auto MemberName =
      ref<ID> | ikw<"this"> | ikw<"get"> | ikw<"set"> | ikw<"insert"> | ikw<"upsert"> |
      ikw<"update"> | ikw<"delete"> | ikw<"undelete">;

  // -- forward declarations ----
  struct QualifiedName;
  struct AnnotationValue;
  struct AnnotationArg;
  struct AnnotationUse;
  struct TypeArg;
  struct TypeParameter;
  struct PrimitiveType;
  struct ClassOrInterfaceType;
  struct TypeNode;
  struct TypeOrVoid;
  struct CreatedName;
  struct Expression;
  struct AssignmentExpr;
  struct TernaryExpr;
  struct EqualityAndAbove;
  struct InstanceOfExpr;
  struct RelationalAndBelow;
  struct ShiftLevel;
  struct AdditiveLevel;
  struct CastExpr;
  struct UnaryExpr;
  struct PostfixExpr;
  struct Primary;
  struct Creator;
  struct ArrayInitializer;
  struct VariableInitializerExpr;
  struct VariableDeclarator;
  struct FormalParameter;
  struct LastFormalParameter;
  struct Block;
  struct BlockItem;
  struct LocalVariableDeclaration;
  struct Statement;
  struct ForEachStatement;
  struct TryStatement;
  struct CatchClause;
  struct Resource;
  struct MethodDecl;
  struct FieldDecl;
  struct ConstructorDecl;
  struct PropertyAccessor;
  struct PropertyDecl;
  struct ClassBodyDeclaration;
  struct ClassDecl;
  struct InterfaceDecl;
  struct EnumConstant;
  struct EnumDecl;
  struct AnnotationTypeDecl;
  struct TypeDeclaration;
  struct PackageDeclaration;
  struct ImportDeclaration;
  struct CompilationUnit;

  // -- names ----

  struct QualifiedName : rule<some(ref<ID>, lit<".">)> {};

  // -- annotations ----

  struct AnnotationValue
      : rule<create<ast::AnnotationArrayValue>(
                 lit<"{"> +
                 many(append<&ast::AnnotationArrayValue::items>(ref<AnnotationValue>), lit<",">) +
                 option(lit<",">) + lit<"}">) |
             ref<AnnotationUse> |
             create<ast::AnnotationExprValue>(assign<&ast::AnnotationExprValue::value>(ref<Expression>))> {
  };

  struct AnnotationArg
      : rule<create<ast::AnnotationArg>(
                 (assign<&ast::AnnotationArg::name>(ref<ID>) + lit<"="> +
                  assign<&ast::AnnotationArg::value>(ref<AnnotationValue>)) |
                 assign<&ast::AnnotationArg::value>(ref<AnnotationValue>))> {};

  // Annotation arguments may be joined by `,` or by nothing at all.
  struct AnnotationUse
      : rule<create<ast::Annotation>(
                 lit<"@"> + assign<&ast::Annotation::name>(ref<QualifiedName>) +
                 option(lit<"("> +
                        option(append<&ast::Annotation::args>(ref<AnnotationArg>) +
                               many(option(lit<",">) +
                                    append<&ast::Annotation::args>(ref<AnnotationArg>))) +
                        lit<")">))> {};

  // -- types ----

  struct PrimitiveType
      : rule<create<ast::PrimitiveTypeRef>(assign<&ast::PrimitiveTypeRef::keyword>(PrimitiveKw))> {
  };

  struct TypeArg
      : rule<create<ast::TypeArgType>(assign<&ast::TypeArgType::type>(ref<TypeNode>)) |
             create<ast::WildcardTypeArg>(
                 lit<"?"> +
                 option((ikw<"extends"> | flag<&ast::WildcardTypeArg::superBound>(ikw<"super">)) +
                        assign<&ast::WildcardTypeArg::bound>(ref<TypeNode>)))> {};

  // The diamond is one `<>` token, never `<` then `>`: the not-equal
  // operator shares that spelling and the lexer reads it whole.
  static constexpr auto TypeArgsBody =
      lit<"<>"> |
      (lit<"<"> + many(append<&ast::ClassTypeRef::typeArguments>(ref<TypeArg>), lit<",">) +
       lit<">">);

  // A dotted chain of `Identifier typeArguments?` segments, or `set<T>` on
  // its own: `set`/`get` are reserved for property accessors elsewhere, so
  // the generic collection type needs this one mandatory, non-dotted
  // carve-out.
  struct ClassOrInterfaceType
      : rule<(create<ast::ClassTypeRef>(assign<&ast::ClassTypeRef::name>(ref<ID>) +
                                         option(TypeArgsBody)) +
              many(nest<ast::ClassTypeRef, &ast::ClassTypeRef::qualifier>(
                  lit<"."> + assign<&ast::ClassTypeRef::name>(ref<ID>) + option(TypeArgsBody)))) |
             create<ast::ClassTypeRef>(assign<&ast::ClassTypeRef::name>(ikw<"set">) + TypeArgsBody)> {
  };

// `type_`, `TypeNode` below, split into several identical copies: a rule
// with a trailing bracket-pair repeat and no other content, referenced from
// this many unrelated positions, made the LR table generator merge two
// states that should stay apart and drop the shift that continues the
// loop -- `Integer[] x` then refused `[` outright. Splitting the callers
// across several named copies of the same body is the workaround; this
// macro shares their definition.
#define APEX_TYPE_NODE_BODY                                                    \
  (ref<ClassOrInterfaceType> | ref<PrimitiveType>) +many(                       \
      nest<ast::ArrayTypeRef, &ast::ArrayTypeRef::element>(lit<"["> +     \
                                                                  lit<"]">))

  struct TypeNode : rule<APEX_TYPE_NODE_BODY> {};

#undef APEX_TYPE_NODE_BODY

  // A creator's type name reuses `ClassOrInterfaceType` rather than a second,
  // near-identical dotted-chain parse (over-accepting wildcards in a
  // creator name, a deviation with no real price) -- two such near-twins
  // is what caused the LALR states to merge and corrupt the type's own
  // trailing `[]` in the first place.
  // The empty diamond `new Foo<>()` is a separate engine limitation:
  // adjacent `<>` lexes as the not-equal operator, so a creator needs
  // `new Foo<T>()` spelled out.
  struct CreatedName : rule<ref<ClassOrInterfaceType> | ref<PrimitiveType>> {};

  struct TypeParameter
      : rule<create<ast::TypeParam>(
                 assign<&ast::TypeParam::name>(ref<ID>) +
                 option(ikw<"extends"> + append<&ast::TypeParam::bounds>(ref<TypeNode>) +
                        many(lit<"&"> + append<&ast::TypeParam::bounds>(ref<TypeNode>))))> {};

  // A type, or `void` on its own: used for a `.class` literal's operand
  // (`String.class`, `void.class`) and a method's return type.
  struct TypeOrVoid : rule<ref<TypeNode> | create<ast::VoidTypeRef>(ikw<"void">)> {};

  // -- expressions ----

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) |
             create<ast::StringLit>(assign<&ast::StringLit::text>(ref<STRING_LITERAL>)) |
             create<ast::FloatLit>(
                 assign<&ast::FloatLit::text>(ref<FLOAT_LITERAL> | ref<HEX_FLOAT_LITERAL>)) |
             create<ast::IntLit>(assign<&ast::IntLit::text>(
                 ref<DECIMAL_LITERAL> | ref<HEX_LITERAL> | ref<OCT_LITERAL> |
                 ref<BINARY_LITERAL>)) |
             create<ast::BoolLit>(assign<&ast::BoolLit::text>(ref<BOOL_LITERAL>)) |
             create<ast::NullLit>(ikw<"null">) |
             create<ast::SoqlExpr>(assign<&ast::SoqlExpr::text>(ref<SOQL_LITERAL>)) |
             create<ast::ClassLit>(assign<&ast::ClassLit::type>(ref<TypeOrVoid>) + lit<"."> +
                                    ikw<"class">) |
             create<ast::ExplicitGenericInvokeExpr>(
                 lit<"<"> +
                 many(append<&ast::ExplicitGenericInvokeExpr::typeArguments>(ref<TypeArg>),
                      lit<",">) +
                 lit<">"> +
                 ((assign<&ast::ExplicitGenericInvokeExpr::target>(ikw<"super">) +
                   ((flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                     many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                          lit<",">) +
                     lit<")">) |
                    (lit<"."> +
                     assign<&ast::ExplicitGenericInvokeExpr::member>(ref<ID>) +
                     option(flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                            many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                                 lit<",">) +
                            lit<")">)))) |
                  (assign<&ast::ExplicitGenericInvokeExpr::target>(ref<ID>) +
                   flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                   many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>), lit<",">) +
                   lit<")">) |
                  (assign<&ast::ExplicitGenericInvokeExpr::target>(ikw<"this">) +
                   flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                   many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>), lit<",">) +
                   lit<")">))) |
             create<ast::ThisExpr>(ikw<"this">) | create<ast::SuperExpr>(ikw<"super">) |
             create<ast::Ident>(assign<&ast::Ident::name>(ref<ID>))> {};

  struct ArrayInitializer
      : rule<create<ast::ArrayInit>(
                 lit<"{"> +
                 many(append<&ast::ArrayInit::items>(ref<VariableInitializerExpr>), lit<",">) +
                 option(lit<",">) + lit<"}">)> {};

  struct Creator
      : rule<create<ast::NewObjectExpr>(
                 option(lit<"<"> +
                        many(append<&ast::NewObjectExpr::typeArguments>(ref<TypeArg>), lit<",">) +
                        lit<">">) +
                 assign<&ast::NewObjectExpr::type>(ref<CreatedName>) + lit<"("> +
                 many(append<&ast::NewObjectExpr::args>(ref<Expression>), lit<",">) + lit<")"> +
                 option(flag<&ast::NewObjectExpr::anonymousBody>(lit<"{">) +
                        many(append<&ast::NewObjectExpr::classBody>(ref<ClassBodyDeclaration>) |
                             lit<";">) +
                        lit<"}">)) |
             // The sized-dimension and initializer-only array-creation
             // forms merge into one shape (any mix of sized/unsized
             // brackets, an optional initializer) -- a superset that
             // over-accepts a mix real code never writes.
             create<ast::NewArrayExpr>(
                 assign<&ast::NewArrayExpr::elementType>(ref<CreatedName>) +
                 some(lit<"["> +
                      option(append<&ast::NewArrayExpr::dimExprs>(ref<Expression>)) + lit<"]">) +
                 option(assign<&ast::NewArrayExpr::init>(ref<ArrayInitializer>))) |
             // The map and set forms spell identically when empty (`T{}`);
             // merged into one node (see ast::NewCollectionExpr), so there
             // is only one alternative to offer here.
             create<ast::NewCollectionExpr>(
                 assign<&ast::NewCollectionExpr::type>(ref<CreatedName>) + lit<"{"> +
                 option(append<&ast::NewCollectionExpr::keys>(ref<Expression>) +
                        ((lit<"=>"> + append<&ast::NewCollectionExpr::values>(ref<Expression>) +
                          many(lit<","> +
                               append<&ast::NewCollectionExpr::keys>(ref<Expression>) +
                               lit<"=>"> +
                               append<&ast::NewCollectionExpr::values>(ref<Expression>))) |
                         many(lit<","> +
                              append<&ast::NewCollectionExpr::keys>(ref<Expression>)))) +
                 lit<"}">)> {};

  struct PostfixExpr
      : rule<(ref<Primary> | (ikw<"new"> + ref<Creator>)) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<Expression>), lit<",">) + lit<")">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      option(flag<&ast::MemberExpr::safe>(lit<"?">)) + lit<"."> +
                      assign<&ast::MemberExpr::field>(MemberName)) |
                  nest<ast::QualifiedNewExpr, &ast::QualifiedNewExpr::outer>(lit<"."> +
                                                                              ikw<"new">) |
                  nest<ast::QualifiedSuperExpr, &ast::QualifiedSuperExpr::outer>(
                      lit<"."> + ikw<"super"> +
                      ((flag<&ast::QualifiedSuperExpr::invoked>(lit<"(">) +
                        many(append<&ast::QualifiedSuperExpr::args>(ref<Expression>), lit<",">) +
                        lit<")">) |
                       (lit<"."> + assign<&ast::QualifiedSuperExpr::member>(ref<ID>) +
                        option(flag<&ast::QualifiedSuperExpr::invoked>(lit<"(">) +
                               many(append<&ast::QualifiedSuperExpr::args>(ref<Expression>),
                                    lit<",">) +
                               lit<")">)))) |
                  nest<ast::ExplicitGenericInvokeExpr, &ast::ExplicitGenericInvokeExpr::outer>(
                      lit<"."> + lit<"<"> +
                      many(append<&ast::ExplicitGenericInvokeExpr::typeArguments>(ref<TypeArg>),
                           lit<",">) +
                      lit<">"> +
                      ((assign<&ast::ExplicitGenericInvokeExpr::target>(ikw<"super">) +
                        ((flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                          many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                               lit<",">) +
                          lit<")">) |
                         (lit<"."> + assign<&ast::ExplicitGenericInvokeExpr::member>(ref<ID>) +
                          option(flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                                 many(append<&ast::ExplicitGenericInvokeExpr::args>(
                                          ref<Expression>),
                                      lit<",">) +
                                 lit<")">)))) |
                       (assign<&ast::ExplicitGenericInvokeExpr::target>(ref<ID>) +
                        flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                        many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                             lit<",">) +
                        lit<")">))) |
                  nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                      assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(lit<"("> + assign<&ast::CastExpr::type>(ref<TypeNode>) +
                                    lit<")"> +
                                    assign<&ast::CastExpr::operand>(ref<UnaryExpr>)) |
             ref<UnaryExpr>> {};

  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(PrefixOp) +
                                      assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(NotOp) +
                                      assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             ref<PostfixExpr>> {};

  struct AdditiveLevel
      : infix<CastExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd))> {};

  // `<<`/`>>`/`>>>` open on the same tokens as the relational operators and
  // generic brackets one level up, so they are hand-rolled from individual
  // `<`/`>` matches rather than sitting in the infix table -- the same
  // shape the csharp and unreal_angelscript ports use for their shift level.
  struct ShiftOp : rule<(Lt + Lt) | (Gt + Gt + Gt) | (Gt + Gt)> {};
  struct ShiftLevel
      : rule<ref<AdditiveLevel> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<ShiftOp>) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveLevel>)))> {};

  struct RelationalAndBelow
      : infix<ShiftLevel, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              left(OpRel)> {};

  struct InstanceOfExpr
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::value>(ref<RelationalAndBelow>) +
                 ikw<"instanceof"> + assign<&ast::InstanceOfExpr::type>(ref<TypeNode>)) |
             ref<RelationalAndBelow>> {};

  struct EqualityAndAbove
      : infix<InstanceOfExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >> left(OpBitOr) >>
               left(OpLogAnd) >> left(OpLogOr) >> left(OpNullCoalesce))> {};

  struct TernaryExpr
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::cond>(ref<EqualityAndAbove>) + lit<"?"> +
                 assign<&ast::TernaryExpr::thenValue>(ref<Expression>) + lit<":"> +
                 assign<&ast::TernaryExpr::elseValue>(ref<TernaryExpr>)) |
             ref<EqualityAndAbove>> {};

  struct AssignmentExpr
      : rule<create<ast::AssignExpr>(assign<&ast::AssignExpr::target>(ref<TernaryExpr>) +
                                      assign<&ast::AssignExpr::op>(AssignOp) +
                                      assign<&ast::AssignExpr::value>(ref<Expression>)) |
             ref<TernaryExpr>> {};

  struct Expression : rule<ref<AssignmentExpr>> {};

  // -- variable initializers, declarators, parameters ----

  struct VariableInitializerExpr
      : rule<create<ast::ExprInit>(assign<&ast::ExprInit::value>(ref<Expression>)) |
             ref<ArrayInitializer>> {};

  struct VariableDeclarator
      : rule<create<ast::VarDeclarator>(
                 assign<&ast::VarDeclarator::name>(ref<ID>) +
                 option(assign<&ast::VarDeclarator::extraDims>(ref<ExtraDims>)) +
                 option(lit<"="> +
                        assign<&ast::VarDeclarator::init>(ref<VariableInitializerExpr>)))> {};

  struct FormalParameter
      : rule<create<ast::Param>(
                 many(append<&ast::Param::modifiers>(VariableModifierKw) |
                      append<&ast::Param::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::Param::type>(ref<TypeNode>) +
                 assign<&ast::Param::name>(ref<ID>) +
                 option(assign<&ast::Param::extraDims>(ref<ExtraDims>)))> {};

  struct LastFormalParameter
      : rule<create<ast::Param>(
                 many(append<&ast::Param::modifiers>(VariableModifierKw) |
                      append<&ast::Param::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::Param::type>(ref<TypeNode>) +
                 flag<&ast::Param::varargs>(lit<"...">) +
                 assign<&ast::Param::name>(ref<ID>) +
                 option(assign<&ast::Param::extraDims>(ref<ExtraDims>)))> {};

  // -- statements ----

  struct Block
      : rule<create<ast::Block>(lit<"{"> + many(append<&ast::Block::items>(ref<BlockItem>)) +
                                 lit<"}">)> {};

  struct LocalVariableDeclaration
      : rule<create<ast::LocalVarDecl>(
                 many(append<&ast::LocalVarDecl::modifiers>(VariableModifierKw) |
                      append<&ast::LocalVarDecl::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::LocalVarDecl::type>(ref<TypeNode>) +
                 some(append<&ast::LocalVarDecl::declarators>(ref<VariableDeclarator>),
                      lit<",">))> {};

  struct BlockItem
      : rule<(ref<LocalVariableDeclaration> + lit<";">) | ref<ClassDecl> | ref<InterfaceDecl> |
             ref<AnnotationTypeDecl> | ref<EnumDecl> | ref<Statement>> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
                 ikw<"catch"> + lit<"("> +
                 many(append<&ast::CatchClause::modifiers>(VariableModifierKw) |
                      append<&ast::CatchClause::annotations>(ref<AnnotationUse>)) +
                 some(append<&ast::CatchClause::types>(ref<QualifiedName>), lit<"|">) +
                 assign<&ast::CatchClause::name>(ref<ID>) + lit<")"> +
                 assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct Resource
      : rule<create<ast::Resource>(
                 many(append<&ast::Resource::modifiers>(VariableModifierKw) |
                      append<&ast::Resource::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::Resource::type>(ref<ClassOrInterfaceType>) +
                 assign<&ast::Resource::name>(ref<ID>) +
                 option(assign<&ast::Resource::extraDims>(ref<ExtraDims>)) + lit<"="> +
                 assign<&ast::Resource::init>(ref<Expression>))> {};

  struct TryStatement
      : rule<create<ast::TryStmt>(
                 ikw<"try"> +
                 ((lit<"("> +
                   some(append<&ast::TryStmt::resources>(ref<Resource>), lit<";">) +
                   option(lit<";">) + lit<")"> + assign<&ast::TryStmt::body>(ref<Block>) +
                   many(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                   option(assign<&ast::TryStmt::finallyBlock>(ikw<"finally"> + ref<Block>))) |
                  (assign<&ast::TryStmt::body>(ref<Block>) +
                   ((some(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                     option(assign<&ast::TryStmt::finallyBlock>(ikw<"finally"> + ref<Block>))) |
                    assign<&ast::TryStmt::finallyBlock>(ikw<"finally"> + ref<Block>)))))> {};

  struct ForEachStatement
      : rule<create<ast::ForEachStmt>(
                 ikw<"for"> + lit<"("> +
                 many(append<&ast::ForEachStmt::modifiers>(VariableModifierKw) |
                      append<&ast::ForEachStmt::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::ForEachStmt::type>(ref<TypeNode>) +
                 assign<&ast::ForEachStmt::name>(ref<ID>) +
                 option(assign<&ast::ForEachStmt::extraDims>(ref<ExtraDims>)) + lit<":"> +
                 assign<&ast::ForEachStmt::iterable>(ref<Expression>) + lit<")"> +
                 assign<&ast::ForEachStmt::body>(ref<Statement>))> {};

  // `insert x;`/`update x;`/`delete x;`/`undelete x;`, or
  // `upsert x;`/`upsert x field;`: the second upsert operand is at most one
  // field expression, the only shape the language actually uses.
  struct ApexDbExpr
      : rule<create<ast::DmlExpr>(assign<&ast::DmlExpr::op>(ikw<"insert"> | ikw<"update"> |
                                                              ikw<"delete"> | ikw<"undelete">) +
                                   assign<&ast::DmlExpr::value>(ref<Expression>)) |
             create<ast::UpsertExpr>(ikw<"upsert"> +
                                      assign<&ast::UpsertExpr::value>(ref<Expression>) +
                                      option(assign<&ast::UpsertExpr::field>(ref<Expression>)))> {
  };

  struct Statement
      : rule<ref<Block> |
             create<ast::IfStmt>(
                 ikw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(commit(ikw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))) |
             ref<ForEachStatement> |
             create<ast::ForStmt>(
                 ikw<"for"> + lit<"("> +
                 option(assign<&ast::ForStmt::initDecl>(ref<LocalVariableDeclaration>) |
                        some(append<&ast::ForStmt::initExprs>(ref<Expression>), lit<",">)) +
                 lit<";"> + option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
                 option(some(append<&ast::ForStmt::update>(ref<Expression>), lit<",">)) +
                 lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)) |
             create<ast::WhileStmt>(ikw<"while"> + lit<"("> +
                                     assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
                                     assign<&ast::WhileStmt::body>(ref<Statement>)) |
             create<ast::DoStmt>(ikw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
                                  ikw<"while"> + lit<"("> +
                                  assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
                                  lit<";">) |
             create<ast::RunAsStmt>(ikw<"system.runas"> + lit<"("> +
                                     assign<&ast::RunAsStmt::value>(ref<Expression>) + lit<")"> +
                                     assign<&ast::RunAsStmt::body>(ref<Statement>)) |
             ref<TryStatement> |
             create<ast::ReturnStmt>(ikw<"return"> +
                                      option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                                      lit<";">) |
             create<ast::ThrowStmt>(ikw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expression>) +
                                     lit<";">) |
             create<ast::BreakStmt>(ikw<"break"> +
                                     option(assign<&ast::BreakStmt::label>(ref<ID>)) + lit<";">) |
             create<ast::ContinueStmt>(ikw<"continue"> +
                                        option(assign<&ast::ContinueStmt::label>(ref<ID>)) +
                                        lit<";">) |
             create<ast::ExprStmt>(lit<";">) |
             create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<ApexDbExpr>) + lit<";">) |
             create<ast::LabeledStmt>(assign<&ast::LabeledStmt::label>(ref<ID>) + lit<":"> +
                                       assign<&ast::LabeledStmt::body>(ref<Statement>)) |
             create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">)> {};

  // -- members ----

  struct FieldDecl
      : rule<create<ast::FieldDecl>(
                 many(append<&ast::FieldDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::FieldDecl::modifiers>(ModifierKw)) +
                 assign<&ast::FieldDecl::type>(ref<TypeNode>) +
                 some(append<&ast::FieldDecl::declarators>(ref<VariableDeclarator>), lit<",">) +
                 lit<";">)> {};

  // A getter/setter block, its modifier set the full one (annotations
  // included), not just `final`.
  struct PropertyAccessor
      : rule<create<ast::PropertyAccessor>(
                 many(append<&ast::PropertyAccessor::modifiers>(ModifierKw) |
                      append<&ast::PropertyAccessor::annotations>(ref<AnnotationUse>)) +
                 ((ikw<"get"> + (lit<";"> | assign<&ast::PropertyAccessor::body>(ref<Block>))) |
                  (flag<&ast::PropertyAccessor::isSetter>(ikw<"set">) +
                   (lit<";"> | assign<&ast::PropertyAccessor::body>(ref<Block>)))))> {};

  // One mandatory accessor, one optional second one (get/set, or set/get).
  struct PropertyDecl
      : rule<create<ast::PropertyDecl>(
                 many(append<&ast::PropertyDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::PropertyDecl::modifiers>(ModifierKw)) +
                 assign<&ast::PropertyDecl::type>(ref<TypeNode>) +
                 some(append<&ast::PropertyDecl::declarators>(ref<VariableDeclarator>), lit<",">) +
                 lit<"{"> + append<&ast::PropertyDecl::accessors>(ref<PropertyAccessor>) +
                 option(append<&ast::PropertyDecl::accessors>(ref<PropertyAccessor>)) +
                 lit<"}">)> {};

  struct MethodDecl
      : rule<create<ast::MethodDecl>(
                 many(append<&ast::MethodDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::MethodDecl::modifiers>(ModifierKw)) +
                 option(lit<"<"> +
                        some(append<&ast::MethodDecl::typeParams>(ref<TypeParameter>), lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodDecl::returnType>(ref<TypeOrVoid>) +
                 assign<&ast::MethodDecl::name>(ref<ID>) + lit<"("> +
                 option(append<&ast::MethodDecl::params>(ref<FormalParameter>) +
                        many(lit<","> + append<&ast::MethodDecl::params>(ref<FormalParameter>)) +
                        option(lit<","> +
                               append<&ast::MethodDecl::params>(ref<LastFormalParameter>)) |
                        append<&ast::MethodDecl::params>(ref<LastFormalParameter>)) +
                 lit<")"> +
                 option(ikw<"throws"> +
                        some(append<&ast::MethodDecl::throwsList>(ref<QualifiedName>), lit<",">)) +
                 (assign<&ast::MethodDecl::body>(ref<Block>) | lit<";">))> {};

  struct ConstructorDecl
      : rule<create<ast::ConstructorDecl>(
                 many(append<&ast::ConstructorDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::ConstructorDecl::modifiers>(ModifierKw)) +
                 option(lit<"<"> +
                        some(append<&ast::ConstructorDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::ConstructorDecl::name>(ref<ID>) + lit<"("> +
                 option(append<&ast::ConstructorDecl::params>(ref<FormalParameter>) +
                        many(lit<","> +
                             append<&ast::ConstructorDecl::params>(ref<FormalParameter>)) +
                        option(lit<","> +
                               append<&ast::ConstructorDecl::params>(ref<LastFormalParameter>)) |
                        append<&ast::ConstructorDecl::params>(ref<LastFormalParameter>)) +
                 lit<")"> +
                 option(ikw<"throws"> +
                        some(append<&ast::ConstructorDecl::throwsList>(ref<QualifiedName>),
                             lit<",">)) +
                 assign<&ast::ConstructorDecl::body>(ref<Block>))> {};

  // A member declaration inside a class, interface, enum, anonymous or
  // `@interface` body: every container accepts the same shapes, so one
  // rule serves all of them, as the java port does for the same reason.
  struct ClassBodyDeclaration
      : rule<create<ast::InitBlock>(flag<&ast::InitBlock::isStatic>(ikw<"static">) +
                                     assign<&ast::InitBlock::body>(ref<Block>)) |
             create<ast::InitBlock>(assign<&ast::InitBlock::body>(ref<Block>)) |
             ref<MethodDecl> | ref<FieldDecl> | ref<ConstructorDecl> | ref<PropertyDecl> |
             ref<InterfaceDecl> | ref<AnnotationTypeDecl> | ref<ClassDecl> | ref<EnumDecl>> {};

  // -- type declarations ----

  struct ClassDecl
      : rule<create<ast::ClassDecl>(
                 many(append<&ast::ClassDecl::modifiers>(ModifierKw) |
                      append<&ast::ClassDecl::annotations>(ref<AnnotationUse>)) +
                 ikw<"class"> + assign<&ast::ClassDecl::name>(ref<ID>) +
                 option(lit<"<"> +
                        some(append<&ast::ClassDecl::typeParams>(ref<TypeParameter>), lit<",">) +
                        lit<">">) +
                 option(ikw<"extends"> + assign<&ast::ClassDecl::superclass>(ref<TypeNode>)) +
                 option(ikw<"implements"> +
                        some(append<&ast::ClassDecl::interfaces>(ref<TypeNode>), lit<",">)) +
                 lit<"{"> +
                 many(append<&ast::ClassDecl::members>(ref<ClassBodyDeclaration>) | lit<";">) +
                 lit<"}">)> {};

  struct InterfaceDecl
      : rule<create<ast::InterfaceDecl>(
                 many(append<&ast::InterfaceDecl::modifiers>(ModifierKw) |
                      append<&ast::InterfaceDecl::annotations>(ref<AnnotationUse>)) +
                 ikw<"interface"> + assign<&ast::InterfaceDecl::name>(ref<ID>) +
                 option(lit<"<"> +
                        some(append<&ast::InterfaceDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 option(ikw<"extends"> +
                        some(append<&ast::InterfaceDecl::extendsList>(ref<TypeNode>), lit<",">)) +
                 lit<"{"> +
                 many(append<&ast::InterfaceDecl::members>(ref<ClassBodyDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  struct EnumConstant
      : rule<create<ast::EnumConstant>(
                 assign<&ast::EnumConstant::name>(ref<ID>) +
                 option(lit<"("> +
                        many(append<&ast::EnumConstant::args>(ref<Expression>), lit<",">) +
                        lit<")">) +
                 option(flag<&ast::EnumConstant::hasBody>(lit<"{">) +
                        many(append<&ast::EnumConstant::classBody>(ref<ClassBodyDeclaration>) |
                             lit<";">) +
                        lit<"}">))> {};

  struct EnumDecl
      : rule<create<ast::EnumDecl>(
                 many(append<&ast::EnumDecl::modifiers>(ModifierKw) |
                      append<&ast::EnumDecl::annotations>(ref<AnnotationUse>)) +
                 ikw<"enum"> + assign<&ast::EnumDecl::name>(ref<ID>) +
                 option(ikw<"implements"> +
                        some(append<&ast::EnumDecl::interfaces>(ref<TypeNode>), lit<",">)) +
                 lit<"{"> +
                 option(some(append<&ast::EnumDecl::constants>(ref<EnumConstant>), lit<",">)) +
                 option(lit<",">) +
                 option(lit<";"> +
                        many(append<&ast::EnumDecl::members>(ref<ClassBodyDeclaration>) |
                             lit<";">)) +
                 lit<"}">)> {};

  // `@interface` element declarations. Real Apex has no custom
  // annotations, but the grammar reaches this shape regardless.
  struct AnnotationTypeDecl
      : rule<create<ast::AnnotationTypeDecl>(
                 many(append<&ast::AnnotationTypeDecl::modifiers>(ModifierKw) |
                      append<&ast::AnnotationTypeDecl::annotations>(ref<AnnotationUse>)) +
                 lit<"@"> + ikw<"interface"> +
                 assign<&ast::AnnotationTypeDecl::name>(ref<ID>) + lit<"{"> +
                 many(append<&ast::AnnotationTypeDecl::members>(
                          create<ast::AnnotationTypeElement>(
                              many(append<&ast::AnnotationTypeElement::modifiers>(ModifierKw)) +
                              assign<&ast::AnnotationTypeElement::type>(ref<TypeNode>) +
                              assign<&ast::AnnotationTypeElement::name>(ref<ID>) + lit<"("> +
                              lit<")"> +
                              option(ikw<"default"> +
                                     assign<&ast::AnnotationTypeElement::defaultValue>(
                                         ref<AnnotationValue>)) +
                              lit<";">)) |
                      append<&ast::AnnotationTypeDecl::members>(ref<ClassBodyDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  struct TypeDeclaration
      : rule<ref<ClassDecl> | ref<EnumDecl> | ref<InterfaceDecl> | ref<AnnotationTypeDecl>> {};

  // -- top level ----

  struct PackageDeclaration
      : rule<create<ast::PackageDecl>(
                 many(append<&ast::PackageDecl::annotations>(ref<AnnotationUse>)) +
                 ikw<"package"> + assign<&ast::PackageDecl::name>(ref<QualifiedName>) + lit<";">)> {
  };

  struct ImportDeclaration
      : rule<create<ast::ImportDecl>(
                 ikw<"import"> +
                 option(flag<&ast::ImportDecl::isStatic>(ikw<"static">)) +
                 assign<&ast::ImportDecl::qualifiedName>(ref<QualifiedName>) +
                 option(lit<"."> + flag<&ast::ImportDecl::onDemand>(lit<"*">)) + lit<";">)> {
  };

  struct CompilationUnit
      : rule<create<ast::CompilationUnit>(
                 option(assign<&ast::CompilationUnit::package_>(ref<PackageDeclaration>)) +
                 many(append<&ast::CompilationUnit::imports>(ref<ImportDeclaration>)) +
                 many(append<&ast::CompilationUnit::types>(ref<TypeDeclaration>) | lit<";">))> {};

  using entry = CompilationUnit;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, COMMENT>;
};

}  // namespace apex
