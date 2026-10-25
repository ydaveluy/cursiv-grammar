// The aspectj language: its grammar. Java plus the aspect constructs --
// aspects, pointcuts, advice, inter-type declarations, declare forms and the
// annotation-style (@Aspect) forms.
#pragma once

#include <cstdint>
#include <string_view>

#include "aspectj/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace aspectj {

struct Aspectj : cursiv::GrammarSpec {
  // A "java letter": ASCII letters, `$`, `_`, and every scalar past ASCII.
  static constexpr auto JavaLetter = cr<"a-zA-Z$_"> | cr<R"(-􏿿)">;
  static constexpr auto JavaLetterOrDigit =
      cr<"a-zA-Z0-9$_"> | cr<R"(-􏿿)">;
  static constexpr auto word_boundary = JavaLetterOrDigit;

  static constexpr auto CountBrackets = [](std::string_view t) -> std::uint8_t {
    std::uint8_t n = 0;
    for (const char c : t)
      if (c == '[') ++n;
    return n;
  };
  struct ExtraDims : rule<some(lit<"["> + lit<"]">), text_conv<CountBrackets>> {};
  struct ArrayDims : rule<many(lit<"["> + lit<"]">), text_conv<CountBrackets>> {};

  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // `new` is never a name: `Foo.new(..)` reads as a constructor pattern.
  // Kept for cost too: dropping it changes no tree but costs +4 % instructions.
  struct ID : terminal<!kw<"new"> + JavaLetter + many(JavaLetterOrDigit)> {};

  static constexpr auto Digits = d + option(many(d | lit<"_">) + d);
  static constexpr auto ExponentPart = cr<"eE"> + option(cr<"+-">) + Digits;
  static constexpr auto HexDigit = cr<"0-9a-fA-F">;
  static constexpr auto HexDigits = HexDigit + option(many(HexDigit | lit<"_">) + HexDigit);
  static constexpr auto EscapeSeq =
      lit<"\\"> + (option(lit<"u005c">) + (cr<R"(bstnfr"'\\)">
                   | (option(option(cr<"0-3">) + cr<"0-7">) + cr<"0-7">))
                   | (some(lit<"u">) + repeat<4>(HexDigit)));

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
  struct BOOL_LITERAL : terminal<kw<"true"> | kw<"false">> {};
  struct CHAR_LITERAL
      : terminal<lit<"'"> + (EscapeSeq | (!cr<R"('\\\r\n)"> + any)) + lit<"'">> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(EscapeSeq | (!cr<R"("\\\r\n)"> + any)) + lit<"\"">> {};
  struct TEXT_BLOCK
      : terminal<through(lit<"\"\"\""> + many(cr<R"( \t)">) + cr<R"(\r\n)">, lit<"\"\"\"">)> {};

  static constexpr auto ModifierKw =
      kw<"public"> | kw<"protected"> | kw<"private"> | kw<"static"> | kw<"abstract"> |
      kw<"final"> | kw<"strictfp"> | kw<"sealed"> | kw<"non-sealed"> | kw<"native"> |
      kw<"synchronized"> | kw<"transient"> | kw<"volatile"> | kw<"default">;

  // Every pointcut/advice/declare word AspectJ adds on top of Java: a soft
  // keyword wherever a state expects one of these spellings, a plain name
  // everywhere else -- so `int call = 5;` still parses.
  static constexpr auto AspectKw =
      kw<"args"> | kw<"after"> | kw<"around"> | kw<"aspect"> | kw<"before"> | kw<"call"> |
      kw<"cflow"> | kw<"cflowbelow"> | kw<"declare"> | kw<"error"> | kw<"execution"> |
      kw<"get"> | kw<"handler"> | kw<"initialization"> | kw<"issingleton"> | kw<"parents"> |
      kw<"percflow"> | kw<"percflowbelow"> | kw<"pertarget"> | kw<"perthis"> |
      kw<"pertypewithin"> | kw<"pointcut"> | kw<"precedence"> | kw<"preinitialization"> |
      kw<"privileged"> | kw<"returning"> | kw<"set"> | kw<"soft"> |
      kw<"staticinitialization"> | kw<"target"> | kw<"throwing"> | kw<"warning"> |
      kw<"within"> | kw<"withincode"> | kw<"value"> | kw<"interfaces"> | kw<"defaultImpl"> |
      kw<"type"> | kw<"method"> | kw<"constructor"> | kw<"field">;

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
  static constexpr auto PrefixOp = lit<"++"> | lit<"--"> | lit<"+"> | lit<"-"> | lit<"~"> | lit<"!">;
  static constexpr auto AssignOp =
      lit<"<<="> | lit<">>>="> | lit<">>="> | lit<"+="> | lit<"-="> | lit<"*="> | lit<"/="> |
      lit<"%="> | lit<"&="> | lit<"^="> | lit<"|="> | lit<"=", !cr<"=">>;

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
  struct SwitchLabeledRule;
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

  // -- aspectj forward declarations ----
  struct AspectDeclaration;
  struct AspectBodyDeclaration;
  struct PerClause;
  struct Advice;
  struct PointcutDeclaration;
  struct PointcutUnary;
  struct PointcutExpr;
  struct PointcutPrimitive;
  struct ReferencePointcut;
  struct ArgPattern;
  struct AnnotationArgPattern;
  struct TypePatternUnary;
  struct TypePattern;
  struct DottedNamePattern;
  struct SimpleTypePattern;
  struct SimpleNamePattern;
  struct AnnotationPatternClause;
  struct ModifierPatternClause;
  struct FieldPattern;
  struct MethodPattern;
  struct ConstructorPattern;
  struct FormalsItem;
  struct ClassPatternRef;
  struct InterTypeMethodDecl;
  struct InterTypeConstructorDecl;
  struct InterTypeFieldDecl;
  struct DeclareParents;
  struct DeclareMessage;
  struct DeclareSoft;
  struct DeclarePrecedence;
  struct DeclareAnnotation;

  // -- names ----

  struct Identifier
      : rule<ref<ID> | kw<"module"> | kw<"open"> | kw<"requires"> | kw<"exports"> |
             kw<"opens"> | kw<"to"> | kw<"uses"> | kw<"provides"> | kw<"when"> | kw<"with"> |
             kw<"transitive"> | kw<"yield"> | kw<"sealed"> | kw<"permits"> | kw<"record"> |
             kw<"var"> | AspectKw> {};

  // AspectKw is deliberately absent here (unlike Identifier below): a word
  // like `pointcut` or `aspect` as a soft TYPE name would collide with the
  // declaration it introduces (`abstract pointcut foo();` would also read
  // as a method returning a type named `pointcut`), so type-name position
  // keeps only Java's own contextual keywords.
  struct TypeIdentifier
      : rule<ref<ID> | kw<"module"> | kw<"open"> | kw<"requires"> | kw<"exports"> |
             kw<"opens"> | kw<"to"> | kw<"uses"> | kw<"provides"> | kw<"with"> |
             kw<"transitive"> | kw<"sealed">> {};

  struct QualifiedName : rule<some(ref<Identifier>, lit<".">)> {};

  // -- annotations ----
  // `Annotation` is a base: an ordinary `@Name(...)` use, or one of the
  // annotation-style aspect forms (`@Aspect`, `@Before("pc()")`...) sharing
  // its position -- a class or method can carry either.

  struct AnnotationValue
      : rule<create<ast::AnnotationArrayValue>(
                 lit<"{"> +
                 many(append<&ast::AnnotationArrayValue::items>(ref<AnnotationValue>), lit<",">) +
                 option(lit<",">) + lit<"}">) |
             ref<AnnotationUse> |
             create<ast::AnnotationExprValue>(assign<&ast::AnnotationExprValue::value>(ref<Expression>))> {
  };

  struct AnnotationArgument
      : rule<create<ast::AnnotationArg>(
                 (assign<&ast::AnnotationArg::name>(ref<Identifier>) + lit<"="> +
                  assign<&ast::AnnotationArg::value>(ref<AnnotationValue>)) |
                 assign<&ast::AnnotationArg::value>(ref<AnnotationValue>))> {};

  struct AnnotationUse
      : rule<
            create<ast::PlainAnnotation>(
                lit<"@"> + assign<&ast::PlainAnnotation::name>(ref<QualifiedName>) +
                option(lit<"("> +
                       many(append<&ast::PlainAnnotation::args>(ref<AnnotationArgument>),
                            lit<",">) +
                       lit<")">)) |
            create<ast::AdvicePointcutAnnotation>(
                lit<"@"> +
                assign<&ast::AdvicePointcutAnnotation::kind>(
                    kw<"After"> | kw<"AfterThrowing"> | kw<"Around"> | kw<"Before"> |
                    kw<"DeclareError"> | kw<"DeclareWarning">) +
                lit<"("> + lit<"\""> +
                assign<&ast::AdvicePointcutAnnotation::pointcut>(ref<PointcutExpr>) +
                lit<"\""> + lit<")">) |
            create<ast::AfterReturningAnnotation>(
                lit<"@"> + kw<"AfterReturning"> + lit<"("> +
                ((lit<"\""> +
                  assign<&ast::AfterReturningAnnotation::pointcut>(ref<PointcutExpr>) +
                  lit<"\"">) |
                 (kw<"pointcut"> + lit<"="> + lit<"\""> +
                  assign<&ast::AfterReturningAnnotation::pointcut>(ref<PointcutExpr>) +
                  lit<"\""> + lit<","> + kw<"returning"> + lit<"="> + lit<"\""> +
                  assign<&ast::AfterReturningAnnotation::returning>(ref<Identifier>) +
                  lit<"\"">)) +
                lit<")">) |
            create<ast::AspectAnnotation>(
                lit<"@"> + kw<"Aspect"> +
                option(lit<"("> + lit<"\""> +
                       assign<&ast::AspectAnnotation::perClause>(ref<PerClause>) + lit<"\""> +
                       lit<")">)) |
            create<ast::DeclareMixinAnnotation>(
                lit<"@"> + kw<"DeclareMixin"> + lit<"("> + kw<"value"> + lit<"="> + lit<"\""> +
                assign<&ast::DeclareMixinAnnotation::valuePattern>(ref<TypePattern>) +
                lit<"\""> + lit<","> + kw<"interfaces"> + lit<"="> + lit<"{"> +
                some(append<&ast::DeclareMixinAnnotation::interfaces>(ref<ClassPatternRef>),
                     lit<",">) +
                lit<"}"> + lit<")">) |
            create<ast::DeclareParentsAnnotation>(
                lit<"@"> + kw<"DeclareParents"> + lit<"("> +
                ((lit<"\""> +
                  assign<&ast::DeclareParentsAnnotation::pattern>(ref<TypePattern>) +
                  lit<"\"">) |
                 (kw<"value"> + lit<"="> + lit<"\""> +
                  assign<&ast::DeclareParentsAnnotation::pattern>(ref<TypePattern>) +
                  lit<"\""> + lit<","> + kw<"defaultImpl"> + lit<"="> +
                  assign<&ast::DeclareParentsAnnotation::defaultImpl>(ref<ClassPatternRef>))) +
                lit<")">) |
            create<ast::DeclarePrecedenceAnnotation>(
                lit<"@"> + kw<"DeclarePrecedence"> + lit<"("> + lit<"\""> +
                some(append<&ast::DeclarePrecedenceAnnotation::order>(ref<TypePattern>),
                     lit<",">) +
                lit<"\""> + lit<")">) |
            create<ast::PointcutAnnotation>(
                lit<"@"> + kw<"Pointcut"> + lit<"("> + lit<"\""> +
                option(assign<&ast::PointcutAnnotation::pointcut>(ref<PointcutExpr>)) +
                lit<"\""> + lit<")">)> {};

  // -- types ----

  static constexpr auto PrimitiveKw = kw<"boolean"> | kw<"char"> | kw<"byte"> | kw<"short"> |
                                       kw<"int"> | kw<"long"> | kw<"float"> | kw<"double">;

  struct PrimitiveType
      : rule<create<ast::PrimitiveTypeRef>(assign<&ast::PrimitiveTypeRef::keyword>(PrimitiveKw))> {
  };

  static constexpr auto ClassTypeArgsBody =
      lit<"<"> + many(append<&ast::ClassTypeRef::typeArguments>(ref<TypeArgument>), lit<",">) +
      lit<">">;

  struct ClassType
      : rule<create<ast::ClassTypeRef>(
                 many(append<&ast::ClassTypeRef::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::ClassTypeRef::name>(ref<TypeIdentifier>) +
                 option(ClassTypeArgsBody)) +
             many(nest<ast::ClassTypeRef, &ast::ClassTypeRef::qualifier>(
                 lit<"."> + many(append<&ast::ClassTypeRef::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::ClassTypeRef::name>(ref<TypeIdentifier>) +
                 option(ClassTypeArgsBody)))> {};

  struct TypeType
      : rule<(ref<ClassType> | ref<PrimitiveType>) +
             many(nest<ast::ArrayTypeRef, &ast::ArrayTypeRef::element>(
                 many(append<&ast::ArrayTypeRef::annotations>(ref<AnnotationUse>)) + lit<"["> +
                 lit<"]">))> {};

  struct TypeTypeOrVoid : rule<ref<TypeType> | create<ast::VoidTypeRef>(kw<"void">)> {};

  struct CreatedName : rule<ref<ClassType> | ref<PrimitiveType>> {};

  struct TypeArgument
      : rule<create<ast::TypeArgType>(assign<&ast::TypeArgType::type>(ref<TypeType>)) |
             create<ast::WildcardTypeArg>(
                 many(append<&ast::WildcardTypeArg::annotations>(ref<AnnotationUse>)) +
                 lit<"?"> +
                 option((kw<"extends"> |
                         flag<&ast::WildcardTypeArg::superBound>(kw<"super">)) +
                        assign<&ast::WildcardTypeArg::bound>(ref<TypeType>)))> {};

  struct TypeParameter
      : rule<create<ast::TypeParam>(
                 many(append<&ast::TypeParam::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::TypeParam::name>(ref<Identifier>) +
                 option(kw<"extends"> + append<&ast::TypeParam::bounds>(ref<TypeType>) +
                        many(lit<"&"> + append<&ast::TypeParam::bounds>(ref<TypeType>))))> {};

  // -- patterns (instanceof, switch) ----

  struct Pattern
      : rule<create<ast::RecordPattern>(
                 assign<&ast::RecordPattern::type>(ref<TypeType>) + lit<"("> +
                 many(append<&ast::RecordPattern::components>(ref<Pattern>), lit<",">) +
                 lit<")">) |
             create<ast::InstanceOfPattern>(
                 many(append<&ast::InstanceOfPattern::modifiers>(kw<"final">) |
                      append<&ast::InstanceOfPattern::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::InstanceOfPattern::type>(ref<TypeType>) +
                 many(append<&ast::InstanceOfPattern::annotations>(ref<AnnotationUse>)) +
                 some(append<&ast::InstanceOfPattern::declarators>(ref<VariableDeclarator>),
                      lit<",">))> {};

  // -- literals ----

  struct IntegerLiteral
      : rule<create<ast::IntLit>(assign<&ast::IntLit::text>(
                 ref<DECIMAL_LITERAL> | ref<HEX_LITERAL> | ref<OCT_LITERAL> |
                 ref<BINARY_LITERAL>))> {};
  struct FloatLiteral
      : rule<create<ast::FloatLit>(
                 assign<&ast::FloatLit::text>(ref<FLOAT_LITERAL> | ref<HEX_FLOAT_LITERAL>))> {};
  struct CharLiteral
      : rule<create<ast::CharLit>(assign<&ast::CharLit::text>(ref<CHAR_LITERAL>))> {};
  struct StringLiteral
      : rule<create<ast::StringLit>(assign<&ast::StringLit::text>(ref<STRING_LITERAL>))> {};
  struct TextBlockLiteral
      : rule<create<ast::TextBlockLit>(assign<&ast::TextBlockLit::text>(ref<TEXT_BLOCK>))> {};
  struct BooleanLiteral
      : rule<create<ast::BoolLit>(assign<&ast::BoolLit::text>(ref<BOOL_LITERAL>))> {};

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
                     many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                          lit<",">) +
                     lit<")">) |
                    (lit<"."> +
                     option(
                         lit<"<"> +
                         many(append<&ast::ExplicitGenericInvokeExpr::memberTypeArguments>(
                                  ref<TypeArgument>),
                              lit<",">) +
                         lit<">">) +
                     assign<&ast::ExplicitGenericInvokeExpr::qualifiedMember>(ref<Identifier>) +
                     option(flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                            many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                                 lit<",">) +
                            lit<")">)))) |
                  (assign<&ast::ExplicitGenericInvokeExpr::target>(kw<"this"> | ref<Identifier>) +
                   flag<&ast::ExplicitGenericInvokeExpr::invoked>(lit<"(">) +
                   many(append<&ast::ExplicitGenericInvokeExpr::args>(ref<Expression>),
                        lit<",">) +
                   lit<")">)))> {};

  struct Primary
      : rule<(lit<"("> + ref<Expression> + lit<")">) | ref<SwitchExpression> |
             ref<TextBlockLiteral> | ref<FloatLiteral> | ref<IntegerLiteral> |
             ref<CharLiteral> | ref<StringLiteral> | ref<BooleanLiteral> |
             create<ast::NullLit>(kw<"null">) |
             create<ast::ClassLit>(assign<&ast::ClassLit::type>(ref<TypeTypeOrVoid>) +
                                    lit<"."> + kw<"class">) |
             ref<ExplicitGenericInvocation> |
             create<ast::CallExpr>(
                 assign<&ast::CallExpr::callee>(
                     create<ast::ThisExpr>(kw<"this">) | create<ast::SuperExpr>(kw<"super">) |
                     create<ast::Ident>(assign<&ast::Ident::name>(ref<Identifier>))) +
                 lit<"("> + many(append<&ast::CallExpr::args>(ref<Expression>), lit<",">) +
                 lit<")">) |
             create<ast::ThisExpr>(kw<"this">) | create<ast::SuperExpr>(kw<"super">) |
             create<ast::Ident>(assign<&ast::Ident::name>(ref<Identifier>)) |
             create<ast::MethodRefExpr>(
                 assign<&ast::MethodRefExpr::typeTarget>(ref<TypeType>) + lit<"::"> +
                 option(lit<"<"> +
                        many(append<&ast::MethodRefExpr::typeArguments>(ref<TypeArgument>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodRefExpr::name>(ref<Identifier> | kw<"new">))> {};

  struct PostfixExpr
      : rule<(ref<Primary> | (kw<"new"> + ref<Creator>)) +
             many(nest<ast::IndexExpr, &ast::IndexExpr::array>(
                      lit<"["> + assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">) |
                  nest<ast::CallExpr, &ast::CallExpr::callee>(
                      lit<"("> +
                      many(append<&ast::CallExpr::args>(ref<Expression>), lit<",">) + lit<")">) |
                  nest<ast::MemberExpr, &ast::MemberExpr::object>(
                      lit<"."> +
                      option(lit<"<"> +
                             many(append<&ast::MemberExpr::typeArguments>(ref<TypeArgument>),
                                  lit<",">) +
                             lit<">">) +
                      assign<&ast::MemberExpr::field>(ref<Identifier> | kw<"this"> |
                                                       kw<"super">)) |
                  nest<ast::NewObjectExpr, &ast::NewObjectExpr::outer>(
                      lit<"."> + kw<"new"> +
                      option(lit<"<"> +
                             many(append<&ast::NewObjectExpr::typeArguments>(
                                      ref<TypeArgument>),
                                  lit<",">) +
                             lit<">">) +
                      assign<&ast::NewObjectExpr::type>(ref<CreatedName>) + lit<"("> +
                      many(append<&ast::NewObjectExpr::args>(ref<Expression>), lit<",">) +
                      lit<")"> +
                      option(flag<&ast::NewObjectExpr::anonymousBody>(lit<"{">) +
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
                      assign<&ast::MethodRefExpr::name>(ref<Identifier> | kw<"new">)) |
                  nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                      assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct ArrayDimensionRule
      : rule<lit<"["> +
             option(assign<&ast::ArrayDimension::value>(ref<Expression>)) +
             lit<"]">> {};

  struct Creator
      : rule<create<ast::NewObjectExpr>(
                 option(lit<"<"> +
                        many(append<&ast::NewObjectExpr::typeArguments>(ref<TypeArgument>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::NewObjectExpr::type>(ref<CreatedName>) + lit<"("> +
                 many(append<&ast::NewObjectExpr::args>(ref<Expression>), lit<",">) + lit<")"> +
                 option(flag<&ast::NewObjectExpr::anonymousBody>(lit<"{">) +
                        many(append<&ast::NewObjectExpr::classBody>(
                                 ref<ClassBodyDeclaration>) |
                             lit<";">) +
                        lit<"}">)) |
             create<ast::NewArrayExpr>(
                 assign<&ast::NewArrayExpr::elementType>(ref<CreatedName>) +
                 some(append<&ast::NewArrayExpr::dimensions>(ref<ArrayDimensionRule>)) +
                 option(assign<&ast::NewArrayExpr::init>(ref<ArrayInitializer>)))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(
                 lit<"("> + many(append<&ast::CastExpr::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::CastExpr::type>(ref<TypeType>) +
                 many(lit<"&"> + append<&ast::CastExpr::intersectionTypes>(ref<TypeType>)) +
                 lit<")"> +
                 assign<&ast::CastExpr::operand>(ref<LambdaExpr> | ref<UnaryExpr>)) |
             ref<UnaryExpr>> {};

  struct UnaryExpr
      : rule<create<ast::PrefixExpr>(assign<&ast::PrefixExpr::op>(PrefixOp) +
                                      assign<&ast::PrefixExpr::operand>(ref<UnaryExpr>)) |
             ref<PostfixExpr>> {};

  struct RelationalAndBelow
      : infix<CastExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op, &ast::BinaryExpr::right,
              (left(OpMul) >> left(OpAdd) >> left(OpShift) >> left(OpRel))> {};

  struct InstanceOfExpr
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::value>(ref<RelationalAndBelow>) +
                 kw<"instanceof"> +
                 (assign<&ast::InstanceOfExpr::pattern>(ref<Pattern>) |
                  assign<&ast::InstanceOfExpr::type>(ref<TypeType>))) |
             ref<RelationalAndBelow>> {};

  struct EqualityAndAbove
      : infix<InstanceOfExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(OpEq) >> left(OpBitAnd) >> left(OpBitXor) >> left(OpBitOr) >>
               left(OpLogAnd) >> left(OpLogOr))> {};

  struct TernaryExpr
      : rule<create<ast::TernaryExpr>(
                 assign<&ast::TernaryExpr::cond>(ref<EqualityAndAbove>) + lit<"?"> +
                 assign<&ast::TernaryExpr::thenValue>(ref<Expression>) + lit<":"> +
                 assign<&ast::TernaryExpr::elseValue>(ref<TernaryExpr>)) |
             ref<EqualityAndAbove>> {};

  struct AssignmentExpr
      : rule<create<ast::AssignExpr>(assign<&ast::AssignExpr::target>(ref<UnaryExpr>) +
                                      assign<&ast::AssignExpr::op>(AssignOp) +
                                      assign<&ast::AssignExpr::value>(ref<Expression>)) |
             ref<TernaryExpr>> {};

  struct TypedLambdaParam
      : rule<create<ast::LambdaParam>(
                 many(append<&ast::LambdaParam::modifiers>(kw<"final">)) +
                 (flag<&ast::LambdaParam::isVar>(kw<"var">) |
                  assign<&ast::LambdaParam::type>(ref<TypeType>)) +
                 assign<&ast::LambdaParam::name>(ref<Identifier>))> {};
  struct BareLambdaParam
      : rule<create<ast::LambdaParam>(assign<&ast::LambdaParam::name>(ref<Identifier>))> {};

  struct LambdaExpr
      : rule<create<ast::LambdaExpr>(
                 ((lit<"("> +
                   many(append<&ast::LambdaExpr::params>(ref<TypedLambdaParam>), lit<",">) +
                   lit<")">) |
                  (lit<"("> +
                   some(append<&ast::LambdaExpr::params>(ref<BareLambdaParam>), lit<",">) +
                   lit<")">) |
                  append<&ast::LambdaExpr::params>(ref<BareLambdaParam>)) +
                 lit<"->"> +
                 (assign<&ast::LambdaExpr::bodyBlock>(ref<Block>) |
                  assign<&ast::LambdaExpr::bodyExpr>(ref<Expression>)))> {};

  struct Expression : rule<ref<LambdaExpr> | ref<AssignmentExpr>> {};

  // -- variable initializers, parameters ----

  struct VariableInitializerExpr
      : rule<create<ast::ExprInit>(assign<&ast::ExprInit::value>(ref<Expression>)) |
             ref<ArrayInitializer>> {};

  struct ArrayInitializer
      : rule<create<ast::ArrayInit>(
                 lit<"{"> +
                 many(append<&ast::ArrayInit::items>(ref<VariableInitializerExpr>), lit<",">) +
                 option(lit<",">) + lit<"}">)> {};

  struct VariableDeclarator
      : rule<create<ast::VarDeclarator>(
                 assign<&ast::VarDeclarator::name>(ref<Identifier>) +
                 option(assign<&ast::VarDeclarator::extraDims>(ref<ExtraDims>)) +
                 option(lit<"="> +
                        assign<&ast::VarDeclarator::init>(ref<VariableInitializerExpr>)))> {};

  struct VariableDeclaratorInit
      : rule<create<ast::VarDeclarator>(
                 assign<&ast::VarDeclarator::name>(ref<Identifier>) + lit<"="> +
                 assign<&ast::VarDeclarator::init>(ref<VariableInitializerExpr>))> {};

  struct FormalParameter
      : rule<create<ast::Param>(
                 many(append<&ast::Param::modifiers>(kw<"final">) |
                      append<&ast::Param::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::Param::type>(ref<TypeType>) +
                 option(many(append<&ast::Param::annotations>(ref<AnnotationUse>)) +
                        flag<&ast::Param::varargs>(lit<"...">)) +
                 assign<&ast::Param::name>(ref<Identifier>) +
                 option(assign<&ast::Param::extraDims>(ref<ExtraDims>)))> {};

  struct ReceiverParameter
      : rule<create<ast::Param>(assign<&ast::Param::type>(ref<TypeType>) +
                                 option(assign<&ast::Param::name>(ref<QualifiedName>) +
                                        lit<".">) +
                                 flag<&ast::Param::isReceiver>(kw<"this">))> {};

  // -- statements ----

  struct Block
      : rule<create<ast::Block>(lit<"{"> + many(append<&ast::Block::items>(ref<BlockItem>)) +
                                 lit<"}">)> {};

  struct LocalVariableDeclaration
      : rule<create<ast::LocalVarDecl>(
                 many(append<&ast::LocalVarDecl::modifiers>(kw<"final">) |
                      append<&ast::LocalVarDecl::annotations>(ref<AnnotationUse>)) +
                 ((flag<&ast::LocalVarDecl::isVar>(kw<"var">) +
                   append<&ast::LocalVarDecl::declarators>(ref<VariableDeclaratorInit>)) |
                  (assign<&ast::LocalVarDecl::type>(ref<TypeType>) +
                   some(append<&ast::LocalVarDecl::declarators>(ref<VariableDeclarator>),
                        lit<",">))))> {};

  struct LocalTypeDeclaration
      : rule<ref<ClassDeclaration> | ref<InterfaceDeclaration> | ref<RecordDeclaration> |
             ref<EnumDeclaration> | ref<AspectDeclaration>> {};

  struct BlockItem
      : rule<(ref<LocalVariableDeclaration> + lit<";">) | ref<LocalTypeDeclaration> |
             ref<Statement>> {};

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
                   option(assign<&ast::Resource::extraDims>(ref<ExtraDims>)) + lit<"="> +
                   assign<&ast::Resource::init>(ref<Expression>)) |
                  assign<&ast::Resource::name>(ref<QualifiedName>)))> {};

  struct TryStatement
      : rule<create<ast::TryStmt>(
                 kw<"try"> +
                 ((lit<"("> +
                   some(append<&ast::TryStmt::resources>(ref<Resource>), lit<";">) +
                   option(lit<";">) + lit<")"> + assign<&ast::TryStmt::body>(ref<Block>) +
                   many(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                   option(assign<&ast::TryStmt::finallyBlock>(kw<"finally"> + ref<Block>))) |
                  (assign<&ast::TryStmt::body>(ref<Block>) +
                   ((some(append<&ast::TryStmt::catches>(ref<CatchClause>)) +
                     option(
                         assign<&ast::TryStmt::finallyBlock>(kw<"finally"> + ref<Block>))) |
                    assign<&ast::TryStmt::finallyBlock>(kw<"finally"> + ref<Block>)))))> {};

  struct ForEachStatement
      : rule<create<ast::ForEachStmt>(
                 kw<"for"> + lit<"("> +
                 many(append<&ast::ForEachStmt::modifiers>(kw<"final">)) +
                 (flag<&ast::ForEachStmt::isVar>(kw<"var">) |
                  assign<&ast::ForEachStmt::type>(ref<TypeType>)) +
                 assign<&ast::ForEachStmt::name>(ref<Identifier>) +
                 option(assign<&ast::ForEachStmt::extraDims>(ref<ExtraDims>)) + lit<":"> +
                 assign<&ast::ForEachStmt::iterable>(ref<Expression>) + lit<")"> +
                 assign<&ast::ForEachStmt::body>(ref<Statement>))> {};

  struct CaseOrDefaultLabel
      : rule<create<ast::CaseLabel>(
                 kw<"case"> +
                 (assign<&ast::CaseLabel::constant>(ref<Expression>) |
                  (assign<&ast::CaseLabel::patternType>(ref<TypeType>) +
                   assign<&ast::CaseLabel::patternName>(ref<Identifier>)))) |
             create<ast::DefaultLabel>(kw<"default">)> {};

  static constexpr auto SwitchLabels =
      some(append<&ast::SwitchGroup::labels>(ref<CaseOrDefaultLabel>) + lit<":">);
  struct SwitchGroup
      : rule<create<ast::SwitchGroup>(
                 SwitchLabels + some(append<&ast::SwitchGroup::statements>(ref<BlockItem>)))> {};
  struct BareSwitchGroup : rule<create<ast::SwitchGroup>(SwitchLabels)> {};

  struct SwitchOutcome
      : rule<create<ast::Block>(
                 (lit<"{"> + many(append<&ast::Block::items>(ref<BlockItem>)) + lit<"}">) |
                 many(append<&ast::Block::items>(ref<BlockItem>)))> {};

  struct SwitchLabeledRule
      : rule<create<ast::ExprCaseRule>(
                 kw<"case"> +
                 some(append<&ast::ExprCaseRule::values>(ref<Expression>), lit<",">) +
                 option(lit<","> +
                        flag<&ast::ExprCaseRule::alsoDefault>(kw<"default">)) +
                 (flag<&ast::ExprCaseRule::arrow>(lit<"->">) | lit<":">) +
                 assign<&ast::ExprCaseRule::body>(ref<SwitchOutcome>)) |
             create<ast::PatternCaseRule>(
                 kw<"case"> +
                 some(append<&ast::PatternCaseRule::patterns>(ref<Pattern>), lit<",">) +
                 option(kw<"when"> +
                        assign<&ast::PatternCaseRule::guard>(ref<Expression>)) +
                 (flag<&ast::PatternCaseRule::arrow>(lit<"->">) | lit<":">) +
                 assign<&ast::PatternCaseRule::body>(ref<SwitchOutcome>)) |
             create<ast::DefaultCaseRule>(
                 kw<"default"> +
                 (flag<&ast::DefaultCaseRule::arrow>(lit<"->">) | lit<":">) +
                 assign<&ast::DefaultCaseRule::body>(ref<SwitchOutcome>))> {};

  struct SwitchExpression
      : rule<create<ast::SwitchExpr>(
                 kw<"switch"> + lit<"("> + assign<&ast::SwitchExpr::value>(ref<Expression>) +
                 lit<")"> + lit<"{"> +
                 many(append<&ast::SwitchExpr::rules>(ref<SwitchLabeledRule>)) + lit<"}">)> {};

  struct SwitchExprStatement
      : rule<create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<SwitchExpression>) +
                                    option(lit<";">))> {};

  struct Statement
      : rule<ref<Block> |
             create<ast::AssertStmt>(
                 kw<"assert"> + assign<&ast::AssertStmt::cond>(ref<Expression>) +
                 option(lit<":"> + assign<&ast::AssertStmt::message>(ref<Expression>)) +
                 lit<";">) |
             create<ast::IfStmt>(
                 kw<"if"> + lit<"("> + assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
                 assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
                 option(commit(kw<"else">, assign<&ast::IfStmt::elseBranch>(ref<Statement>)))) |
             ref<ForEachStatement> |
             create<ast::ForStmt>(
                 kw<"for"> + lit<"("> +
                 option(assign<&ast::ForStmt::initDecl>(ref<LocalVariableDeclaration>) |
                        some(append<&ast::ForStmt::initExprs>(ref<Expression>), lit<",">)) +
                 lit<";"> + option(assign<&ast::ForStmt::cond>(ref<Expression>)) + lit<";"> +
                 option(some(append<&ast::ForStmt::update>(ref<Expression>), lit<",">)) +
                 lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>)) |
             create<ast::WhileStmt>(kw<"while"> + lit<"("> +
                                     assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
                                     assign<&ast::WhileStmt::body>(ref<Statement>)) |
             create<ast::DoStmt>(kw<"do"> + assign<&ast::DoStmt::body>(ref<Statement>) +
                                  kw<"while"> + lit<"("> +
                                  assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> +
                                  lit<";">) |
             ref<TryStatement> |
             create<ast::SwitchStmt>(
                 kw<"switch"> + lit<"("> + assign<&ast::SwitchStmt::value>(ref<Expression>) +
                 lit<")"> + lit<"{"> + many(append<&ast::SwitchStmt::groups>(ref<SwitchGroup>)) +
                 option(append<&ast::SwitchStmt::groups>(ref<BareSwitchGroup>)) + lit<"}">) |
             create<ast::SyncStmt>(kw<"synchronized"> + lit<"("> +
                                    assign<&ast::SyncStmt::lock>(ref<Expression>) + lit<")"> +
                                    assign<&ast::SyncStmt::body>(ref<Block>)) |
             create<ast::ReturnStmt>(kw<"return"> +
                                      option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                                      lit<";">) |
             create<ast::ThrowStmt>(kw<"throw"> + assign<&ast::ThrowStmt::value>(ref<Expression>) +
                                     lit<";">) |
             create<ast::BreakStmt>(kw<"break"> +
                                     option(assign<&ast::BreakStmt::label>(ref<Identifier>)) +
                                     lit<";">) |
             create<ast::ContinueStmt>(
                 kw<"continue"> + option(assign<&ast::ContinueStmt::label>(ref<Identifier>)) +
                 lit<";">) |
             create<ast::YieldStmt>(kw<"yield"> + assign<&ast::YieldStmt::value>(ref<Expression>) +
                                     lit<";">) |
             create<ast::ExprStmt>(lit<";">) | ref<SwitchExprStatement> |
             create<ast::ExprStmt>(assign<&ast::ExprStmt::value>(ref<Expression>) + lit<";">) |
             create<ast::LabeledStmt>(assign<&ast::LabeledStmt::label>(ref<Identifier>) +
                                       lit<":"> +
                                       assign<&ast::LabeledStmt::body>(ref<Statement>))> {};

  // -- members ----

  struct FieldDeclaration
      : rule<create<ast::FieldDecl>(
                 many(append<&ast::FieldDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::FieldDecl::modifiers>(ModifierKw)) +
                 assign<&ast::FieldDecl::type>(ref<TypeType>) +
                 some(append<&ast::FieldDecl::declarators>(ref<VariableDeclarator>), lit<",">) +
                 lit<";">)> {};

  struct MethodDeclaration
      : rule<create<ast::MethodDecl>(
                 many(append<&ast::MethodDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::MethodDecl::modifiers>(ModifierKw)) +
                 option(lit<"<"> +
                        some(append<&ast::MethodDecl::typeParams>(ref<TypeParameter>), lit<",">) +
                        lit<">">) +
                 assign<&ast::MethodDecl::returnType>(ref<TypeTypeOrVoid>) +
                 assign<&ast::MethodDecl::name>(ref<Identifier>) + lit<"("> +
                 option((append<&ast::MethodDecl::params>(ref<ReceiverParameter>) |
                         append<&ast::MethodDecl::params>(ref<FormalParameter>)) +
                        many(lit<","> + append<&ast::MethodDecl::params>(ref<FormalParameter>))) +
                 lit<")"> +
                 option(assign<&ast::MethodDecl::returnExtraDims>(ref<ExtraDims>)) +
                 option(kw<"throws"> +
                        some(append<&ast::MethodDecl::throwsList>(ref<ClassType>), lit<",">)) +
                 (assign<&ast::MethodDecl::body>(ref<Block>) | lit<";">))> {};

  struct ConstructorDeclaration
      : rule<create<ast::ConstructorDecl>(
                 many(append<&ast::ConstructorDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::ConstructorDecl::modifiers>(ModifierKw)) +
                 option(lit<"<"> +
                        some(append<&ast::ConstructorDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 assign<&ast::ConstructorDecl::name>(ref<Identifier>) + lit<"("> +
                 option(
                     (append<&ast::ConstructorDecl::params>(ref<ReceiverParameter>) |
                      append<&ast::ConstructorDecl::params>(ref<FormalParameter>)) +
                     many(lit<","> +
                          append<&ast::ConstructorDecl::params>(ref<FormalParameter>))) +
                 lit<")"> +
                 option(kw<"throws"> +
                        some(append<&ast::ConstructorDecl::throwsList>(ref<ClassType>),
                             lit<",">)) +
                 assign<&ast::ConstructorDecl::body>(ref<Block>))> {};

  struct CompactConstructorDeclaration
      : rule<create<ast::CompactConstructorDecl>(
                 many(append<&ast::CompactConstructorDecl::modifiers>(ModifierKw)) +
                 assign<&ast::CompactConstructorDecl::name>(ref<Identifier>) +
                 assign<&ast::CompactConstructorDecl::body>(ref<Block>))> {};

  struct ClassBodyDeclaration
      : rule<create<ast::InitBlock>(flag<&ast::InitBlock::isStatic>(kw<"static">) +
                                     assign<&ast::InitBlock::body>(ref<Block>)) |
             create<ast::InitBlock>(assign<&ast::InitBlock::body>(ref<Block>)) |
             ref<RecordDeclaration> | ref<MethodDeclaration> | ref<FieldDeclaration> |
             ref<ConstructorDeclaration> | ref<InterfaceDeclaration> |
             ref<AnnotationTypeDeclaration> | ref<ClassDeclaration> | ref<EnumDeclaration> |
             ref<AspectDeclaration> | ref<PointcutDeclaration>> {};

  struct RecordBodyDeclaration
      : rule<ref<CompactConstructorDeclaration> | ref<ClassBodyDeclaration>> {};

  // -- type declarations ----

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
                 many(append<&ast::ClassDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::ClassDecl::modifiers>(ModifierKw)) +
                 kw<"class"> + assign<&ast::ClassDecl::name>(ref<Identifier>) +
                 option(lit<"<"> +
                        some(append<&ast::ClassDecl::typeParams>(ref<TypeParameter>), lit<",">) +
                        lit<">">) +
                 option(kw<"extends"> + assign<&ast::ClassDecl::superclass>(ref<ClassType>)) +
                 option(kw<"implements"> +
                        some(append<&ast::ClassDecl::interfaces>(ref<ClassType>), lit<",">)) +
                 option(kw<"permits"> +
                        some(append<&ast::ClassDecl::permits>(ref<ClassType>), lit<",">)) +
                 lit<"{"> +
                 many(append<&ast::ClassDecl::members>(ref<ClassBodyDeclaration>) | lit<";">) +
                 lit<"}">)> {};

  struct InterfaceDeclaration
      : rule<create<ast::InterfaceDecl>(
                 many(append<&ast::InterfaceDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::InterfaceDecl::modifiers>(ModifierKw)) +
                 kw<"interface"> + assign<&ast::InterfaceDecl::name>(ref<Identifier>) +
                 option(lit<"<"> +
                        some(append<&ast::InterfaceDecl::typeParams>(ref<TypeParameter>),
                             lit<",">) +
                        lit<">">) +
                 option(kw<"extends"> +
                        some(append<&ast::InterfaceDecl::extendsList>(ref<ClassType>),
                             lit<",">)) +
                 option(kw<"permits"> +
                        some(append<&ast::InterfaceDecl::permits>(ref<ClassType>), lit<",">)) +
                 lit<"{"> +
                 many(append<&ast::InterfaceDecl::members>(ref<ClassBodyDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  struct EnumConstant
      : rule<create<ast::EnumConstant>(
                 many(append<&ast::EnumConstant::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::EnumConstant::name>(ref<Identifier>) +
                 option(lit<"("> +
                        many(append<&ast::EnumConstant::args>(ref<Expression>), lit<",">) +
                        lit<")">) +
                 option(flag<&ast::EnumConstant::hasBody>(lit<"{">) +
                        many(append<&ast::EnumConstant::classBody>(ref<ClassBodyDeclaration>) |
                             lit<";">) +
                        lit<"}">))> {};

  struct EnumDeclaration
      : rule<create<ast::EnumDecl>(
                 many(append<&ast::EnumDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::EnumDecl::modifiers>(ModifierKw)) +
                 kw<"enum"> + assign<&ast::EnumDecl::name>(ref<Identifier>) +
                 option(kw<"implements"> +
                        some(append<&ast::EnumDecl::interfaces>(ref<ClassType>), lit<",">)) +
                 lit<"{"> +
                 option(some(append<&ast::EnumDecl::constants>(ref<EnumConstant>), lit<",">)) +
                 option(lit<",">) +
                 option(lit<";"> +
                        many(append<&ast::EnumDecl::members>(ref<ClassBodyDeclaration>) |
                             lit<";">)) +
                 lit<"}">)> {};

  struct RecordComponent
      : rule<create<ast::RecordComponent>(
                 many(append<&ast::RecordComponent::annotations>(ref<AnnotationUse>)) +
                 assign<&ast::RecordComponent::type>(ref<TypeType>) +
                 option(many(append<&ast::RecordComponent::annotations>(ref<AnnotationUse>)) +
                        flag<&ast::RecordComponent::varargs>(lit<"...">)) +
                 assign<&ast::RecordComponent::name>(ref<Identifier>))> {};

  struct RecordDeclaration
      : rule<create<ast::RecordDecl>(
                 many(append<&ast::RecordDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::RecordDecl::modifiers>(ModifierKw)) +
                 kw<"record"> + assign<&ast::RecordDecl::name>(ref<Identifier>) +
                 option(lit<"<"> +
                        some(append<&ast::RecordDecl::typeParams>(ref<TypeParameter>), lit<",">) +
                        lit<">">) +
                 lit<"("> +
                 option(some(append<&ast::RecordDecl::components>(ref<RecordComponent>),
                             lit<",">)) +
                 lit<")"> +
                 option(kw<"implements"> +
                        some(append<&ast::RecordDecl::interfaces>(ref<ClassType>), lit<",">)) +
                 lit<"{"> +
                 many(append<&ast::RecordDecl::members>(ref<RecordBodyDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  struct AnnotationTypeElementDeclaration
      : rule<create<ast::AnnotationTypeElement>(
                 many(append<&ast::AnnotationTypeElement::annotations>(ref<AnnotationUse>) |
                      append<&ast::AnnotationTypeElement::modifiers>(ModifierKw)) +
                 assign<&ast::AnnotationTypeElement::type>(ref<TypeType>) +
                 assign<&ast::AnnotationTypeElement::name>(ref<Identifier>) + lit<"("> +
                 lit<")"> +
                 option(kw<"default"> +
                        assign<&ast::AnnotationTypeElement::defaultValue>(
                            ref<AnnotationValue>)) +
                 lit<";">) |
             create<ast::FieldDecl>(
                 many(append<&ast::FieldDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::FieldDecl::modifiers>(ModifierKw)) +
                 assign<&ast::FieldDecl::type>(ref<TypeType>) +
                 some(append<&ast::FieldDecl::declarators>(ref<VariableDeclarator>), lit<",">) +
                 lit<";">) |
             ref<ClassBodyDeclaration>> {};

  struct AnnotationTypeDeclaration
      : rule<create<ast::AnnotationTypeDecl>(
                 many(append<&ast::AnnotationTypeDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::AnnotationTypeDecl::modifiers>(ModifierKw)) +
                 lit<"@"> + kw<"interface"> +
                 assign<&ast::AnnotationTypeDecl::name>(ref<Identifier>) + lit<"{"> +
                 many(append<&ast::AnnotationTypeDecl::members>(
                          ref<AnnotationTypeElementDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  struct TypeDeclaration
      : rule<ref<ClassDeclaration> | ref<EnumDeclaration> | ref<InterfaceDeclaration> |
             ref<AnnotationTypeDeclaration> | ref<RecordDeclaration> | ref<AspectDeclaration>> {};

  // -- aspects ----

  struct PerClause
      : rule<create<ast::PerTargetClause>(
                 kw<"pertarget"> + lit<"("> +
                 assign<&ast::PerTargetClause::pointcut>(ref<PointcutExpr>) + lit<")">) |
             create<ast::PerThisClause>(
                 kw<"perthis"> + lit<"("> +
                 assign<&ast::PerThisClause::pointcut>(ref<PointcutExpr>) + lit<")">) |
             create<ast::PerCflowClause>(
                 kw<"percflow"> + lit<"("> +
                 assign<&ast::PerCflowClause::pointcut>(ref<PointcutExpr>) + lit<")">) |
             create<ast::PerCflowBelowClause>(
                 kw<"percflowbelow"> + lit<"("> +
                 assign<&ast::PerCflowBelowClause::pointcut>(ref<PointcutExpr>) + lit<")">) |
             create<ast::PerTypeWithinClause>(
                 kw<"pertypewithin"> + lit<"("> +
                 assign<&ast::PerTypeWithinClause::pattern>(ref<TypePattern>) + lit<")">) |
             create<ast::IssingletonClause>(kw<"issingleton"> + lit<"("> + lit<")">)> {};

  struct AspectBodyDeclaration
      : rule<ref<ClassBodyDeclaration> | ref<Advice> | ref<InterTypeMethodDecl> |
             ref<InterTypeConstructorDecl> | ref<InterTypeFieldDecl> | ref<DeclareParents> |
             ref<DeclareMessage> | ref<DeclareSoft> | ref<DeclarePrecedence> |
             ref<DeclareAnnotation>> {};

  struct AspectDeclaration
      : rule<create<ast::AspectDecl>(
                 many(append<&ast::AspectDecl::annotations>(ref<AnnotationUse>) |
                      append<&ast::AspectDecl::modifiers>(ModifierKw) |
                      flag<&ast::AspectDecl::privileged>(kw<"privileged">)) +
                 kw<"aspect"> + assign<&ast::AspectDecl::name>(ref<Identifier>) +
                 option(kw<"extends"> + assign<&ast::AspectDecl::superclass>(ref<ClassType>)) +
                 option(kw<"implements"> +
                        some(append<&ast::AspectDecl::interfaces>(ref<ClassType>), lit<",">)) +
                 option(assign<&ast::AspectDecl::perClause>(ref<PerClause>)) + lit<"{"> +
                 many(append<&ast::AspectDecl::members>(ref<AspectBodyDeclaration>) |
                      lit<";">) +
                 lit<"}">)> {};

  static constexpr auto AdviceParams =
      lit<"("> +
      option((append<&ast::Advice::params>(ref<ReceiverParameter>) |
              append<&ast::Advice::params>(ref<FormalParameter>)) +
             many(lit<","> + append<&ast::Advice::params>(ref<FormalParameter>))) +
      lit<")">;

  struct Advice
      : rule<create<ast::Advice>(
                 option(flag<&ast::Advice::strictfp_>(kw<"strictfp">)) +
                 ((assign<&ast::Advice::kind>(kw<"before">) + AdviceParams) |
                  (assign<&ast::Advice::kind>(kw<"after">) + AdviceParams +
                   option((flag<&ast::Advice::returning>(kw<"returning">) +
                           option(lit<"("> +
                                  option(assign<&ast::Advice::boundParam>(
                                      ref<FormalParameter>)) +
                                  lit<")">)) |
                          (flag<&ast::Advice::throwing>(kw<"throwing">) +
                           option(lit<"("> +
                                  option(assign<&ast::Advice::boundParam>(
                                      ref<FormalParameter>)) +
                                  lit<")">)))) |
                  ((assign<&ast::Advice::returnType>(ref<TypeType>) | kw<"void">) +
                   assign<&ast::Advice::kind>(kw<"around">) + AdviceParams)) +
                 option(kw<"throws"> +
                        some(append<&ast::Advice::throwsList>(ref<ClassType>), lit<",">)) +
                 lit<":"> + assign<&ast::Advice::pointcut>(ref<PointcutExpr>) +
                 assign<&ast::Advice::body>(ref<Block>))> {};

  struct PointcutDeclaration
      : rule<create<ast::PointcutDecl>(
                 many(append<&ast::PointcutDecl::modifiers>(ModifierKw)) + kw<"pointcut"> +
                 assign<&ast::PointcutDecl::name>(ref<Identifier>) + lit<"("> +
                 option(append<&ast::PointcutDecl::params>(ref<FormalParameter>) +
                        many(lit<","> +
                             append<&ast::PointcutDecl::params>(ref<FormalParameter>))) +
                 lit<")"> +
                 (lit<";"> |
                  (lit<":"> + assign<&ast::PointcutDecl::body>(ref<PointcutExpr>) +
                   lit<";">)))> {};

  // -- inter-type declarations ----

  struct InterTypeMethodDecl
      : rule<create<ast::InterTypeMethodDecl>(
                 many(append<&ast::InterTypeMethodDecl::modifiers>(ModifierKw)) +
                 (assign<&ast::InterTypeMethodDecl::returnType>(ref<TypeType>) | kw<"void">) +
                 assign<&ast::InterTypeMethodDecl::targetType>(ref<TypeType>) + lit<"."> +
                 assign<&ast::InterTypeMethodDecl::name>(ref<Identifier>) + lit<"("> +
                 option((append<&ast::InterTypeMethodDecl::params>(ref<ReceiverParameter>) |
                         append<&ast::InterTypeMethodDecl::params>(ref<FormalParameter>)) +
                        many(lit<","> +
                             append<&ast::InterTypeMethodDecl::params>(ref<FormalParameter>))) +
                 lit<")"> +
                 option(kw<"throws"> +
                        some(append<&ast::InterTypeMethodDecl::throwsList>(ref<ClassType>),
                             lit<",">)) +
                 (assign<&ast::InterTypeMethodDecl::body>(ref<Block>) | lit<";">))> {};

  struct InterTypeConstructorDecl
      : rule<create<ast::InterTypeConstructorDecl>(
                 many(append<&ast::InterTypeConstructorDecl::modifiers>(ModifierKw)) +
                 assign<&ast::InterTypeConstructorDecl::targetType>(ref<TypeType>) + lit<"."> +
                 kw<"new"> + lit<"("> +
                 option((append<&ast::InterTypeConstructorDecl::params>(ref<ReceiverParameter>) |
                         append<&ast::InterTypeConstructorDecl::params>(ref<FormalParameter>)) +
                        many(lit<","> +
                             append<&ast::InterTypeConstructorDecl::params>(
                                 ref<FormalParameter>))) +
                 lit<")"> +
                 option(kw<"throws"> +
                        some(append<&ast::InterTypeConstructorDecl::throwsList>(ref<ClassType>),
                             lit<",">)) +
                 assign<&ast::InterTypeConstructorDecl::body>(ref<Block>))> {};

  struct InterTypeFieldDecl
      : rule<create<ast::InterTypeFieldDecl>(
                 many(append<&ast::InterTypeFieldDecl::modifiers>(ModifierKw)) +
                 (assign<&ast::InterTypeFieldDecl::fieldType>(ref<TypeType>) | kw<"void">) +
                 assign<&ast::InterTypeFieldDecl::targetType>(ref<TypeType>) + lit<"."> +
                 assign<&ast::InterTypeFieldDecl::name>(ref<Identifier>) +
                 option(lit<"="> + assign<&ast::InterTypeFieldDecl::init>(ref<Expression>)) +
                 lit<";">)> {};

  // -- declare forms ----

  struct DeclareParents
      : rule<create<ast::DeclareParents>(
                 kw<"declare"> + kw<"parents"> + lit<":"> +
                 assign<&ast::DeclareParents::pattern>(ref<TypePattern>) +
                 ((kw<"extends"> + assign<&ast::DeclareParents::extendsType>(ref<TypeType>)) |
                  (flag<&ast::DeclareParents::isImplements>(kw<"implements">) +
                   some(append<&ast::DeclareParents::implementsTypes>(ref<TypeType>),
                        lit<",">))) +
                 lit<";">)> {};

  struct DeclareMessage
      : rule<create<ast::DeclareMessage>(
                 kw<"declare"> +
                 (kw<"warning"> | flag<&ast::DeclareMessage::isError>(kw<"error">)) +
                 lit<":"> + assign<&ast::DeclareMessage::pointcut>(ref<PointcutExpr>) +
                 lit<":"> + assign<&ast::DeclareMessage::message>(ref<STRING_LITERAL>) +
                 lit<";">)> {};

  struct DeclareSoft
      : rule<create<ast::DeclareSoft>(
                 kw<"declare"> + kw<"soft"> + lit<":"> +
                 assign<&ast::DeclareSoft::exceptionType>(ref<ClassType>) + lit<":"> +
                 assign<&ast::DeclareSoft::pointcut>(ref<PointcutExpr>) + lit<";">)> {};

  struct DeclarePrecedence
      : rule<create<ast::DeclarePrecedence>(
                 kw<"declare"> + kw<"precedence"> + lit<":"> +
                 some(append<&ast::DeclarePrecedence::order>(ref<TypePattern>), lit<",">) +
                 lit<";">)> {};

  struct DeclareAnnotation
      : rule<create<ast::DeclareAnnotation>(
                 kw<"declare"> + lit<"@"> +
                 ((assign<&ast::DeclareAnnotation::kind>(kw<"type">) + lit<":"> +
                   assign<&ast::DeclareAnnotation::typePattern>(ref<TypePattern>)) |
                  (assign<&ast::DeclareAnnotation::kind>(kw<"method">) + lit<":"> +
                   assign<&ast::DeclareAnnotation::methodPattern>(ref<MethodPattern>)) |
                  (assign<&ast::DeclareAnnotation::kind>(kw<"constructor">) + lit<":"> +
                   assign<&ast::DeclareAnnotation::constructorPattern>(
                       ref<ConstructorPattern>)) |
                  (assign<&ast::DeclareAnnotation::kind>(kw<"field">) + lit<":"> +
                   assign<&ast::DeclareAnnotation::fieldPattern>(ref<FieldPattern>))) +
                 lit<":"> + assign<&ast::DeclareAnnotation::annotation>(ref<AnnotationUse>) +
                 lit<";">)> {};

  // -- type patterns ----

  // A dotted/wildcarded type name: `com.foo.*`, `com..*`, `int`. The first
  // segment anchors it so a bare `.`/`..` cannot also read as one (kept
  // disjoint from FormalsItem's own dotdot marker).
  struct DottedNamePattern
      : rule<(ref<ID> | lit<"*"> | PrimitiveKw) +
                 many(ref<ID> | lit<"*"> | lit<"."> | lit<".."> | PrimitiveKw) |
             kw<"void">> {};

  struct SimpleTypePattern
      : rule<create<ast::SimpleTypePattern>(
                 assign<&ast::SimpleTypePattern::name>(ref<DottedNamePattern>) +
                 option(flag<&ast::SimpleTypePattern::plus>(lit<"+">)) +
                 assign<&ast::SimpleTypePattern::arrayDims>(ref<ArrayDims>))> {};

  struct AnnotationPatternClause
      : rule<create<ast::AnnotationPatternClause>(
                 option(flag<&ast::AnnotationPatternClause::negated>(lit<"!">)) +
                 lit<"@"> +
                 (assign<&ast::AnnotationPatternClause::type>(ref<QualifiedName>) |
                  (lit<"("> +
                   assign<&ast::AnnotationPatternClause::typeExpr>(ref<TypePattern>) +
                   lit<")">)))> {};

  struct TypePatternUnary
      : rule<ref<SimpleTypePattern> |
             create<ast::NotTypePattern>(
                 lit<"!"> + assign<&ast::NotTypePattern::operand>(ref<TypePatternUnary>)) |
             create<ast::ParenTypePattern>(
                 lit<"("> +
                 some(append<&ast::ParenTypePattern::annotations>(ref<AnnotationPatternClause>)) +
                 assign<&ast::ParenTypePattern::pattern>(ref<TypePattern>) + lit<")">) |
             (lit<"("> + ref<TypePattern> + lit<")">)> {};

  struct TypePattern
      : infix<TypePatternUnary, &ast::BinaryTypePattern::left, &ast::BinaryTypePattern::op,
              &ast::BinaryTypePattern::right, (left(lit<"&&">) >> left(lit<"||">))> {};

  struct ModifierPatternClause
      : rule<create<ast::ModifierPatternClause>(
                 option(flag<&ast::ModifierPatternClause::negated>(lit<"!">)) +
                 assign<&ast::ModifierPatternClause::modifier>(
                     kw<"public"> | kw<"private"> | kw<"protected"> | kw<"static"> |
                     kw<"transient"> | kw<"synchronized"> | kw<"final">))> {};

  struct SimpleNamePattern : rule<some(ref<ID> | lit<"*">)> {};

  struct FieldPattern
      : rule<create<ast::FieldPattern>(
                 many(append<&ast::FieldPattern::annotations>(ref<AnnotationPatternClause>)) +
                 many(append<&ast::FieldPattern::modifiers>(ref<ModifierPatternClause>)) +
                 assign<&ast::FieldPattern::type>(ref<TypePattern>) +
                 option(assign<&ast::FieldPattern::declaringType>(ref<TypePattern>) +
                        assign<&ast::FieldPattern::dotOrDotDot>(lit<"."> | lit<"..">)) +
                 assign<&ast::FieldPattern::name>(ref<SimpleNamePattern>))> {};

  struct FormalsItem
      : rule<create<ast::FormalsItem>(flag<&ast::FormalsItem::dotdot>(lit<"..">)) |
             create<ast::FormalsItem>(
                 assign<&ast::FormalsItem::type>(ref<TypePattern>) +
                 flag<&ast::FormalsItem::varargs>(lit<"...">)) |
             create<ast::FormalsItem>(
                 many(append<&ast::FormalsItem::annotations>(ref<AnnotationPatternClause>)) +
                 assign<&ast::FormalsItem::type>(ref<TypePattern>))> {};

  struct MethodPattern
      : rule<create<ast::MethodPattern>(
                 many(append<&ast::MethodPattern::annotations>(ref<AnnotationPatternClause>)) +
                 many(append<&ast::MethodPattern::modifiers>(ref<ModifierPatternClause>)) +
                 assign<&ast::MethodPattern::returnType>(ref<TypePattern>) +
                 option(assign<&ast::MethodPattern::declaringType>(ref<TypePattern>) +
                        assign<&ast::MethodPattern::dotOrDotDot>(lit<"."> | lit<"..">)) +
                 assign<&ast::MethodPattern::name>(ref<SimpleNamePattern>) + lit<"("> +
                 option(append<&ast::MethodPattern::params>(ref<FormalsItem>) +
                        many(lit<","> + append<&ast::MethodPattern::params>(ref<FormalsItem>))) +
                 lit<")"> +
                 option(kw<"throws"> +
                        some(append<&ast::MethodPattern::throwsList>(ref<TypePattern>),
                             lit<",">)))> {};

  struct ConstructorPattern
      : rule<create<ast::ConstructorPattern>(
                 many(append<&ast::ConstructorPattern::annotations>(
                     ref<AnnotationPatternClause>)) +
                 many(append<&ast::ConstructorPattern::modifiers>(ref<ModifierPatternClause>)) +
                 option(assign<&ast::ConstructorPattern::declaringType>(ref<TypePattern>) +
                        assign<&ast::ConstructorPattern::dotOrDotDot>(lit<"."> | lit<"..">)) +
                 kw<"new"> + lit<"("> +
                 option(append<&ast::ConstructorPattern::params>(ref<FormalsItem>) +
                        many(lit<","> +
                             append<&ast::ConstructorPattern::params>(ref<FormalsItem>))) +
                 lit<")"> +
                 option(kw<"throws"> +
                        some(append<&ast::ConstructorPattern::throwsList>(ref<TypePattern>),
                             lit<",">)))> {};

  struct ClassPatternRef
      : rule<create<ast::ClassPatternRef>(
                 assign<&ast::ClassPatternRef::name>(ref<QualifiedName>) + lit<"."> +
                 kw<"class">)> {};

  // -- pointcut expressions ----

  struct ArgPattern
      : rule<create<ast::ArgPattern>(assign<&ast::ArgPattern::wildcard>(lit<"*"> | lit<"..">)) |
             create<ast::ArgPattern>(assign<&ast::ArgPattern::type>(ref<TypeType>))> {};

  struct AnnotationArgPattern
      : rule<create<ast::AnnotationArgPattern>(
                 assign<&ast::AnnotationArgPattern::wildcard>(lit<".."> | lit<"*">)) |
             create<ast::AnnotationArgPattern>(
                 assign<&ast::AnnotationArgPattern::name>(ref<QualifiedName>))> {};

  struct ReferencePointcut
      : rule<create<ast::ReferencePointcut>(
                 option(assign<&ast::ReferencePointcut::qualifier>(ref<TypePattern>) +
                        lit<".">) +
                 assign<&ast::ReferencePointcut::name>(ref<Identifier>) + lit<"("> +
                 option(append<&ast::ReferencePointcut::params>(ref<FormalsItem>) +
                        many(lit<","> +
                             append<&ast::ReferencePointcut::params>(ref<FormalsItem>))) +
                 lit<")">)> {};

  struct PointcutPrimitive
      : rule<
            create<ast::SignaturePointcut>(
                assign<&ast::SignaturePointcut::kind>(
                    kw<"call"> | kw<"execution"> | kw<"withincode">) +
                lit<"("> +
                (assign<&ast::SignaturePointcut::method>(ref<MethodPattern>) |
                 assign<&ast::SignaturePointcut::ctor>(ref<ConstructorPattern>)) +
                lit<")">) |
            create<ast::SignaturePointcut>(
                assign<&ast::SignaturePointcut::kind>(
                    kw<"initialization"> | kw<"preinitialization">) +
                lit<"("> + assign<&ast::SignaturePointcut::ctor>(ref<ConstructorPattern>) +
                lit<")">) |
            create<ast::TypePatternPointcut>(
                assign<&ast::TypePatternPointcut::kind>(
                    kw<"staticinitialization"> | kw<"handler"> | kw<"within">) +
                lit<"("> +
                many(append<&ast::TypePatternPointcut::annotations>(
                    ref<AnnotationPatternClause>)) +
                assign<&ast::TypePatternPointcut::pattern>(ref<TypePattern>) + lit<")">) |
            create<ast::FieldPointcut>(
                assign<&ast::FieldPointcut::kind>(kw<"get"> | kw<"set">) + lit<"("> +
                assign<&ast::FieldPointcut::pattern>(ref<FieldPattern>) + lit<")">) |
            create<ast::AdviceExecutionPointcut>(kw<"adviceexecution"> + lit<"("> + lit<")">) |
            create<ast::NestedPointcut>(
                assign<&ast::NestedPointcut::kind>(kw<"cflow"> | kw<"cflowbelow">) + lit<"("> +
                assign<&ast::NestedPointcut::pointcut>(ref<PointcutExpr>) + lit<")">) |
            create<ast::IfPointcut>(
                kw<"if"> + lit<"("> +
                option(assign<&ast::IfPointcut::condition>(ref<Expression>)) + lit<")">) |
            create<ast::BindingPointcut>(
                assign<&ast::BindingPointcut::kind>(kw<"this"> | kw<"target">) + lit<"("> +
                assign<&ast::BindingPointcut::type>(ref<TypeType>) + lit<")">) |
            create<ast::ArgsPointcut>(
                kw<"args"> + lit<"("> +
                many(append<&ast::ArgsPointcut::args>(ref<ArgPattern>), lit<",">) + lit<")">) |
            create<ast::AnnotationBindingPointcut>(
                lit<"@"> +
                assign<&ast::AnnotationBindingPointcut::kind>(
                    kw<"this"> | kw<"target"> | kw<"within"> | kw<"withincode"> |
                    kw<"annotation">) +
                lit<"("> +
                assign<&ast::AnnotationBindingPointcut::annotationType>(ref<QualifiedName>) +
                lit<")">) |
            create<ast::AnnotationArgsPointcut>(
                lit<"@"> + kw<"args"> + lit<"("> +
                many(append<&ast::AnnotationArgsPointcut::args>(ref<AnnotationArgPattern>),
                     lit<",">) +
                lit<")">)> {};

  struct PointcutUnary
      : rule<ref<PointcutPrimitive> | ref<ReferencePointcut> |
             create<ast::NotPointcut>(
                 lit<"!"> + assign<&ast::NotPointcut::operand>(ref<PointcutUnary>)) |
             (lit<"("> + ref<PointcutExpr> + lit<")">)> {};

  struct PointcutExpr
      : infix<PointcutUnary, &ast::BinaryPointcut::left, &ast::BinaryPointcut::op,
              &ast::BinaryPointcut::right, (left(lit<"&&">) >> left(lit<"||">))> {};

  // -- top level ----

  struct PackageDeclaration
      : rule<create<ast::PackageDecl>(
                 many(append<&ast::PackageDecl::annotations>(ref<AnnotationUse>)) +
                 kw<"package"> + assign<&ast::PackageDecl::name>(ref<QualifiedName>) + lit<";">)> {
  };

  struct ImportDeclaration
      : rule<create<ast::ImportDecl>(
                 kw<"import"> +
                 option(flag<&ast::ImportDecl::isStatic>(kw<"static">)) +
                 assign<&ast::ImportDecl::qualifiedName>(ref<QualifiedName>) +
                 option(lit<"."> + flag<&ast::ImportDecl::onDemand>(lit<"*">)) + lit<";">)> {
  };

  struct ModuleDirective
      : rule<create<ast::RequiresDirective>(
                 kw<"requires"> +
                 many(flag<&ast::RequiresDirective::isTransitive>(kw<"transitive">) |
                      flag<&ast::RequiresDirective::isStatic>(kw<"static">)) +
                 assign<&ast::RequiresDirective::module_>(ref<QualifiedName>) + lit<";">) |
             create<ast::ExportsDirective>(
                 kw<"exports"> + assign<&ast::ExportsDirective::package_>(ref<QualifiedName>) +
                 option(kw<"to"> +
                        some(append<&ast::ExportsDirective::to>(ref<QualifiedName>), lit<",">)) +
                 lit<";">) |
             create<ast::OpensDirective>(
                 kw<"opens"> + assign<&ast::OpensDirective::package_>(ref<QualifiedName>) +
                 option(kw<"to"> +
                        some(append<&ast::OpensDirective::to>(ref<QualifiedName>), lit<",">)) +
                 lit<";">) |
             create<ast::UsesDirective>(kw<"uses"> +
                                         assign<&ast::UsesDirective::service>(ref<QualifiedName>) +
                                         lit<";">) |
             create<ast::ProvidesDirective>(
                 kw<"provides"> + assign<&ast::ProvidesDirective::service>(ref<QualifiedName>) +
                 kw<"with"> +
                 some(append<&ast::ProvidesDirective::with>(ref<QualifiedName>), lit<",">) +
                 lit<";">)> {};

  struct ModuleDeclaration
      : rule<create<ast::ModuleDecl>(
                 many(append<&ast::ModuleDecl::annotations>(ref<AnnotationUse>)) +
                 option(flag<&ast::ModuleDecl::open>(kw<"open">)) + kw<"module"> +
                 assign<&ast::ModuleDecl::name>(ref<QualifiedName>) + lit<"{"> +
                 many(append<&ast::ModuleDecl::directives>(ref<ModuleDirective>)) + lit<"}">)> {};

  struct OrdinaryCompilationUnit
      : rule<create<ast::CompilationUnit>(
                 option(assign<&ast::CompilationUnit::package_>(ref<PackageDeclaration>)) +
                 many(append<&ast::CompilationUnit::imports>(ref<ImportDeclaration>) |
                      lit<";">) +
                 many(append<&ast::CompilationUnit::types>(ref<TypeDeclaration>) | lit<";">))> {};

  struct ModularCompilationUnit
      : rule<create<ast::CompilationUnit>(
                 many(append<&ast::CompilationUnit::imports>(ref<ImportDeclaration>)) +
                 assign<&ast::CompilationUnit::module_>(ref<ModuleDeclaration>))> {};

  struct CompilationUnit
      : rule<ref<OrdinaryCompilationUnit> | ref<ModularCompilationUnit>> {};

  using entry = CompilationUnit;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, COMMENT>;
};

}  // namespace aspectj
