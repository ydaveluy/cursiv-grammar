// The java language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "java/core/ast.hpp"

namespace java {

struct Java : cursiv::GrammarSpec {
  // Character.isJavaIdentifierStart/Part categories (JLS 3.8).
  static constexpr auto JavaLetter = uc::L | uc::Nl | uc::Sc | uc::Pc;
  static constexpr auto JavaLetterOrDigit =
      JavaLetter | uc::Mn | uc::Mc | uc::Nd | uc::Cf |
      cr<R"(\x00-\x08\x0e-\x1b\x7f-\x9f)">;
  static constexpr auto word_boundary = JavaLetterOrDigit;
  static constexpr auto ReservedKeyword =
      kw<"abstract"> | kw<"assert"> | kw<"boolean"> | kw<"break"> | kw<"byte"> |
      kw<"case"> | kw<"catch"> | kw<"char"> | kw<"class"> | kw<"const"> |
      kw<"continue"> | kw<"default"> | kw<"do"> | kw<"double"> | kw<"else"> |
      kw<"enum"> | kw<"extends"> | kw<"final"> | kw<"finally"> | kw<"float"> |
      kw<"for"> | kw<"goto"> | kw<"if"> | kw<"implements"> | kw<"import"> |
      kw<"instanceof"> | kw<"int"> | kw<"interface"> | kw<"long"> | kw<"native"> |
      kw<"new"> | kw<"package"> | kw<"private"> | kw<"protected"> | kw<"public"> |
      kw<"return"> | kw<"short"> | kw<"static"> | kw<"strictfp"> | kw<"super"> |
      kw<"switch"> | kw<"synchronized"> | kw<"this"> | kw<"throw"> | kw<"throws"> |
      kw<"transient"> | kw<"try"> | kw<"void"> | kw<"volatile"> | kw<"while"> |
      kw<"_"> | kw<"true"> | kw<"false"> | kw<"null">;

  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  struct ID : terminal<!ReservedKeyword + JavaLetter + many(JavaLetterOrDigit)> {};

  static constexpr auto Digits = d + option(many(d | lit<"_">) + d);
  static constexpr auto ExponentPart = cr<"eE"> + option(cr<"+-">) + Digits;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexDigits =
      HexDigit + option(many(HexDigit | lit<"_">) + HexDigit);
  // Unicode escapes are translated by the language's translator
  // (`translate_unicode_escapes`) before lexing. An isolated UTF-16 surrogate keeps a lossless escaped spelling,
  // because the engine reads UTF-8 scalars. No other Unicode escape is lexical.
  static constexpr auto EscapeSeq =
      lit<"\\"> + (cr<R"(bstnfr"'\\)"> |
                   (option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">) |
                   (lit<"u"> + cr<"dD"> + cr<"89a-fA-F"> + repeat<2>(HexDigit)));

  struct DECIMAL_LITERAL
      : terminal<(lit<"0"> |
                  (cr<"1-9"> + (option(Digits) | (some(lit<"_">) + Digits)))) +
                 option(cr<"lL">)> {};
  struct HEX_LITERAL : terminal<lit<"0"> + cr<"xX"> + HexDigit +
                                option(many(HexDigit | lit<"_">) + HexDigit) +
                                option(cr<"lL">)> {};
  struct OCT_LITERAL : terminal<lit<"0"> + many(lit<"_">) + cr<"0-7"> +
                                option(many(cr<"0-7"> | lit<"_">) + cr<"0-7">) +
                                option(cr<"lL">)> {};
  struct BINARY_LITERAL
      : terminal<lit<"0"> + cr<"bB"> + cr<"01"> +
                 option(many(cr<"01"> | lit<"_">) + cr<"01">) +
                 option(cr<"lL">)> {};
  struct FLOAT_LITERAL
      : terminal<(((Digits + lit<"."> + option(Digits)) | (lit<"."> + Digits)) +
                  option(ExponentPart) + option(cr<"fFdD">)) |
                 (Digits +
                  ((ExponentPart + option(cr<"fFdD">)) | cr<"fFdD">))> {};
  struct HEX_FLOAT_LITERAL
      : terminal<lit<"0"> + cr<"xX"> +
                 ((HexDigits + option(lit<".">)) |
                  (option(HexDigits) + lit<"."> + HexDigits)) +
                 cr<"pP"> + option(cr<"+-">) + Digits + option(cr<"fFdD">)> {};
  struct BOOL_LITERAL : terminal<kw<"true"> | kw<"false">> {};
  struct CHAR_LITERAL
      : terminal<lit<"'"> +
                 (EscapeSeq | cr<R"(\x00-\x09\x0b-\x0c\x0e-\x26\x28-\x5b\x5d-\uffff)">) +
                 lit<"'">> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(EscapeSeq | (!cr<R"("\\\r\n)"> + any)) +
                 lit<"\"">> {};
  struct TEXT_BLOCK
      : terminal<lit<"\"\"\""> + many(cr<R"( \t\f)">) + eol +
                 many(EscapeSeq | (lit<"\\"> + eol) |
                      (!lit<"\"\"\""> + cr<R"(^\\)">)) +
                 lit<"\"\"\"">> {};

  static constexpr auto ModifierKw =
      kw<"public"> | kw<"protected"> | kw<"private"> | kw<"static"> |
      kw<"abstract"> | kw<"final"> | kw<"strictfp"> | kw<"sealed"> |
      kw<"non-sealed"> | kw<"native"> | kw<"synchronized"> | kw<"transient"> |
      kw<"volatile"> | kw<"default">;

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | lit<"%">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-">;
  static constexpr auto OpShift = lit<"<<"> | lit<">>>"> | lit<">>">;
  static constexpr auto OpRel = lit<"<="> | lit<">="> | lit<">"> | lit<"<">;
  static constexpr auto OpEq = lit<"=="> | lit<"!=">;
  static constexpr auto OpBitAnd = lit<"&">;
  static constexpr auto OpBitXor = lit<"^">;
  static constexpr auto OpBitOr = lit<"|">;
  static constexpr auto OpLogAnd = lit<"&&">;
  static constexpr auto OpLogOr = lit<"||">;
  static constexpr auto PrefixOp =
      lit<"++"> | lit<"--"> | lit<"+"> | lit<"-"> | lit<"~"> | lit<"!">;
  static constexpr auto AssignOp = lit<"<<="> | lit<">>>="> | lit<">>="> |
                                   lit<"+="> | lit<"-="> | lit<"*="> |
                                   lit<"/="> | lit<"%="> | lit<"&="> |
                                   lit<"^="> | lit<"|="> | lit<"=", !cr<"=">>;

  // -- forward declarations ----
  struct Identifier;
  struct TypeIdentifier;
  struct QualifiedName;
  struct AnnotationUse;
  struct AnnotationValue;
  struct AnnotationArgument;
  struct TypeArgument;
  struct TypeParameter;
  struct PrimitiveType;
  struct ClassType;
  struct TypeType;
  struct TypeTypeOrVoid;
  struct CreatedName;
  struct Pattern;
  struct Expression;
  struct ExplicitGenericInvocation;
  struct LambdaExpr;
  struct AssignmentExpr;
  struct TernaryExpr;
  struct EqualityAndAbove;
  struct InstanceOfExpr;
  struct RelationalAndBelow;
  struct CastExpr;
  struct UnaryExpr;
  struct PostfixExpr;
  struct Primary;
  struct IntegerLiteral;
  struct FloatLiteral;
  struct CharLiteral;
  struct StringLiteral;
  struct TextBlockLiteral;
  struct BooleanLiteral;
  struct Creator;
  struct TypedLambdaParam;
  struct BareLambdaParam;
  struct VariableInitializerExpr;
  struct ArrayInitializer;
  struct VariableDeclarator;
  struct VariableDeclaratorInit;
  struct FormalParameter;
  struct ReceiverParameter;
  struct Block;
  struct BlockItem;
  struct LocalVariableDeclaration;
  struct LocalTypeDeclaration;
  struct Statement;
  struct ForEachStatement;
  struct TryStatement;
  struct CatchClause;
  struct Resource;
  struct CaseOrDefaultLabel;
  struct SwitchGroup;
  struct SwitchOutcome;
  struct SwitchLabeled;
  struct SwitchExpression;
  struct SwitchExprStatement;
  struct FieldDeclaration;
  struct MethodDeclaration;
  struct ConstructorDeclaration;
  struct CompactConstructorDeclaration;
  struct ClassBodyDeclaration;
  struct RecordBodyDeclaration;
  struct ClassDeclaration;
  struct InterfaceDeclaration;
  struct EnumConstant;
  struct EnumDeclaration;
  struct RecordComponent;
  struct RecordDeclaration;
  struct AnnotationTypeElementDeclaration;
  struct AnnotationTypeDeclaration;
  struct TypeDeclaration;
  struct PackageDeclaration;
  struct ImportDeclaration;
  struct ModuleDirective;
  struct ModuleDeclaration;
  struct OrdinaryCompilationUnit;
  struct ModularCompilationUnit;
  struct CompilationUnit;

  // -- names ----

  struct Identifier
      : rule<ref<ID> | kw<"module"> | kw<"open"> | kw<"requires"> |
             kw<"exports"> | kw<"opens"> | kw<"to"> | kw<"uses"> |
             kw<"provides"> | kw<"when"> | kw<"with"> | kw<"transitive"> |
             kw<"yield"> | kw<"sealed"> | kw<"permits"> | kw<"record"> |
             kw<"var">> {};

  struct TypeIdentifier
      : rule<!(kw<"permits"> | kw<"record"> | kw<"sealed"> | kw<"var"> | kw<"yield">) +
             (ref<ID> | kw<"module"> | kw<"open"> | kw<"requires"> |
             kw<"exports"> | kw<"opens"> | kw<"to"> | kw<"uses"> |
             kw<"provides"> | kw<"when"> | kw<"with"> | kw<"transitive">)> {};

  struct QualifiedName : rule<some(ref<Identifier>, lit<".">)> {};
  // Package-name segments may use a contextual type-restricted word.
  struct ClassTypeIdentifier : rule<ref<TypeIdentifier> |
      (ref<Identifier> + &lit<".">)> {};

  // -- annotations ----

  struct AnnotationValue
      : rule<create<ast::AnnotationArrayValue>(
                 lit<"{"> +
                 many(append<&ast::AnnotationArrayValue::items>(
                          ref<AnnotationValue>),
                      lit<",">) +
                 option(lit<",">) + lit<"}">) |
             ref<AnnotationUse> |
             create<ast::AnnotationExprValue>(
                 assign<&ast::AnnotationExprValue::value>(ref<Expression>))> {};

  struct AnnotationArgument
      : rule<create<ast::AnnotationArg>(
            (assign<&ast::AnnotationArg::name>(ref<Identifier>) + lit<"="> +
             assign<&ast::AnnotationArg::value>(ref<AnnotationValue>)) |
            assign<&ast::AnnotationArg::value>(ref<AnnotationValue>))> {};

  struct AnnotationUse
      : rule<create<ast::Annotation>(
            lit<"@"> + assign<&ast::Annotation::name>(ref<QualifiedName>) +
            option(lit<"("> +
                   many(append<&ast::Annotation::args>(ref<AnnotationArgument>),
                        lit<",">) +
                   lit<")">))> {};

  struct EmptyArrayDimension
      : rule<create<ast::ArrayDimension>(
            many(append<&ast::ArrayDimension::annotations>(ref<AnnotationUse>)) +
            lit<"["> + lit<"]">)> {};

  // -- types ----

  static constexpr auto PrimitiveKw = kw<"boolean"> | kw<"char"> | kw<"byte"> |
                                      kw<"short"> | kw<"int"> | kw<"long"> |
                                      kw<"float"> | kw<"double">;

  struct PrimitiveType
      : rule<create<ast::PrimitiveTypeRef>(
            many(append<&ast::PrimitiveTypeRef::annotations>(
                ref<AnnotationUse>)) +
            assign<&ast::PrimitiveTypeRef::keyword>(PrimitiveKw))> {};

  static constexpr auto ClassTypeArgsBody =
      lit<"<"> +
      many(append<&ast::ClassTypeRef::typeArguments>(ref<TypeArgument>),
           lit<",">) +
      lit<">">;

  struct ClassType
      : rule<create<ast::ClassTypeRef>(
                 many(append<&ast::ClassTypeRef::annotations>(
                     ref<AnnotationUse>)) +
                 assign<&ast::ClassTypeRef::name>(ref<ClassTypeIdentifier>) +
                 option(ClassTypeArgsBody)) +
             many(nest<ast::ClassTypeRef, &ast::ClassTypeRef::qualifier>(
                 lit<"."> +
                 many(append<&ast::ClassTypeRef::annotations>(
                     ref<AnnotationUse>)) +
                 assign<&ast::ClassTypeRef::name>(ref<ClassTypeIdentifier>) +
                 option(ClassTypeArgsBody)))> {};

  struct TypeType : rule<(ref<ClassType> | ref<PrimitiveType>)+many(
                        nest<ast::ArrayTypeRef, &ast::ArrayTypeRef::element>(
                            many(append<&ast::ArrayTypeRef::annotations>(
                                ref<AnnotationUse>)) +
                            lit<"["> + lit<"]">))> {};

  struct TypeTypeOrVoid
      : rule<ref<TypeType> | create<ast::VoidTypeRef>(kw<"void">)> {};

  struct CreatedName : rule<ref<ClassType> | ref<PrimitiveType>> {};

  struct TypeArgument
      : rule<create<ast::TypeArgType>(
                 assign<&ast::TypeArgType::type>(ref<TypeType>)) |
             create<ast::WildcardTypeArg>(
                 many(append<&ast::WildcardTypeArg::annotations>(
                     ref<AnnotationUse>)) +
                 lit<"?"> +
                 option((kw<"extends"> |
                         flag<&ast::WildcardTypeArg::superBound>(
                             kw<"super">)) +
                        assign<&ast::WildcardTypeArg::bound>(ref<TypeType>)))> {
  };

  struct TypeParameter
      : rule<create<ast::TypeParam>(
            many(append<&ast::TypeParam::annotations>(ref<AnnotationUse>)) +
            assign<&ast::TypeParam::name>(ref<TypeIdentifier>) +
            option(kw<"extends"> +
                   some(append<&ast::TypeParam::bounds>(ref<TypeType>), lit<"&">)))> {};

  // -- patterns (instanceof, switch) ----
  struct PatternBinding : rule<assign<&ast::VarDeclarator::name>(ref<Identifier>)> {};

  struct Pattern
      : rule<create<ast::RecordPattern>(
                 assign<&ast::RecordPattern::type>(ref<TypeType>) + lit<"("> +
                 many(append<&ast::RecordPattern::components>(ref<Pattern>),
                      lit<",">) +
                 lit<")">) |
             create<ast::TypePattern>(
                 many(append<&ast::TypePattern::modifiers>(kw<"final">) |
                      append<&ast::TypePattern::annotations>(
                          ref<AnnotationUse>)) +
                 (flag<&ast::TypePattern::isVar>(kw<"var">) |
                  assign<&ast::TypePattern::type>(ref<TypeType>)) +
                 many(append<&ast::TypePattern::annotations>(
                     ref<AnnotationUse>)) +
                 append<&ast::TypePattern::declarators>(ref<PatternBinding>))> {};

  // -- literals ----

  struct IntegerLiteral : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(
                              ref<DECIMAL_LITERAL> | ref<HEX_LITERAL> |
                              ref<OCT_LITERAL> | ref<BINARY_LITERAL>))> {};
  struct FloatLiteral : rule<create<ast::FloatLit>(assign<&ast::FloatLit::text>(
                            ref<FLOAT_LITERAL> | ref<HEX_FLOAT_LITERAL>))> {};
  struct CharLiteral : rule<assign<&ast::CharLit::text>(ref<CHAR_LITERAL>)> {};
  struct StringLiteral
      : rule<assign<&ast::StringLit::text>(ref<STRING_LITERAL>)> {};
  struct TextBlockLiteral
      : rule<assign<&ast::TextBlockLit::text>(ref<TEXT_BLOCK>)> {};
  struct BooleanLiteral : rule<assign<&ast::BoolLit::text>(ref<BOOL_LITERAL>)> {
  };

  // -- expressions ----

  struct ExplicitGenericInvocation
      : rule<create<ast::ExplicitGenericInvokeExpr>(
            lit<"<"> +
            many(append<&ast::ExplicitGenericInvokeExpr::typeArguments>(
                     ref<TypeArgument>),
                 lit<",">) +
            lit<">"> +
            ((assign<&ast::ExplicitGenericInvokeExpr::target>(kw<"super">) +
              ((flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                many(append<&ast::ExplicitGenericInvokeExpr::args>(
                         ref<Expression>),
                     lit<",">) +
                lit<")">) |
               (lit<"."> +
                option(lit<"<"> +
                       many(append<&ast::ExplicitGenericInvokeExpr::
                                       memberTypeArguments>(ref<TypeArgument>),
                            lit<",">) +
                       lit<">">) +
                assign<&ast::ExplicitGenericInvokeExpr::qualifiedMember>(
                    ref<Identifier>) +
                option(flag<&ast::ExplicitGenericInvokeExpr::invoked>(
                           lit<"(">) +
                       many(append<&ast::ExplicitGenericInvokeExpr::args>(
                                ref<Expression>),
                            lit<",">) +
                       lit<")">)))) |
             (assign<&ast::ExplicitGenericInvokeExpr::target>(kw<"this"> |
                                                              ref<Identifier>) +
              flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
              many(append<&ast::ExplicitGenericInvokeExpr::args>(
                       ref<Expression>),
                   lit<",">) +
              lit<")">)))> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<SwitchExpression> |
             ref<TextBlockLiteral> | ref<FloatLiteral> | ref<IntegerLiteral> |
             ref<CharLiteral> | ref<StringLiteral> | ref<BooleanLiteral> |
             create<ast::NullLit>(kw<"null">) |
             create<ast::ClassLit>(
                 assign<&ast::ClassLit::type>(ref<TypeTypeOrVoid>) + lit<"."> +
                 kw<"class">) |
             ref<ExplicitGenericInvocation> |
             create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(
                     create<ast::ThisExpr>(kw<"this">) |
                     create<ast::SuperExpr>(kw<"super">) |
                     create<ast::Ident>(
                         assign<&ast::Ident::name>(ref<Identifier>))) +
                 lit<"("> +
                 many(append<&ast::CallExpr::args>(ref<Expression>), lit<",">) +
                 lit<")">) |
             create<ast::ThisExpr>(kw<"this">) |
             create<ast::SuperExpr>(kw<"super">) |
             create<ast::Ident>(assign<&ast::Ident::name>(ref<Identifier>)) |
             create<ast::MethodRefExpr>(
                 assign<&ast::MethodRefExpr::typeTarget>(ref<TypeType>) +
                 lit<"::"> +
                 option(lit<"<"> +
                        many(append<&ast::MethodRefExpr::typeArguments>(
                                 ref<TypeArgument>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodRefExpr::name>(ref<Identifier> |
                                                   kw<"new">))> {};

  struct PostfixExpr
      : rule<(ref<Primary> | (kw<"new"> + ref<Creator>)) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> +
                      assign<&ast::IndexExpr::index>(ref<Expression>) +
                      lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<Expression>),
                           lit<",">) +
                      lit<")">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> +
                      option(lit<"<"> +
                             many(append<&ast::MemberExpr::typeArguments>(
                                      ref<TypeArgument>),
                                  lit<",">) +
                             lit<">">) +
                      assign<&ast::MemberExpr::field>(
                          ref<Identifier> | kw<"this"> | kw<"super">)) |
                  nest<ast::NewObjectExpr, &ast::NewObjectExpr::outer>(
                      lit<"."> + kw<"new"> +
                      option(lit<"<"> +
                             many(append<&ast::NewObjectExpr::typeArguments>(
                                      ref<TypeArgument>),
                                  lit<",">) +
                             lit<">">) +
                      assign<&ast::NewObjectExpr::type>(ref<CreatedName>) +
                      lit<"("> +
                      many(append<&ast::NewObjectExpr::args>(ref<Expression>),
                           lit<",">) +
                      lit<")"> +
                      option(flag<&ast::NewObjectExpr::anonymousBody>(
                                 lit<"{">) +
                             many(append<&ast::NewObjectExpr::classBody>(
                                      ref<ClassBodyDeclaration>) |
                                  lit<";">) +
                             lit<"}">)) |
                  nest<ast::MethodRefExpr, &ast::MethodRefExpr::target>(
                      lit<"::"> +
                      option(lit<"<"> +
                             many(append<&ast::MethodRefExpr::typeArguments>(
                                      ref<TypeArgument>),
                                  lit<",">) +
                             lit<">">) +
                      assign<&ast::MethodRefExpr::name>(ref<Identifier> |
                                                        kw<"new">)) |
                  nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                      assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {
  };

  struct Creator
      : rule<
            create<ast::NewObjectExpr>(
                option(lit<"<"> +
                       many(append<&ast::NewObjectExpr::typeArguments>(
                                ref<TypeArgument>),
                            lit<",">) +
                       lit<">">) +
                assign<&ast::NewObjectExpr::type>(ref<CreatedName>) + lit<"("> +
                many(append<&ast::NewObjectExpr::args>(ref<Expression>),
                     lit<",">) +
                lit<")"> +
                option(flag<&ast::NewObjectExpr::anonymousBody>(lit<"{">) +
                       many(append<&ast::NewObjectExpr::classBody>(
                                ref<ClassBodyDeclaration>) |
                            lit<";">) +
                       lit<"}">)) |
            create<ast::NewArrayExpr>(
                assign<&ast::NewArrayExpr::elementType>(ref<CreatedName>) +
                some(append<&ast::NewArrayExpr::dimensions>(
                    create<ast::ArrayDimension>(
                        many(append<&ast::ArrayDimension::annotations>(
                            ref<AnnotationUse>)) +
                        lit<"["> +
                        option(assign<&ast::ArrayDimension::size>(
                            ref<Expression>)) +
                        lit<"]">))) +
                option(
                    assign<&ast::NewArrayExpr::init>(ref<ArrayInitializer>)))> {
  };

  struct CastExpr
      : rule<create<ast::CastExpr>(
                 lit<"("> + assign<&ast::CastExpr::type>(ref<TypeType>) +
                 many(lit<"&"> + append<&ast::CastExpr::intersectionTypes>(
                                     ref<TypeType>)) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<LambdaExpr> |
                                                 ref<UnaryExpr>)) |
             ref<UnaryExpr>> {};

  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(PrefixOp) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             ref<PostfixExpr>> {};

  struct RelationalAndBelow
      : infix<CastExpr, ast::BinaryExpr,
              levels(left(OpMul), left(OpAdd), left(OpShift), left(OpRel))> {};

  struct InstanceOfExpr
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::value>(ref<RelationalAndBelow>) +
                 kw<"instanceof"> +
                 (assign<&ast::InstanceOfExpr::pattern>(ref<Pattern>) |
                  assign<&ast::InstanceOfExpr::type>(ref<TypeType>))) |
             ref<RelationalAndBelow>> {};

  struct EqualityAndAbove
      : infix<InstanceOfExpr, ast::BinaryExpr,
              levels(left(OpEq), left(OpBitAnd), left(OpBitXor), left(OpBitOr),
                     left(OpLogAnd), left(OpLogOr))> {};

  struct TernaryExpr
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::cond>(ref<EqualityAndAbove>) +
                 lit<"?"> +
                 assign<&ast::TernaryExpr::thenValue>(ref<Expression>) +
                 lit<":"> +
                 assign<&ast::TernaryExpr::elseValue>(ref<TernaryExpr>)) |
             ref<EqualityAndAbove>> {};

  struct AssignmentExpr
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<UnaryExpr>) +
                 assign<&ast::AssignExpr::op>(AssignOp) +
                 assign<&ast::AssignExpr::value>(ref<Expression>)) |
             ref<TernaryExpr>> {};

  struct TypedLambdaParam
      : rule<create<ast::LambdaParam>(
            many(append<&ast::LambdaParam::modifiers>(kw<"final">) |
                 append<&ast::LambdaParam::annotations>(ref<AnnotationUse>)) +
            ((flag<&ast::LambdaParam::isVar>(kw<"var">) +
              assign<&ast::LambdaParam::name>(ref<Identifier>)) |
             (assign<&ast::LambdaParam::type>(ref<TypeType>) +
              option(many(append<&ast::LambdaParam::varargsAnnotations>(ref<AnnotationUse>)) +
                     flag<&ast::LambdaParam::varargs>(lit<"...">)) +
              assign<&ast::LambdaParam::name>(ref<Identifier>) +
              many(append<&ast::LambdaParam::extraDims>(ref<EmptyArrayDimension>)))))> {};
  struct BareLambdaParam
      : rule<assign<&ast::LambdaParam::name>(ref<Identifier>)> {};

  struct LambdaExpr
      : rule<create<ast::LambdaExpr>(
            ((lit<"("> +
              many(append<&ast::LambdaExpr::params>(ref<TypedLambdaParam>),
                   lit<",">) +
              lit<")">) |
             (lit<"("> +
              some(append<&ast::LambdaExpr::params>(ref<BareLambdaParam>),
                   lit<",">) +
              lit<")">) |
             append<&ast::LambdaExpr::params>(ref<BareLambdaParam>)) +
            lit<"->"> +
            (assign<&ast::LambdaExpr::bodyBlock>(ref<Block>) |
             assign<&ast::LambdaExpr::bodyExpr>(ref<Expression>)))> {};

  struct Expression : rule<ref<LambdaExpr> | ref<AssignmentExpr>> {};

  // -- variable initializers, parameters ----

  struct VariableInitializerExpr
      : rule<create<ast::ExprInit>(
                 assign<&ast::ExprInit::value>(ref<Expression>)) |
             ref<ArrayInitializer>> {};

  struct ArrayInitializer
      : rule<create<ast::ArrayInit>(
            lit<"{"> +
            many(append<&ast::ArrayInit::items>(ref<VariableInitializerExpr>),
                 lit<",">) +
            option(lit<",">) + lit<"}">)> {};

  struct VariableDeclarator
      : rule<create<ast::VarDeclarator>(
            assign<&ast::VarDeclarator::name>(ref<Identifier>) +
            many(append<&ast::VarDeclarator::extraDims>(
                ref<EmptyArrayDimension>)) +
            option(lit<"="> + assign<&ast::VarDeclarator::init>(
                                  ref<VariableInitializerExpr>)))> {};

  struct VariableDeclaratorInit
      : rule<create<ast::VarDeclarator>(
            assign<&ast::VarDeclarator::name>(ref<Identifier>) + lit<"="> +
            assign<&ast::VarDeclarator::init>(ref<VariableInitializerExpr>))> {
  };

  struct FormalParameter
      : rule<create<ast::Param>(
            many(append<&ast::Param::modifiers>(kw<"final">) |
                 append<&ast::Param::annotations>(ref<AnnotationUse>)) +
            assign<&ast::Param::type>(ref<TypeType>) +
            option(many(append<&ast::Param::varargsAnnotations>(
                       ref<AnnotationUse>)) +
                   flag<&ast::Param::varargs>(lit<"...">)) +
            assign<&ast::Param::name>(ref<Identifier>) +
            many(append<&ast::Param::extraDims>(ref<EmptyArrayDimension>)))> {};

  struct ReceiverParameter
      : rule<create<ast::Param>(
            assign<&ast::Param::type>(ref<TypeType>) +
            option(assign<&ast::Param::name>(ref<QualifiedName>) + lit<".">) +
            flag<&ast::Param::isReceiver>(kw<"this">))> {};

  // -- statements ----

  struct Block
      : rule<create<ast::Block>(
            lit<"{"> + many(append<&ast::Block::items>(ref<BlockItem>)) +
            lit<"}">)> {};

  struct LocalVariableDeclaration
      : rule<create<ast::LocalVarDecl>(
            many(append<&ast::LocalVarDecl::modifiers>(kw<"final">) |
                 append<&ast::LocalVarDecl::annotations>(ref<AnnotationUse>)) +
            ((flag<&ast::LocalVarDecl::isVar>(kw<"var">) +
              append<&ast::LocalVarDecl::declarators>(
                  ref<VariableDeclaratorInit>)) |
             (assign<&ast::LocalVarDecl::type>(ref<TypeType>) +
              some(append<&ast::LocalVarDecl::declarators>(
                       ref<VariableDeclarator>),
                   lit<",">))))> {};

  struct LocalTypeDeclaration
      : rule<ref<ClassDeclaration> | ref<InterfaceDeclaration> |
             ref<RecordDeclaration> | ref<EnumDeclaration>> {};

  struct BlockItem : rule<(ref<LocalVariableDeclaration> + lit<";">) |
                          ref<LocalTypeDeclaration> | ref<Statement>> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
            kw<"catch"> + lit<"("> +
            many(append<&ast::CatchClause::modifiers>(kw<"final">) |
                 append<&ast::CatchClause::annotations>(ref<AnnotationUse>)) +
            some(append<&ast::CatchClause::types>(ref<ClassType>), lit<"|">) +
            assign<&ast::CatchClause::name>(ref<Identifier>) + lit<")"> +
            assign<&ast::CatchClause::body>(ref<Block>))> {};

  struct Resource
      : rule<create<ast::Resource>(
            many(append<&ast::Resource::modifiers>(kw<"final">) |
                 append<&ast::Resource::annotations>(ref<AnnotationUse>)) +
            ((flag<&ast::Resource::isVar>(kw<"var">) +
              assign<&ast::Resource::name>(ref<Identifier>) + lit<"="> +
              assign<&ast::Resource::init>(ref<Expression>)) |
             (assign<&ast::Resource::type>(ref<ClassType>) +
              assign<&ast::Resource::name>(ref<Identifier>) +
              many(append<&ast::Resource::extraDims>(
                  ref<EmptyArrayDimension>)) +
              lit<"="> + assign<&ast::Resource::init>(ref<Expression>)) |
             assign<&ast::Resource::name>(ref<QualifiedName>)))> {};

  struct TryStatement
      : rule<create<ast::TryStmt>(
            kw<"try"> +
            ((lit<"("> +
              some(append<&ast::TryStmt::resources>(ref<Resource>), lit<";">) +
              option(lit<";">) + lit<")"> +
              assign<&ast::TryStmt::body>(ref<Block>) +
              many(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
              option(assign<&ast::TryStmt::finallyBlock>(kw<"finally"> +
                                                         ref<Block>))) |
             (assign<&ast::TryStmt::body>(ref<Block>) +
              ((some(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                option(assign<&ast::TryStmt::finallyBlock>(kw<"finally"> +
                                                           ref<Block>))) |
               assign<&ast::TryStmt::finallyBlock>(kw<"finally"> +
                                                   ref<Block>)))))> {};

  struct ForEachStatement
      : rule<create<ast::ForEachStmt>(
            kw<"for"> + lit<"("> +
            many(append<&ast::ForEachStmt::modifiers>(kw<"final">) |
                 append<&ast::ForEachStmt::annotations>(ref<AnnotationUse>)) +
            (flag<&ast::ForEachStmt::isVar>(kw<"var">) |
             assign<&ast::ForEachStmt::type>(ref<TypeType>)) +
            assign<&ast::ForEachStmt::name>(ref<Identifier>) +
            many(append<&ast::ForEachStmt::extraDims>(
                ref<EmptyArrayDimension>)) +
            lit<":"> + assign<&ast::ForEachStmt::iterable>(ref<Expression>) +
            lit<")"> + assign<&ast::ForEachStmt::body>(ref<Statement>))> {};

  struct CaseOrDefaultLabel
      : rule<create<ast::CaseLabel>(
                 kw<"case"> +
                 (assign<&ast::CaseLabel::constant>(ref<Expression>) |
                  (assign<&ast::CaseLabel::patternType>(ref<TypeType>) +
                   assign<&ast::CaseLabel::patternName>(ref<Identifier>)))) |
             create<ast::DefaultLabel>(kw<"default">)> {};

  static constexpr auto SwitchLabels = some(
      append<&ast::SwitchGroup::labels>(ref<CaseOrDefaultLabel>) + lit<":">);
  struct SwitchGroup
      : rule<create<ast::SwitchGroup>(
            SwitchLabels +
            some(append<&ast::SwitchGroup::statements>(ref<BlockItem>)))> {};
  struct BareSwitchGroup : rule<create<ast::SwitchGroup>(SwitchLabels)> {};

  struct SwitchOutcome
      : rule<create<ast::Block>(
            (lit<"{"> + many(append<&ast::Block::items>(ref<BlockItem>)) +
             lit<"}">) |
            many(append<&ast::Block::items>(ref<BlockItem>)))> {};

  struct SwitchLabeled
      : rule<create<ast::ExprCaseRule>(
                 kw<"case"> +
                 some(append<&ast::ExprCaseRule::values>(ref<Expression>),
                      lit<",">) +
                 option(lit<","> + flag<&ast::ExprCaseRule::alsoDefault>(
                                       kw<"default">)) +
                 (flag<&ast::ExprCaseRule::arrow>(lit<"->">) |
                  lit<":">)+assign<&ast::ExprCaseRule::
                                       body>(ref<SwitchOutcome>)) |
             create<ast::PatternCaseRule>(
                 kw<"case"> +
                 some(append<&ast::PatternCaseRule::patterns>(ref<Pattern>),
                      lit<",">) +
                 option(kw<"when"> +
                        assign<&ast::PatternCaseRule::guard>(ref<Expression>)) +
                 (flag<&ast::PatternCaseRule::arrow>(lit<"->">) |
                  lit<":">)+assign<&ast::PatternCaseRule::
                                       body>(ref<SwitchOutcome>)) |
             create<ast::DefaultCaseRule>(
                 kw<"default"> +
                 (flag<&ast::DefaultCaseRule::arrow>(lit<"->">) |
                  lit<":">)+assign<&ast::DefaultCaseRule::
                                       body>(ref<SwitchOutcome>))> {};

  struct SwitchExpression
      : rule<create<ast::SwitchExpr>(
            kw<"switch"> + lit<"("> +
            assign<&ast::SwitchExpr::value>(ref<Expression>) + lit<")"> +
            lit<"{"> +
            many(append<&ast::SwitchExpr::rules>(ref<SwitchLabeled>)) +
            lit<"}">)> {};

  struct SwitchExprStatement
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::value>(ref<SwitchExpression>) +
            option(lit<";">))> {};

  struct Statement
      : rule<
            ref<Block> |
            create<ast::AssertStmt>(
                kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<Expression>) +
                option(lit<":"> +
                       assign<&ast::AssertStmt::message>(ref<Expression>)) +
                lit<";">) |
            create<ast::IfStmt>(
                kw<"if"> + lit<"("> +
                assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))) |
            ref<ForEachStatement> |
            create<ast::ForStmt>(
                kw<"for"> + lit<"("> +
                option(assign<&ast::ForStmt::initDecl>(
                           ref<LocalVariableDeclaration>) |
                       some(append<&ast::ForStmt::initExprs>(ref<Expression>),
                            lit<",">)) +
                lit<";"> +
                option(assign<&ast::ForStmt::cond>(ref<Expression>)) +
                lit<";"> +
                option(some(append<&ast::ForStmt::update>(ref<Expression>),
                            lit<",">)) +
                lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)) |
            create<ast::WhileStmt>(
                kw<"while"> + lit<"("> +
                assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
                assign<&ast::WhileStmt::body>(ref<Statement>)) |
            create<ast::DoStmt>(kw<"do"> +
                                assign<&ast::DoStmt::body>(ref<Statement>) +
                                kw<"while"> + lit<"("> +
                                assign<&ast::DoStmt::cond>(ref<Expression>) +
                                lit<")"> + lit<";">) |
            ref<TryStatement> |
            create<ast::SwitchStmt>(
                kw<"switch"> + lit<"("> +
                assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
                lit<"{"> +
                many(append<&ast::SwitchStmt::groups>(ref<SwitchGroup>)) +
                option(append<&ast::SwitchStmt::groups>(ref<BareSwitchGroup>)) +
                lit<"}">) |
            create<ast::SyncStmt>(
                kw<"synchronized"> + lit<"("> +
                assign<&ast::SyncStmt::lock>(ref<Expression>) + lit<")"> +
                assign<&ast::SyncStmt::body>(ref<Block>)) |
            create<ast::ReturnStmt>(
                kw<"return"> +
                option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                lit<";">) |
            create<ast::ThrowStmt>(
                kw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expression>) +
                lit<";">) |
            create<ast::BreakStmt>(
                kw<"break"> +
                option(assign<&ast::BreakStmt::label>(ref<Identifier>)) +
                lit<";">) |
            create<ast::ContinueStmt>(
                kw<"continue"> +
                option(assign<&ast::ContinueStmt::label>(ref<Identifier>)) +
                lit<";">) |
            create<ast::YieldStmt>(
                kw<"yield"> + assign<&ast::YieldStmt::value>(ref<Expression>) +
                lit<";">) |
            create<ast::ExprStmt>(lit<";">) | ref<SwitchExprStatement> |
            create<ast::ExprStmt>(
                assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) |
            create<ast::LabeledStmt>(
                assign<&ast::LabeledStmt::label>(ref<Identifier>) + lit<":"> +
                assign<&ast::LabeledStmt::body>(ref<Statement>))> {};

  // -- members ----

  struct FieldDeclaration
      : rule<create<ast::FieldDecl>(
            many(append<&ast::FieldDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::FieldDecl::modifiers>(ModifierKw)) +
            assign<&ast::FieldDecl::type>(ref<TypeType>) +
            some(append<&ast::FieldDecl::declarators>(ref<VariableDeclarator>),
                 lit<",">) +
            lit<";">)> {};

  struct MethodDeclaration
      : rule<create<ast::MethodDecl>(
            many(append<&ast::MethodDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::MethodDecl::modifiers>(ModifierKw)) +
            option(
                lit<"<"> +
                some(append<&ast::MethodDecl::typeParams>(ref<TypeParameter>),
                     lit<",">) +
                lit<">">) +
            assign<&ast::MethodDecl::returnType>(ref<TypeTypeOrVoid>) +
            assign<&ast::MethodDecl::name>(ref<Identifier>) + lit<"("> +
            option((append<&ast::MethodDecl::params>(ref<ReceiverParameter>) |
                    append<&ast::MethodDecl::params>(ref<FormalParameter>)) +
                   many(lit<","> + append<&ast::MethodDecl::params>(
                                       ref<FormalParameter>))) +
            lit<")"> +
            many(append<&ast::MethodDecl::returnExtraDims>(
                ref<EmptyArrayDimension>)) +
            option(kw<"throws"> +
                   some(append<&ast::MethodDecl::throwsList>(ref<ClassType>),
                        lit<",">)) +
            (assign<&ast::MethodDecl::body>(ref<Block>) | lit<";">))> {};

  struct ConstructorDeclaration
      : rule<create<ast::ConstructorDecl>(
            many(
                append<&ast::ConstructorDecl::annotations>(ref<AnnotationUse>) |
                append<&ast::ConstructorDecl::modifiers>(ModifierKw)) +
            option(lit<"<"> +
                   some(append<&ast::ConstructorDecl::typeParams>(
                            ref<TypeParameter>),
                        lit<",">) +
                   lit<">">) +
            assign<&ast::ConstructorDecl::name>(ref<Identifier>) + lit<"("> +
            option(
                (append<&ast::ConstructorDecl::params>(ref<ReceiverParameter>) |
                 append<&ast::ConstructorDecl::params>(ref<FormalParameter>)) +
                many(lit<","> + append<&ast::ConstructorDecl::params>(
                                    ref<FormalParameter>))) +
            lit<")"> +
            option(
                kw<"throws"> +
                some(append<&ast::ConstructorDecl::throwsList>(ref<ClassType>),
                     lit<",">)) +
            assign<&ast::ConstructorDecl::body>(ref<Block>))> {};

  struct CompactConstructorDeclaration
      : rule<create<ast::CompactConstructorDecl>(
            many(append<&ast::CompactConstructorDecl::modifiers>(ModifierKw) |
                 append<&ast::CompactConstructorDecl::annotations>(ref<AnnotationUse>)) +
            assign<&ast::CompactConstructorDecl::name>(ref<Identifier>) +
            assign<&ast::CompactConstructorDecl::body>(ref<Block>))> {};

  struct ClassBodyDeclaration
      : rule<create<ast::InitBlock>(
                 flag<&ast::InitBlock::isStatic>(kw<"static">) +
                 assign<&ast::InitBlock::body>(ref<Block>)) |
             create<ast::InitBlock>(assign<&ast::InitBlock::body>(ref<Block>)) |
             ref<RecordDeclaration> | ref<MethodDeclaration> |
             ref<FieldDeclaration> | ref<ConstructorDeclaration> |
             ref<InterfaceDeclaration> | ref<AnnotationTypeDeclaration> |
             ref<ClassDeclaration> | ref<EnumDeclaration>> {};

  struct RecordBodyDeclaration
      : rule<ref<CompactConstructorDeclaration> | ref<ClassBodyDeclaration>> {};

  // -- type declarations ----

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::ClassDecl::modifiers>(ModifierKw)) +
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<TypeIdentifier>) +
            option(lit<"<"> +
                   some(append<&ast::ClassDecl::typeParams>(ref<TypeParameter>),
                        lit<",">) +
                   lit<">">) +
            option(kw<"extends"> +
                   assign<&ast::ClassDecl::superclass>(ref<ClassType>)) +
            option(kw<"implements"> +
                   some(append<&ast::ClassDecl::interfaces>(ref<ClassType>),
                        lit<",">)) +
            option(kw<"permits"> +
                   some(append<&ast::ClassDecl::permits>(ref<ClassType>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::ClassDecl::members>(ref<ClassBodyDeclaration>) |
                 lit<";">) +
            lit<"}">)> {};

  struct InterfaceDeclaration
      : rule<create<ast::InterfaceDecl>(
            many(append<&ast::InterfaceDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::InterfaceDecl::modifiers>(ModifierKw)) +
            kw<"interface"> +
            assign<&ast::InterfaceDecl::name>(ref<TypeIdentifier>) +
            option(lit<"<"> +
                   some(append<&ast::InterfaceDecl::typeParams>(
                            ref<TypeParameter>),
                        lit<",">) +
                   lit<">">) +
            option(
                kw<"extends"> +
                some(append<&ast::InterfaceDecl::extendsList>(ref<ClassType>),
                     lit<",">)) +
            option(kw<"permits"> +
                   some(append<&ast::InterfaceDecl::permits>(ref<ClassType>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::InterfaceDecl::members>(
                     ref<ClassBodyDeclaration>) |
                 lit<";">) +
            lit<"}">)> {};

  struct EnumConstant
      : rule<create<ast::EnumConstant>(
            many(append<&ast::EnumConstant::annotations>(ref<AnnotationUse>)) +
            assign<&ast::EnumConstant::name>(ref<Identifier>) +
            option(lit<"("> +
                   many(append<&ast::EnumConstant::args>(ref<Expression>),
                        lit<",">) +
                   lit<")">) +
            option(flag<&ast::EnumConstant::hasBody>(lit<"{">) +
                   many(append<&ast::EnumConstant::classBody>(
                            ref<ClassBodyDeclaration>) |
                        lit<";">) +
                   lit<"}">))> {};

  struct EnumDeclaration
      : rule<create<ast::EnumDecl>(
            many(append<&ast::EnumDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::EnumDecl::modifiers>(ModifierKw)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<TypeIdentifier>) +
            option(kw<"implements"> +
                   some(append<&ast::EnumDecl::interfaces>(ref<ClassType>),
                        lit<",">)) +
            lit<"{"> +
            option(some(append<&ast::EnumDecl::constants>(ref<EnumConstant>),
                        lit<",">)) +
            option(lit<",">) +
            option(lit<";"> + many(append<&ast::EnumDecl::members>(
                                       ref<ClassBodyDeclaration>) |
                                   lit<";">)) +
            lit<"}">)> {};

  struct RecordComponent
      : rule<create<ast::RecordComponent>(
            many(append<&ast::RecordComponent::annotations>(
                ref<AnnotationUse>)) +
            assign<&ast::RecordComponent::type>(ref<TypeType>) +
            option(many(append<&ast::RecordComponent::varargsAnnotations>(
                       ref<AnnotationUse>)) +
                   flag<&ast::RecordComponent::varargs>(lit<"...">)) +
            assign<&ast::RecordComponent::name>(ref<Identifier>))> {};

  struct RecordDeclaration
      : rule<create<ast::RecordDecl>(
            many(append<&ast::RecordDecl::annotations>(ref<AnnotationUse>) |
                 append<&ast::RecordDecl::modifiers>(ModifierKw)) +
            kw<"record"> + assign<&ast::RecordDecl::name>(ref<TypeIdentifier>) +
            option(
                lit<"<"> +
                some(append<&ast::RecordDecl::typeParams>(ref<TypeParameter>),
                     lit<",">) +
                lit<">">) +
            lit<"("> +
            option(
                some(append<&ast::RecordDecl::components>(ref<RecordComponent>),
                     lit<",">)) +
            lit<")"> +
            option(kw<"implements"> +
                   some(append<&ast::RecordDecl::interfaces>(ref<ClassType>),
                        lit<",">)) +
            lit<"{"> +
            many(append<&ast::RecordDecl::members>(ref<RecordBodyDeclaration>) |
                 lit<";">) +
            lit<"}">)> {};

  struct AnnotationTypeElementDeclaration
      : rule<create<ast::AnnotationTypeElement>(
                 many(append<&ast::AnnotationTypeElement::annotations>(
                          ref<AnnotationUse>) |
                      append<&ast::AnnotationTypeElement::modifiers>(
                          ModifierKw)) +
                 assign<&ast::AnnotationTypeElement::type>(ref<TypeType>) +
                 assign<&ast::AnnotationTypeElement::name>(ref<Identifier>) +
                 lit<"("> + lit<")"> +
                 option(kw<"default"> +
                        assign<&ast::AnnotationTypeElement::defaultValue>(
                            ref<AnnotationValue>)) +
                 lit<";">) |
             create<ast::FieldDecl>(
                 many(append<&ast::FieldDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::FieldDecl::modifiers>(ModifierKw)) +
                 assign<&ast::FieldDecl::type>(ref<TypeType>) +
                 some(append<&ast::FieldDecl::declarators>(
                          ref<VariableDeclarator>),
                      lit<",">) +
                 lit<";">) |
             ref<ClassBodyDeclaration>> {};

  struct AnnotationTypeDeclaration
      : rule<create<ast::AnnotationTypeDecl>(
            many(append<&ast::AnnotationTypeDecl::annotations>(
                     ref<AnnotationUse>) |
                 append<&ast::AnnotationTypeDecl::modifiers>(ModifierKw)) +
            lit<"@"> + kw<"interface"> +
            assign<&ast::AnnotationTypeDecl::name>(ref<TypeIdentifier>) + lit<"{"> +
            many(append<&ast::AnnotationTypeDecl::members>(
                     ref<AnnotationTypeElementDeclaration>) |
                 lit<";">) +
            lit<"}">)> {};

  struct TypeDeclaration
      : rule<ref<ClassDeclaration> | ref<EnumDeclaration> |
             ref<InterfaceDeclaration> | ref<AnnotationTypeDeclaration> |
             ref<RecordDeclaration>> {};

  // -- top level ----

  struct PackageDeclaration
      : rule<create<ast::PackageDecl>(
            many(append<&ast::PackageDecl::annotations>(ref<AnnotationUse>)) +
            kw<"package"> +
            assign<&ast::PackageDecl::name>(ref<QualifiedName>) + lit<";">)> {};

  struct ImportDeclaration
      : rule<create<ast::ImportDecl>(
            kw<"import"> +
            option(flag<&ast::ImportDecl::isStatic>(kw<"static">)) +
            assign<&ast::ImportDecl::qualifiedName>(ref<QualifiedName>) +
            option(lit<"."> + flag<&ast::ImportDecl::onDemand>(lit<"*">)) +
            lit<";">)> {};

  struct ModuleDirective
      : rule<create<ast::RequiresDirective>(
                 kw<"requires"> +
                 many(flag<&ast::RequiresDirective::isTransitive>(
                          kw<"transitive">) |
                      flag<&ast::RequiresDirective::isStatic>(
                          kw<"static">)) +
                 assign<&ast::RequiresDirective::module_>(ref<QualifiedName>) +
                 lit<";">) |
             create<ast::ExportsDirective>(
                 kw<"exports"> +
                 assign<&ast::ExportsDirective::package_>(ref<QualifiedName>) +
                 option(kw<"to"> + some(append<&ast::ExportsDirective::to>(
                                            ref<QualifiedName>),
                                        lit<",">)) +
                 lit<";">) |
             create<ast::OpensDirective>(
                 kw<"opens"> +
                 assign<&ast::OpensDirective::package_>(ref<QualifiedName>) +
                 option(kw<"to"> + some(append<&ast::OpensDirective::to>(
                                            ref<QualifiedName>),
                                        lit<",">)) +
                 lit<";">) |
             create<ast::UsesDirective>(
                 kw<"uses"> +
                 assign<&ast::UsesDirective::service>(ref<QualifiedName>) +
                 lit<";">) |
             create<ast::ProvidesDirective>(
                 kw<"provides"> +
                 assign<&ast::ProvidesDirective::service>(ref<QualifiedName>) +
                 kw<"with"> +
                 some(append<&ast::ProvidesDirective::with>(ref<QualifiedName>),
                      lit<",">) +
                 lit<";">)> {};

  struct ModuleDeclaration
      : rule<create<ast::ModuleDecl>(
            many(append<&ast::ModuleDecl::annotations>(ref<AnnotationUse>)) +
            option(flag<&ast::ModuleDecl::open>(kw<"open">)) +
            kw<"module"> + assign<&ast::ModuleDecl::name>(ref<QualifiedName>) +
            lit<"{"> +
            many(append<&ast::ModuleDecl::directives>(ref<ModuleDirective>)) +
            lit<"}">)> {};

  struct OrdinaryCompilationUnit
      : rule<create<ast::CompilationUnit>(
            option(assign<&ast::CompilationUnit::package_>(
                ref<PackageDeclaration>)) +
            many(
                append<&ast::CompilationUnit::imports>(ref<ImportDeclaration>) |
                lit<";">) +
            many(append<&ast::CompilationUnit::types>(ref<TypeDeclaration>) |
                 lit<";">))> {};

  struct ModularCompilationUnit
      : rule<create<ast::CompilationUnit>(
            many(append<&ast::CompilationUnit::imports>(
                ref<ImportDeclaration>)) +
            assign<&ast::CompilationUnit::module_>(ref<ModuleDeclaration>))> {};

  struct CompilationUnit
      : rule<ref<OrdinaryCompilationUnit> | ref<ModularCompilationUnit>> {};

  using entry = CompilationUnit;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, COMMENT>;
};

} // namespace java
