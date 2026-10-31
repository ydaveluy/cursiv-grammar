// The typescript language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "typescript/core/IdentifierValue.hpp"
#include "typescript/core/ast.hpp"

namespace typescript {

inline constexpr auto template_open_text = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::string_view>;
  if (text.empty() || text.front() != '`')
    return Result::err("A template opening fragment needs its backtick.");
  for (std::size_t i = 1; i < text.size(); ++i) {
    if (text[i] == '\\') {
      if (++i == text.size())
        return Result::err("Incomplete template escape.");
    } else if (text[i] == '`' || (text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{')) {
      return Result::err("A template opening fragment stops before its next delimiter.");
    }
  }
  return Result::ok(text.substr(1));
};

struct TypeScriptLexical : cursiv::GrammarSpec {
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
  struct RawId : terminal<as<validate_identifier>(IdChars)> {};

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
  static constexpr auto ContextualKeywordWithoutYield =
      kw<"async"> | kw<"as"> | kw<"from"> | kw<"of"> |
      kw<"any"> | kw<"number"> | kw<"boolean"> | kw<"string"> |
      kw<"unique"> | kw<"symbol"> | kw<"never"> | kw<"undefined"> |
      kw<"object"> | kw<"keyof"> | kw<"type"> | kw<"constructor"> |
      kw<"namespace"> | kw<"abstract"> | kw<"module"> | kw<"get"> |
      kw<"set"> | kw<"readonly"> | kw<"is"> | kw<"declare"> | kw<"asserts"> |
      kw<"infer"> | kw<"satisfies"> | kw<"global"> | kw<"override"> |
      kw<"accessor"> | kw<"out"> | kw<"using">;
  static constexpr auto ContextualKeyword = ContextualKeywordWithoutYield | kw<"yield">;
  static constexpr auto AllKeywords = StrictReserved | ContextualKeyword;

  struct PlainId : terminal<as<validate_identifier>(!AllKeywords + IdChars)> {};
  struct AnyName : rule<ref<RawId> | AllKeywords> {};
  struct ContextualName : rule<ref<RawId> | ContextualKeyword> {};
  struct IdentName : rule<ref<PlainId> | ContextualKeyword> {};
  struct GeneratorIdentName : rule<ref<PlainId> | ContextualKeywordWithoutYield> {};
  struct GeneratorIdent : rule<assign<&ast::Ident::name>(ref<GeneratorIdentName>)> {};
  struct NonYieldRawId : terminal<as<validate_identifier>(!kw<"yield"> + IdChars)> {};
  struct GeneratorBindingName : rule<ref<NonYieldRawId> | StrictReserved | ContextualKeywordWithoutYield> {};
  struct KeywordName : rule<AllKeywords> {};
  struct Ident : rule<assign<&ast::Ident::name>(ref<IdentName>)> {};
  // Any word, keywords included: `.foo` and `.#foo` both take one.
  struct PrivateOrName : terminal<as<validate_identifier>(option(lit<"#">) + IdChars)> {};
  struct PrivateId : terminal<as<validate_identifier>(lit<"#"> + IdChars)> {};
  struct PropertyName : rule<ref<PrivateOrName> | AllKeywords> {};
  struct MemberName : rule<assign<&ast::Ident::name>(ref<PropertyName>)> {};

  // A type name, a namespace name, or a `typeof` query, folded into one:
  // any word (keyword or not), joined by one or more dots.
  struct QualifiedName
      : rule<(ref<RawId> | AllKeywords) +
             many(lit<"."> + (ref<RawId> | AllKeywords))> {};

  // The first word of a type name is never `this`: that is the `this` type.
  struct TypeRefName
      : rule<(ref<RawId> | StrictReservedButThis | ContextualKeyword) +
             many(lit<"."> + (ref<RawId> | AllKeywords))> {};

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

  struct ImportKey : rule<ref<AnyName> | ref<STRING>> {};

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
      : terminal<as<template_open_text>(lit<"`"> + many(TemplateChar))> {};
  struct TplText : terminal<some(TemplateChar)> {};
  struct TplQuasi : rule<option(ref<TplText>)> {};

  template <auto Quasis, auto Values, class ValueRule>
  static constexpr auto TemplateContents =
      append<Quasis>(ref<TplOpen>) +
      tight(many(&lit<"${"> +
                skip(ignore<WS>,
                     hide<SL_COMMENT, ML_COMMENT, HTML_COMMENT, CDATA_COMMENT>,
                     lit<"${"> + append<Values>(ref<ValueRule>) + lit<"}">) +
                append<Quasis>(ref<TplQuasi>)) +
               lit<"`">);


  // -- Keys shared by object/class/type/enum members ---- `PrivateOrName`, not
  // `RawId`: a property/type-member/enum-member name may spell any keyword
  // (`class: string;` inside an interface body, e.g.) the same way a class
  // member's own key already could.
  struct PropKeyIdent
      : rule<assign<&ast::Ident::name>(ref<PrivateOrName>)> {};
  struct NumberLit_ : rule<assign<&ast::NumberLit::text>(ref<NUMBER>)> {};
  struct BigIntLit_ : rule<assign<&ast::BigIntLit::text>(ref<BIGINT>)> {};
  struct StringLiteral : rule<assign<&ast::StringLit::text>(ref<STRING>)> {};
  // `AllKeywords` stands beside `PropKeyIdent` and not only inside it: a
  // keyword-shaped word offered only through a general terminal loses to
  // the specific keyword token wherever both are reachable (`class: string`
  // as a type/enum/object member) -- naming it here directly admits it.
  struct KeywordAsKey
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<KeywordName>))> {};
  static constexpr auto PropKey =
      ref<PropKeyIdent> | ref<StringLiteral> | ref<NumberLit_> |
      ref<BigIntLit_> | ref<KeywordAsKey>;

  static constexpr auto AccessMod =
      kw<"public"> | kw<"private"> | kw<"protected">;

  // A statement ends on `;`, or at a line break, a `}` or the end of the text.
  static constexpr auto Eos = implied(lit<";">, LineBreak | &lit<"}"> | eof);

  // `declare` is a modifier only when its declaration starts on the same line.
  template <auto Ptr>
  static constexpr auto DeclareFor =
      option(flag<Ptr>(kw<"declare">) + !LineBreak);

  struct ThisExpr_ : rule<create<ast::ThisExpr>(kw<"this">)> {};
  struct SuperExpr_ : rule<create<ast::SuperExpr>(kw<"super">)> {};
  struct NullLit_ : rule<create<ast::NullLit>(kw<"null">)> {};
  struct BOOL_TOK : terminal<as<bool>(kw<"true"> | kw<"false">)> {};
  struct BoolLit_ : rule<assign<&ast::BoolLit::value>(ref<BOOL_TOK>)> {};
  struct RegexLit_ : rule<assign<&ast::RegexLit::text>(ref<REGEX>)> {};

  struct MetaProperty_
      : rule<create<ast::MetaProperty>(
            (assign<&ast::MetaProperty::meta>(kw<"new">) + lit<"."> +
             assign<&ast::MetaProperty::property>(kw<"target">))  //
            | (assign<&ast::MetaProperty::meta>(kw<"import">) + lit<"."> +
               assign<&ast::MetaProperty::property>(kw<"meta">)))> {};

  struct EmptyStatement : rule<create<ast::EmptyStmt>(lit<";">)> {};

  struct DebuggerStatement
      : rule<create<ast::DebuggerStmt>(kw<"debugger"> + Eos)> {};

  struct ModuleExportName : rule<ref<AnyName> | ref<STRING>> {};

  struct ImportAttribute
      : rule<create<ast::ImportAttribute>(
            assign<&ast::ImportAttribute::key>(ref<ImportKey>) +
            lit<":"> + assign<&ast::ImportAttribute::value>(ref<STRING>))> {};

  template <auto Ptr>
  static constexpr auto ImportAttributesFor =
      option(kw<"with"> + lit<"{"> +
             option(some(append<Ptr>(ref<ImportAttribute>), lit<",">) +
                    option(lit<",">)) +
             lit<"}">);

  struct ExportSpecifier
      : rule<create<ast::ExportSpecifier>(
            option(flag<&ast::ExportSpecifier::isTypeOnly>(kw<"type">)) +
            assign<&ast::ExportSpecifier::local>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ExportSpecifier::exported>(
                                   ref<ModuleExportName>)))> {};

  static constexpr auto ExportNamedList =
      lit<"{"> +
      option(append<&ast::ExportStmt::specifiers>(ref<ExportSpecifier>) +
             many(lit<","> + append<&ast::ExportStmt::specifiers>(
                                  ref<ExportSpecifier>)) +
             option(lit<",">)) +
      lit<"}">;

  struct ExportStarSpecifier
      : rule<assign<&ast::ExportSpecifier::exported>(ref<ModuleExportName>)> {
  };

};

// Named bridges keep the two context graphs and the shared ordinary type graph
// independent of C++ class-body/auto initialization order.
template <bool Generator> struct TypeScriptContext;
template <bool Generator> struct ContextParam;
template <bool Generator> struct ContextExpression;
template <bool Generator> struct ContextUnary;
template <bool Generator> struct ContextStatement;
template <bool Generator> struct ContextFunctionBody;
template <bool Generator> struct ContextBlock;
template <bool Generator> struct ContextAssign;
template <bool Generator> struct ContextNoInAssign;
template <bool Generator> struct ContextMethod;
struct SharedType;
struct SharedTypeAnnotation;
struct SharedTypeReference;
struct SharedTypeMember;
struct SharedTypeParam;

template <bool GeneratorContext>
struct TypeScriptContext : TypeScriptLexical {
  // -- Forward declarations ----
  struct ImportStatement;
  struct ImportEqualsStatement;
  struct NamespaceExportDeclaration;
  struct Expression;
  struct AssignExpr;
  struct ConditionalExpr;
  struct RelationalExpr;
  struct BinaryExpr;
  struct UnaryExpr;
  struct PostfixExpr;
  struct PrimaryExpr;
  struct NewExpr_;
  struct FunctionExpr_;
  struct ArrowFunctionExpr_;
  struct NoInArrowFunctionExpr;
  struct NoInAssignExpr;
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
  struct ClassMember;
  struct TypeMember;
  struct CallSignature;
  struct AbstractDecl;
  struct AbstractFieldDecl;
  struct VariableStatement;
  struct Declarator_;
  struct ExportStatement;
  struct NamespaceDeclaration;
  struct InterfaceDeclaration;
  struct TypeAliasDeclaration;
  struct EnumDeclaration;
  struct ClassDeclaration;
  struct FunctionDeclaration;
  struct TypeParam;

  // -- Params (shared by every parameter list the grammar has) ----
  struct TypeAnnotation : rule<lit<":"> + ref<TypeOrPredicate>> {};

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
                     ref<SharedTypeAnnotation>)))  //
             | create<ast::Param>(
                   many(append<&ast::Param::decorators>(ref<DecoratorExpr>)) +
                   option(assign<&ast::Param::accessibility>(AccessMod)) +
                   option(flag<&ast::Param::isOverride>(kw<"override">)) +
                   option(flag<&ast::Param::isReadonly>(kw<"readonly">)) +
                   assign<&ast::Param::pattern>(ref<Assignable>) +
                   option(flag<&ast::Param::optional>(lit<"?">)) +
                   option(assign<&ast::Param::typeAnnotation>(
                       ref<SharedTypeAnnotation>)) +
                   option(lit<"="> + assign<&ast::Param::defaultValue>(
                                          ref<ContextAssign<GeneratorContext>>)))> {};

  template <auto Ptr, bool Generator = GeneratorContext>
  static constexpr auto ParamTailFor =
      option(some(append<Ptr>(ref<ContextParam<Generator>>), lit<",">) +
             option(lit<",">)) +
      lit<")">;
  template <auto Ptr, bool Generator = GeneratorContext>
  static constexpr auto ParamListFor = lit<"("> + ParamTailFor<Ptr, Generator>;

  template <auto Ptr>
  static constexpr auto TypeParamsClauseFor =
      option(lit<"<"> +
             option(some(append<Ptr>(ref<SharedTypeParam>), lit<",">) +
                    option(lit<",">)) +
             lit<">">);
  template <auto Ptr>
  static constexpr auto TypeArgsClauseFor =
      lit<"<"> +
      option(some(append<Ptr>(ref<SharedType>), lit<",">)) +
      lit<">">;

  // -- Types ----
  struct TypeParam
      : rule<create<ast::TypeParam>(
            many(flag<&ast::TypeParam::isConst>(kw<"const">) |
                 flag<&ast::TypeParam::isIn>(kw<"in">) |
                 flag<&ast::TypeParam::isOut>(kw<"out">)) +
            assign<&ast::TypeParam::name>(ref<ContextualName>) +
            option(kw<"extends"> +
                   assign<&ast::TypeParam::constraint>(ref<SharedType>)) +
            option(lit<"="> +
                   assign<&ast::TypeParam::defaultType>(ref<SharedType>)))> {};

  struct PredefinedType
      : rule<create<ast::PredefinedType>(
            option(flag<&ast::PredefinedType::unique>(kw<"unique">)) +
            assign<&ast::PredefinedType::name>(
                kw<"any"> | kw<"number"> | ref<NUMBER> | ref<BIGINT> |
                ref<NEGATIVE_LITERAL> | kw<"boolean"> | kw<"true"> | kw<"false"> |
                kw<"string"> | ref<STRING> | kw<"symbol"> | kw<"never"> |
                kw<"undefined"> | kw<"object"> | kw<"void"> | kw<"null">))> {};

  struct TypeReference
      : rule<create<ast::TypeRef>(
            assign<&ast::TypeRef::name>(ref<TypeRefName>) +
            option(TypeArgsClauseFor<&ast::TypeRef::typeArgs>))> {};

  struct TypeQuery
      : rule<create<ast::TypeQuery>(
            kw<"typeof"> + assign<&ast::TypeQuery::name>(ref<QualifiedName>) +
            option(TypeArgsClauseFor<&ast::TypeQuery::typeArgs>))> {};

  static constexpr auto TypeMemberEnd = lit<";"> | lit<",">;
  template <auto Ptr>
  static constexpr auto TypeMembers =
      many(append<Ptr>(ref<SharedTypeMember>) +
           option(TypeMemberEnd));

  struct ObjectType
      : rule<create<ast::ObjectType>(
            lit<"{"> +
            TypeMembers<&ast::ObjectType::members> +
            lit<"}">)> {};

  struct SignedReadonly : rule<(lit<"+"> | lit<"-">) + kw<"readonly">> {};
  struct SignedOptional : rule<(lit<"+"> | lit<"-">) + lit<"?">> {};

  struct MappedType
      : rule<create<ast::MappedType>(
            lit<"{"> +
            option(assign<&ast::MappedType::readonlyMod>(
                kw<"readonly"> | ref<SignedReadonly>)) +
            lit<"["> +
            assign<&ast::MappedType::name>(ref<ContextualName>) +
            kw<"in"> + assign<&ast::MappedType::constraint>(ref<SharedType>) +
            option(kw<"as"> + assign<&ast::MappedType::nameType>(ref<SharedType>)) +
            lit<"]"> +
            option(assign<&ast::MappedType::optionalMod>(
                lit<"?"> | ref<SignedOptional>)) +
            option(assign<&ast::MappedType::type>(ref<SharedTypeAnnotation>)) +
            option(TypeMemberEnd) + lit<"}">)> {};

  static constexpr auto IndexedTypeTail =
      nest<ast::IndexedType, &ast::IndexedType::object>(
          !LineBreak + lit<"["> +
          option(assign<&ast::IndexedType::index>(ref<SharedType>)) + lit<"]">);

  struct ThisType : rule<create<ast::ThisType>(kw<"this">)> {};

  struct NamedTupleMember
      : rule<create<ast::NamedTupleMember>(
            option(flag<&ast::NamedTupleMember::rest>(lit<"...">)) +
            assign<&ast::NamedTupleMember::name>(ref<AnyName>) +
            option(flag<&ast::NamedTupleMember::optional>(lit<"?">)) +
            lit<":"> + assign<&ast::NamedTupleMember::type>(ref<SharedType>))> {};
  struct RestType
      : rule<create<ast::RestType>(
            lit<"..."> + assign<&ast::RestType::type>(ref<SharedType>))> {};
  struct OptionalType
      : rule<create<ast::OptionalType>(
            assign<&ast::OptionalType::type>(ref<SharedType>) + lit<"?">)> {};
  struct TupleElement
      : rule<ref<NamedTupleMember> | ref<RestType> |
             ref<OptionalType> | ref<SharedType>> {};

  struct TemplateType
      : rule<create<ast::TemplateLiteralType>(
            TemplateContents<&ast::TemplateLiteralType::quasis,
                             &ast::TemplateLiteralType::types, SharedType>)> {};

  struct InferTypeParam
      : rule<create<ast::TypeParam>(
            assign<&ast::TypeParam::name>(ref<ContextualName>) +
            option(kw<"extends"> +
                   assign<&ast::TypeParam::constraint>(ref<TypeNoCond>)))> {};
  struct InferType
      : rule<create<ast::InferType>(
            kw<"infer"> +
            assign<&ast::InferType::typeParam>(ref<InferTypeParam>))> {};

  struct ImportType
      : rule<create<ast::ImportType>(
            option(flag<&ast::ImportType::isTypeOf>(kw<"typeof">)) +
            kw<"import"> + lit<"("> +
            assign<&ast::ImportType::argument>(ref<SharedType>) +
            option(lit<","> +
                   option(assign<&ast::ImportType::options>(
                              ref<ObjectLiteral>) +
                          option(lit<",">))) +
            lit<")"> +
            option(lit<"."> +
                   assign<&ast::ImportType::qualifier>(ref<QualifiedName>)) +
            option(TypeArgsClauseFor<&ast::ImportType::typeArgs>))> {};

  struct PrimaryTypeBase
      : rule<ref<PredefinedType>  //
             | ref<ImportType>  //
             | ref<TypeQuery>  //
             | ref<TemplateType>  //
             | ref<InferType>  //
             | ref<ThisType>  //
             | create<ast::KeyOfType>(
                   kw<"keyof"> +
                   assign<&ast::KeyOfType::operand>(ref<PrimaryType>))  //
             | create<ast::ReadonlyType>(
                   kw<"readonly"> +
                   assign<&ast::ReadonlyType::operand>(ref<PrimaryType>))  //
             | (lit<"["> +
                create<ast::TupleType>(option(
                    some(append<&ast::TupleType::elements>(ref<TupleElement>), lit<",">) +
                    option(lit<",">))) +
                lit<"]">)  //
             | ref<MappedType>  //
             | ref<ObjectType>  //
             | ref<SharedTypeReference>  //
             | (lit<"("> + ref<SharedType> + lit<")">)> {};

  struct PrimaryType : rule<ref<PrimaryTypeBase> + many(IndexedTypeTail)> {};

  struct TypeBinExpr
      : infix<PrimaryType, ast::TypeBinExpr,
              levels(left(lit<"&">), left(lit<"|">))> {};

  struct FunctionType
      : rule<create<ast::FunctionType>(
            TypeParamsClauseFor<&ast::FunctionType::typeParams> +
            ParamListFor<&ast::FunctionType::params> + lit<"=>"> +
            assign<&ast::FunctionType::returnType>(ref<TypeOrPredicate>))> {};

  struct ConstructorType
      : rule<create<ast::ConstructorType>(
            option(flag<&ast::ConstructorType::isAbstract>(
                kw<"abstract">)) +
            kw<"new"> +
            TypeParamsClauseFor<&ast::ConstructorType::typeParams> +
            ParamListFor<&ast::ConstructorType::params> + lit<"=>"> +
            assign<&ast::ConstructorType::returnType>(ref<SharedType>))> {};

  // The extends type of a conditional and the constraint of an `infer` hold
  // no conditional type of their own.
  struct TypeNoCond
      : rule<(option(lit<"|"> | lit<"&">) + ref<TypeBinExpr>)  //
             | ref<FunctionType>  //
             | ref<ConstructorType>> {};

  struct Type_
      : rule<ref<TypeNoCond> +
             option(nest<ast::ConditionalType, &ast::ConditionalType::checkType>(
                 !LineBreak + kw<"extends"> +
                 assign<&ast::ConditionalType::extendsType>(ref<TypeNoCond>) +
                 lit<"?"> + assign<&ast::ConditionalType::trueType>(ref<SharedType>) +
                 lit<":"> + assign<&ast::ConditionalType::falseType>(ref<SharedType>)))> {};

  // `x is T` and `this is T` take a whole type; `asserts x` may have none.
  struct TypePredicate
      : rule<create<ast::TypePredicateType>(
                 assign<&ast::TypePredicateType::param>(
                     ref<ThisType> | ref<SharedTypeReference>) +
                 !LineBreak + kw<"is"> +
                 assign<&ast::TypePredicateType::type>(ref<SharedType>))  //
             | create<ast::TypePredicateType>(
                   flag<&ast::TypePredicateType::asserts>(kw<"asserts">) +
                   !LineBreak +
                   assign<&ast::TypePredicateType::param>(
                       ref<ThisType> | ref<SharedTypeReference>) +
                   option(!LineBreak + kw<"is"> +
                          assign<&ast::TypePredicateType::type>(
                              ref<SharedType>)))> {};
  struct TypeOrPredicate : rule<ref<SharedType> | ref<TypePredicate>> {};

  // No binary operator in a computed key: `[K in T]` is a mapped type.
  static constexpr auto TypeMemberPropName =
      (flag<&ast::TypeMember::computed>(lit<"[">) +
       assign<&ast::TypeMember::key>(ref<ContextUnary<GeneratorContext>>) + lit<"]">)  //
      | assign<&ast::TypeMember::key>(PropKey);

  struct CallSignature
      : rule<create<ast::TypeMember>(
            TypeParamsClauseFor<&ast::TypeMember::typeParams> +
            flag<&ast::TypeMember::isCall>(lit<"(">) +
            ParamTailFor<&ast::TypeMember::params> +
            option(assign<&ast::TypeMember::type>(
                ref<SharedTypeAnnotation>)))> {};

  struct TypeMember
      : rule<create<ast::TypeMember>(  // index signature
                 option(flag<&ast::TypeMember::readonly_>(
                     kw<"readonly">)) +
                 flag<&ast::TypeMember::isIndex>(lit<"[">) +
                 assign<&ast::TypeMember::indexName>(ref<ContextualName>) +
                 lit<":"> +
                 assign<&ast::TypeMember::indexKeyType>(ref<SharedType>) +
                 lit<"]"> +
                 assign<&ast::TypeMember::type>(ref<SharedTypeAnnotation>))  //
             | create<ast::TypeMember>(  // construct signature
                   flag<&ast::TypeMember::isConstruct>(kw<"new">) +
                   TypeParamsClauseFor<&ast::TypeMember::typeParams> +
                   ParamListFor<&ast::TypeMember::params> +
                   option(assign<&ast::TypeMember::type>(
                       ref<SharedTypeAnnotation>)))  //
             | ref<CallSignature>  //
             | create<ast::TypeMember>(  // getter signature
                   flag<&ast::TypeMember::isGetter>(kw<"get">) +
                   TypeMemberPropName + lit<"("> + lit<")"> +
                   option(assign<&ast::TypeMember::type>(
                       ref<SharedTypeAnnotation>)))  //
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
                                ref<SharedTypeAnnotation>)  //
                            | (lit<"=>"> + assign<&ast::TypeMember::type>(
                                              ref<SharedType>))))  //
                    | option(assign<&ast::TypeMember::type>(
                          ref<SharedTypeAnnotation>))))> {};

  // -- Templates (tag stays null for the untagged use in PrimaryExpr) ----
  static constexpr auto TemplateBody =
      TemplateContents<&ast::TemplateLit::quasis,
                       &ast::TemplateLit::expressions, ContextExpression<GeneratorContext>>;
  struct TemplateLiteral : rule<create<ast::TemplateLit>(TemplateBody)> {};

  // -- Primary expressions ----
  struct Literal
      : rule<ref<NullLit_> | ref<BoolLit_> | ref<StringLiteral> |
             ref<TemplateLiteral> | ref<RegexLit_> | ref<BigIntLit_> |
             ref<NumberLit_>> {};

  struct ParenExpr : rule<lit<"("> + ref<ContextExpression<GeneratorContext>> + lit<")">> {};

  template <class Operand>
  struct YieldExpr_
      : rule<create<ast::YieldExpr>(
            kw<"yield"> +
            option(!LineBreak +
                   option(flag<&ast::YieldExpr::delegate>(lit<"*">)) +
                   assign<&ast::YieldExpr::argument>(ref<Operand>)))> {};

  // Outside a generator the native parser recognizes only this same-line
  // identifier/keyword/number/bigint/string lookahead. Templates remain tags.
  static constexpr auto YieldStart = [] {
    if constexpr (GeneratorContext) return &kw<"yield">;
    else return &(kw<"yield"> + !LineBreak +
        (ref<RawId> | ref<NUMBER> | ref<BIGINT> | ref<STRING>));
  }();

  // A cast takes a unary expression. Arrows live at assignment level, so
  // `<T>(x) => y` is a generic arrow rather than a cast of an arrow.
  struct TypeAssertionExpr_
      : rule<create<ast::TypeAssertionExpr>(
            TypeArgsClauseFor<&ast::TypeAssertionExpr::typeArgs> +
            assign<&ast::TypeAssertionExpr::expression>(ref<ContextUnary<GeneratorContext>>))> {};

  struct ImportCallExpr_
      : rule<create<ast::ImportCallExpr>(
            kw<"import"> + lit<"("> +
            assign<&ast::ImportCallExpr::source>(ref<ContextAssign<GeneratorContext>>) +
            option(lit<","> +
                   option(assign<&ast::ImportCallExpr::options>(
                              ref<ContextAssign<GeneratorContext>>) +
                          option(lit<",">))) +
            lit<")">)> {};

  struct PrimaryExpr
      : rule<ref<ThisExpr_> | ref<SuperExpr_> | ref<MetaProperty_> |
             ref<ImportCallExpr_> | ref<Literal> | ref<ArrayLiteral> |
             ref<ObjectLiteral> | ref<ParenExpr> |
             ref<FunctionExpr_> | ref<ClassExpr_> | ref<TypeAssertionExpr_> |
             ref<IdentPrimary>> {};

  // -- Member / call / new chain ----
  static constexpr auto Spread = lit<"...">;
  struct SpreadArg
      : rule<create<ast::Spread>(
            Spread + assign<&ast::Spread::argument>(ref<ContextAssign<GeneratorContext>>))> {};
  static constexpr auto OneArg = ref<SpreadArg> | ref<ContextAssign<GeneratorContext>>;

  // `?.` is not an optional-chain punctuator before a decimal digit.
  static constexpr auto OptionalChain = lit<"?.", !d>;
  static constexpr auto MemberTail =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          option(flag<&ast::MemberExpr::optional>(OptionalChain)) +
          flag<&ast::MemberExpr::computed>(lit<"[">) +
          assign<&ast::MemberExpr::property>(ref<ContextExpression<GeneratorContext>>) + lit<"]">) |
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          (flag<&ast::MemberExpr::optional>(OptionalChain) | lit<".">) +
          assign<&ast::MemberExpr::property>(ref<MemberName>));

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
          option(flag<&ast::CallExpr::optional>(OptionalChain)) +
          option(TypeArgsClauseFor<&ast::CallExpr::typeArgs>) + CallArguments) |
      nest<ast::TemplateLit, &ast::TemplateLit::tag>(TemplateBody) |
      // No line break before a postfix operator: `a \n ++b` is two statements.
      nest<ast::PostfixUpdateExpr, &ast::PostfixUpdateExpr::operand>(
          !LineBreak + assign<&ast::PostfixUpdateExpr::op>(lit<"++"> | lit<"--">)) |
      nest<ast::NonNullExpr, &ast::NonNullExpr::operand>(!LineBreak + lit<"!", !cr<"=">>);

  struct PostfixExpr
      : rule<(ref<PrimaryExpr> | ref<NewExpr_>) + many(PostfixTail)> {};

  static constexpr auto IdentPrimaryBody = ref<Ident> | create<ast::Ident>(
      assign<&ast::Ident::name>(ref<PrivateId>));
  static constexpr auto ContextIdentPrimary = [] {
    if constexpr (GeneratorContext)
      return ref<GeneratorIdent> | create<ast::Ident>(assign<&ast::Ident::name>(ref<PrivateId>));
    else return IdentPrimaryBody;
  }();
  struct IdentPrimary : rule<ContextIdentPrimary> {};

  // -- Unary chain ----
  template <class Recursive, class Chain>
  static constexpr auto UnaryBody =
      create<ast::UnaryExpr>(
          assign<&ast::UnaryExpr::op>(kw<"delete"> | kw<"void"> | kw<"typeof">) +
          assign<&ast::UnaryExpr::operand>(ref<Recursive>)) |
      create<ast::PrefixUpdateExpr>(
          assign<&ast::PrefixUpdateExpr::op>(lit<"++"> | lit<"--">) +
          assign<&ast::PrefixUpdateExpr::operand>(ref<Recursive>)) |
      create<ast::UnaryExpr>(
          assign<&ast::UnaryExpr::op>(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">> |
                                     lit<"~"> | lit<"!", !cr<"=">>) +
          assign<&ast::UnaryExpr::operand>(ref<Recursive>)) |
      create<ast::AwaitExpr>(kw<"await"> +
          assign<&ast::AwaitExpr::operand>(ref<Recursive>)) | ref<Chain>;
  struct UnaryExpr : rule<UnaryBody<UnaryExpr, PostfixExpr>> {};

  static constexpr auto MultiplicativeOp =
      lit<"*", !cr<"*=">> | lit<"/", !cr<"=">> | lit<"%", !cr<"=">>;
  static constexpr auto AdditiveOp = lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>;

  struct PowerExpr
      : infix<UnaryExpr, ast::BinaryExpr, right(lit<"**", !cr<"=">>)> {};
  struct MultiplyExpr : infix<PowerExpr, ast::BinaryExpr, left(MultiplicativeOp)> {};
  struct AddExpr : infix<MultiplyExpr, ast::BinaryExpr, left(AdditiveOp)> {};
  static constexpr auto ShiftOp =
      lit<"<<", !cr<"=">> |
      (lit<">"> + gap::none + lit<">"> + gap::none + lit<">", !cr<"=">>) |
      (lit<">"> + gap::none + lit<">", !cr<"=>">>);

  struct ShiftExpr
      : infix<AddExpr, ast::BinaryExpr, left(ShiftOp)> {};

  template <class Power, class Multiply, class Add>
  static constexpr auto AssertionTailFor =
      option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(lit<"**", !cr<"=">>) +
          assign<&ast::BinaryExpr::right>(ref<Power>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(
              lit<"*", !cr<"*=">> | lit<"/", !cr<"=">> |
              lit<"%", !cr<"=">>) +
          assign<&ast::BinaryExpr::right>(ref<Power>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(
              lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>) +
          assign<&ast::BinaryExpr::right>(ref<Multiply>))) +
      many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
          assign<&ast::BinaryExpr::op>(ShiftOp) +
          assign<&ast::BinaryExpr::right>(ref<Add>)));

  static constexpr auto AssertionTail =
      AssertionTailFor<PowerExpr, MultiplyExpr, AddExpr>;
  static constexpr auto RelationalOpNoIn =
      lit<"<="> | lit<">="> | lit<"<", !cr<"=<">> |
      lit<">", !cr<"=>">> | kw<"instanceof">;

  struct RelationalExpr
      : rule<(create<ast::AsExpr>(
                  assign<&ast::AsExpr::expression>(ref<RelationalExpr>) +
                  !LineBreak + kw<"as"> +
                  assign<&ast::AsExpr::target>(ref<SharedType>)) + AssertionTail) |
             (create<ast::SatisfiesExpr>(
                  assign<&ast::SatisfiesExpr::expression>(ref<RelationalExpr>) +
                  !LineBreak + kw<"satisfies"> +
                  assign<&ast::SatisfiesExpr::target>(ref<SharedType>)) + AssertionTail) |
             create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<RelationalExpr>) +
                 assign<&ast::BinaryExpr::op>(RelationalOpNoIn | kw<"in">) +
                 assign<&ast::BinaryExpr::right>(ref<ShiftExpr>)) |
             ref<ShiftExpr>> {};

  static constexpr auto BinaryLevels = levels(left(lit<"==="> | lit<"!=="> | lit<"==", !cr<"=">> |
                          lit<"!=", !cr<"=">>),
                     left(lit<"&", !cr<"&=">>),  //
                     left(lit<"^", !cr<"=">>),   //
                     left(lit<"|", !cr<"|=">>),  //
                     left(lit<"&&", !cr<"=">>),  //
                     left(lit<"||", !cr<"=">>),  //
                     left(lit<"??", !cr<"=">>));
  struct BinaryExpr : infix<RelationalExpr, ast::BinaryExpr, BinaryLevels> {};

  struct ConditionalExpr
      : rule<create<ast::ConditionalExpr>(
                 assign<&ast::ConditionalExpr::test>(ref<BinaryExpr>) +
                 lit<"?", !cr<"?"> + !(lit<"."> + !d)> +
                 assign<&ast::ConditionalExpr::consequent>(ref<ContextAssign<GeneratorContext>>) +
                 lit<":"> +
                 assign<&ast::ConditionalExpr::alternate>(ref<ContextAssign<GeneratorContext>>))  //
             | ref<BinaryExpr>> {};

  static constexpr auto AssignmentOp = lit<"=", !cr<"=>">> | lit<"*="> | lit<"/="> | lit<"%="> |
                    lit<"+="> | lit<"-="> | lit<"<<="> | lit<">>>="> |
                    lit<">>="> | lit<"&="> | lit<"^="> | lit<"|="> |
                    lit<"**="> |
                    lit<"?"
                        "?="> |
                    lit<"&&="> | lit<"||=">;
  struct AssignmentOperator : rule<AssignmentOp> {};
  template <class Conditional, class Recursive, class Arrow>
  static constexpr auto AssignmentBody = [] {
    constexpr auto ordinary = ref<Conditional> +
        option(nest<ast::AssignExpr, &ast::AssignExpr::target>(
            assign<&ast::AssignExpr::op>(ref<AssignmentOperator>) +
            assign<&ast::AssignExpr::value>(ref<Recursive>)));
    // Native5.9.3 dispatches Yield, then Arrow, then conditional/assignment.
    // The generator identifier and single-parameter paths exclude yield.
    if constexpr (GeneratorContext) return ref<YieldExpr_<Recursive>> | ref<Arrow> | ordinary;
    else return (YieldStart + ref<YieldExpr_<Recursive>>) |
        (!YieldStart + (ref<Arrow> | ordinary));
  }();
  struct AssignExpr : rule<AssignmentBody<ConditionalExpr, AssignExpr, ArrowFunctionExpr_>> {};

  struct Expression
      : infix<AssignExpr, ast::BinaryExpr, left(lit<",">)> {};

  // -- Functions ----
  template <bool Generator, auto Types, auto Params, auto Return>
  static constexpr auto FunctionSignatureFor =
      TypeParamsClauseFor<Types> + ParamListFor<Params, Generator> +
      option(assign<Return>(ref<SharedTypeAnnotation>));

  template <bool Generator>
  static constexpr auto FunctionExpressionTail =
      option(assign<&ast::FunctionExpr::name>(ref<RawId>)) +
      FunctionSignatureFor<Generator, &ast::FunctionExpr::typeParams,
                           &ast::FunctionExpr::params, &ast::FunctionExpr::returnType> +
      assign<&ast::FunctionExpr::body>(ref<ContextFunctionBody<Generator>>);
  struct FunctionExpr_
      : rule<create<ast::FunctionExpr>(
            option(flag<&ast::FunctionExpr::isAsync>(kw<"async">) + !LineBreak) +
            kw<"function"> +
            ((flag<&ast::FunctionExpr::isGenerator>(lit<"*">) + FunctionExpressionTail<true>) |
             FunctionExpressionTail<false>))> {};

  static constexpr auto ArrowSingleName = [] {
    if constexpr (GeneratorContext) return ref<GeneratorIdent>;
    else return ref<Ident>;
  }();
  struct ArrowSingleParam
      : rule<assign<&ast::Param::pattern>(ArrowSingleName)> {};

  static constexpr auto ArrowParams =
      ParamListFor<&ast::ArrowFunctionExpr::params, false> |
      append<&ast::ArrowFunctionExpr::params>(ref<ArrowSingleParam>);

  static constexpr auto ArrowSignature = [] {
    constexpr auto signature =
        TypeParamsClauseFor<&ast::ArrowFunctionExpr::typeParams> + ArrowParams;
    // Parenthesis lookahead runs in the outer Yield context, before params
    // reset it. A generic arrow starts with < and follows its own lookahead.
    if constexpr (GeneratorContext) return !(lit<"("> + kw<"yield">) + signature;
    else return signature;
  }();

  template <class ExpressionBody>
  static constexpr auto ArrowFunctionBody = create<ast::ArrowFunctionExpr>(
      option(flag<&ast::ArrowFunctionExpr::isAsync>(kw<"async">) + !LineBreak) +
      ArrowSignature +
      option(assign<&ast::ArrowFunctionExpr::returnType>(ref<SharedTypeAnnotation>)) +
      !LineBreak + lit<"=>"> +
      (assign<&ast::ArrowFunctionExpr::block>(ref<ContextFunctionBody<false>>) |
       assign<&ast::ArrowFunctionExpr::exprBody>(ref<ExpressionBody>)));
  struct ArrowFunctionExpr_ : rule<ArrowFunctionBody<ContextAssign<false>>> {};

  // -- Array / object literals ----
  struct ArrayElementBeforeComma : rule<(OneArg | create<ast::Elision>)+lit<",">> {};
  struct ArrayLiteral
      : rule<create<ast::ArrayLit>(
            lit<"["> +
            many(append<&ast::ArrayLit::elements>(ref<ArrayElementBeforeComma>)) +
            option(append<&ast::ArrayLit::elements>(OneArg)) + lit<"]">)> {};

  static constexpr auto PropName =
      (flag<&ast::Prop::computed>(lit<"[">) +
       assign<&ast::Prop::key>(ref<ContextAssign<GeneratorContext>>) + lit<"]">)  //
      | assign<&ast::Prop::key>(PropKey);

  struct MethodBody
      : rule<create<ast::FunctionExpr>(
            ParamListFor<&ast::FunctionExpr::params, false> +
            option(assign<&ast::FunctionExpr::returnType>(ref<SharedTypeAnnotation>)) +
            assign<&ast::FunctionExpr::body>(ref<ContextFunctionBody<false>>))> {};
  struct GenericMethodBody
      : rule<create<ast::FunctionExpr>(
            FunctionSignatureFor<GeneratorContext, &ast::FunctionExpr::typeParams,
                                 &ast::FunctionExpr::params, &ast::FunctionExpr::returnType> +
            assign<&ast::FunctionExpr::body>(ref<ContextFunctionBody<GeneratorContext>>))> {};

  template <bool Generator>
  static constexpr auto ObjectMethodTail = PropName +
      flag<&ast::Prop::isMethod>(assign<&ast::Prop::value>(ref<ContextMethod<Generator>>));
  struct Property
      : rule<create<ast::Prop>(
            (PropName + lit<":"> + assign<&ast::Prop::value>(ref<ContextAssign<GeneratorContext>>)) |
            (option(flag<&ast::Prop::isAsync>(kw<"async"> + !LineBreak)) +
             ((flag<&ast::Prop::isGenerator>(lit<"*">) + ObjectMethodTail<true>) |
              ObjectMethodTail<false>)) |
            (flag<&ast::Prop::isGetter>(kw<"get">) + PropName +
             assign<&ast::Prop::value>(ref<MethodBody>)) |
            (flag<&ast::Prop::isSetter>(kw<"set">) + PropName +
             assign<&ast::Prop::value>(ref<MethodBody>)) |
            assign<&ast::Prop::value>(OneArg))> {};

  struct ObjectLiteral
      : rule<create<ast::ObjectLit>(
            lit<"{"> +
            option(append<&ast::ObjectLit::properties>(ref<Property>) +
                   many(lit<","> + append<&ast::ObjectLit::properties>(
                                       ref<Property>)) +
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
             (ref<Ident> | (lit<"("> + ref<ContextExpression<GeneratorContext>> + lit<")">)) +
             many(DecoratorMemberTail | DecoratorCallTail)> {};

  // -- Classes ----
  struct ClassMemberKeyIdent
      : rule<assign<&ast::Ident::name>(ref<PrivateOrName>)> {};
  static constexpr auto ClassMemberKey =
      (flag<&ast::ClassMember::computed>(lit<"[">) +
       assign<&ast::ClassMember::key>(ref<ContextAssign<GeneratorContext>>) + lit<"]">)  //
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

  template <bool Generator>
  static constexpr auto ClassMethodTail =
      ClassMemberKey + option(flag<&ast::ClassMember::optional>(lit<"?">)) +
      TypeParamsClauseFor<&ast::ClassMember::typeParams> +
      flag<&ast::ClassMember::isMethod>(lit<"(">) +
      ParamTailFor<&ast::ClassMember::params, Generator> +
      option(assign<&ast::ClassMember::returnType>(ref<SharedTypeAnnotation>)) +
      option(assign<&ast::ClassMember::body>(ref<ContextFunctionBody<Generator>>));

  struct ClassMember
      : rule<
            create<ast::ClassMember>(  // static initialisation block
                many(append<&ast::ClassMember::decorators>(ref<DecoratorExpr>)) +
                flag<&ast::ClassMember::isStatic>(kw<"static">) +
                assign<&ast::ClassMember::body>(ref<ContextBlock<false>>))  //
            | create<ast::ClassMember>(  // `abstract name(...): T;` / field
                  many(append<&ast::ClassMember::decorators>(
                      ref<DecoratorExpr>)) +
                  option(assign<&ast::ClassMember::accessibility>(AccessMod)) +
                  assign<&ast::ClassMember::abstractDecl>(
                      ref<AbstractDecl>))  //
            | create<ast::ClassMember>(  // method, getter or setter
                  MemberBase +
                  (((flag<&ast::ClassMember::isGenerator>(lit<"*">) + ClassMethodTail<true>) |
                    ClassMethodTail<false>)  //
                   | (flag<&ast::ClassMember::isGetter>(kw<"get">) +
                      ClassMemberKey + lit<"("> + lit<")"> +
                      option(assign<&ast::ClassMember::returnType>(
                          ref<SharedTypeAnnotation>)) +
                      option(assign<&ast::ClassMember::body>(
                          ref<ContextFunctionBody<false>>)))  //
                   | (flag<&ast::ClassMember::isSetter>(kw<"set">) +
                      ClassMemberKey + lit<"("> +
                      ParamTailFor<&ast::ClassMember::params, false> +
                      option(assign<&ast::ClassMember::body>(
                          ref<ContextFunctionBody<false>>)))))  //
            | (create<ast::ClassMember>(  // index signature
                   MemberBase +
                   flag<&ast::ClassMember::isIndex>(lit<"[">) +
                   assign<&ast::ClassMember::indexName>(ref<ContextualName>) +
                   lit<":"> +
                   assign<&ast::ClassMember::indexKeyType>(ref<SharedType>) +
                   lit<"]"> +
                   assign<&ast::ClassMember::typeAnnotation>(
                       ref<SharedTypeAnnotation>)) +
               Eos)  //
            // A field ends at `;`, a line break or `}`: without it a modifier
            // word reads as a field of its own (`readonly type: T` as two).
            | (create<ast::ClassMember>(  // a field, optionally static
                   MemberBase + ClassMemberKey +
                   option(flag<&ast::ClassMember::optional>(lit<"?">)) +
                   option(flag<&ast::ClassMember::definite>(lit<"!">)) +
                   option(assign<&ast::ClassMember::typeAnnotation>(
                       ref<SharedTypeAnnotation>)) +
                   option(lit<"="> + assign<&ast::ClassMember::value>(
                                          ref<ContextAssign<false>>))) +
               Eos)> {};

  template <auto Ptr>
  static constexpr auto ClassBodyFor =
      lit<"{"> + many(append<Ptr>(ref<ClassMember>) | lit<";">) + lit<"}">;

  // A plain type reference reads as one; any other expression (a call, a
  // parenthesised cast) lands in the expression member.
  template <auto SuperPtr, auto ExprPtr, auto ImplPtr>
  static constexpr auto ClassHeritageFor =
      option(kw<"extends"> + (assign<SuperPtr>(ref<SharedTypeReference>)  //
                              | assign<ExprPtr>(ref<PostfixExpr>))) +
      option(kw<"implements"> + some(append<ImplPtr>(ref<SharedTypeReference>), lit<",">));

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
                       ref<SharedTypeReference>) +
                   many(lit<","> + append<&ast::InterfaceDecl::extendsClause>(
                                        ref<SharedTypeReference>))) +
            lit<"{"> +
            TypeMembers<&ast::InterfaceDecl::members> +
            lit<"}"> + option(lit<";">))> {};

  struct TypeAliasDeclaration
      : rule<create<ast::TypeAliasDecl>(
            DeclareFor<&ast::TypeAliasDecl::isDeclare> + kw<"type"> +
            assign<&ast::TypeAliasDecl::name>(ref<RawId>) +
            TypeParamsClauseFor<&ast::TypeAliasDecl::typeParams> + lit<"="> +
            assign<&ast::TypeAliasDecl::type>(ref<SharedType>) + Eos)> {};

  struct EnumMember
      : rule<create<ast::EnumMember>(
            assign<&ast::EnumMember::key>(PropKey) +
            option(lit<"="> +
                   assign<&ast::EnumMember::value>(ref<ContextAssign<GeneratorContext>>)))> {};

  struct EnumDeclaration
      : rule<create<ast::EnumDecl>(
            DeclareFor<&ast::EnumDecl::isDeclare> +
            option(flag<&ast::EnumDecl::isConst>(kw<"const">)) +
            kw<"enum"> + assign<&ast::EnumDecl::name>(ref<RawId>) + lit<"{"> +
            option(append<&ast::EnumDecl::members>(ref<EnumMember>) +
                   many(lit<","> + append<&ast::EnumDecl::members>(
                                        ref<EnumMember>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  static constexpr auto NamespaceBody =
      lit<"{"> + many(append<&ast::NamespaceDecl::body>(ref<ContextStatement<GeneratorContext>>)) +
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

  // `abstract name: Type;`: a bare declarator, no `var`/`let`/`const` --
  // kept narrow to this one caller instead of widening `VariableStatement`
  // itself, which would put a bare identifier back at the head of every
  // statement position.
  struct AbstractDeclarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<Assignable>) +
            option(flag<&ast::Declarator::optional>(lit<"?">)) +
            option(assign<&ast::Declarator::typeAnnotation>(
                ref<SharedTypeAnnotation>)) +
            option(lit<"="> +
                   assign<&ast::Declarator::init>(ref<ContextAssign<GeneratorContext>>)))> {};

  struct AbstractFieldDecl
      : rule<create<ast::VarDecl>(
            option(flag<&ast::VarDecl::isReadonly>(kw<"readonly">)) +
            append<&ast::VarDecl::declarations>(ref<AbstractDeclarator_>))> {};

  struct AbstractDecl
      : rule<create<ast::AbstractDecl>(
                 kw<"abstract"> +
                 assign<&ast::AbstractDecl::name>(ref<AnyName>) +
                 TypeParamsClauseFor<&ast::AbstractDecl::typeParams> +
                 lit<"("> + ParamTailFor<&ast::AbstractDecl::params> +
                 option(assign<&ast::AbstractDecl::returnType>(
                     ref<SharedTypeAnnotation>)) +
                 Eos)  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   flag<&ast::AbstractDecl::isGetter>(kw<"get">) +
                   assign<&ast::AbstractDecl::name>(ref<AnyName>) +
                   lit<"("> + lit<")"> +
                   option(assign<&ast::AbstractDecl::returnType>(
                       ref<SharedTypeAnnotation>)) +
                   Eos)  //
             | create<ast::AbstractDecl>(
                   kw<"abstract"> +
                   flag<&ast::AbstractDecl::isSetter>(kw<"set">) +
                   assign<&ast::AbstractDecl::name>(ref<AnyName>) +
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

  // Resource for-header initializers keep NoIn through concise arrows;
  // parentheses, arguments and function blocks use ordinary expressions.
  struct NoInArrowFunctionExpr : rule<ArrowFunctionBody<ContextNoInAssign<false>>> {};
  struct NoInRelationalExpr : rule<
      (create<ast::AsExpr>(
          assign<&ast::AsExpr::expression>(ref<NoInRelationalExpr>) +
          !LineBreak + kw<"as"> + assign<&ast::AsExpr::target>(ref<SharedType>)) +
       AssertionTail) |
      (create<ast::SatisfiesExpr>(
          assign<&ast::SatisfiesExpr::expression>(ref<NoInRelationalExpr>) +
          !LineBreak + kw<"satisfies"> + assign<&ast::SatisfiesExpr::target>(ref<SharedType>)) +
       AssertionTail) |
      create<ast::BinaryExpr>(
          assign<&ast::BinaryExpr::left>(ref<NoInRelationalExpr>) +
          assign<&ast::BinaryExpr::op>(RelationalOpNoIn) +
          assign<&ast::BinaryExpr::right>(ref<ShiftExpr>)) | ref<ShiftExpr>> {};
  struct NoInBinaryExpr : infix<NoInRelationalExpr, ast::BinaryExpr, BinaryLevels> {};
  struct NoInConditionalExpr : rule<create<ast::ConditionalExpr>(
      assign<&ast::ConditionalExpr::test>(ref<NoInBinaryExpr>) +
      lit<"?", !cr<"?"> + !(lit<"."> + !d)> +
      assign<&ast::ConditionalExpr::consequent>(ref<ContextAssign<GeneratorContext>>) + lit<":"> +
      assign<&ast::ConditionalExpr::alternate>(ref<NoInAssignExpr>)) | ref<NoInBinaryExpr>> {};
  struct NoInAssignExpr : rule<AssignmentBody<NoInConditionalExpr, NoInAssignExpr, NoInArrowFunctionExpr>> {};
  struct NoInExpression : infix<NoInAssignExpr, ast::BinaryExpr, left(lit<",">)> {};

  // -- Assignable (var/let/const target, parameter, for-in/of target) ----
  // `AllKeywords`, not only `Ident`: a keyword is a legal binding name too
  // (`const function = ...`), spelled once through the same `KeywordAsKey`
  // sibling `PropKey`/`ClassMemberKey` already use.
  struct Assignable
      : rule<ref<Ident> | ref<ArrayLiteral> | ref<ObjectLiteral> |
             ref<KeywordAsKey>> {};

  // -- Statements ----

  template <class Initializer>
  static constexpr auto DeclaratorBody =
      assign<&ast::Declarator::id>(ref<Assignable>) +
      option(flag<&ast::Declarator::definite>(lit<"!", !cr<"=">>)) +
      option(assign<&ast::Declarator::typeAnnotation>(ref<SharedTypeAnnotation>)) +
      option(lit<"="> + assign<&ast::Declarator::init>(ref<Initializer>));
  struct Declarator_ : rule<DeclaratorBody<AssignExpr>> {};
  struct ForDeclarator : rule<DeclaratorBody<NoInAssignExpr>> {};

  // parser.ts isBindingIdentifier includes future/contextual words. Keep
  // this lexical fragment free of AST construction so heads can read it.
  static constexpr auto ResourceBindingName = ref<PlainId> | ContextualKeyword |
      kw<"implements"> | kw<"interface"> | kw<"let"> | kw<"package"> |
      kw<"private"> | kw<"protected"> | kw<"public"> | kw<"static"> | kw<"await">;
  struct ResourceBindingValue : rule<ResourceBindingName> {};
  struct ResourceBindingIdent : rule<
      assign<&ast::Ident::name>(ref<ResourceBindingValue>)> {};
  static constexpr auto ResourceBinding =
      ref<ResourceBindingIdent> | ref<ArrayLiteral> | ref<ObjectLiteral>;
  static constexpr auto ResourceFirstBinding = ResourceBindingName | lit<"{">;
  static constexpr auto ResourceFollower = !LineBreak + &ResourceFirstBinding;
  static constexpr auto ResourceHead =
      (kw<"using"> + ResourceFollower) |
      (kw<"await"> + kw<"using"> + ResourceFollower);
  // Plain `using of xs` is an expression initializer; `await using` differs.
  // The plain `of` lookahead precedes the native same-line check.
  static constexpr auto ResourceForHead =
      (kw<"using"> +
       ((kw<"of"> + &(lit<"="> | lit<";"> | lit<":">)) |
        (!LineBreak + !kw<"of"> + ResourceFirstBinding))) |
      (kw<"await"> + kw<"using"> + ResourceFollower);
  static constexpr auto ResourceKind =
      option(flag<&ast::VarDecl::isAwaitUsing>(kw<"await">)) +
      assign<&ast::VarDecl::kind>(kw<"using">);
  template <class Initializer>
  static constexpr auto ResourceTypeAndInit =
      option(assign<&ast::Declarator::typeAnnotation>(ref<SharedTypeAnnotation>)) +
      option(lit<"="> + assign<&ast::Declarator::init>(ref<Initializer>));
  struct ResourceDeclarator : rule<create<ast::Declarator>(
      assign<&ast::Declarator::id>(ResourceBinding) +
      option(!LineBreak + flag<&ast::Declarator::definite>(lit<"!", !cr<"=">>)) +
      ResourceTypeAndInit<AssignExpr>)> {};
  struct ResourceForDeclarator : rule<create<ast::Declarator>(
      assign<&ast::Declarator::id>(ResourceBinding) +
      ResourceTypeAndInit<NoInAssignExpr>)> {};
  static constexpr auto ResourceListElementStart =
      ResourceBindingName | ref<PrivateId> | lit<"["> | lit<"{">;
  static constexpr auto ResourceTrailingComma =
      flag<&ast::VarDecl::hasTrailingComma>(lit<",">) + !ResourceListElementStart;
  struct ResourceStatement : rule<
      create<ast::VarDecl>(
          option(flag<&ast::VarDecl::isDeclare>(kw<"declare">) + !LineBreak) +
          &ResourceHead + ResourceKind + ResourceFollower +
          append<&ast::VarDecl::declarations>(ref<ResourceDeclarator>) +
          many(lit<","> + append<&ast::VarDecl::declarations>(ref<ResourceDeclarator>)) +
          option(ResourceTrailingComma)) + Eos> {};
  static constexpr auto EmptyResourceForList =
      &(kw<"of"> + ResourceBindingName + lit<")">);
  struct ResourceForDeclList : rule<create<ast::VarDecl>(
      &ResourceForHead + ResourceKind +
      (EmptyResourceForList |
       (!EmptyResourceForList +
        append<&ast::VarDecl::declarations>(ref<ResourceForDeclarator>) +
        many(lit<","> + append<&ast::VarDecl::declarations>(ref<ResourceForDeclarator>)) +
        option(ResourceTrailingComma))))> {};

  static constexpr auto VarKind = kw<"var"> | kw<"let"> | kw<"const">;

  struct VarDeclList
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            some(append<&ast::VarDecl::declarations>(ref<ForDeclarator>), lit<",">))> {};

  struct BareDeclarator_
      : rule<create<ast::Declarator>(
            assign<&ast::Declarator::id>(ref<ArrayLiteral> | ref<ObjectLiteral>) +
            option(assign<&ast::Declarator::typeAnnotation>(
                ref<SharedTypeAnnotation>)) +
            lit<"="> +
            assign<&ast::Declarator::init>(ref<ContextAssign<GeneratorContext>>))> {};

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
            !(lit<"{"> | kw<"class"> | kw<"function"> | kw<"interface"> |
              kw<"abstract"> | ResourceHead) +
            assign<&ast::ExprStmt::expression>(ref<ContextExpression<GeneratorContext>>) + Eos)> {};

  struct BlockStmt_
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<ContextStatement<GeneratorContext>>)) +
            lit<"}">)> {};

  struct FunctionBody
      : rule<create<ast::BlockStmt>(
            lit<"{"> + many(append<&ast::BlockStmt::body>(ref<ContextStatement<GeneratorContext>>)) +
            lit<"}">)> {};

  struct IfStatement
      : rule<create<ast::IfStmt>(
            kw<"if"> + lit<"("> +
            assign<&ast::IfStmt::test>(ref<ContextExpression<GeneratorContext>>) + lit<")"> +
            assign<&ast::IfStmt::consequent>(ref<ContextStatement<GeneratorContext>>) +
            option(commit(kw<"else">, assign<&ast::IfStmt::alternate>(ref<ContextStatement<GeneratorContext>>))))> {};

  struct DoWhileStatement
      : rule<create<ast::DoWhileStmt>(
            kw<"do"> + assign<&ast::DoWhileStmt::body>(ref<ContextStatement<GeneratorContext>>) +
            kw<"while"> + lit<"("> +
            assign<&ast::DoWhileStmt::test>(ref<ContextExpression<GeneratorContext>>) + lit<")"> +
            option(lit<";">))> {};

  struct WhileStatement
      : rule<create<ast::WhileStmt>(
            kw<"while"> + lit<"("> +
            assign<&ast::WhileStmt::test>(ref<ContextExpression<GeneratorContext>>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  struct ForClassicStatement
      : rule<create<ast::ForStmt>(
            kw<"for"> + lit<"("> +
            option(assign<&ast::ForStmt::initDecl>(ref<VarDeclList> | ref<ResourceForDeclList>) |
                   assign<&ast::ForStmt::initExpr>(!ResourceForHead + ref<NoInExpression>)) +
            lit<";"> + option(assign<&ast::ForStmt::test>(ref<ContextExpression<GeneratorContext>>)) +
            lit<";"> +
            option(assign<&ast::ForStmt::update>(ref<ContextExpression<GeneratorContext>>)) +
            lit<")"> + assign<&ast::ForStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  // All for-header declarations share the native NoIn initializer shape.
  struct ForBindingDecl
      : rule<create<ast::VarDecl>(
            assign<&ast::VarDecl::kind>(VarKind) +
            append<&ast::VarDecl::declarations>(ref<ForDeclarator>))> {};

  struct ForInStatement
      : rule<create<ast::ForInStmt>(
            kw<"for"> + lit<"("> +
            (assign<&ast::ForInStmt::leftDecl>(ref<ForBindingDecl> | ref<ResourceForDeclList>)  //
             | assign<&ast::ForInStmt::leftExpr>(!ResourceForHead + ref<UnaryExpr>)) +
            kw<"in"> + assign<&ast::ForInStmt::right>(ref<ContextExpression<GeneratorContext>>) +
            lit<")"> + assign<&ast::ForInStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  struct ForOfStatement
      : rule<create<ast::ForOfStmt>(
            kw<"for"> +
            option(flag<&ast::ForOfStmt::isAwait>(kw<"await">)) +
            lit<"("> +
            (assign<&ast::ForOfStmt::leftDecl>(ref<ForBindingDecl> | ref<ResourceForDeclList>)  //
             | assign<&ast::ForOfStmt::leftExpr>(!ResourceForHead + ref<UnaryExpr>)) +
            kw<"of"> + assign<&ast::ForOfStmt::right>(ref<ContextAssign<GeneratorContext>>) +
            option(kw<"as"> + assign<&ast::ForOfStmt::asType>(ref<SharedType>)) +
            lit<")"> + assign<&ast::ForOfStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  struct IterationStatement
      : rule<ref<DoWhileStatement> | ref<WhileStatement> |
             ref<ForOfStatement> | ref<ForInStatement> |
             ref<ForClassicStatement>> {};

  static constexpr auto LabelName = [] {
    if constexpr (GeneratorContext) return ref<GeneratorIdentName> | kw<"await">;
    else return ref<IdentName> | kw<"await">;
  }();
  struct ContinueStatement
      : rule<create<ast::ContinueStmt>(
            kw<"continue"> +
            option(!LineBreak + assign<&ast::ContinueStmt::label>(LabelName)) +
            Eos)> {};

  struct BreakStatement
      : rule<create<ast::BreakStmt>(
            kw<"break"> +
            option(!LineBreak + assign<&ast::BreakStmt::label>(LabelName)) +
            Eos)> {};

  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
            kw<"return"> +
            option(!LineBreak +
                   assign<&ast::ReturnStmt::argument>(ref<ContextExpression<GeneratorContext>>)) +
            Eos)> {};

  struct ThrowStatement
      : rule<create<ast::ThrowStmt>(
            kw<"throw"> + !LineBreak +
            assign<&ast::ThrowStmt::argument>(ref<ContextExpression<GeneratorContext>>) + Eos)> {};

  struct WithStatement
      : rule<create<ast::WithStmt>(
            kw<"with"> + lit<"("> +
            assign<&ast::WithStmt::object>(ref<ContextExpression<GeneratorContext>>) + lit<")"> +
            assign<&ast::WithStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  struct LabelledStatement
      : rule<create<ast::LabeledStmt>(
            assign<&ast::LabeledStmt::label>(LabelName) +
            lit<":"> +
            assign<&ast::LabeledStmt::body>(ref<ContextStatement<GeneratorContext>>))> {};

  struct CaseClause
      : rule<create<ast::SwitchCase>(
            kw<"case"> + assign<&ast::SwitchCase::test>(ref<ContextExpression<GeneratorContext>>) +
            lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<ContextStatement<GeneratorContext>>)))> {};
  struct DefaultClause
      : rule<create<ast::SwitchCase>(
            kw<"default"> + lit<":"> +
            many(append<&ast::SwitchCase::consequent>(ref<ContextStatement<GeneratorContext>>)))> {};

  struct SwitchStatement
      : rule<create<ast::SwitchStmt>(
            kw<"switch"> + lit<"("> +
            assign<&ast::SwitchStmt::discriminant>(ref<ContextExpression<GeneratorContext>>) +
            lit<")"> + lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<CaseClause>)) +
            option(append<&ast::SwitchStmt::cases>(ref<DefaultClause>) +
                   many(append<&ast::SwitchStmt::cases>(ref<CaseClause>))) +
            lit<"}">)> {};

  struct CatchClause_
      : rule<create<ast::CatchClause>(
            kw<"catch"> +
            option(lit<"("> +
                   assign<&ast::CatchClause::param>(ref<Assignable>) +
                   option(assign<&ast::CatchClause::typeAnnotation>(
                       ref<SharedTypeAnnotation>)) +
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

  template <bool Generator>
  static constexpr auto FunctionDeclarationTail =
      FunctionSignatureFor<Generator, &ast::FunctionDecl::typeParams,
                           &ast::FunctionDecl::params, &ast::FunctionDecl::returnType> +
      (assign<&ast::FunctionDecl::body>(ref<ContextFunctionBody<Generator>>) | Eos);
  struct FunctionDeclaration
      : rule<create<ast::FunctionDecl>(
            DeclareFor<&ast::FunctionDecl::isDeclare> +
            option(flag<&ast::FunctionDecl::isAsync>(kw<"async">) + !LineBreak) +
            kw<"function"> +
            ((flag<&ast::FunctionDecl::isGenerator>(lit<"*">) +
              assign<&ast::FunctionDecl::name>(ref<RawId>) + FunctionDeclarationTail<true>) |
             (assign<&ast::FunctionDecl::name>(ref<RawId>) + FunctionDeclarationTail<false>)))> {};

  struct Statement
      : rule<ref<BlockStmt_> | ref<ResourceStatement> | ref<VariableStatement> | ref<ImportStatement> |
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
             ref<TryStatement> |
             ref<AbstractDecl> | ref<ExpressionStatement>> {};

  // -- Modules ----
  static constexpr auto ImportBinding = [] {
    if constexpr (GeneratorContext) return ref<GeneratorBindingName>;
    else return ref<AnyName>;
  }();
  // `type` leads a specifier as a modifier or as the name itself: the words
  // after it say which (`type`, `type as`, `type as as`, `type as as as`).
  struct ImportSpecifier
      : rule<create<ast::ImportSpecifier>(
            option(flag<&ast::ImportSpecifier::isTypeOnly>(kw<"type">)) +
            assign<&ast::ImportSpecifier::imported>(ref<ModuleExportName>) +
            option(kw<"as"> + assign<&ast::ImportSpecifier::local>(ImportBinding)))> {};

  static constexpr auto ImportNamedList =
      lit<"{"> +
      option(append<&ast::ImportStmt::specifiers>(ref<ImportSpecifier>) +
             many(lit<","> + append<&ast::ImportStmt::specifiers>(
                                  ref<ImportSpecifier>)) +
             option(lit<",">)) +
      lit<"}">;

  struct ImportNamespace
      : rule<create<ast::ImportSpecifier>(
            flag<&ast::ImportSpecifier::isNamespace>(lit<"*">) + kw<"as"> +
            assign<&ast::ImportSpecifier::local>(ImportBinding))> {};

  struct ImportDefault
      : rule<create<ast::ImportSpecifier>(
            assign<&ast::ImportSpecifier::local>(ImportBinding))> {};

  struct ImportStatement
      : rule<create<ast::ImportStmt>(
            kw<"import"> +
            option(flag<&ast::ImportStmt::isTypeOnly>(kw<"type"> +
                                                           !kw<"from">)) +
            ((((append<&ast::ImportStmt::specifiers>(ref<ImportDefault>) +
               option(lit<","> +
                      (append<&ast::ImportStmt::specifiers>(ref<ImportNamespace>) |
                       ImportNamedList))) |
              append<&ast::ImportStmt::specifiers>(ref<ImportNamespace>) |
              ImportNamedList) +
              kw<"from"> + assign<&ast::ImportStmt::source>(ref<STRING>) +
              ImportAttributesFor<&ast::ImportStmt::attributes> + Eos)  //
             | (assign<&ast::ImportStmt::source>(ref<STRING>) +
                ImportAttributesFor<&ast::ImportStmt::attributes> + Eos)))> {};

  struct ImportEqualsStatement
      : rule<create<ast::ImportEqualsDecl>(
            kw<"import"> +
            option(flag<&ast::ImportEqualsDecl::isTypeOnly>(
                kw<"type">)) +
            assign<&ast::ImportEqualsDecl::name>(ImportBinding) +
            lit<"="> +
            ((kw<"require"> + lit<"("> +
              assign<&ast::ImportEqualsDecl::source>(ref<STRING>) +
              lit<")">)  //
             | assign<&ast::ImportEqualsDecl::entity>(ref<QualifiedName>)) +
            Eos)> {};


  struct NamespaceExportDeclaration
      : rule<create<ast::NamespaceExportDecl>(
            kw<"export"> + kw<"as"> + kw<"namespace"> +
            assign<&ast::NamespaceExportDecl::name>(ref<RawId>) + Eos)> {};

  // Default function/class declarations may be anonymous. Keep declaration
  // nodes, rather than routing them through an expression-shaped fallback.
  struct DefaultFunctionDeclaration
      : rule<create<ast::FunctionDecl>(
            option(flag<&ast::FunctionDecl::isAsync>(kw<"async">) + !LineBreak) +
            kw<"function"> +
            ((flag<&ast::FunctionDecl::isGenerator>(lit<"*">) +
              option(assign<&ast::FunctionDecl::name>(ref<RawId>)) + FunctionDeclarationTail<true>) |
             (option(assign<&ast::FunctionDecl::name>(ref<RawId>)) + FunctionDeclarationTail<false>)))> {};

  struct DefaultClassDeclaration
      : rule<create<ast::ClassDecl>(
            option(flag<&ast::ClassDecl::isAbstract>(kw<"abstract">)) +
            kw<"class"> + option(assign<&ast::ClassDecl::name>(ref<RawId>)) +
            TypeParamsClauseFor<&ast::ClassDecl::typeParams> +
            ClassHeritageFor<&ast::ClassDecl::superClass,
                             &ast::ClassDecl::superExpr,
                             &ast::ClassDecl::implementsClause> +
            ClassBodyFor<&ast::ClassDecl::members>)> {};

  // Every declaration kind may be exported: var/class/function plus
  // interface/type/enum/namespace.
  struct ExportDeclaration_
      : rule<ref<ResourceStatement> | ref<VariableStatement> | ref<ClassDeclaration> |
             ref<FunctionDeclaration> | ref<InterfaceDeclaration> |
             ref<TypeAliasDeclaration> | ref<EnumDeclaration> |
             ref<NamespaceDeclaration> | ref<ImportEqualsStatement>> {};

  struct ExportStatement
      : rule<create<ast::ExportStmt>(
            many(append<&ast::ExportStmt::decorators>(ref<DecoratorExpr>)) +
            kw<"export"> +
            ((flag<&ast::ExportStmt::isDefault>(kw<"default">) +
              (assign<&ast::ExportStmt::declaration>(
                   ref<DefaultFunctionDeclaration> | ref<DefaultClassDeclaration> |
                   ref<VariableStatement> | ref<InterfaceDeclaration> |
                   ref<TypeAliasDeclaration> | ref<EnumDeclaration> |
                   ref<NamespaceDeclaration> | ref<ImportEqualsStatement>)  //
               | (assign<&ast::ExportStmt::defaultExpr>(ref<ContextAssign<GeneratorContext>>) +
                  Eos)))  //
             | (flag<&ast::ExportStmt::isEquals>(lit<"=">) +
                assign<&ast::ExportStmt::defaultExpr>(ref<ContextAssign<GeneratorContext>>) +
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
            many(append<&ast::Program::body>(ref<ContextStatement<GeneratorContext>>)))> {};

  using entry = Program_;
  using ignored = WS;
  // A keyword may not run into a name's byte, past ASCII included.
  static constexpr auto word_boundary = IdentifierContinue | cr<R"(\\)">;
  using hidden = list<SL_COMMENT, ML_COMMENT, HTML_COMMENT, CDATA_COMMENT>;
  // Soft keywords (`ContextualKeyword`, `get`/`set` included) are spelled
  // once, referenced both as the keyword and, through `AllKeywords`, as a
  // name, so a state that reads both decides by what follows.
};

// Complete both envelopes while their cross-context bridge classes are still
// only declarations. ADL must not instantiate a later nested rule while an
// earlier auto fragment in that same envelope is being deduced.
struct TypeScript : TypeScriptContext<false> {};
struct GeneratorContextRules : TypeScriptContext<true> {};

template <bool Generator>
struct ContextExpression : TypeScriptContext<Generator>::Expression {};
template <bool Generator>
struct ContextUnary : TypeScriptContext<Generator>::UnaryExpr {};
template <bool Generator>
struct ContextStatement : TypeScriptContext<Generator>::Statement {};
template <bool Generator>
struct ContextParam : TypeScriptContext<Generator>::ParamItem {};
template <bool Generator>
struct ContextFunctionBody : TypeScriptContext<Generator>::FunctionBody {};
template <bool Generator>
struct ContextBlock : TypeScriptContext<Generator>::BlockStmt_ {};
template <bool Generator>
struct ContextAssign : TypeScriptContext<Generator>::AssignExpr {};
template <bool Generator>
struct ContextNoInAssign : TypeScriptContext<Generator>::NoInAssignExpr {};
template <bool Generator>
struct ContextMethod : TypeScriptContext<Generator>::GenericMethodBody {};
struct SharedType : TypeScriptContext<false>::Type_ {};
struct SharedTypeAnnotation : TypeScriptContext<false>::TypeAnnotation {};
struct SharedTypeReference : TypeScriptContext<false>::TypeReference {};
struct SharedTypeMember : TypeScriptContext<false>::TypeMember {};
struct SharedTypeParam : TypeScriptContext<false>::TypeParam {};

}  // namespace typescript
