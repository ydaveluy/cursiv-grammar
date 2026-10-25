// The typescript language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "typescript/core/ast.hpp"

namespace typescript {

inline constexpr auto template_open_text = [](std::string_view text) {
  return text.substr(1);
};

struct TypeScript : cursiv::GrammarSpec {
  // -- Trivia ----
  static constexpr auto LineTerminator = cr<R"(\r\n\u2028\u2029)">;
  struct WS
      : terminal<some(cr<R"( \t\v\f\r\n\uFEFF\u2028\u2029)"> | uc::Zs)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(~LineTerminator)> {};
  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct HTML_COMMENT : terminal<through(lit<"<!--">, lit<"-->">)> {};
  struct CDATA_COMMENT : terminal<through(lit<"<![CDATA[">, lit<"]]>">)> {};

  // A byte-order mark and a `#!` line open the text and nothing else.
  struct PROLOGUE
      : terminal<(lit<"\xEF\xBB\xBF"> +
                  option(lit<"#!"> + many(~LineTerminator))) |
                 (lit<"#!"> + many(~LineTerminator))> {};

  static constexpr auto LineBreak =
      gap::has(eol | lit<"\xE2\x80\xA8"> | lit<"\xE2\x80\xA9">);

  // -- Identifiers ----
  static constexpr auto IdEscape =
      lit<"\\u"> +
      (repeat<4>(cr<"0-9a-fA-F">) | (lit<"{"> + some(cr<"0-9a-fA-F">) + lit<"}">));
  static constexpr auto IdentifierStart = uc::L | uc::Nl | cr<"_$ᢅᢆ℘℮゛゜">;
  static constexpr auto IdentifierContinue =
      IdentifierStart | uc::Mn | uc::Mc | uc::Nd | uc::Pc |
      cr<"‌‍··፩-፱᧚・･">;
  static constexpr auto IdChars =
      (IdentifierStart | IdEscape) + many(IdentifierContinue | IdEscape);
  struct RawId : terminal<IdChars> {};

  // Reserved unconditionally: every keyword minus the words a plain
  // identifier also admits.
  static constexpr auto StrictReservedButThis =
      kw<"break"> | kw<"do"> | kw<"instanceof"> | kw<"typeof"> | kw<"case"> |
      kw<"else"> | kw<"new"> | kw<"var"> | kw<"catch"> | kw<"finally"> |
      kw<"return"> | kw<"void"> | kw<"continue"> | kw<"for"> | kw<"switch"> |
      kw<"while"> | kw<"debugger"> | kw<"function"> | kw<"with"> |
      kw<"default"> | kw<"if"> | kw<"throw"> | kw<"delete"> | kw<"in"> |
      kw<"try"> | kw<"class"> | kw<"enum"> | kw<"extends"> | kw<"super"> |
      kw<"const"> | kw<"export"> | kw<"import"> | kw<"implements"> |
      kw<"let"> | kw<"private"> | kw<"public"> | kw<"interface"> |
      kw<"package"> | kw<"protected"> | kw<"static"> | kw<"await"> |
      kw<"null"> | kw<"true"> | kw<"false">;
  static constexpr auto StrictReserved = StrictReservedButThis | kw<"this">;
  // Usable as a plain name outside their own constructs (plus `require`,
  // `module`, `get`, `set`, widened here to match real TypeScript/
  // JavaScript, where all four are ordinary identifiers everywhere).
  static constexpr auto ContextualKeyword =
      kw<"async"> | kw<"as"> | kw<"from"> | kw<"yield"> | kw<"of"> |
      kw<"any"> | kw<"number"> | kw<"boolean"> | kw<"string"> |
      kw<"unique"> | kw<"symbol"> | kw<"never"> | kw<"undefined"> |
      kw<"object"> | kw<"keyof"> | kw<"type"> | kw<"constructor"> |
      kw<"namespace"> | kw<"abstract"> | kw<"module"> | kw<"get"> |
      kw<"set"> | kw<"readonly"> | kw<"is"> | kw<"declare"> | kw<"asserts"> |
      kw<"infer"> | kw<"satisfies"> | kw<"global"> | kw<"override"> |
      kw<"accessor"> | kw<"out">;
  static constexpr auto AllKeywords = StrictReserved | ContextualKeyword;

  struct PlainId : terminal<!AllKeywords + IdChars> {};
  struct Ident
      : rule<assign<&ast::Ident::name>(ref<PlainId> | ContextualKeyword)> {};
  // Any word, keywords included: `.foo` and `.#foo` both take one.
  struct PrivateOrName : terminal<option(lit<"#">) + IdChars> {};
  struct MemberName
      : rule<assign<&ast::Ident::name>(ref<PrivateOrName> | AllKeywords)> {};

  // A type name, a namespace name, or a `typeof` query, folded into one:
  // any word (keyword or not), joined by one or more dots.
  struct QualifiedName
      : rule<(ref<RawId> | AllKeywords) +
             many(some(lit<".">) + (ref<RawId> | AllKeywords))> {};

  // The first word of a type name is never `this`: that is the `this` type.
  struct TypeRefName
      : rule<(ref<RawId> | StrictReservedButThis | ContextualKeyword) +
             many(some(lit<".">) + (ref<RawId> | AllKeywords))> {};

  // -- Literals ---- A digit separator stands only between two digits; a dot
  // or a radix prefix must be followed by an actual digit: an under-guarded
  // number out-munches a member access's leading '.'.
  static constexpr auto DecRest = many(option(lit<"_">) + cr<"0-9">);
  static constexpr auto HexRest = many(option(lit<"_">) + cr<"0-9a-fA-F">);
  static constexpr auto OctRest = many(option(lit<"_">) + cr<"0-7">);
  static constexpr auto BinRest = many(option(lit<"_">) + cr<"01">);
  static constexpr auto DecInt = lit<"0"> | (cr<"1-9"> + DecRest);
  static constexpr auto Exponent =
      cr<"eE"> + option(cr<"+-">) + cr<"0-9"> + DecRest;

  static constexpr auto NumberBody =
      (DecInt + lit<"."> + option(cr<"0-9"> + DecRest) + option(Exponent)) |
      (lit<"."> + cr<"0-9"> + DecRest + option(Exponent)) |
      (DecInt + option(Exponent)) | (lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> + HexRest) |
      (lit<"0"> + cr<"oO"> + cr<"0-7"> + OctRest) |
      (lit<"0"> + cr<"bB"> + cr<"01"> + BinRest) |
      // Legacy octal: accepted unconditionally.
      (lit<"0"> + some(cr<"0-7">));
  static constexpr auto BigIntBody =
      ((lit<"0"> + cr<"xX"> + cr<"0-9a-fA-F"> + HexRest) |
       (lit<"0"> + cr<"oO"> + cr<"0-7"> + OctRest) |
       (lit<"0"> + cr<"bB"> + cr<"01"> + BinRest) | DecInt) +
      lit<"n">;

  struct NUMBER : terminal<NumberBody> {};
  struct BIGINT : terminal<BigIntBody> {};
  // `-1` and `-1n` as literal types; read where a type is, never in an
  // expression, where `-` stays an operator.
  struct NEGATIVE_LITERAL : terminal<lit<"-"> + (BigIntBody | NumberBody)> {};

  static constexpr auto EscapeSeq = lit<"\\"> + (eol | any);
  struct STRING
      : terminal<(lit<"\""> + many(EscapeSeq | cr<R"(^"\\\r\n)">) + lit<"\"">) |
                 (lit<"'"> + many(EscapeSeq | cr<R"(^'\\\r\n)">) + lit<"'">)> {};

  static constexpr auto RegexBackslash = lit<"\\"> + cr<R"(^\r\n)">;
  static constexpr auto RegexClass =
      lit<"["> + many(RegexBackslash | cr<R"(^\r\n\\])">) + lit<"]">;
  struct REGEX
      : terminal<lit<"/"> + (RegexBackslash | RegexClass | cr<R"(^*\r\n\\/[)">) +
                 many(RegexBackslash | RegexClass | cr<R"(^\r\n\\/[)">) +
                 lit<"/"> + many(cr<"a-zA-Z">)> {};

  static constexpr auto TemplateChar =
      EscapeSeq | cr<R"(^`$\\)"> | lit<"$", !cr<"{">>;
  struct TplOpen
      : terminal<lit<"`"> + many(TemplateChar), text_conv<template_open_text>> {};
  struct TplText : terminal<some(TemplateChar)> {};
  struct TplQuasi : rule<option(ref<TplText>)> {};

  template <auto Quasis, auto Values, class ValueRule>
  static constexpr auto TemplateContents =
      append<Quasis>(ref<TplOpen>) +
      tight(many(&lit<"${"> +
                skip(ignored<WS>,
                     hide<SL_COMMENT, ML_COMMENT, HTML_COMMENT, CDATA_COMMENT>,
                     lit<"${"> + append<Values>(ref<ValueRule>) + lit<"}">) +
                append<Quasis>(ref<TplQuasi>)) +
               lit<"`">);

  // -- Forward declarations ----
  struct Expression;
  struct AssignExpr;
  struct ConditionalExpr;
  struct RelationalExpr;
  struct BinaryExpr;
  struct UnaryExpr;
  struct CastOperand;
  struct PostfixExpr;
  struct PrimaryExpr;
  struct NewExpr_;
  struct FunctionExpr_;
  struct ArrowFunctionExpr_;
  struct ClassExpr_;
  struct ArrayLiteral;
  struct ObjectLiteral;
  struct IdentPrimary;
  struct Statement;
  struct BlockStmt_;
  struct FunctionBody;
  struct Assignable;
  struct Type_;
  struct TypeNoCond;
  struct TypeOrPredicate;
  struct PrimaryType;
  struct DecoratorExpr;
  struct ClassMemberRule;
  struct TypeMemberRule;
  struct CallSignatureRule;
  struct AbstractDeclRule;
  struct AbstractFieldDecl;
  struct VariableStatement;
  struct Declarator_;
  struct ImportStatement;
  struct ImportEqualsStatement;
  struct ExportStatement;
  struct NamespaceDeclaration;
  struct InterfaceDeclaration;
  struct TypeAliasDeclaration;
  struct EnumDeclaration;
  struct ClassDeclaration;
  struct FunctionDeclaration;
  struct TypeParamRule;

  // -- Keys shared by object/class/type/enum members ---- `PrivateOrName`, not
  // `RawId`: a property/type-member/enum-member name may spell any keyword
  // (`class: string;` inside an interface body, e.g.) the same way a class
  // member's own key already could.
  struct PropKeyIdent
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<PrivateOrName>))> {};
  struct NumberLit_ : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct BigIntLit_ : rule<assign<&ast::BigIntLit::text>(ref<BIGINT>)> {};
  struct StringLiteral : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  // `AllKeywords` stands beside `PropKeyIdent` and not only inside it: a
  // keyword-shaped word offered only through a general terminal loses to
  // the specific keyword token wherever both are reachable (`class: string`
  // as a type/enum/object member) -- naming it here directly admits it.
  struct KeywordAsKey
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(AllKeywords))> {};
  static constexpr auto PropKey =
      ref<PropKeyIdent> | ref<StringLiteral> | ref<NumberLit_> |
      ref<BigIntLit_> | ref<KeywordAsKey>;

  // -- Params (shared by every parameter list the grammar has) ----
  struct TypeAnnotationRule : rule<lit<":"> + ref<TypeOrPredicate>> {};

  static constexpr auto AccessMod =
      kw<"public"> | kw<"private"> | kw<"protected">;

  // Real TypeScript only ever accepts a parameter decorator on a class
  // constructor or method, which this port does not distinguish per
  // parameter list -- ported onto every parameter list alike instead
  // (over-acceptance, price nil).
  struct ParamItem
      : rule<create<ast::Param>(
                 many(append<&ast::Param::decorators>(ref<DecoratorExpr>)) +
                 flag<&ast::Param::rest>(lit<"...">) +
                 assign<&ast::Param::pattern>(ref<Assignable>) +
                 option(assign<&ast::Param::typeAnnotation>(
                     ref<TypeAnnotationRule>)))  //
             | create<ast::Param>(
                   many(append<&ast::Param::decorators>(ref<DecoratorExpr>)) +
                   option(assign<&ast::Param::accessibility>(AccessMod)) +
                   option(flag<&ast::Param::isOverride>(kw<"override">)) +
                   option(flag<&ast::Param::isReadonly>(kw<"readonly">)) +
                   assign<&ast::Param::pattern>(ref<Assignable>) +
                   option(flag<&ast::Param::optional>(lit<"?">)) +
                   option(assign<&ast::Param::typeAnnotation>(
                       ref<TypeAnnotationRule>)) +
                   option(lit<"="> + assign<&ast::Param::defaultValue>(
                                          ref<AssignExpr>)))> {};

  template <auto Ptr>
  static constexpr auto ParamTailFor =
      option(append<Ptr>(ref<ParamItem>) +
             many(lit<","> + append<Ptr>(ref<ParamItem>)) +
             option(lit<",">)) +
      lit<")">;
  template <auto Ptr>
  static constexpr auto ParamListFor = lit<"("> + ParamTailFor<Ptr>;

  template <auto Ptr>
  static constexpr auto TypeParamsClauseFor =
      option(lit<"<"> +
             option(append<Ptr>(ref<TypeParamRule>) +
                    many(lit<","> + append<Ptr>(ref<TypeParamRule>)) +
                    option(lit<",">)) +
             lit<">">);
  template <auto Ptr>
  static constexpr auto TypeArgsClauseFor =
      lit<"<"> +
      option(append<Ptr>(ref<Type_>) + many(lit<","> + append<Ptr>(ref<Type_>))) +
      lit<">">;

  // -- Types ----
  struct TypeParamRule
      : rule<create<ast::TypeParam>(
            many(flag<&ast::TypeParam::isConst>(kw<"const">) |
                 flag<&ast::TypeParam::isIn>(kw<"in">) |
                 flag<&ast::TypeParam::isOut>(kw<"out">)) +
            assign<&ast::TypeParam::name>(ref<RawId> | ContextualKeyword) +
            option(kw<"extends"> +
                   assign<&ast::TypeParam::constraint>(ref<Type_>)) +
            option(lit<"="> +
                   assign<&ast::TypeParam::defaultType>(ref<Type_>)))> {};

  struct PredefinedTypeRule
      : rule<create<ast::PredefinedType>(
            option(flag<&ast::PredefinedType::unique>(kw<"unique">)) +
            assign<&ast::PredefinedType::name>(
                kw<"any"> | kw<"number"> | ref<NUMBER> | ref<BIGINT> |
                ref<NEGATIVE_LITERAL> | kw<"boolean"> | kw<"true"> | kw<"false"> |
                kw<"string"> | ref<STRING> | kw<"symbol"> | kw<"never"> |
                kw<"undefined"> | kw<"object"> | kw<"void"> | kw<"null">))> {};

  struct TypeReferenceRule
      : rule<create<ast::TypeRef>(
            assign<&ast::TypeRef::name>(ref<TypeRefName>) +
            option(TypeArgsClauseFor<&ast::TypeRef::typeArgs>))> {};

  struct TypeQueryRule
      : rule<create<ast::TypeQuery>(
            kw<"typeof"> + assign<&ast::TypeQuery::name>(ref<QualifiedName>) +
            option(TypeArgsClauseFor<&ast::TypeQuery::typeArgs>))> {};

  static constexpr auto TypeMemberEnd = lit<";"> | lit<",">;
  template <auto Ptr>
  static constexpr auto TypeMembers =
      many(append<Ptr>(ref<TypeMemberRule>) +
           option(TypeMemberEnd));

  struct ObjectTypeRule
      : rule<create<ast::ObjectType>(
            lit<"{"> +
            TypeMembers<&ast::ObjectType::members> +
            lit<"}">)> {};

  struct SignedReadonly : terminal<cr<"+-"> + kw<"readonly">> {};

  struct MappedTypeRule
      : rule<create<ast::MappedType>(
            lit<"{"> +
            option(assign<&ast::MappedType::readonlyMod>(
                kw<"readonly"> | ref<SignedReadonly>)) +
            lit<"["> +
            assign<&ast::MappedType::name>(ref<RawId> | ContextualKeyword) +
            kw<"in"> + assign<&ast::MappedType::constraint>(ref<Type_>) +
            option(kw<"as"> + assign<&ast::MappedType::nameType>(ref<Type_>)) +
            lit<"]"> +
            option(assign<&ast::MappedType::optionalMod>(
                lit<"?"> | lit<"+?"> | lit<"-?">)) +
            option(assign<&ast::MappedType::type>(ref<TypeAnnotationRule>)) +
            option(TypeMemberEnd) + lit<"}">)> {};

  static constexpr auto IndexedTypeTail =
      nest<ast::IndexedType, &ast::IndexedType::object>(
          !LineBreak + lit<"["> +
          option(assign<&ast::IndexedType::index>(ref<Type_>)) + lit<"]">);

  struct ThisTypeRule : rule<create<ast::ThisType>(kw<"this">)> {};

  struct NamedTupleMemberRule
      : rule<create<ast::NamedTupleMember>(
            option(flag<&ast::NamedTupleMember::rest>(lit<"...">)) +
            assign<&ast::NamedTupleMember::name>(ref<RawId> | AllKeywords) +
            option(flag<&ast::NamedTupleMember::optional>(lit<"?">)) +
            lit<":"> + assign<&ast::NamedTupleMember::type>(ref<Type_>))> {};
  struct RestTypeRule
      : rule<create<ast::RestType>(
            lit<"..."> + assign<&ast::RestType::type>(ref<Type_>))> {};
  struct OptionalTypeRule
      : rule<create<ast::OptionalType>(
            assign<&ast::OptionalType::type>(ref<Type_>) + lit<"?">)> {};
  struct TupleElement
      : rule<ref<NamedTupleMemberRule> | ref<RestTypeRule> |
             ref<OptionalTypeRule> | ref<Type_>> {};

  struct TemplateTypeRule
      : rule<create<ast::TemplateLiteralType>(
            TemplateContents<&ast::TemplateLiteralType::quasis,
                             &ast::TemplateLiteralType::types, Type_>)> {};

  struct InferTypeParam
      : rule<create<ast::TypeParam>(
            assign<&ast::TypeParam::name>(ref<RawId> | ContextualKeyword) +
            option(kw<"extends"> +
                   assign<&ast::TypeParam::constraint>(ref<TypeNoCond>)))> {};
  struct InferTypeRule
      : rule<create<ast::InferType>(
            kw<"infer"> +
            assign<&ast::InferType::typeParam>(ref<InferTypeParam>))> {};

  struct ImportTypeRule
      : rule<create<ast::ImportType>(
            option(flag<&ast::ImportType::isTypeOf>(kw<"typeof">)) +
            kw<"import"> + lit<"("> +
            assign<&ast::ImportType::argument>(ref<Type_>) +
            option(lit<","> +
                   option(assign<&ast::ImportType::options>(
                              ref<ObjectLiteral>) +
                          option(lit<",">))) +
            lit<")"> +
            option(lit<"."> +
                   assign<&ast::ImportType::qualifier>(ref<QualifiedName>)) +
            option(TypeArgsClauseFor<&ast::ImportType::typeArgs>))> {};

  struct PrimaryTypeBase
      : rule<ref<PredefinedTypeRule>  //
             | ref<ImportTypeRule>  //
             | ref<TypeQueryRule>  //
             | ref<TemplateTypeRule>  //
             | ref<InferTypeRule>  //
             | ref<ThisTypeRule>  //
             | create<ast::KeyOfType>(
                   kw<"keyof"> +
                   assign<&ast::KeyOfType::operand>(ref<PrimaryType>))  //
             | create<ast::ReadonlyType>(
                   kw<"readonly"> +
                   assign<&ast::ReadonlyType::operand>(ref<PrimaryType>))  //
             | (lit<"["> +
                create<ast::TupleType>(option(
                    append<&ast::TupleType::elements>(ref<TupleElement>) +
                    many(lit<","> +
                         append<&ast::TupleType::elements>(ref<TupleElement>)) +
                    option(lit<",">))) +
                lit<"]">)  //
             | ref<MappedTypeRule>  //
             | ref<ObjectTypeRule>  //
             | ref<TypeReferenceRule>  //
             | (lit<"("> + ref<Type_> + lit<")">)> {};

  struct PrimaryType : rule<ref<PrimaryTypeBase> + many(IndexedTypeTail)> {};

  struct TypeBinExprRule
      : infix<PrimaryType, &ast::TypeBinExpr::left, &ast::TypeBinExpr::op,
              &ast::TypeBinExpr::right, (left(lit<"&">) >> left(lit<"|">))> {};

  struct FunctionTypeRule
      : rule<create<ast::FunctionType>(
            TypeParamsClauseFor<&ast::FunctionType::typeParams> +
            ParamListFor<&ast::FunctionType::params> + lit<"=>"> +
            assign<&ast::FunctionType::returnType>(ref<TypeOrPredicate>))> {};

  struct ConstructorTypeRule
      : rule<create<ast::ConstructorType>(
            option(flag<&ast::ConstructorType::isAbstract>(
                kw<"abstract">)) +
            kw<"new"> +
            TypeParamsClauseFor<&ast::ConstructorType::typeParams> +
            ParamListFor<&ast::ConstructorType::params> + lit<"=>"> +
            assign<&ast::ConstructorType::returnType>(ref<Type_>))> {};

  // The extends type of a conditional and the constraint of an `infer` hold
  // no conditional type of their own.
  struct TypeNoCond
      : rule<(option(lit<"|"> | lit<"&">) + ref<TypeBinExprRule>)  //
             | ref<FunctionTypeRule>  //
             | ref<ConstructorTypeRule>> {};

  struct ConditionalTypeRule
      : rule<create<ast::ConditionalType>(
            assign<&ast::ConditionalType::checkType>(ref<TypeBinExprRule>) +
            !LineBreak + kw<"extends"> +
            assign<&ast::ConditionalType::extendsType>(ref<TypeNoCond>) +
            lit<"?"> + assign<&ast::ConditionalType::trueType>(ref<Type_>) +
            lit<":"> +
            assign<&ast::ConditionalType::falseType>(ref<Type_>))> {};

  // A bare, self-nested generic (with an optional self-nesting before its
  // closing '>') has no coherent meaning as a type and no example needs
  // it: not ported.
  struct Type_ : rule<ref<TypeNoCond> | ref<ConditionalTypeRule>> {};

  // `x is T` and `this is T` take a whole type; `asserts x` may have none.
  struct TypePredicateRule
      : rule<create<ast::TypePredicateType>(
                 assign<&ast::TypePredicateType::param>(
                     ref<ThisTypeRule> | ref<TypeReferenceRule>) +
                 !LineBreak + kw<"is"> +
                 assign<&ast::TypePredicateType::type>(ref<Type_>))  //
             | create<ast::TypePredicateType>(
                   flag<&ast::TypePredicateType::asserts>(kw<"asserts">) +
                   !LineBreak +
                   assign<&ast::TypePredicateType::param>(
                       ref<ThisTypeRule> | ref<TypeReferenceRule>) +
                   option(!LineBreak + kw<"is"> +
                          assign<&ast::TypePredicateType::type>(
                              ref<Type_>)))> {};
  struct TypeOrPredicate : rule<ref<Type_> | ref<TypePredicateRule>> {};

  // No binary operator in a computed key: `[K in T]` is a mapped type.
  static constexpr auto TypeMemberPropName =
      (flag<&ast::TypeMember::computed>(lit<"[">) +
       assign<&ast::TypeMember::key>(ref<UnaryExpr>) + lit<"]">)  //
      | assign<&ast::TypeMember::key>(PropKey);

  struct CallSignatureRule
      : rule<create<ast::TypeMember>(
            TypeParamsClauseFor<&ast::TypeMember::typeParams> +
            flag<&ast::TypeMember::isCall>(lit<"(">) +
            ParamTailFor<&ast::TypeMember::params> +
            option(assign<&ast::TypeMember::type>(
                ref<TypeAnnotationRule>)))> {};

  struct TypeMemberRule
      : rule<create<ast::TypeMember>(  // index signature
                 option(flag<&ast::TypeMember::readonly_>(
                     kw<"readonly">)) +
                 flag<&ast::TypeMember::isIndex>(lit<"[">) +
                 assign<&ast::TypeMember::indexName>(ref<RawId> |
                                                     ContextualKeyword) +
                 lit<":"> +
                 assign<&ast::TypeMember::indexKeyType>(ref<Type_>) +
                 lit<"]"> +
                 assign<&ast::TypeMember::type>(ref<TypeAnnotationRule>))  //
             | create<ast::TypeMember>(  // construct signature
                   flag<&ast::TypeMember::isConstruct>(kw<"new">) +
                   TypeParamsClauseFor<&ast::TypeMember::typeParams> +
                   ParamListFor<&ast::TypeMember::params> +
                   option(assign<&ast::TypeMember::type>(
                       ref<TypeAnnotationRule>)))  //
             | ref<CallSignatureRule>  //
             | create<ast::TypeMember>(  // getter signature
                   flag<&ast::TypeMember::isGetter>(kw<"get">) +
                   TypeMemberPropName + lit<"("> + lit<")"> +
                   option(assign<&ast::TypeMember::type>(
                       ref<TypeAnnotationRule>)))  //
             | create<ast::TypeMember>(  // setter signature
                   flag<&ast::TypeMember::isSetter>(kw<"set">) +
                   TypeMemberPropName + lit<"("> +
                   ParamTailFor<&ast::TypeMember::params>)  //
             | create<ast::TypeMember>(  // method or property signature
                   option(flag<&ast::TypeMember::readonly_>(
                       kw<"readonly">)) +
                   TypeMemberPropName +
                   option(flag<&ast::TypeMember::optional>(lit<"?">)) +
                   ((TypeParamsClauseFor<&ast::TypeMember::typeParams> +
                     flag<&ast::TypeMember::isMethod>(lit<"(">) +
                     ParamTailFor<&ast::TypeMember::params> +
                     option(assign<&ast::TypeMember::type>(
                                ref<TypeAnnotationRule>)  //
                            | (lit<"=>"> + assign<&ast::TypeMember::type>(
                                              ref<Type_>))))  //
                    | option(assign<&ast::TypeMember::type>(
                          ref<TypeAnnotationRule>))))> {};

  // A statement ends on `;`, or at a line break, a `}` or the end of the text.
  static constexpr auto Eos = implied(lit<";">, LineBreak | &lit<"}"> | eof);

  // `declare` is a modifier only when its declaration starts on the same line.
  template <auto Ptr>
  static constexpr auto DeclareFor =
      option(flag<Ptr>(kw<"declare">) + !LineBreak);

  // -- Templates (tag stays null for the untagged use in PrimaryExpr) ----
  static constexpr auto TemplateBody =
      TemplateContents<&ast::TemplateLit::quasis,
                       &ast::TemplateLit::expressions, Expression>;
  struct TemplateLiteral : rule<create<ast::TemplateLit>(TemplateBody)> {};

  // -- Primary expressions ----
  struct ThisExpr_ : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct SuperExpr_ : rule<create<ast::SuperExpr>(kw<"super">)> {};
  struct NullLit_ : rule<create<ast::NullLit>(kw<"null">)> {};
  struct BOOL_TOK : terminal<kw<"true"> | kw<"false">, bool> {};
  struct BoolLit_ : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct RegexLit_ : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};

  struct Literal
      : rule<ref<NullLit_> | ref<BoolLit_> | ref<StringLiteral> |
             ref<TemplateLiteral> | ref<RegexLit_> | ref<BigIntLit_> |
             ref<NumberLit_>> {};

  struct ParenExpr : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct YieldExpr_
      : rule<create<ast::YieldExpr>(
            kw<"yield"> +
            option(!LineBreak +
                   option(flag<&ast::YieldExpr::delegate>(lit<"*">)) +
                   assign<&ast::YieldExpr::argument>(ref<AssignExpr>)))> {};

  // The old-style cast `<Type>expr`: its operand is a unary expression that is
  // no arrow function, so that `<T>(x) => y` can only be a generic arrow.
  struct TypeAssertionExpr_
      : rule<create<ast::TypeAssertionExpr>(
            TypeArgsClauseFor<&ast::TypeAssertionExpr::typeArgs> +
            assign<&ast::TypeAssertionExpr::expression>(ref<CastOperand>))> {};

  struct MetaProperty_
      : rule<create<ast::MetaProperty>(
            (assign<&ast::MetaProperty::meta>(kw<"new">) + lit<"."> +
             assign<&ast::MetaProperty::property>(kw<"target">))  //
            | (assign<&ast::MetaProperty::meta>(kw<"import">) + lit<"."> +
               assign<&ast::MetaProperty::property>(kw<"meta">)))> {};

  struct ImportCallExpr_
      : rule<create<ast::ImportCallExpr>(
            kw<"import"> + lit<"("> +
            assign<&ast::ImportCallExpr::source>(ref<AssignExpr>) +
            option(lit<","> +
                   option(assign<&ast::ImportCallExpr::options>(
                              ref<AssignExpr>) +
                          option(lit<",">))) +
            lit<")">)> {};

  struct PrimaryExpr
      : rule<ref<ThisExpr_> | ref<SuperExpr_> | ref<MetaProperty_> |
             ref<ImportCallExpr_> | ref<Literal> | ref<ArrayLiteral> |
             ref<ObjectLiteral> | ref<ParenExpr> | ref<ArrowFunctionExpr_> |
             ref<FunctionExpr_> | ref<ClassExpr_> | ref<TypeAssertionExpr_> |
             ref<IdentPrimary>> {};
  struct PrimaryNoArrow
      : rule<ref<ThisExpr_> | ref<SuperExpr_> | ref<MetaProperty_> |
             ref<ImportCallExpr_> | ref<Literal> | ref<ArrayLiteral> |
             ref<ObjectLiteral> | ref<ParenExpr> | ref<FunctionExpr_> |
             ref<ClassExpr_> | ref<TypeAssertionExpr_> | ref<IdentPrimary>> {};

  // -- Member / call / new chain ----
  static constexpr auto Spread = lit<"...">;
  struct SpreadArg
      : rule<create<ast::Spread>(
            Spread + assign<&ast::Spread::argument>(ref<AssignExpr>))> {};
  static constexpr auto OneArg = ref<SpreadArg> | ref<AssignExpr>;

  static constexpr auto MemberTail =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          option(flag<&ast::MemberExpr::optional>(lit<"?.">)) +
          flag<&ast::MemberExpr::computed>(lit<"[">) +
          assign<&ast::MemberExpr::property>(ref<Expression>) + lit<"]">) |
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          (flag<&ast::MemberExpr::optional>(lit<"?.">) | lit<".">) +
          assign<&ast::MemberExpr::property>(ref<MemberName>) +
          option(TypeArgsClauseFor<&ast::MemberExpr::typeArgs>));

  static constexpr auto CallArguments =
      lit<"("> +
      option(append<&ast::CallExpr::args>(OneArg) +
             many(lit<","> + append<&ast::CallExpr::args>(OneArg)) +
             option(lit<",">)) +
      lit<")">;
  static constexpr auto NewArguments =
      lit<"("> +
      option(append<&ast::NewExpr::args>(OneArg) +
             many(lit<","> + append<&ast::NewExpr::args>(OneArg)) +
             option(lit<",">)) +
      lit<")">;

  struct MemberOnlyExpr
      : rule<(ref<PrimaryExpr> | ref<NewExpr_>) + many(MemberTail)> {};

  struct NewExpr_
      : rule<create<ast::NewExpr>(
            kw<"new"> + assign<&ast::NewExpr::callee>(ref<MemberOnlyExpr>) +
            option(TypeArgsClauseFor<&ast::NewExpr::typeArgs>) +
            option(NewArguments))> {};

  static constexpr auto PostfixTail =
      MemberTail |
      nest<ast::InstantiationExpr, &ast::InstantiationExpr::expression>(
          TypeArgsClauseFor<&ast::InstantiationExpr::typeArgs> + !lit<"(">) |
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          option(flag<&ast::CallExpr::optional>(lit<"?.">)) + CallArguments) |
      nest<ast::TemplateLit, &ast::TemplateLit::tag>(TemplateBody) |
      // No line break before a postfix operator: `a \n ++b` is two statements.
      nest<ast::PostfixUpdateExpr, &ast::PostfixUpdateExpr::operand>(
          !LineBreak + assign<&ast::PostfixUpdateExpr::op>(lit<"++"> | lit<"--">)) |
      nest<ast::NonNullExpr, &ast::NonNullExpr::operand>(lit<"!", !cr<"=">>);

  struct PostfixExpr
      : rule<(ref<PrimaryExpr> | ref<NewExpr_>) + many(PostfixTail)> {};
  struct PostfixNoArrow
      : rule<(ref<PrimaryNoArrow> | ref<NewExpr_>) + many(PostfixTail)> {};

  // The identifier's generic-call suffix (a bare name applied to type
  // arguments and then called): `foo<T>(x)`, narrowed to its only
  // realistic shape.
  static constexpr auto GenericCallTail =
      nest<ast::CallExpr, &ast::CallExpr::callee>(
          TypeArgsClauseFor<&ast::CallExpr::typeArgs> + CallArguments);

  struct IdentPrimary : rule<ref<Ident> + option(GenericCallTail)> {};

  // -- Unary chain ----
  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(kw<"delete"> | kw<"void"> |
                                             kw<"typeof">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::PrefixUpdateExpr>(
                   assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
                   assign<&ast::PrefixUpdateExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(
                       lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">> | lit<"~"> |
                       lit<"!", !cr<"=">>) +
                   assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>))  //
             | create<ast::AwaitExpr>(
                   kw<"await"> +
                   assign<&ast::AwaitExpr::operand>(ref<UnaryExpr>))  //
             | ref<PostfixExpr>> {};

  struct CastOperand
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(kw<"delete"> | kw<"void"> |
                                             kw<"typeof">) +
                 assign<&ast::UnaryExpr::operand>(ref<CastOperand>))  //
             | create<ast::PrefixUpdateExpr>(
                   assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
                   assign<&ast::PrefixUpdateExpr::operand>(
                       ref<CastOperand>))  //
             | create<ast::UnaryExpr>(
                   assign<&ast::UnaryExpr::op>(
                       lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">> | lit<"~"> |
                       lit<"!", !cr<"=">>) +
                   assign<&ast::UnaryExpr::operand>(ref<CastOperand>))  //
             | create<ast::AwaitExpr>(
                   kw<"await"> +
                   assign<&ast::AwaitExpr::operand>(ref<CastOperand>))  //
             | ref<PostfixNoArrow>> {};

  struct PowerExpr
      : infix<UnaryExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, right(lit<"**", !cr<"=">>)> {};
  struct MultiplyExpr
      : infix<PowerExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(lit<"*", !cr<"*=">> | lit<"/", !cr<"=">> |
                   lit<"%", !cr<"=">>)> {};
  struct AddExpr
      : infix<MultiplyExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>)> {};
  static constexpr auto ShiftOp =
      lit<"<<", !cr<"=">> |
      (lit<">"> + gap::none + lit<">"> + gap::none + lit<">", !cr<"=">>) |
      (lit<">"> + gap::none + lit<">", !cr<"=>">>);

  struct ShiftExpr
      : infix<AddExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ShiftOp)> {};

  static constexpr auto AssertionTail =
      option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(lit<"**", !cr<"=">>) +
          assign<&ast::BinaryExpr::right>(ref<PowerExpr>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(
              lit<"*", !cr<"*=">> | lit<"/", !cr<"=">> |
              lit<"%", !cr<"=">>) +
          assign<&ast::BinaryExpr::right>(ref<PowerExpr>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(
              lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>) +
          assign<&ast::BinaryExpr::right>(ref<MultiplyExpr>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(ShiftOp) +
          assign<&ast::BinaryExpr::right>(ref<AddExpr>)));

  struct RelationalExpr
      : rule<(create<ast::AsExpr>(
                  assign<&ast::AsExpr::expression>(ref<RelationalExpr>) +
                  !LineBreak + kw<"as"> +
                  assign<&ast::AsExpr::target>(ref<Type_>)) + AssertionTail) |
             (create<ast::SatisfiesExpr>(
                  assign<&ast::SatisfiesExpr::expression>(ref<RelationalExpr>) +
                  !LineBreak + kw<"satisfies"> +
                  assign<&ast::SatisfiesExpr::target>(ref<Type_>)) + AssertionTail) |
             create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<RelationalExpr>) +
                 assign<&ast::BinaryExpr::op>(
                     lit<"<="> | lit<">="> | lit<"<", !cr<"=<">> |
                     lit<">", !cr<"=>">> | kw<"instanceof"> | kw<"in">) +
                 assign<&ast::BinaryExpr::right>(ref<ShiftExpr>)) |
             ref<ShiftExpr>> {};

  struct BinaryExpr
      : infix<RelationalExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"==="> | lit<"!=="> | lit<"==", !cr<"=">> |
                    lit<"!=", !cr<"=">>) >>
               left(lit<"&", !cr<"&=">>) >>  //
               left(lit<"^", !cr<"=">>) >>   //
               left(lit<"|", !cr<"|=">>) >>  //
               left(lit<"&&", !cr<"=">>) >>  //
               left(lit<"||", !cr<"=">>) >>  //
               left(lit<"??", !cr<"=">>))> {};

  struct ConditionalExpr
      : rule<create<ast::ConditionalExpr>(
                 assign<&ast::ConditionalExpr::test>(ref<BinaryExpr>) +
                 lit<"?", !cr<".?">> +
                 assign<&ast::ConditionalExpr::consequent>(ref<AssignExpr>) +
                 lit<":"> +
                 assign<&ast::ConditionalExpr::alternate>(ref<AssignExpr>))  //
             | ref<BinaryExpr>> {};

  struct AssignExpr
      : infix<ConditionalExpr, &ast::AssignExpr::target, &ast::AssignExpr::op,
              &ast::AssignExpr::value,
              right(lit<"=", !cr<"=>">> | lit<"*="> | lit<"/="> | lit<"%="> |
                    lit<"+="> | lit<"-="> | lit<"<<="> | lit<">>>="> |
                    lit<">>="> | lit<"&="> | lit<"^="> | lit<"|="> |
                    lit<"**="> | lit<"??="> | lit<"&&="> | lit<"||=">)> {};

  struct Expression
      : infix<AssignExpr, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(lit<",">)> {};

  // -- Functions ----
  struct FunctionExpr_
      : rule<create<ast::FunctionExpr>(
            option(flag<&ast::FunctionExpr::isAsync>(kw<"async">)) +
            kw<"function"> +
            option(flag<&ast::FunctionExpr::isGenerator>(lit<"*">)) +
            option(assign<&ast::FunctionExpr::name>(ref<RawId>)) +
            TypeParamsClauseFor<&ast::FunctionExpr::typeParams> +
            ParamListFor<&ast::FunctionExpr::params> +
            option(assign<&ast::FunctionExpr::returnType>(
                ref<TypeAnnotationRule>)) +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};

  struct ArrowSingleParam
      : rule<create<ast::Param>(assign<&ast::Param::pattern>(ref<Ident>))> {};

  static constexpr auto ArrowParams =
      ParamListFor<&ast::ArrowFunctionExpr::params>  //
      | append<&ast::ArrowFunctionExpr::params>(ref<ArrowSingleParam>);

  struct ArrowFunctionExpr_
      : rule<create<ast::ArrowFunctionExpr>(
            option(flag<&ast::ArrowFunctionExpr::isAsync>(kw<"async">)) +
            TypeParamsClauseFor<&ast::ArrowFunctionExpr::typeParams> +
            ArrowParams +
            option(assign<&ast::ArrowFunctionExpr::returnType>(
                ref<TypeAnnotationRule>)) +
            !LineBreak + lit<"=>"> +
            (assign<&ast::ArrowFunctionExpr::block>(ref<FunctionBody>)  //
             | assign<&ast::ArrowFunctionExpr::exprBody>(ref<AssignExpr>)))> {
  };

  // -- Array / object literals ----
  struct ArrayElementBeforeComma : rule<(OneArg | create<ast::Elision>)+lit<",">> {};
  struct ArrayLiteral
      : rule<create<ast::ArrayLit>(
            lit<"["> +
            many(append<&ast::ArrayLit::elements>(ref<ArrayElementBeforeComma>)) +
            option(append<&ast::ArrayLit::elements>(OneArg)) + lit<"]">)> {};

  static constexpr auto PropName =
      (flag<&ast::Prop::computed>(lit<"[">) +
       assign<&ast::Prop::key>(ref<AssignExpr>) + lit<"]">)  //
      | assign<&ast::Prop::key>(PropKey);

  struct MethodBody
      : rule<create<ast::FunctionExpr>(
            ParamListFor<&ast::FunctionExpr::params> +
            option(assign<&ast::FunctionExpr::returnType>(
                ref<TypeAnnotationRule>)) +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};
  struct GenericMethodBody
      : rule<create<ast::FunctionExpr>(
            TypeParamsClauseFor<&ast::FunctionExpr::typeParams> +
            ParamListFor<&ast::FunctionExpr::params> +
            option(assign<&ast::FunctionExpr::returnType>(
                ref<TypeAnnotationRule>)) +
            assign<&ast::FunctionExpr::body>(ref<FunctionBody>))> {};

  struct PropertyRule
      : rule<create<ast::Prop>(
            (PropName + lit<":"> +
             assign<&ast::Prop::value>(ref<AssignExpr>))  //
            | (option(flag<&ast::Prop::isAsync>(kw<"async"> + !LineBreak)) +
               option(flag<&ast::Prop::isGenerator>(lit<"*">)) +
               PropName +
               flag<&ast::Prop::isMethod>(
                   assign<&ast::Prop::value>(ref<GenericMethodBody>)))  //
            | (flag<&ast::Prop::isGetter>(kw<"get">) + PropName +
               assign<&ast::Prop::value>(ref<MethodBody>))  //
            | (flag<&ast::Prop::isSetter>(kw<"set">) + PropName +
               assign<&ast::Prop::value>(ref<MethodBody>))  //
            | assign<&ast::Prop::value>(OneArg))> {};

  struct ObjectLiteral
      : rule<create<ast::ObjectLit>(
            lit<"{"> +
            option(append<&ast::ObjectLit::properties>(ref<PropertyRule>) +
                   many(lit<","> + append<&ast::ObjectLit::properties>(
                                       ref<PropertyRule>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  // -- Decorators ---- A decorator's own expression chain is narrower than
  // a general one; reused here through the general member/call machinery,
  // which is strictly not narrower.
  static constexpr auto DecoratorMemberTail =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          lit<"."> + assign<&ast::MemberExpr::property>(ref<MemberName>));
  static constexpr auto DecoratorCallTail =
      nest<ast::CallExpr, &ast::CallExpr::callee>(CallArguments);

  struct DecoratorExpr
      : rule<lit<"@"> +
             (ref<Ident> | (lit<"("> + ref<Expression> + lit<")">)) +
             many(DecoratorMemberTail | DecoratorCallTail)> {};

  // -- Classes ----
  struct ClassMemberKeyIdent
      : rule<create<ast::Ident>(
            assign<&ast::Ident::name>(ref<PrivateOrName>))> {};
  static constexpr auto ClassMemberKey =
      (flag<&ast::ClassMember::computed>(lit<"[">) +
       assign<&ast::ClassMember::key>(ref<AssignExpr>) + lit<"]">)  //
      | assign<&ast::ClassMember::key>(
            ref<ClassMemberKeyIdent> | ref<StringLiteral> | ref<NumberLit_> |
            ref<BigIntLit_> | ref<KeywordAsKey>);

  // The modifiers in any order, an accessibility word among them.
  static constexpr auto ClassModifier =
      flag<&ast::ClassMember::isDeclare>(kw<"declare">) |
      flag<&ast::ClassMember::isStatic>(kw<"static"> + !lit<"{">) |
      flag<&ast::ClassMember::isOverride>(kw<"override">) |
      flag<&ast::ClassMember::isAccessor>(kw<"accessor">);

  static constexpr auto MemberBase =
      many(append<&ast::ClassMember::decorators>(ref<DecoratorExpr>)) +
      option((assign<&ast::ClassMember::accessibility>(AccessMod) +
              many(ClassModifier))  //
             | (some(ClassModifier) +
                option(assign<&ast::ClassMember::accessibility>(AccessMod) +
                       many(ClassModifier)))) +
      option(flag<&ast::ClassMember::isAsync>(kw<"async"> + !LineBreak)) +
      option(flag<&ast::ClassMember::isReadonly>(kw<"readonly">));

  struct ClassMemberRule
      : rule<
            create<ast::ClassMember>(  // static initialisation block
                many(append<&ast::ClassMember::decorators>(ref<DecoratorExpr>)) +
                kw<"static"> +
                assign<&ast::ClassMember::body>(ref<BlockStmt_>))  //
            | create<ast::ClassMember>(  // `abstract name(...): T;` / field
                  many(append<&ast::ClassMember::decorators>(
                      ref<DecoratorExpr>)) +
                  option(assign<&ast::ClassMember::accessibility>(AccessMod)) +
                  assign<&ast::ClassMember::abstractDecl>(
                      ref<AbstractDeclRule>))  //
            | create<ast::ClassMember>(  // method, getter or setter
                  MemberBase +
                  ((option(flag<&ast::ClassMember::isGenerator>(
                        lit<"*">)) +
                    ClassMemberKey +
                    option(flag<&ast::ClassMember::optional>(lit<"?">)) +
                    TypeParamsClauseFor<&ast::ClassMember::typeParams> +
                    flag<&ast::ClassMember::isMethod>(lit<"(">) +
                    ParamTailFor<&ast::ClassMember::params> +
                    option(assign<&ast::ClassMember::returnType>(
                        ref<TypeAnnotationRule>)) +
                    option(assign<&ast::ClassMember::body>(
                        ref<FunctionBody>)))  //
                   | (flag<&ast::ClassMember::isGetter>(kw<"get">) +
                      ClassMemberKey + lit<"("> + lit<")"> +
                      option(assign<&ast::ClassMember::returnType>(
                          ref<TypeAnnotationRule>)) +
                      option(assign<&ast::ClassMember::body>(
                          ref<FunctionBody>)))  //
                   | (flag<&ast::ClassMember::isSetter>(kw<"set">) +
                      ClassMemberKey + lit<"("> +
                      ParamTailFor<&ast::ClassMember::params> +
                      option(assign<&ast::ClassMember::body>(
                          ref<FunctionBody>)))))  //
            | (create<ast::ClassMember>(  // index signature
                   MemberBase +
                   flag<&ast::ClassMember::isIndex>(lit<"[">) +
                   assign<&ast::ClassMember::indexName>(ref<RawId> |
                                                        ContextualKeyword) +
                   lit<":"> +
                   assign<&ast::ClassMember::indexKeyType>(ref<Type_>) +
                   lit<"]"> +
                   assign<&ast::ClassMember::typeAnnotation>(
                       ref<TypeAnnotationRule>)) +
               Eos)  //
            // A field ends at `;`, a line break or `}`: without it a modifier
            // word reads as a field of its own (`readonly type: T` as two).
            | (create<ast::ClassMember>(  // a field, optionally static
                   MemberBase + ClassMemberKey +
                   option(flag<&ast::ClassMember::optional>(lit<"?">)) +
                   option(flag<&ast::ClassMember::definite>(lit<"!">)) +
                   option(assign<&ast::ClassMember::typeAnnotation>(
                       ref<TypeAnnotationRule>)) +
                   option(lit<"="> + assign<&ast::ClassMember::value>(
                                          ref<AssignExpr>))) +
               Eos)> {};

  template <auto Ptr>
  static constexpr auto ClassBodyFor =
      lit<"{"> + many(append<Ptr>(ref<ClassMemberRule>) | lit<";">) + lit<"}">;

  // A plain type reference reads as one; any other expression (a call, a
  // parenthesised cast) lands in the expression member.
  template <auto SuperPtr, auto ExprPtr, auto ImplPtr>
  static constexpr auto ClassHeritageFor =
      option(kw<"extends"> + (assign<SuperPtr>(ref<TypeReferenceRule>)  //
                              | assign<ExprPtr>(ref<PostfixExpr>))) +
      option(kw<"implements"> + append<ImplPtr>(ref<TypeReferenceRule>) +
             many(lit<","> + append<ImplPtr>(ref<TypeReferenceRule>)));

  // No decorator list here: a decorator applies to a class declaration,
  // never to the expression form.
  struct ClassExpr_
      : rule<create<ast::ClassExpr>(
            option(flag<&ast::ClassExpr::isAbstract>(kw<"abstract">)) +
            kw<"class"> + option(assign<&ast::ClassExpr::name>(ref<RawId>)) +
            TypeParamsClauseFor<&ast::ClassExpr::typeParams> +
            ClassHeritageFor<&ast::ClassExpr::superClass,
                             &ast::ClassExpr::superExpr,
                             &ast::ClassExpr::implementsClause> +
            ClassBodyFor<&ast::ClassExpr::members>)> {};

  struct ClassDeclaration
      : rule<create<ast::ClassDecl>(
            many(append<&ast::ClassDecl::decorators>(ref<DecoratorExpr>)) +
            DeclareFor<&ast::ClassDecl::isDeclare> +
            option(flag<&ast::ClassDecl::isAbstract>(kw<"abstract">)) +
            kw<"class"> + assign<&ast::ClassDecl::name>(ref<RawId>) +
            TypeParamsClauseFor<&ast::ClassDecl::typeParams> +
            ClassHeritageFor<&ast::ClassDecl::superClass,
                             &ast::ClassDecl::superExpr,
                             &ast::ClassDecl::implementsClause> +
            ClassBodyFor<&ast::ClassDecl::members>)> {};

  // -- Interfaces / type alias / enum / namespace / abstract ----
  struct InterfaceDeclaration
      : rule<create<ast::InterfaceDecl>(
            DeclareFor<&ast::InterfaceDecl::isDeclare> + kw<"interface"> +
            assign<&ast::InterfaceDecl::name>(ref<RawId>) +
            TypeParamsClauseFor<&ast::InterfaceDecl::typeParams> +
            option(kw<"extends"> +
                   append<&ast::InterfaceDecl::extendsClause>(
                       ref<TypeReferenceRule>) +
                   many(lit<","> + append<&ast::InterfaceDecl::extendsClause>(
                                        ref<TypeReferenceRule>))) +
            lit<"{"> +
            TypeMembers<&ast::InterfaceDecl::members> +
            lit<"}"> + option(lit<";">))> {};

  struct TypeAliasDeclaration
      : rule<create<ast::TypeAliasDecl>(
            DeclareFor<&ast::TypeAliasDecl::isDeclare> + kw<"type"> +
            assign<&ast::TypeAliasDecl::name>(ref<RawId>) +
            TypeParamsClauseFor<&ast::TypeAliasDecl::typeParams> + lit<"="> +
            assign<&ast::TypeAliasDecl::type>(ref<Type_>) + Eos)> {};

  struct EnumMemberRule
      : rule<create<ast::EnumMember>(
            assign<&ast::EnumMember::key>(PropKey) +
            option(lit<"="> +
                   assign<&ast::EnumMember::value>(ref<AssignExpr>)))> {};

  struct EnumDeclaration
      : rule<create<ast::EnumDecl>(
            DeclareFor<&ast::EnumDecl::isDeclare> +
            option(flag<&ast::EnumDecl::isConst>(kw<"const">)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<RawId>) + lit<"{"> +
            option(append<&ast::EnumDecl::members>(ref<EnumMemberRule>) +
                   many(lit<","> + append<&ast::EnumDecl::members>(
                                        ref<EnumMemberRule>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  static constexpr auto NamespaceBody =
      lit<"{"> + many(append<&ast::NamespaceDecl::body>(ref<Statement>)) +
      lit<"}">;

  // The name stays on the keyword's line: `module` and `namespace` are plain
  // names too.
  struct NamespaceDeclaration
      : rule<create<ast::NamespaceDecl>(
            DeclareFor<&ast::NamespaceDecl::isDeclare> +
            ((assign<&ast::NamespaceDecl::kind>(kw<"namespace">) +
              !LineBreak +
              assign<&ast::NamespaceDecl::name>(ref<QualifiedName>) +
              NamespaceBody)  //
             | (assign<&ast::NamespaceDecl::kind>(kw<"module">) + !LineBreak +
                ((assign<&ast::NamespaceDecl::name>(ref<QualifiedName>) +
                  NamespaceBody)  //
                 | (assign<&ast::NamespaceDecl::name>(ref<STRING>) +
                    (NamespaceBody | Eos))))  //
             | (assign<&ast::NamespaceDecl::kind>(kw<"global">) +
                NamespaceBody)))> {};

  struct NamespaceExportDeclaration
      : rule<create<ast::NamespaceExportDecl>(
            kw<"export"> + kw<"as"> + kw<"namespace"> +
            assign<&ast::NamespaceExportDecl::name>(ref<RawId>) + Eos)> {};

  // `abstract name: Type;`: a bare declarator, no `var`/`let`/`const` --
  // kept narrow to this one caller instead of widening `VariableStatement`
  // itself, which would put a bare identifier back at the head of every
  // statement position.
  struct AbstractDeclarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<Assignable>) +
            option(flag<&ast::Declarator::optional>(lit<"?">)) +
            option(assign<&ast::Declarator::typeAnnotation>(
                ref<TypeAnnotationRule>)) +
            option(lit<"="> +
                   assign<&ast::Declarator::init>(ref<AssignExpr>)))> {};

  struct AbstractFieldDecl
      : rule<create<ast::VarDecl>(
            option(flag<&ast::VarDecl::isReadonly>(kw<"readonly">)) +
            append<&ast::VarDecl::declarations>(ref<AbstractDeclarator_>))> {};

  struct AbstractDeclRule
      : rule<create<ast::AbstractDecl>(
                 kw<"abstract"> +
                 assign<&ast::AbstractDecl::name>(ref<RawId> | AllKeywords) +
                 TypeParamsClauseFor<&ast::AbstractDecl::typeParams> +
                 lit<"("> + ParamTailFor<&ast::AbstractDecl::params> +
                 option(assign<&ast::AbstractDecl::returnType>(
                     ref<TypeAnnotationRule>)) +
                 Eos)  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   flag<&ast::AbstractDecl::isGetter>(kw<"get">) +
                   assign<&ast::AbstractDecl::name>(ref<RawId> | AllKeywords) +
                   lit<"("> + lit<")"> +
                   option(assign<&ast::AbstractDecl::returnType>(
                       ref<TypeAnnotationRule>)) +
                   Eos)  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   flag<&ast::AbstractDecl::isSetter>(kw<"set">) +
                   assign<&ast::AbstractDecl::name>(ref<RawId> | AllKeywords) +
                   lit<"("> + ParamTailFor<&ast::AbstractDecl::params> +
                   Eos)  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   assign<&ast::AbstractDecl::declaration>(
                       ref<VariableStatement>))  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   assign<&ast::AbstractDecl::declaration>(
                       ref<AbstractFieldDecl>) +
                   Eos)> {};

  // -- Assignable (var/let/const target, parameter, for-in/of target) ----
  // `AllKeywords`, not only `Ident`: a keyword is a legal binding name too
  // (`const function = ...`), spelled once through the same `KeywordAsKey`
  // sibling `PropKey`/`ClassMemberKey` already use.
  struct Assignable
      : rule<ref<Ident> | ref<ArrayLiteral> | ref<ObjectLiteral> |
             ref<KeywordAsKey>> {};

  // -- Statements ----

  struct Declarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<Assignable>) +
            option(flag<&ast::Declarator::definite>(lit<"!", !cr<"=">>)) +
            option(assign<&ast::Declarator::typeAnnotation>(
                ref<TypeAnnotationRule>)) +
            option(lit<"="> +
                   assign<&ast::Declarator::init>(ref<AssignExpr>)))> {};

  static constexpr auto VarKind = kw<"var"> | kw<"let"> | kw<"const">;

  struct VarDeclList
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            append<&ast::VarDecl::declarations>(ref<Declarator_>) +
            many(lit<","> +
                 append<&ast::VarDecl::declarations>(ref<Declarator_>)))> {};

  struct BareDeclarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<ArrayLiteral> | ref<ObjectLiteral>) +
            option(assign<&ast::Declarator::typeAnnotation>(
                ref<TypeAnnotationRule>)) +
            lit<"="> +
            assign<&ast::Declarator::init>(ref<AssignExpr>))> {};

  // A statement that opens on `{` is a block: an object destructuring
  // assignment there is written in parentheses, as an expression.
  struct VariableStatement
      : rule<(create<ast::VarDecl>(
                  !lit<"{"> +
                  append<&ast::VarDecl::declarations>(ref<BareDeclarator_>)) +
              Eos)  //
             | (create<ast::VarDecl>(
                    option(assign<&ast::VarDecl::accessibility>(AccessMod)) +
                    assign<&ast::VarDecl::kind>(VarKind) +
                    option(flag<&ast::VarDecl::isReadonly>(
                        kw<"readonly">)) +
                    append<&ast::VarDecl::declarations>(ref<Declarator_>) +
                    many(lit<","> + append<&ast::VarDecl::declarations>(
                                         ref<Declarator_>))) +
                Eos)  //
             | (create<ast::VarDecl>(
                    flag<&ast::VarDecl::isDeclare>(kw<"declare">) +
                    !LineBreak + assign<&ast::VarDecl::kind>(VarKind) +
                    append<&ast::VarDecl::declarations>(ref<Declarator_>) +
                    many(lit<","> + append<&ast::VarDecl::declarations>(
                                         ref<Declarator_>))) +
                Eos)> {};

  struct ExpressionStatement
      : rule<create<ast::ExprStmt>(
            !lit<"{"> + !kw<"class"> + !kw<"function"> + !kw<"interface"> +
            !kw<"abstract"> +
            assign<&ast::ExprStmt::expression>(ref<Expression>) + Eos)> {};

  struct EmptyStatement : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct BlockStmt_
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<Statement>)) +
            lit<"}">)> {};

  struct FunctionBody
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<Statement>)) +
            lit<"}">)> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> +
            assign<&ast::IfStmt::test>(ref<Expression>) + lit<")"> +
            assign<&ast::IfStmt::consequent>(ref<Statement>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::alternate>(ref<Statement>))))> {};

  struct DoWhileStatement
      : rule<create<ast::DoWhileStmt>(
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<Statement>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhileStmt::test>(ref<Expression>) + lit<")"> +
            option(lit<";">))> {};

  struct WhileStatement
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::test>(ref<Expression>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct ForClassicStatement
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::initDecl>(ref<VarDeclList>) |
                   assign<&ast::ForStmt::initExpr>(ref<Expression>)) +
            lit<";"> + option(assign<&ast::ForStmt::test>(ref<Expression>)) +
            lit<";"> +
            option(assign<&ast::ForStmt::update>(ref<Expression>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<Statement>))> {};

  // Shares `Declarator_` (not a narrower binding-only rule) with
  // `VarDeclList`: the common prefix through the type annotation put both
  // in one merged LALR state, and a separate rule stopping short of
  // `Declarator_`'s optional `= init` lost the reduce to it there. The
  // over-acceptance (an initializer on a for-of/for-in binding) is free.
  struct ForBindingDecl
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            append<&ast::VarDecl::declarations>(ref<Declarator_>))> {};

  struct ForInStatement
      : rule<create<ast::ForInStmt>(
            kw<"for"> + lit<"("> +
            (assign<&ast::ForInStmt::leftDecl>(ref<ForBindingDecl>)  //
             | assign<&ast::ForInStmt::leftExpr>(ref<UnaryExpr>)) +
            kw<"in"> + assign<&ast::ForInStmt::right>(ref<Expression>) +
            lit<")"> + assign<&ast::ForInStmt::body>(ref<Statement>))> {};

  struct ForOfStatement
      : rule<create<ast::ForOfStmt>(
            kw<"for"> +
            option(flag<&ast::ForOfStmt::isAwait>(kw<"await">)) +
            lit<"("> +
            (assign<&ast::ForOfStmt::leftDecl>(ref<ForBindingDecl>)  //
             | assign<&ast::ForOfStmt::leftExpr>(ref<UnaryExpr>)) +
            kw<"of"> + assign<&ast::ForOfStmt::right>(ref<AssignExpr>) +
            option(kw<"as"> + assign<&ast::ForOfStmt::asType>(ref<Type_>)) +
            lit<")"> + assign<&ast::ForOfStmt::body>(ref<Statement>))> {};

  struct IterationStatement
      : rule<ref<DoWhileStatement> | ref<WhileStatement> |
             ref<ForOfStatement> | ref<ForInStatement> |
             ref<ForClassicStatement>> {};

  struct ContinueStatement
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(!LineBreak + assign<&ast::ContinueStmt::label>(
                                    ref<PlainId> | ContextualKeyword)) +
            Eos)> {};

  struct BreakStatement
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(!LineBreak + assign<&ast::BreakStmt::label>(
                                    ref<PlainId> | ContextualKeyword)) +
            Eos)> {};

  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(!LineBreak +
                   assign<&ast::ReturnStmt::argument>(ref<Expression>)) +
            Eos)> {};

  struct YieldStatement
      : rule<create<ast::ExprStmt>(
            assign<&ast::ExprStmt::expression>(ref<YieldExpr_>) + Eos)> {};

  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + !LineBreak +
            assign<&ast::ThrowStmt::argument>(ref<Expression>) + Eos)> {};

  struct WithStatement
      : rule<create<ast::WithStmt>(
            kw<"with"> + lit<"("> +
            assign<&ast::WithStmt::object>(ref<Expression>) + lit<")"> +
            assign<&ast::WithStmt::body>(ref<Statement>))> {};

  struct LabelledStatement
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(ref<PlainId> | ContextualKeyword) +
            lit<":"> +
            assign<&ast::LabeledStmt::body>(ref<Statement>))> {};

  struct CaseClause
      : rule<create<ast::SwitchCase>(
            kw<"case"> + assign<&ast::SwitchCase::test>(ref<Expression>) +
            lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};
  struct DefaultClause
      : rule<create<ast::SwitchCase>(
            kw<"default"> + lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<Statement>)))> {};

  struct SwitchStatement
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + lit<"("> +
            assign<&ast::SwitchStmt::discriminant>(ref<Expression>) +
            lit<")"> + lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) +
            option(append<&ast::SwitchStmt::cases>(ref<DefaultClause>) +
                   many(append<&ast::SwitchStmt::cases>(ref<CaseClause>))) +
            lit<"}">)> {};

  struct DebuggerStatement
      : rule<create<ast::DebuggerStmt>(kw<"debugger"> + Eos)> {};

  struct CatchClause_
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   assign<&ast::CatchClause::param>(ref<Assignable>) +
                   option(assign<&ast::CatchClause::typeAnnotation>(
                       ref<TypeAnnotationRule>)) +
                   lit<")">) +
            assign<&ast::CatchClause::body>(ref<BlockStmt_>))> {};

  struct TryStatement
      : rule<create<ast::TryStmt>(
            kw<"try"> + assign<&ast::TryStmt::block>(ref<BlockStmt_>) +
            ((assign<&ast::TryStmt::handler>(ref<CatchClause_>) +
              option(kw<"finally"> + assign<&ast::TryStmt::finalizer>(
                                          ref<BlockStmt_>)))  //
             | (kw<"finally"> +
                assign<&ast::TryStmt::finalizer>(ref<BlockStmt_>))))> {};

  struct FunctionDeclaration
      : rule<create<ast::FunctionDecl>(
            DeclareFor<&ast::FunctionDecl::isDeclare> +
            option(flag<&ast::FunctionDecl::isAsync>(kw<"async">)) +
            kw<"function"> +
            option(flag<&ast::FunctionDecl::isGenerator>(lit<"*">)) +
            assign<&ast::FunctionDecl::name>(ref<RawId>) +
            TypeParamsClauseFor<&ast::FunctionDecl::typeParams> +
            ParamListFor<&ast::FunctionDecl::params> +
            option(assign<&ast::FunctionDecl::returnType>(
                ref<TypeAnnotationRule>)) +
            (assign<&ast::FunctionDecl::body>(ref<FunctionBody>) | Eos))> {};

  struct Statement
      : rule<ref<BlockStmt_> | ref<VariableStatement> | ref<ImportStatement> |
             ref<ImportEqualsStatement> | ref<NamespaceExportDeclaration> |
             ref<ExportStatement> | ref<EmptyStatement> |
             ref<ClassDeclaration> | ref<FunctionDeclaration> |
             ref<InterfaceDeclaration> | ref<IfStatement> |
             ref<IterationStatement> | ref<ReturnStatement> |
             ref<BreakStatement> | ref<ContinueStatement> |
             ref<SwitchStatement> | ref<ThrowStatement> |
             ref<TypeAliasDeclaration> | ref<EnumDeclaration> |
             ref<NamespaceDeclaration> | ref<WithStatement> |
             ref<DebuggerStatement> | ref<LabelledStatement> |
             ref<TryStatement> | ref<YieldStatement> |
             ref<AbstractDeclRule> | ref<ExpressionStatement>> {};

  // -- Modules ----
  struct ModuleExportName : rule<ref<RawId> | AllKeywords | ref<STRING>> {};

  struct ImportAttributeRule
      : rule<create<ast::ImportAttribute>(
            assign<&ast::ImportAttribute::key>(ref<RawId> | AllKeywords |
                                               ref<STRING>) +
            lit<":"> + assign<&ast::ImportAttribute::value>(ref<STRING>))> {};

  template <auto Ptr>
  static constexpr auto ImportAttributesFor =
      option(kw<"with"> + lit<"{"> +
             option(append<Ptr>(ref<ImportAttributeRule>) +
                    many(lit<","> + append<Ptr>(ref<ImportAttributeRule>)) +
                    option(lit<",">)) +
             lit<"}">);

  // `type` leads a specifier as a modifier or as the name itself: the words
  // after it say which (`type`, `type as`, `type as as`, `type as as as`).
  struct ImportSpecifierRule
      : rule<create<ast::ImportSpecifier>(
            option(flag<&ast::ImportSpecifier::isTypeOnly>(kw<"type">)) +
            assign<&ast::ImportSpecifier::imported>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ImportSpecifier::local>(
                                   ref<RawId> | AllKeywords)))> {};

  static constexpr auto ImportNamedList =
      lit<"{"> +
      option(append<&ast::ImportStmt::specifiers>(ref<ImportSpecifierRule>) +
             many(lit<","> + append<&ast::ImportStmt::specifiers>(
                                  ref<ImportSpecifierRule>)) +
             option(lit<",">)) +
      lit<"}">;

  struct ImportNamespaceRule
      : rule<create<ast::ImportSpecifier>(
            (flag<&ast::ImportSpecifier::isNamespace>(lit<"*">) +
             option(kw<"as"> +
                    assign<&ast::ImportSpecifier::local>(ref<RawId>)))  //
            | assign<&ast::ImportSpecifier::local>(
                ref<RawId> | ContextualKeyword))> {};

  struct ImportDefaultRule
      : rule<create<ast::ImportSpecifier>(
            assign<&ast::ImportSpecifier::local>(ref<RawId> |
                                                 ContextualKeyword))> {};

  struct ImportStatement
      : rule<create<ast::ImportStmt>(
            kw<"import"> +
            option(flag<&ast::ImportStmt::isTypeOnly>(kw<"type"> +
                                                           !kw<"from">)) +
            ((option(append<&ast::ImportStmt::specifiers>(
                         ref<ImportDefaultRule>) +
                     lit<",">) +
              (append<&ast::ImportStmt::specifiers>(ref<ImportNamespaceRule>)  //
               | ImportNamedList) +
              kw<"from"> + assign<&ast::ImportStmt::source>(ref<STRING>) +
              ImportAttributesFor<&ast::ImportStmt::attributes> + Eos)  //
             | (assign<&ast::ImportStmt::source>(ref<STRING>) +
                ImportAttributesFor<&ast::ImportStmt::attributes> + Eos)))> {};

  struct ImportEqualsStatement
      : rule<create<ast::ImportEqualsDecl>(
            kw<"import"> +
            option(flag<&ast::ImportEqualsDecl::isTypeOnly>(
                kw<"type">)) +
            assign<&ast::ImportEqualsDecl::name>(ref<RawId> |
                                                 ContextualKeyword) +
            lit<"="> +
            ((kw<"require"> + lit<"("> +
              assign<&ast::ImportEqualsDecl::source>(ref<STRING>) +
              lit<")">)  //
             | assign<&ast::ImportEqualsDecl::entity>(ref<QualifiedName>)) +
            Eos)> {};

  struct ExportSpecifierRule
      : rule<create<ast::ExportSpecifier>(
            option(flag<&ast::ExportSpecifier::isTypeOnly>(kw<"type">)) +
            assign<&ast::ExportSpecifier::local>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ExportSpecifier::exported>(
                                   ref<ModuleExportName>)))> {};

  static constexpr auto ExportNamedList =
      lit<"{"> +
      option(append<&ast::ExportStmt::specifiers>(ref<ExportSpecifierRule>) +
             many(lit<","> + append<&ast::ExportStmt::specifiers>(
                                  ref<ExportSpecifierRule>)) +
             option(lit<",">)) +
      lit<"}">;

  struct ExportStarSpecifier
      : rule<create<ast::ExportSpecifier>(
            assign<&ast::ExportSpecifier::exported>(ref<ModuleExportName>))> {
  };

  // `export default function (): T;`, an overload with no body.
  struct DefaultFunctionSignature
      : rule<create<ast::FunctionDecl>(
            option(flag<&ast::FunctionDecl::isAsync>(kw<"async">)) +
            kw<"function"> +
            option(flag<&ast::FunctionDecl::isGenerator>(lit<"*">)) +
            TypeParamsClauseFor<&ast::FunctionDecl::typeParams> +
            ParamListFor<&ast::FunctionDecl::params> +
            option(assign<&ast::FunctionDecl::returnType>(
                ref<TypeAnnotationRule>)) +
            Eos)> {};

  // Every declaration kind may be exported: var/class/function plus
  // interface/type/enum/namespace.
  struct ExportDeclaration_
      : rule<ref<VariableStatement> | ref<ClassDeclaration> |
             ref<FunctionDeclaration> | ref<InterfaceDeclaration> |
             ref<TypeAliasDeclaration> | ref<EnumDeclaration> |
             ref<NamespaceDeclaration> | ref<ImportEqualsStatement>> {};

  struct ExportStatement
      : rule<create<ast::ExportStmt>(
            many(append<&ast::ExportStmt::decorators>(ref<DecoratorExpr>)) +
            kw<"export"> +
            ((flag<&ast::ExportStmt::isDefault>(kw<"default">) +
              (assign<&ast::ExportStmt::declaration>(ref<ExportDeclaration_>)  //
               | assign<&ast::ExportStmt::declaration>(
                     ref<DefaultFunctionSignature>)  //
               | (assign<&ast::ExportStmt::defaultExpr>(ref<AssignExpr>) +
                  Eos)))  //
             | (flag<&ast::ExportStmt::isEquals>(lit<"=">) +
                assign<&ast::ExportStmt::defaultExpr>(ref<AssignExpr>) +
                Eos)  //
             | (option(flag<&ast::ExportStmt::isTypeOnly>(kw<"type">)) +
                flag<&ast::ExportStmt::isNamespace>(lit<"*">) +
                option(kw<"as"> + append<&ast::ExportStmt::specifiers>(
                                       ref<ExportStarSpecifier>)) +
                kw<"from"> + assign<&ast::ExportStmt::source>(ref<STRING>) +
                ImportAttributesFor<&ast::ExportStmt::attributes> + Eos)  //
             | (option(flag<&ast::ExportStmt::isTypeOnly>(kw<"type">)) +
                ExportNamedList +
                option(kw<"from"> +
                       assign<&ast::ExportStmt::source>(ref<STRING>) +
                       ImportAttributesFor<&ast::ExportStmt::attributes>) +
                Eos)  //
             | assign<&ast::ExportStmt::declaration>(ref<ExportDeclaration_>)))> {
  };

  struct Program_
      : rule<create<ast::Program>(
            option(assign<&ast::Program::prologue>(ref<PROLOGUE>)) +
            many(append<&ast::Program::body>(ref<Statement>)))> {};

  using entry = Program_;
  using ignore = WS;
  // A keyword may not run into a name's byte, past ASCII included.
  static constexpr auto word_boundary = IdentifierContinue | cr<R"(\\)">;
  using hidden = list<SL_COMMENT, ML_COMMENT, HTML_COMMENT, CDATA_COMMENT>;
  // Soft keywords (`ContextualKeyword`, `get`/`set` included) are spelled
  // once, referenced both as the keyword and, through `AllKeywords`, as a
  // name, so a state that reads both decides by what follows.
};

}  // namespace typescript
