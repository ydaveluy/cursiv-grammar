// The dart2 language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "dart2/core/ast.hpp"

namespace dart2 {

inline constexpr auto drop_first = [](std::string_view t) { return t.substr(1); };

struct Dart2 : cursiv::GrammarSpec {
  // Dart identifiers are ASCII only, but include `$`.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_$">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct ML_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<ML_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  struct ID : terminal<cr<"a-zA-Z_$"> + many(cr<"a-zA-Z0-9_$">)> {};

  // -- keywords ----
  struct K_ABSTRACT : terminal<kw<"abstract">> {};
  struct K_AS : terminal<kw<"as">> {};
  struct K_ASSERT : terminal<kw<"assert">> {};
  struct K_ASYNC : terminal<kw<"async">> {};
  struct K_AWAIT : terminal<kw<"await">> {};
  struct K_BREAK : terminal<kw<"break">> {};
  struct K_CASE : terminal<kw<"case">> {};
  struct K_CATCH : terminal<kw<"catch">> {};
  struct K_CLASS : terminal<kw<"class">> {};
  struct K_CONST : terminal<kw<"const">> {};
  struct K_CONTINUE : terminal<kw<"continue">> {};
  struct K_COVARIANT : terminal<kw<"covariant">> {};
  struct K_DEFAULT : terminal<kw<"default">> {};
  struct K_DEFERRED : terminal<kw<"deferred">> {};
  struct K_DO : terminal<kw<"do">> {};
  struct K_ELSE : terminal<kw<"else">> {};
  struct K_ENUM : terminal<kw<"enum">> {};
  struct K_EXPORT : terminal<kw<"export">> {};
  struct K_EXTENDS : terminal<kw<"extends">> {};
  struct K_EXTENSION : terminal<kw<"extension">> {};
  struct K_EXTERNAL : terminal<kw<"external">> {};
  struct K_FACTORY : terminal<kw<"factory">> {};
  struct K_FALSE : terminal<kw<"false">> {};
  struct K_FINAL : terminal<kw<"final">> {};
  struct K_FINALLY : terminal<kw<"finally">> {};
  struct K_FOR : terminal<kw<"for">> {};
  struct K_FUNCTION : terminal<kw<"Function">> {};
  struct K_GET : terminal<kw<"get">> {};
  struct K_HIDE : terminal<kw<"hide">> {};
  struct K_IF : terminal<kw<"if">> {};
  struct K_IMPLEMENTS : terminal<kw<"implements">> {};
  struct K_IMPORT : terminal<kw<"import">> {};
  struct K_IN : terminal<kw<"in">> {};
  struct K_INTERFACE : terminal<kw<"interface">> {};
  struct K_IS : terminal<kw<"is">> {};
  struct K_LATE : terminal<kw<"late">> {};
  struct K_LIBRARY : terminal<kw<"library">> {};
  struct K_MIXIN : terminal<kw<"mixin">> {};
  struct K_NATIVE : terminal<kw<"native">> {};
  struct K_NEW : terminal<kw<"new">> {};
  struct K_NULL : terminal<kw<"null">> {};
  struct K_OF : terminal<kw<"of">> {};
  struct K_ON : terminal<kw<"on">> {};
  struct K_OPERATOR : terminal<kw<"operator">> {};
  struct K_PART : terminal<kw<"part">> {};
  struct K_REQUIRED : terminal<kw<"required">> {};
  struct K_RETHROW : terminal<kw<"rethrow">> {};
  struct K_RETURN : terminal<kw<"return">> {};
  struct K_SET : terminal<kw<"set">> {};
  struct K_SHOW : terminal<kw<"show">> {};
  struct K_STATIC : terminal<kw<"static">> {};
  struct K_SUPER : terminal<kw<"super">> {};
  struct K_SWITCH : terminal<kw<"switch">> {};
  struct K_SYNC : terminal<kw<"sync">> {};
  struct K_THIS : terminal<kw<"this">> {};
  struct K_THROW : terminal<kw<"throw">> {};
  struct K_TRUE : terminal<kw<"true">> {};
  struct K_TRY : terminal<kw<"try">> {};
  struct K_TYPEDEF : terminal<kw<"typedef">> {};
  struct K_VAR : terminal<kw<"var">> {};
  struct K_VOID : terminal<kw<"void">> {};
  struct K_WHILE : terminal<kw<"while">> {};
  struct K_WITH : terminal<kw<"with">> {};
  struct K_YIELD : terminal<kw<"yield">> {};

  struct IdentName
      : rule<ref<ID> | ref<K_ABSTRACT> | ref<K_AS> | ref<K_COVARIANT> |
             ref<K_DEFERRED> | ref<K_EXPORT> | ref<K_EXTERNAL> |
             ref<K_EXTENSION> | ref<K_FACTORY> | ref<K_GET> |
             ref<K_IMPLEMENTS> | ref<K_IMPORT> | ref<K_INTERFACE> |
             ref<K_LATE> | ref<K_LIBRARY> | ref<K_MIXIN> | ref<K_OPERATOR> |
             ref<K_PART> | ref<K_REQUIRED> | ref<K_SET> | ref<K_STATIC> |
             ref<K_TYPEDEF> | ref<K_ASYNC> | ref<K_AWAIT> | ref<K_YIELD> |
             ref<K_SYNC> | ref<K_ON> | ref<K_OF> | ref<K_HIDE> | ref<K_SHOW> |
             ref<K_NATIVE> | ref<K_FUNCTION>> {};
  struct TypeIdentName : rule<ref<IdentName>> {};

  // -- metadata ----
  struct Type_;
  struct Arguments;
  struct MetadataItem
      : rule<create<ast::Metadata>(
            lit<"@"> +
            ((assign<&ast::Metadata::qualifier>(ref<IdentName>) + lit<"."> +
              assign<&ast::Metadata::name>(ref<IdentName>))  //
             | assign<&ast::Metadata::name>(ref<IdentName>)) +
            option(lit<"<"> + append<&ast::Metadata::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::Metadata::typeArguments>(ref<Type_>)) +
                   lit<">">) +
            option(lit<"."> +
                   assign<&ast::Metadata::constructorName>(ref<IdentName>)) +
            option(assign<&ast::Metadata::args>(ref<Arguments>)))> {};

  // -- numbers ----
  static constexpr auto EXPONENT = cr<"eE"> + option(cr<"+-">) + some(d);
  struct NUMBER
      : terminal<(some(d) + option(lit<"."> + some(d)) + option(EXPONENT)) |
                 (lit<"."> + some(d) + option(EXPONENT))> {};
  struct HEX_NUMBER
      : terminal<(lit<"0x"> | lit<"0X">) + some(cr<"0-9a-fA-F">)> {};

  // -- strings ---- Text tokens include whitespace, so string content never
  // depends on the language's whitespace skipper. Raw literals are opaque.
  static constexpr auto EscapeSeq = lit<"\\"> + cr<R"(^\r\n)">;
  static constexpr auto BareNameStart = cr<"a-zA-Z_">;
  static constexpr auto LiteralDollar = lit<"$", !(lit<"{"> | BareNameStart)>;
  struct DQText
      : terminal<some(EscapeSeq | LiteralDollar | cr<R"(^"\\$\r\n)">)> {};
  struct SQText
      : terminal<some(EscapeSeq | LiteralDollar | cr<R"(^'\\$\r\n)">)> {};
  struct DQ3Text : terminal<some(EscapeSeq | LiteralDollar | cr<R"(^"\\$)">)> {
  };
  struct SQ3Text : terminal<some(EscapeSeq | LiteralDollar | cr<R"(^'\\$)">)> {
  };
  struct DQ3Quotes : terminal<lit<"\"\""> | lit<"\"">> {};
  struct SQ3Quotes : terminal<lit<"''"> | lit<"'">> {};
  struct InterpolatedName
      : terminal<lit<"$"> + BareNameStart + many(cr<"a-zA-Z0-9_">),
                 text_conv<drop_first>> {};
  static constexpr auto strip_raw1 = [](std::string_view t) {
    return t.substr(2, t.size() - 3);
  };
  static constexpr auto strip_raw3 = [](std::string_view t) {
    return t.substr(4, t.size() - 7);
  };

  struct RawDQ3String
      : terminal<through(lit<"r\"\"\"">, lit<"\"\"\"">), text_conv<strip_raw3>> {};
  struct RawSQ3String
      : terminal<through(lit<"r'''">, lit<"'''">), text_conv<strip_raw3>> {};
  struct RawDQString : terminal<lit<"r\""> + many(cr<R"(^"\r\n)">) + lit<"\"">,
                                text_conv<strip_raw1>> {};
  struct RawSQString : terminal<lit<"r'"> + many(cr<R"(^'\r\n)">) + lit<"'">,
                                text_conv<strip_raw1>> {};

  struct Expression;
  struct AssignExpression;
  struct ConditionalExpression;
  struct IfNullOrLower;
  struct EqualityExpression;
  struct RelationalExpression;
  struct BitwiseOrLevel;
  struct UnaryExpression;
  struct PostfixExpression;
  struct PrimaryExpression;
  struct Statement;
  struct BlockStmt;
  struct Type_;
  struct TypeNotVoid;
  struct Element_;
  struct ParamListRule;
  struct Arguments;

  struct Interpolation : rule<skip(ignored<WS>, hide<SL_COMMENT, ML_COMMENT>,
                                   lit<"${"> + ref<Expression> + lit<"}">) |
                              create<ast::Ident>(assign<&ast::Ident::name>(
                                  ref<InterpolatedName>))> {};
  template <class Text, class... Extra>
  static constexpr auto StringPieces = many(append<&ast::StringLit::parts>(
      (ref<Text> | ... | ref<Extra>) | ref<Interpolation>));

  // The opening delimiter belongs to the region: trivia before the first
  // element is read by the caller, while all string content is kept verbatim.
  struct DQStringLit
      : rule<create<ast::StringLit>(tight(lit<"\""> + StringPieces<DQText> + lit<"\"">))> {
  };
  struct SQStringLit
      : rule<create<ast::StringLit>(tight(lit<"'"> + StringPieces<SQText> + lit<"'">))> {};
  struct DQ3StringLit
      : rule<create<ast::StringLit>(
            tight(flag<&ast::StringLit::multiline>(lit<"\"\"\"">) +
                     StringPieces<DQ3Text, DQ3Quotes> + lit<"\"\"\"">))> {};
  struct SQ3StringLit
      : rule<create<ast::StringLit>(
            tight(flag<&ast::StringLit::multiline>(lit<"'''">) +
                     StringPieces<SQ3Text, SQ3Quotes> + lit<"'''">))> {};
  struct RawDQStringLit
      : rule<create<ast::StringLit>(flag<&ast::StringLit::raw>(
            append<&ast::StringLit::parts>(ref<RawDQString>)))> {};
  struct RawSQStringLit
      : rule<create<ast::StringLit>(flag<&ast::StringLit::raw>(
            append<&ast::StringLit::parts>(ref<RawSQString>)))> {};
  struct RawDQ3StringLit
      : rule<create<ast::StringLit>(flag<&ast::StringLit::raw>(
            flag<&ast::StringLit::multiline>(
                append<&ast::StringLit::parts>(ref<RawDQ3String>))))> {};
  struct RawSQ3StringLit
      : rule<create<ast::StringLit>(flag<&ast::StringLit::raw>(
            flag<&ast::StringLit::multiline>(
                append<&ast::StringLit::parts>(ref<RawSQ3String>))))> {};

  struct SingleStringLiteral
      : rule<ref<RawDQ3StringLit> | ref<RawSQ3StringLit> | ref<DQ3StringLit> |
             ref<SQ3StringLit> | ref<RawDQStringLit> | ref<RawSQStringLit> |
             ref<DQStringLit> | ref<SQStringLit>> {};

  struct StringLiteral
      : rule<create<ast::AdjacentStrings>(
            append<&ast::AdjacentStrings::parts>(ref<SingleStringLiteral>) +
            many(append<&ast::AdjacentStrings::parts>(ref<SingleStringLiteral>)))> {
  };

  struct SymbolName
      : rule<(lit<"["> + lit<"]"> + option(lit<"=">)) |
             (lit<">"> + lit<">"> + option(lit<">">)) |
             (lit<">"> + lit<"=">) | lit<"<<"> |
             lit<"=="> | lit<"<="> | lit<"~/"> |
             cr<"+*/%<>&^|~-"> |
             ((ref<IdentName> | ref<K_VOID>) +
              many(lit<"."> + ref<IdentName>))> {};
  struct SymbolToken : rule<lit<"#"> + ref<SymbolName>> {};
  struct SymbolLiteral
      : rule<assign<&ast::SymbolLit::text>(ref<SymbolToken>)> {};

  // -- types ----
  struct TypeParamRule
      : rule<assign<&ast::TypeParam::name>(ref<IdentName>) +
             option(ref<K_EXTENDS> +
                    assign<&ast::TypeParam::bound>(ref<TypeNotVoid>))> {};

  // Each host class owns its own `typeParameters`: a shared rule writing a
  // fixed member would create a second, independent node of that class
  // wherever it is referenced, on top of the one its caller already builds.
  static constexpr auto TypeParamsOfSeg =
      lit<"<"> +
      append<&ast::FunctionTypeSeg::typeParameters>(ref<TypeParamRule>) +
      many(lit<","> + append<&ast::FunctionTypeSeg::typeParameters>(
                           ref<TypeParamRule>)) +
      lit<">">;
  static constexpr auto TypeParamsOfSuffix =
      lit<"<"> +
      append<&ast::FunctionTypedSuffix::typeParameters>(ref<TypeParamRule>) +
      many(lit<","> + append<&ast::FunctionTypedSuffix::typeParameters>(
                           ref<TypeParamRule>)) +
      lit<">">;

  struct ParamTypeRule
      : rule<create<ast::ParamType>(
            assign<&ast::ParamType::type>(ref<Type_>) +
            option(assign<&ast::ParamType::name>(ref<IdentName>)))> {};

  struct NamedParamTypeRule
      : rule<create<ast::ParamType>(
            option(flag<&ast::ParamType::required>(ref<K_REQUIRED>)) +
            assign<&ast::ParamType::type>(ref<Type_>) +
            assign<&ast::ParamType::name>(ref<IdentName>))> {};

  static constexpr auto OptionalParamTypes =
      (lit<"["> +
       append<&ast::ParamTypeList::optionalPositional>(ref<ParamTypeRule>) +
       many(lit<","> + append<&ast::ParamTypeList::optionalPositional>(
                            ref<ParamTypeRule>)) +
       option(lit<",">) + lit<"]">) |
      (lit<"{"> + append<&ast::ParamTypeList::named>(ref<NamedParamTypeRule>) +
       many(lit<","> + append<&ast::ParamTypeList::named>(
                            ref<NamedParamTypeRule>)) +
       option(lit<",">) + lit<"}">);

  struct ParamTypeListRule
      : rule<create<ast::ParamTypeList>(
            lit<"("> +
            option((append<&ast::ParamTypeList::positional>(ref<ParamTypeRule>) +
                    many(lit<","> + append<&ast::ParamTypeList::positional>(
                                         ref<ParamTypeRule>)) +
                    option(lit<","> + option(OptionalParamTypes))) |
                   OptionalParamTypes) +
            lit<")">)> {};

  struct FunctionTypeSegRule
      : rule<create<ast::FunctionTypeSeg>(
            ref<K_FUNCTION> + option(TypeParamsOfSeg) +
            assign<&ast::FunctionTypeSeg::params>(ref<ParamTypeListRule>) +
            option(flag<&ast::FunctionTypeSeg::nullable>(lit<"?">)))> {};

  // The return type ahead of `Function(...)` may itself be `void`, unlike a
  // plain `TypeNotVoid` position.
  struct FunctionTypeRule
      : rule<create<ast::FunctionType>(
            option(assign<&ast::FunctionType::returnType>(
                ref<TypeNotVoid> |
                create<ast::NamedType>(
                    assign<&ast::NamedType::name>(ref<K_VOID>)))) +
            some(append<&ast::FunctionType::segments>(ref<FunctionTypeSegRule>)))> {
  };

  // `Function` alone, with no parameter list -- the top function type.
  struct BareFunctionType
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(ref<K_FUNCTION>) +
            option(flag<&ast::NamedType::nullable>(lit<"?">)))> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(
            ((assign<&ast::NamedType::qualifier>(ref<TypeIdentName>) + lit<"."> +
              assign<&ast::NamedType::name>(ref<TypeIdentName>))  //
             | assign<&ast::NamedType::name>(ref<TypeIdentName>)) +
            option(lit<"<"> +
                   append<&ast::NamedType::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::NamedType::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            option(flag<&ast::NamedType::nullable>(lit<"?">)))> {};

  // The function-type reading is tried first: with no return type and no
  // `Function` ahead it fails immediately and falls through.
  struct TypeNotVoid
      : rule<ref<FunctionTypeRule> | ref<BareFunctionType> |
             ref<NamedTypeRule>> {};

  struct Type_
      : rule<create<ast::NamedType>(assign<&ast::NamedType::name>(ref<K_VOID>)) |
             ref<TypeNotVoid>> {};

  // -- formal parameters ----
  struct FunctionTypedSuffix
      : rule<create<ast::FunctionTypedSuffix>(
            option(TypeParamsOfSuffix) +
            assign<&ast::FunctionTypedSuffix::params>(ref<ParamListRule>) +
            option(flag<&ast::FunctionTypedSuffix::nullable>(lit<"?">)))> {
  };

  struct DefaultParam
      : rule<create<ast::Param>(
            many(append<&ast::Param::annotations>(ref<MetadataItem>)) +
            option(flag<&ast::Param::isRequired>(ref<K_REQUIRED>)) +
            option(flag<&ast::Param::isCovariant>(ref<K_COVARIANT>)) +
            option(flag<&ast::Param::isFinal>(ref<K_FINAL>) |
                   flag<&ast::Param::isVar>(ref<K_VAR>)) +
            option(assign<&ast::Param::type>(ref<Type_>)) +
            option((flag<&ast::Param::isThis>(ref<K_THIS>) |
                    flag<&ast::Param::isSuper>(ref<K_SUPER>)) +
                   lit<".">) +
            assign<&ast::Param::name>(ref<IdentName>) +
            option(assign<&ast::Param::functionSuffix>(ref<FunctionTypedSuffix>)) +
            option((lit<"="> | lit<":">) +
                   assign<&ast::Param::defaultValue>(ref<Expression>)))> {};

  static constexpr auto OptionalParams =
      (lit<"["> + append<&ast::ParamList::optionalPositional>(ref<DefaultParam>) +
       many(lit<","> + append<&ast::ParamList::optionalPositional>(
                            ref<DefaultParam>)) +
       option(lit<",">) + lit<"]">) |
      (lit<"{"> + append<&ast::ParamList::named>(ref<DefaultParam>) +
       many(lit<","> + append<&ast::ParamList::named>(ref<DefaultParam>)) +
       option(lit<",">) + lit<"}">);

  struct ParamListRule
      : rule<create<ast::ParamList>(
            lit<"("> +
            option((append<&ast::ParamList::positional>(ref<DefaultParam>) +
                    many(lit<","> + append<&ast::ParamList::positional>(
                                         ref<DefaultParam>)) +
                    option(lit<","> + option(OptionalParams))) |
                   OptionalParams) +
            lit<")">)> {};
  // -- primary / postfix expressions ----
  struct IdentExpr
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ref<IdentName>))> {};
  struct ThisExpr_ : rule<create<ast::ThisExpr>(ref<K_THIS>)> {};
  struct SuperExpr_ : rule<create<ast::SuperExpr>(ref<K_SUPER>)> {};
  struct NullLit_ : rule<create<ast::NullLit> + ref<K_NULL>> {};
  struct BoolLit_
      : rule<create<ast::BoolLit>(
            flag<&ast::BoolLit::value>(ref<K_TRUE>) | ref<K_FALSE>)> {};
  struct NumberLit_
      : rule<assign<&ast::NumberLit::text>(ref<NUMBER> | ref<HEX_NUMBER>)> {};

  struct ListLit_
      : rule<create<ast::ListLit>(
            option(flag<&ast::ListLit::isConst>(ref<K_CONST>)) +
            option(lit<"<"> +
                   append<&ast::ListLit::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::ListLit::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            lit<"["> +
            option(append<&ast::ListLit::elements>(ref<Element_>) +
                   many(lit<","> +
                        append<&ast::ListLit::elements>(ref<Element_>)) +
                   option(lit<",">)) +
            lit<"]">)> {};

  struct SetOrMapLit_
      : rule<create<ast::SetOrMapLit>(
            option(flag<&ast::SetOrMapLit::isConst>(ref<K_CONST>)) +
            option(lit<"<"> +
                   append<&ast::SetOrMapLit::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::SetOrMapLit::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            lit<"{"> +
            option(append<&ast::SetOrMapLit::elements>(ref<Element_>) +
                   many(lit<","> +
                        append<&ast::SetOrMapLit::elements>(ref<Element_>)) +
                   option(lit<",">)) +
            lit<"}">)> {};

  struct Literal_
      : rule<ref<NullLit_> | ref<BoolLit_> | ref<NumberLit_> |
             ref<StringLiteral> | ref<SymbolLiteral> | ref<ListLit_> |
             ref<SetOrMapLit_>> {};

  struct ExprElement_
      : rule<create<ast::ExprElement>(
            assign<&ast::ExprElement::value>(ref<Expression>))> {};
  struct MapElement_
      : rule<create<ast::MapElement>(
            assign<&ast::MapElement::key>(ref<Expression>) + lit<":"> +
            assign<&ast::MapElement::value>(ref<Expression>))> {};
  struct SpreadElement_
      : rule<create<ast::SpreadElement>(
            (flag<&ast::SpreadElement::nullAware>(lit<"...?">) |
             lit<"...", !lit<"?">>)+assign<&ast::SpreadElement::
                                               value>(ref<Expression>))> {};
  struct IfElement_;
  struct ForElement_;
  struct Element_
      : rule<ref<IfElement_> | ref<ForElement_> | ref<SpreadElement_> |
             ref<MapElement_> | ref<ExprElement_>> {};
  struct IfElement_
      : rule<create<ast::IfElement>(
            ref<K_IF> + lit<"("> +
            assign<&ast::IfElement::cond>(ref<Expression>) + lit<")"> +
            assign<&ast::IfElement::thenElement>(ref<Element_>) +
            option(ref<K_ELSE> +
                   assign<&ast::IfElement::elseElement>(ref<Element_>)))> {};

  struct ForPartsRule;
  struct ForElement_
      : rule<create<ast::ForElement>(
            option(flag<&ast::ForElement::isAwait>(ref<K_AWAIT>)) +
            ref<K_FOR> + lit<"("> +
            assign<&ast::ForElement::parts>(ref<ForPartsRule>) + lit<")"> +
            assign<&ast::ForElement::body>(ref<Element_>))> {};

  struct ConstructorRefRule
      : rule<create<ast::ConstructorRef>(
            ((assign<&ast::ConstructorRef::qualifier>(ref<TypeIdentName>) +
              lit<"."> +
              assign<&ast::ConstructorRef::typeName>(ref<TypeIdentName>))  //
             | assign<&ast::ConstructorRef::typeName>(ref<TypeIdentName>)) +
            option(lit<"<"> +
                   append<&ast::ConstructorRef::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::ConstructorRef::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            option(lit<"."> +
                   assign<&ast::ConstructorRef::constructorName>(
                       ref<IdentName>)))> {};

  struct PositionalArgument
      : rule<create<ast::Argument>(
            assign<&ast::Argument::value>(ref<Expression>))> {};
  struct NamedArgument
      : rule<create<ast::Argument>(
            assign<&ast::Argument::name>(ref<IdentName>) + lit<":"> +
            assign<&ast::Argument::value>(ref<Expression>))> {};

  struct Arguments
      : rule<create<ast::ArgumentList>(
            lit<"("> +
            option(
                (append<&ast::ArgumentList::items>(ref<NamedArgument>) +
                 many(lit<","> + append<&ast::ArgumentList::items>(
                                      ref<NamedArgument>)))  //
                | (append<&ast::ArgumentList::items>(ref<PositionalArgument>) +
                   many(lit<","> + append<&ast::ArgumentList::items>(
                                        ref<PositionalArgument>)) +
                   many(lit<","> + append<&ast::ArgumentList::items>(
                                        ref<NamedArgument>)))) +
            option(lit<",">) + lit<")">)> {};

  struct NewExpr_
      : rule<create<ast::NewExpr>(
            ref<K_NEW> + assign<&ast::NewExpr::ctor>(ref<ConstructorRefRule>) +
            assign<&ast::NewExpr::args>(ref<Arguments>))> {};

  struct ConstObjectExpr_
      : rule<create<ast::NewExpr>(
            flag<&ast::NewExpr::isConst>(ref<K_CONST>) +
            assign<&ast::NewExpr::ctor>(ref<ConstructorRefRule>) +
            assign<&ast::NewExpr::args>(ref<Arguments>))> {};

  struct ConstructorInvocationExpr_
      : rule<create<ast::ConstructorInvocationExpr>(
            ((assign<&ast::ConstructorInvocationExpr::qualifier>(
                  ref<TypeIdentName>) +
              lit<"."> +
              assign<&ast::ConstructorInvocationExpr::typeName>(
                  ref<TypeIdentName>))  //
             | assign<&ast::ConstructorInvocationExpr::typeName>(
                   ref<TypeIdentName>)) +
            lit<"<"> +
            append<&ast::ConstructorInvocationExpr::typeArguments>(ref<Type_>) +
            many(lit<","> + append<&ast::ConstructorInvocationExpr::typeArguments>(
                                 ref<Type_>)) +
            lit<">"> + lit<"."> +
            assign<&ast::ConstructorInvocationExpr::constructorName>(
                ref<IdentName>) +
            assign<&ast::ConstructorInvocationExpr::args>(ref<Arguments>))> {};

  static constexpr auto FormalParameterPart_ =
      option(lit<"<"> +
             append<&ast::FunctionExpr::typeParameters>(ref<TypeParamRule>) +
             many(lit<","> + append<&ast::FunctionExpr::typeParameters>(
                                  ref<TypeParamRule>)) +
             lit<">">) +
      assign<&ast::FunctionExpr::params>(ref<ParamListRule>);

  // The `=>` form is an expression, never an operand: its body reads to the
  // end of the expression.
  struct FunctionExpr_
      : rule<create<ast::FunctionExpr>(
            FormalParameterPart_ +
            option(flag<&ast::FunctionExpr::isAsync>(ref<K_ASYNC>)) +
            lit<"=>"> +
            assign<&ast::FunctionExpr::bodyExpr>(ref<Expression>))> {};

  struct FunctionPrimary_
      : rule<create<ast::FunctionExpr>(
            FormalParameterPart_ +
            option((flag<&ast::FunctionExpr::isAsync>(ref<K_ASYNC>) +
                    option(flag<&ast::FunctionExpr::isGenerator>(
                        lit<"*">)))  //
                   | (flag<&ast::FunctionExpr::isSync>(ref<K_SYNC>) +
                      flag<&ast::FunctionExpr::isGenerator>(lit<"*">))) +
            assign<&ast::FunctionExpr::bodyBlock>(ref<BlockStmt>))> {};

  struct ParenExpr : rule<lit<"("> + ref<Expression> + lit<")">> {};

  struct PrimaryExpression
      : rule<ref<ThisExpr_> | ref<FunctionPrimary_> | ref<Literal_> |
             ref<NewExpr_> | ref<ConstObjectExpr_> |
             ref<ConstructorInvocationExpr_> | ref<IdentExpr> |
             ref<ParenExpr>> {};

  static constexpr auto SelectorTail =
      nest<ast::MemberExpr, &ast::MemberExpr::object>(
          (flag<&ast::MemberExpr::nullAware>(lit<"?.">) | lit<".">) +
          assign<&ast::MemberExpr::property>(ref<IdentName> | ref<K_NEW>))  //
      | nest<ast::IndexExpr, &ast::IndexExpr::object>(
            (flag<&ast::IndexExpr::nullAware>(lit<"?[">) | lit<"[">) +
            assign<&ast::IndexExpr::index>(ref<Expression>) + lit<"]">)  //
      | nest<ast::CallExpr, &ast::CallExpr::callee>(
            option(lit<"<"> +
                   append<&ast::CallExpr::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::CallExpr::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            assign<&ast::CallExpr::args>(ref<Arguments>))  //
      | nest<ast::TypeApplyExpr, &ast::TypeApplyExpr::expression>(
            lit<"<"> + append<&ast::TypeApplyExpr::typeArguments>(ref<Type_>) +
            many(lit<","> + append<&ast::TypeApplyExpr::typeArguments>(
                                 ref<Type_>)) +
            lit<">">)
      | nest<ast::NullAssertExpr, &ast::NullAssertExpr::operand>(lit<"!">);

  struct PostfixExpression
      : rule<(ref<PrimaryExpression> | ref<SuperExpr_>) + many(SelectorTail) +
             option(nest<ast::PostfixExpr, &ast::PostfixExpr::operand>(
                 assign<&ast::PostfixExpr::op>(lit<"++"> | lit<"--">)))> {};

  struct AwaitExpr_
      : rule<create<ast::AwaitExpr>(
            ref<K_AWAIT> +
            assign<&ast::AwaitExpr::operand>(ref<UnaryExpression>))> {};

  struct UnaryExpression
      : rule<create<ast::PrefixExpr>(
                 assign<&ast::PrefixExpr::op>(lit<"-", !cr<"-=">>) +
                 assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"!", !lit<"=">>) +
                   assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"~", !lit<"/">>) +
                   assign<&ast::PrefixExpr::operand>(ref<UnaryExpression>))  //
             | create<ast::PrefixExpr>(
                   assign<&ast::PrefixExpr::op>(lit<"++"> | lit<"--">) +
                   assign<&ast::PrefixExpr::operand>(ref<PostfixExpression>))  //
             | ref<AwaitExpr_>  //
             | ref<PostfixExpression>> {};

  // -- binary ladder below relational/equality ---- `>>` and `>>>` are two or
  // three separate `>` tokens, kept apart so a single `>` stays free to close
  // a nested generic; wrapped here so a multi-token spelling can sit in one
  // infix level.
  struct Shr3Op : rule<lit<">"> + lit<">"> + lit<">">> {};
  struct Shr2Op : rule<lit<">"> + lit<">">> {};

  struct BitwiseOrLevel
      : infix<UnaryExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"*", !lit<"=">> | lit<"/", !lit<"=">> |
                    lit<"~/", !lit<"=">> | lit<"%", !lit<"=">>) >>
               left(lit<"+", !cr<"+=">> | lit<"-", !cr<"-=">>) >>
               left(ref<Shr3Op> | ref<Shr2Op> | lit<"<<", !lit<"=">>) >>
               left(lit<"&", !cr<"&=">>) >> left(lit<"^", !lit<"=">>) >>
               left(lit<"|", !cr<"|=">>))> {};

  struct GeOp : rule<lit<">"> + lit<"=">> {};

  struct TypeTest_
      : rule<create<ast::IsTest>(
            assign<&ast::IsTest::operand>(ref<BitwiseOrLevel>) + ref<K_IS> +
            option(flag<&ast::IsTest::negated>(lit<"!">)) +
            assign<&ast::IsTest::type>(ref<TypeNotVoid>))> {};
  struct TypeCast_
      : rule<create<ast::AsExpr>(
            assign<&ast::AsExpr::operand>(ref<BitwiseOrLevel>) + ref<K_AS> +
            assign<&ast::AsExpr::type>(ref<TypeNotVoid>))> {};
  struct RelationalOp_
      : rule<create<ast::BinaryExpr>(
            assign<&ast::BinaryExpr::left>(ref<BitwiseOrLevel>) +
            assign<&ast::BinaryExpr::op>(ref<GeOp> | lit<">"> | lit<"<="> |
                                         lit<"<", !cr<"<=">>) +
            assign<&ast::BinaryExpr::right>(ref<BitwiseOrLevel>))> {};

  struct RelationalExpression
      : rule<ref<TypeTest_> | ref<TypeCast_> | ref<RelationalOp_> |
             ref<BitwiseOrLevel>> {};

  struct EqualityExpression
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<RelationalExpression>) +
                 assign<&ast::BinaryExpr::op>(lit<"=="> | lit<"!=">) +
                 assign<&ast::BinaryExpr::right>(ref<RelationalExpression>))  //
             | ref<RelationalExpression>> {};

  struct IfNullOrLower
      : infix<EqualityExpression, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(lit<"&&">) >> left(lit<"||">) >>
               left(lit<"??", !lit<"=">>))> {};

  struct ConditionalExpression
      : rule<create<ast::ConditionalExpr>(
                 assign<&ast::ConditionalExpr::cond>(ref<IfNullOrLower>) +
                 lit<"?", !cr<".?">> +
                 assign<&ast::ConditionalExpr::thenValue>(
                     ref<AssignExpression>) +
                 lit<":"> +
                 assign<&ast::ConditionalExpr::elseValue>(
                     ref<AssignExpression>))  //
             | ref<IfNullOrLower>> {};

  struct Shr3AssignOp : rule<lit<">"> + lit<">"> + lit<">"> + lit<"=">> {};
  struct Shr2AssignOp : rule<lit<">"> + lit<">"> + lit<"=">> {};

  struct AssignOp_
      : rule<ref<Shr3AssignOp> | ref<Shr2AssignOp> | lit<"=", !cr<"=>">> |
             lit<"*="> | lit<"/="> | lit<"~/="> | lit<"%="> | lit<"+="> |
             lit<"-="> | lit<"<<="> | lit<"&="> | lit<"^="> | lit<"|="> |
             lit<"??=">> {};

  struct AssignExpression
      : rule<create<ast::AssignExpr>(
                 assign<&ast::AssignExpr::target>(ref<PostfixExpression>) +
                 assign<&ast::AssignExpr::op>(ref<AssignOp_>) +
                 assign<&ast::AssignExpr::value>(ref<AssignExpression>))  //
             | ref<FunctionExpr_> | ref<ConditionalExpression>> {};

  // A cascade's steps (`..foo`, `[i]`, calls...) hang off an implicit
  // receiver that this AST does not spell out: each step is its own small
  // node instead of reusing MemberExpr/IndexExpr/CallExpr, which all expect
  // a real operand.
  static constexpr auto CascadeFirstStep =
      create<ast::CascadeIndexStep>(
          lit<"["> + assign<&ast::CascadeIndexStep::index>(ref<Expression>) +
          lit<"]">)  //
      | create<ast::CascadeMemberStep>(
            assign<&ast::CascadeMemberStep::property>(ref<IdentName>));

  static constexpr auto CascadeTailStep =
      create<ast::CascadeMemberStep>(
          lit<"."> +
          assign<&ast::CascadeMemberStep::property>(ref<IdentName>))  //
      | create<ast::CascadeIndexStep>(
            lit<"["> + assign<&ast::CascadeIndexStep::index>(ref<Expression>) +
            lit<"]">)  //
      | create<ast::CascadeCallStep>(
            option(lit<"<"> +
                   append<&ast::CascadeCallStep::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::CascadeCallStep::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            assign<&ast::CascadeCallStep::args>(ref<Arguments>))  //
      | create<ast::CascadeNullAssertStep>(lit<"!">);

  struct CascadeSection_
      : rule<create<ast::CascadeSection>(
            append<&ast::CascadeSection::steps>(CascadeFirstStep) +
            many(append<&ast::CascadeSection::steps>(CascadeTailStep)) +
            option(assign<&ast::CascadeSection::assignment>(
                create<ast::AssignExpr>(
                    assign<&ast::AssignExpr::op>(ref<AssignOp_>) +
                    assign<&ast::AssignExpr::value>(ref<Expression>)))))> {};

  struct Expression
      : rule<ref<AssignExpression> |
             create<ast::CascadeExpr>(
                 assign<&ast::CascadeExpr::target>(ref<ConditionalExpression>) +
                 (flag<&ast::CascadeExpr::nullAware>(lit<"?..">)  //
                  | lit<"..">)+append<&ast::CascadeExpr::
                                          sections>(ref<CascadeSection_>) +
                 many(lit<".."> + append<&ast::CascadeExpr::sections>(
                                      ref<CascadeSection_>))) |
             create<ast::ThrowExpr>(
                 ref<K_THROW> +
                 assign<&ast::ThrowExpr::value>(ref<Expression>))> {};

  // -- statements ----
  struct LabelRule
      : rule<assign<&ast::Label::name>(ref<IdentName>) + lit<":">> {};

  struct VarBindingInit
      : rule<assign<&ast::VarBinding::name>(ref<IdentName>) +
             option(lit<"="> +
                    assign<&ast::VarBinding::init>(ref<Expression>))> {};

  static constexpr auto VariableBindings =
      option(flag<&ast::VarSpec::isLate>(ref<K_LATE>)) +
      (((flag<&ast::VarSpec::isFinal>(ref<K_FINAL>) |
         flag<&ast::VarSpec::isConst>(ref<K_CONST>)) +
        option(assign<&ast::VarSpec::type>(ref<Type_>))) |
       flag<&ast::VarSpec::isVar>(ref<K_VAR>) |
       assign<&ast::VarSpec::type>(ref<Type_>)) +
      append<&ast::VarSpec::vars>(ref<VarBindingInit>) +
      many(lit<","> + append<&ast::VarSpec::vars>(ref<VarBindingInit>));

  struct LocalVarSpec_ : rule<create<ast::VarSpec>(VariableBindings)> {};

  struct VarDeclStmt_
      : rule<create<ast::VarDeclStmt>(
            assign<&ast::VarDeclStmt::spec>(ref<LocalVarSpec_>) + lit<";">)> {};

  struct AssertStmt_
      : rule<create<ast::AssertStmt>(
            ref<K_ASSERT> + lit<"("> +
            assign<&ast::AssertStmt::cond>(ref<Expression>) +
            option(lit<","> +
                   assign<&ast::AssertStmt::message>(ref<Expression>)) +
            option(lit<",">) + lit<")"> + lit<";">)> {};

  struct ForClassicParts_
      : rule<create<ast::ForClassicParts>(
            (assign<&ast::ForClassicParts::init>(ref<VarDeclStmt_>)  //
             | (option(assign<&ast::ForClassicParts::initExpr>(
                    ref<Expression>)) +
                lit<";">)) +
            option(assign<&ast::ForClassicParts::cond>(ref<Expression>)) +
            lit<";"> +
            option(append<&ast::ForClassicParts::update>(ref<Expression>) +
                   many(lit<","> + append<&ast::ForClassicParts::update>(
                                        ref<Expression>))))> {};

  struct ForInParts_
      : rule<create<ast::ForInParts>(
            (option(flag<&ast::ForInParts::isFinal>(ref<K_FINAL>) |
                    flag<&ast::ForInParts::isVar>(ref<K_VAR>) |
                    flag<&ast::ForInParts::isConst>(ref<K_CONST>)) +
             option(assign<&ast::ForInParts::type>(ref<TypeNotVoid>)) +
             assign<&ast::ForInParts::name>(ref<IdentName>)) +
            ref<K_IN> +
            assign<&ast::ForInParts::iterable>(ref<Expression>))> {};

  struct ForPartsRule : rule<ref<ForInParts_> | ref<ForClassicParts_>> {};

  struct BlockStmt
      : rule<create<ast::BlockStmt>(
            lit<"{"> +
            many(append<&ast::BlockStmt::statements>(ref<Statement>)) +
            lit<"}">)> {};

  struct FunctionDeclStmt_
      : rule<create<ast::FunctionDeclStmt>(
            option(assign<&ast::FunctionDeclStmt::returnType>(ref<Type_>)) +
            assign<&ast::FunctionDeclStmt::name>(ref<IdentName>) +
            assign<&ast::FunctionDeclStmt::function>(ref<FunctionExpr_> |
                                                     ref<FunctionPrimary_>))> {};

  struct ExprStmt_
      : rule<create<ast::ExprStmt>(
            option(assign<&ast::ExprStmt::value>(ref<Expression>)) +
            lit<";">)> {};

  struct ForStmt_
      : rule<create<ast::ForStmt>(
            option(flag<&ast::ForStmt::isAwait>(ref<K_AWAIT>)) +
            ref<K_FOR> + lit<"("> +
            assign<&ast::ForStmt::parts>(ref<ForPartsRule>) + lit<")"> +
            assign<&ast::ForStmt::body>(ref<Statement>))> {};

  struct WhileStmt_
      : rule<create<ast::WhileStmt>(
            ref<K_WHILE> + lit<"("> +
            assign<&ast::WhileStmt::cond>(ref<Expression>) + lit<")"> +
            assign<&ast::WhileStmt::body>(ref<Statement>))> {};

  struct DoStmt_
      : rule<create<ast::DoStmt>(
            ref<K_DO> + assign<&ast::DoStmt::body>(ref<Statement>) +
            ref<K_WHILE> + lit<"("> +
            assign<&ast::DoStmt::cond>(ref<Expression>) + lit<")"> + lit<";">)> {
  };

  struct SwitchCase_
      : rule<create<ast::SwitchCase>(
            many(append<&ast::SwitchCase::labels>(ref<LabelRule>)) +
            ((ref<K_CASE> + assign<&ast::SwitchCase::value>(ref<Expression>) +
              lit<":">)  //
             | (ref<K_DEFAULT> + lit<":">)) +
            many(append<&ast::SwitchCase::body>(ref<Statement>)))> {};

  struct SwitchStmt_
      : rule<create<ast::SwitchStmt>(
            ref<K_SWITCH> + lit<"("> +
            assign<&ast::SwitchStmt::value>(ref<Expression>) + lit<")"> +
            lit<"{"> +
            many(append<&ast::SwitchStmt::cases>(ref<SwitchCase_>)) +
            lit<"}">)> {};

  struct IfStmt_
      : rule<create<ast::IfStmt>(
            ref<K_IF> + lit<"("> +
            assign<&ast::IfStmt::cond>(ref<Expression>) + lit<")"> +
            assign<&ast::IfStmt::thenBranch>(ref<Statement>) +
            option(ref<K_ELSE> +
                   assign<&ast::IfStmt::elseBranch>(ref<Statement>)))> {};

  struct RethrowStmt_
      : rule<create<ast::RethrowStmt> + ref<K_RETHROW> + lit<";">> {};

  struct CatchClause_
      : rule<create<ast::CatchClause>(
            ((ref<K_CATCH> + lit<"("> +
              assign<&ast::CatchClause::exceptionName>(ref<IdentName>) +
              option(lit<","> + assign<&ast::CatchClause::stackTraceName>(
                                     ref<IdentName>)) +
              lit<")">)  //
             | (ref<K_ON> + assign<&ast::CatchClause::onType>(ref<TypeNotVoid>) +
                option(ref<K_CATCH> + lit<"("> +
                       assign<&ast::CatchClause::exceptionName>(ref<IdentName>) +
                       option(lit<","> +
                              assign<&ast::CatchClause::stackTraceName>(
                                  ref<IdentName>)) +
                       lit<")">))) +
            assign<&ast::CatchClause::body>(ref<BlockStmt>))> {};

  struct TryStmt_
      : rule<create<ast::TryStmt>(
            ref<K_TRY> + assign<&ast::TryStmt::body>(ref<BlockStmt>) +
            ((some(append<&ast::TryStmt::catches>(ref<CatchClause_>)) +
              option(ref<K_FINALLY> + assign<&ast::TryStmt::finallyBlock>(
                                           ref<BlockStmt>)))  //
             | (ref<K_FINALLY> +
                assign<&ast::TryStmt::finallyBlock>(ref<BlockStmt>))))> {};

  struct BreakStmt_
      : rule<create<ast::BreakStmt>(
            ref<K_BREAK> +
            option(assign<&ast::BreakStmt::label>(ref<IdentName>)) +
            lit<";">)> {};
  struct ContinueStmt_
      : rule<create<ast::ContinueStmt>(
            ref<K_CONTINUE> +
            option(assign<&ast::ContinueStmt::label>(ref<IdentName>)) +
            lit<";">)> {};
  struct ReturnStmt_
      : rule<create<ast::ReturnStmt>(
            ref<K_RETURN> +
            option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
            lit<";">)> {};
  struct YieldStmt_
      : rule<create<ast::YieldStmt>(
            ref<K_YIELD> +
            option(flag<&ast::YieldStmt::isEach>(lit<"*">)) +
            assign<&ast::YieldStmt::value>(ref<Expression>) + lit<";">)> {};

  // The statements no top-level declaration also reads.
  struct ExecStatement
      : rule<ref<BlockStmt> | ref<ForStmt_> | ref<WhileStmt_> | ref<DoStmt_> |
             ref<SwitchStmt_> | ref<IfStmt_> | ref<RethrowStmt_> |
             ref<TryStmt_> | ref<BreakStmt_> | ref<ContinueStmt_> |
             ref<ReturnStmt_> | ref<YieldStmt_> | ref<ExprStmt_> |
             ref<AssertStmt_>> {};

  struct NonLabelledStatement
      : rule<ref<VarDeclStmt_> | ref<ExecStatement> | ref<FunctionDeclStmt_>> {};

  static constexpr auto Labelled = create<ast::LabelledStmt>(
      some(append<&ast::LabelledStmt::labels>(ref<LabelRule>)) +
      assign<&ast::LabelledStmt::body>(ref<NonLabelledStatement>));

  struct Statement : rule<Labelled | ref<NonLabelledStatement>> {};

  // A document that is one declaration is a library, never a bare statement.
  struct EntryStatement : rule<Labelled | ref<ExecStatement>> {};

  // -- directives ----
  struct CombinatorRule
      : rule<create<ast::Combinator>(
            (flag<&ast::Combinator::isHide>(ref<K_HIDE>) | ref<K_SHOW>) +
            append<&ast::Combinator::names>(ref<IdentName>) +
            many(lit<","> + append<&ast::Combinator::names>(ref<IdentName>)))> {
  };

  struct ConfigurationUri_
      : rule<create<ast::ConfiguredUri>(
            ref<K_IF> + lit<"("> +
            append<&ast::ConfiguredUri::testName>(ref<IdentName>) +
            many(lit<"."> + append<&ast::ConfiguredUri::testName>(ref<IdentName>)) +
            option(lit<"=="> + assign<&ast::ConfiguredUri::testValue>(
                                    ref<SingleStringLiteral>)) +
            lit<")"> +
            assign<&ast::ConfiguredUri::uri>(ref<SingleStringLiteral>))> {};

  struct ImportDirective_
      : rule<create<ast::ImportDirective>(
            many(append<&ast::ImportDirective::annotations>(ref<MetadataItem>)) +
            ref<K_IMPORT> +
            assign<&ast::ImportDirective::uri>(ref<SingleStringLiteral>) +
            many(append<&ast::ImportDirective::configurations>(
                ref<ConfigurationUri_>)) +
            option(option(flag<&ast::ImportDirective::isDeferred>(
                       ref<K_DEFERRED>)) +
                   ref<K_AS> +
                   assign<&ast::ImportDirective::asName>(ref<IdentName>)) +
            many(append<&ast::ImportDirective::combinators>(
                ref<CombinatorRule>)) +
            lit<";">)> {};

  struct ExportDirective_
      : rule<create<ast::ExportDirective>(
            many(append<&ast::ExportDirective::annotations>(ref<MetadataItem>)) +
            ref<K_EXPORT> +
            assign<&ast::ExportDirective::uri>(ref<SingleStringLiteral>) +
            many(append<&ast::ExportDirective::configurations>(
                ref<ConfigurationUri_>)) +
            many(append<&ast::ExportDirective::combinators>(
                ref<CombinatorRule>)) +
            lit<";">)> {};

  struct PartDirective_
      : rule<create<ast::PartDirective>(
            many(append<&ast::PartDirective::annotations>(ref<MetadataItem>)) +
            ref<K_PART> +
            assign<&ast::PartDirective::uri>(ref<SingleStringLiteral>) +
            lit<";">)> {};

  struct PartOfDirective_
      : rule<create<ast::PartOfDirective>(
            many(append<&ast::PartOfDirective::annotations>(ref<MetadataItem>)) +
            ref<K_PART> + ref<K_OF> +
            ((append<&ast::PartOfDirective::nameParts>(ref<IdentName>) +
              many(lit<"."> +
                   append<&ast::PartOfDirective::nameParts>(ref<IdentName>)))  //
             | assign<&ast::PartOfDirective::uri>(ref<SingleStringLiteral>)) +
            lit<";">)> {};

  struct LibraryName_
      : rule<create<ast::LibraryName>(
            many(append<&ast::LibraryName::annotations>(ref<MetadataItem>)) +
            ref<K_LIBRARY> +
            option(append<&ast::LibraryName::parts>(ref<IdentName>) +
                   many(lit<"."> + append<&ast::LibraryName::parts>(ref<IdentName>))) +
            lit<";">)> {};

  // -- class members ----
  struct FieldSpec_
      : rule<create<ast::VarSpec>(
            option(flag<&ast::VarSpec::isExternal>(ref<K_EXTERNAL>)) +
            option(flag<&ast::VarSpec::isStatic>(ref<K_STATIC>)) +
            option(flag<&ast::VarSpec::isCovariant>(ref<K_COVARIANT>)) +
            VariableBindings)> {};

  static constexpr auto FieldDecl_ = create<ast::FieldDecl>(
      assign<&ast::FieldDecl::spec>(ref<FieldSpec_>) + lit<";">);

  struct FieldInitializer_
      : rule<create<ast::FieldInitializer>(
            option(ref<K_THIS> + lit<".">) +
            assign<&ast::FieldInitializer::name>(ref<IdentName>) + lit<"="> +
            assign<&ast::FieldInitializer::value>(ref<Expression>))> {};

  struct SuperInitializer_
      : rule<create<ast::SuperInitializer>(
            ref<K_SUPER> +
            option(lit<"."> + assign<&ast::SuperInitializer::constructorName>(
                                   ref<IdentName>)) +
            assign<&ast::SuperInitializer::args>(ref<Arguments>))> {};

  struct AssertInitializer_
      : rule<create<ast::AssertInitializer>(
            ref<K_ASSERT> + lit<"("> +
            assign<&ast::AssertInitializer::cond>(ref<Expression>) +
            option(lit<","> +
                   assign<&ast::AssertInitializer::message>(ref<Expression>)) +
            option(lit<",">) + lit<")">)> {};

  struct InitializerEntry
      : rule<ref<SuperInitializer_> | ref<FieldInitializer_> |
             ref<AssertInitializer_>> {};

  // Merges two constructor forms -- a signature-only one ending in `;`, and
  // one reached with a body via a method signature plus function body --
  // into one production that allows a redirect/initializer list before
  // either body form: an over-acceptance, never a refusal of valid Dart.
  static constexpr auto ConstructorDecl_ = create<ast::ConstructorDecl>(
      option(flag<&ast::ConstructorDecl::isExternal>(ref<K_EXTERNAL>)) +
      option(flag<&ast::ConstructorDecl::isConst>(ref<K_CONST>)) +
      assign<&ast::ConstructorDecl::className>(ref<TypeIdentName>) +
      option(lit<"."> + assign<&ast::ConstructorDecl::name>(ref<IdentName>)) +
      assign<&ast::ConstructorDecl::params>(ref<ParamListRule>) +
      option(lit<":"> +
             (assign<&ast::ConstructorDecl::redirect>(create<ast::Redirection>(
                  ref<K_THIS> +
                  option(lit<"."> + assign<&ast::Redirection::constructorName>(
                                        ref<IdentName>)) +
                  assign<&ast::Redirection::args>(ref<Arguments>)))  //
              | (append<&ast::ConstructorDecl::initializers>(
                     ref<InitializerEntry>) +
                 many(lit<","> + append<&ast::ConstructorDecl::initializers>(
                                     ref<InitializerEntry>))))) +
      (lit<";">  //
       | (lit<"=>"> + assign<&ast::ConstructorDecl::bodyExpr>(ref<Expression>) +
          lit<";">)  //
       | assign<&ast::ConstructorDecl::body>(ref<BlockStmt>)));

  static constexpr auto FactoryConstructor_ = create<ast::ConstructorDecl>(
      option(flag<&ast::ConstructorDecl::isExternal>(ref<K_EXTERNAL>)) +
      option(flag<&ast::ConstructorDecl::isConst>(ref<K_CONST>)) +
      flag<&ast::ConstructorDecl::isFactory>(ref<K_FACTORY>) +
      assign<&ast::ConstructorDecl::className>(ref<TypeIdentName>) +
      option(lit<"."> + assign<&ast::ConstructorDecl::name>(ref<IdentName>)) +
      assign<&ast::ConstructorDecl::params>(ref<ParamListRule>) +
      (lit<";">  //
       | (lit<"="> +
          assign<&ast::ConstructorDecl::factoryRedirect>(
              ref<ConstructorRefRule>) +
          lit<";">)  //
       | (lit<"=>"> + assign<&ast::ConstructorDecl::bodyExpr>(ref<Expression>) +
          lit<";">)  //
       | assign<&ast::ConstructorDecl::body>(ref<BlockStmt>)));

  struct GetKw : rule<ref<K_GET>, value<ast::MemberKind::getter>> {};
  struct SetKw : rule<ref<K_SET>, value<ast::MemberKind::setter>> {};
  struct OpKw : rule<ref<K_OPERATOR>, value<ast::MemberKind::op>> {};

  // Each snippet below writes into the same class its caller creates: a
  // separate `rule<>` here would count as a second, independent head of
  // that class (see the `TypeParamsOf*` note above), so these stay plain
  // `static constexpr auto` bodies, spliced in rather than `ref<>`'d.
  static constexpr auto MethodBody_ =
      (flag<&ast::MethodDecl::isNative>(ref<K_NATIVE>) +
       option(assign<&ast::MethodDecl::nativeName>(
           ref<SingleStringLiteral>)) +
       lit<";">)  //
      | (option(flag<&ast::MethodDecl::isAsync>(ref<K_ASYNC>)) +
         lit<"=>"> + assign<&ast::MethodDecl::bodyExpr>(ref<Expression>) +
         lit<";">)  //
      | (option((flag<&ast::MethodDecl::isAsync>(ref<K_ASYNC>) +
                 option(flag<&ast::MethodDecl::isGenerator>(lit<"*">)))  //
                | (flag<&ast::MethodDecl::isSync>(ref<K_SYNC>) +
                   flag<&ast::MethodDecl::isGenerator>(lit<"*">))) +
         assign<&ast::MethodDecl::body>(ref<BlockStmt>))  //
      | lit<";">;

  static constexpr auto GetterSig_ =
      option(assign<&ast::MethodDecl::returnType>(ref<Type_>)) +
      assign<&ast::MethodDecl::kind>(ref<GetKw>) +
      assign<&ast::MethodDecl::name>(ref<IdentName>);
  static constexpr auto SetterSig_ =
      option(assign<&ast::MethodDecl::returnType>(ref<Type_>)) +
      assign<&ast::MethodDecl::kind>(ref<SetKw>) +
      assign<&ast::MethodDecl::name>(ref<IdentName>) +
      assign<&ast::MethodDecl::params>(ref<ParamListRule>);
  // A multi-token spelling (`[]`, `[]=`, `>>`, `>>>`, `>=`) needs its own
  // named rule: fed inline to `assign<>`, the same text crashes the
  // generated parser.
  struct IndexOpTok : rule<lit<"["> + lit<"]">> {};
  struct IndexEqOpTok : rule<lit<"["> + lit<"]"> + lit<"=">> {};

  static constexpr auto OperatorSig_ =
      option(assign<&ast::MethodDecl::returnType>(ref<Type_>)) +
      assign<&ast::MethodDecl::kind>(ref<OpKw>) +
      assign<&ast::MethodDecl::name>(
          lit<"=="> | lit<"~/", !lit<"=">> | lit<"~"> | lit<"*", !lit<"=">> |
          lit<"/", !lit<"=">> | lit<"%", !lit<"=">> | lit<"+", !cr<"+=">> |
          lit<"-", !cr<"-=">> | lit<"<<", !lit<"=">> | ref<Shr3Op> |
          ref<Shr2Op> | lit<"&", !cr<"&=">> | lit<"^", !lit<"=">> |
          lit<"|", !cr<"|=">> | ref<GeOp> | lit<">"> | lit<"<="> |
          lit<"<", !cr<"<=">> | ref<IndexEqOpTok> | ref<IndexOpTok>) +
      assign<&ast::MethodDecl::params>(ref<ParamListRule>);
  static constexpr auto FunctionSig_ =
      option(assign<&ast::MethodDecl::returnType>(ref<Type_>)) +
      assign<&ast::MethodDecl::name>(ref<IdentName>) +
      option(lit<"<"> +
             append<&ast::MethodDecl::typeParameters>(ref<TypeParamRule>) +
             many(lit<","> + append<&ast::MethodDecl::typeParameters>(
                                  ref<TypeParamRule>)) +
             lit<">">) +
      assign<&ast::MethodDecl::params>(ref<ParamListRule>);

  static constexpr auto MethodDecl_ = create<ast::MethodDecl>(
      option(flag<&ast::MethodDecl::isExternal>(ref<K_EXTERNAL>)) +
      option(flag<&ast::MethodDecl::isStatic>(ref<K_STATIC>)) +
      (GetterSig_ | SetterSig_ | OperatorSig_ | FunctionSig_) + MethodBody_);

  // The annotation prefix is SPLICED here, not a rule of its own: a named
  // rule that appends and then dispatches is refused as a second head.
  struct MemberDecl
      : rule<many(append<&ast::Member::annotations>(ref<MetadataItem>)) +
             (FactoryConstructor_ | ConstructorDecl_ | FieldDecl_ |
              MethodDecl_)> {};

  static constexpr auto TypeParamsOfClass =
      lit<"<"> + append<&ast::ClassDecl::typeParameters>(ref<TypeParamRule>) +
      many(lit<","> +
           append<&ast::ClassDecl::typeParameters>(ref<TypeParamRule>)) +
      lit<">">;

  static constexpr auto ClassMixins =
      ref<K_WITH> + append<&ast::ClassDecl::mixins>(ref<TypeNotVoid>) +
      many(lit<","> + append<&ast::ClassDecl::mixins>(ref<TypeNotVoid>));
  static constexpr auto ClassInterfaces =
      ref<K_IMPLEMENTS> + append<&ast::ClassDecl::interfaces>(ref<TypeNotVoid>) +
      many(lit<","> + append<&ast::ClassDecl::interfaces>(ref<TypeNotVoid>));

  static constexpr auto ClassDecl_ = create<ast::ClassDecl>(
      option(flag<&ast::ClassDecl::isAbstract>(ref<K_ABSTRACT>)) +
      ref<K_CLASS> + assign<&ast::ClassDecl::name>(ref<IdentName>) +
      option(TypeParamsOfClass) +
      ((option(ref<K_EXTENDS> +
               assign<&ast::ClassDecl::superclass>(ref<TypeNotVoid>)) +
        option(ClassMixins) + option(ClassInterfaces) + lit<"{"> +
        many(append<&ast::ClassDecl::members>(ref<MemberDecl>)) + lit<"}">) |
       (lit<"="> + assign<&ast::ClassDecl::mixinApplication>(ref<TypeNotVoid>) +
        ClassMixins + option(ClassInterfaces) + lit<";">)));

  static constexpr auto MixinDecl_ = create<ast::MixinDecl>(
      ref<K_MIXIN> + assign<&ast::MixinDecl::name>(ref<TypeIdentName>) +
      option(lit<"<"> +
             append<&ast::MixinDecl::typeParameters>(ref<TypeParamRule>) +
             many(lit<","> +
                  append<&ast::MixinDecl::typeParameters>(ref<TypeParamRule>)) +
             lit<">">) +
      option(
          ref<K_ON> + append<&ast::MixinDecl::onTypes>(ref<TypeNotVoid>) +
          many(lit<","> + append<&ast::MixinDecl::onTypes>(ref<TypeNotVoid>))) +
      option(ref<K_IMPLEMENTS> +
             append<&ast::MixinDecl::interfaces>(ref<TypeNotVoid>) +
             many(lit<","> +
                  append<&ast::MixinDecl::interfaces>(ref<TypeNotVoid>))) +
      lit<"{"> + many(append<&ast::MixinDecl::members>(ref<MemberDecl>)) +
      lit<"}">);

  static constexpr auto ExtensionDecl_ = create<ast::ExtensionDecl>(
      ref<K_EXTENSION> +
      option(assign<&ast::ExtensionDecl::name>(ref<IdentName>)) +
      option(lit<"<"> +
             append<&ast::ExtensionDecl::typeParameters>(ref<TypeParamRule>) +
             many(lit<","> + append<&ast::ExtensionDecl::typeParameters>(
                                 ref<TypeParamRule>)) +
             lit<">">) +
      ref<K_ON> + assign<&ast::ExtensionDecl::onType>(ref<Type_>) + lit<"{"> +
      many(append<&ast::ExtensionDecl::members>(ref<MemberDecl>)) + lit<"}">);

  struct EnumEntry_
      : rule<create<ast::EnumEntry>(
            many(append<&ast::EnumEntry::annotations>(ref<MetadataItem>)) +
            assign<&ast::EnumEntry::name>(ref<IdentName>) +
            option(lit<"<"> +
                   append<&ast::EnumEntry::typeArguments>(ref<Type_>) +
                   many(lit<","> + append<&ast::EnumEntry::typeArguments>(
                                        ref<Type_>)) +
                   lit<">">) +
            option(lit<"."> +
                   assign<&ast::EnumEntry::constructorName>(ref<IdentName>)) +
            option(assign<&ast::EnumEntry::args>(ref<Arguments>)))> {};

  static constexpr auto EnumDecl_ = create<ast::EnumDecl>(
      ref<K_ENUM> + assign<&ast::EnumDecl::name>(ref<IdentName>) +
      option(lit<"<"> +
             append<&ast::EnumDecl::typeParameters>(ref<TypeParamRule>) +
             many(lit<","> +
                  append<&ast::EnumDecl::typeParameters>(ref<TypeParamRule>)) +
             lit<">">) +
      option(
          ref<K_WITH> + append<&ast::EnumDecl::mixins>(ref<TypeNotVoid>) +
          many(lit<","> + append<&ast::EnumDecl::mixins>(ref<TypeNotVoid>))) +
      option(ref<K_IMPLEMENTS> +
             append<&ast::EnumDecl::interfaces>(ref<TypeNotVoid>) +
             many(lit<","> +
                  append<&ast::EnumDecl::interfaces>(ref<TypeNotVoid>))) +
      lit<"{"> +
      option(append<&ast::EnumDecl::entries>(ref<EnumEntry_>) +
             many(lit<","> + append<&ast::EnumDecl::entries>(ref<EnumEntry_>)) +
             option(lit<",">)) +
      option(lit<";"> +
             many(append<&ast::EnumDecl::members>(ref<MemberDecl>))) +
      lit<"}">);

  static constexpr auto TypedefTypeParams =
      lit<"<"> + append<&ast::TypedefDecl::typeParameters>(ref<TypeParamRule>) +
      many(lit<","> + append<&ast::TypedefDecl::typeParameters>(ref<TypeParamRule>)) +
      lit<">">;

  static constexpr auto TypedefDecl_ = create<ast::TypedefDecl>(
      ref<K_TYPEDEF> +
      ((assign<&ast::TypedefDecl::name>(ref<TypeIdentName>) +
        option(TypedefTypeParams) + lit<"="> +
        assign<&ast::TypedefDecl::aliasedType>(ref<Type_>) + lit<";">)  //
       | (option(assign<&ast::TypedefDecl::functionReturnType>(ref<Type_>)) +
          assign<&ast::TypedefDecl::name>(ref<IdentName>) +
          option(TypedefTypeParams) +
          assign<&ast::TypedefDecl::functionParams>(ref<ParamListRule>) +
          lit<";">)));

  struct TopLevelVarSpec_
      : rule<create<ast::VarSpec>(
            option(flag<&ast::VarSpec::isExternal>(ref<K_EXTERNAL>)) +
            VariableBindings)> {};

  static constexpr auto TopLevelVariable_ = create<ast::TopLevelVariable>(
      assign<&ast::TopLevelVariable::spec>(ref<TopLevelVarSpec_>) + lit<";">);

  struct TopLevelGetKw
      : rule<ref<K_GET>, value<ast::MemberKind::getter>> {};
  struct TopLevelSetKw
      : rule<ref<K_SET>, value<ast::MemberKind::setter>> {};

  static constexpr auto TopLevelGetterSig_ =
      option(assign<&ast::TopLevelFunction::returnType>(ref<Type_>)) +
      assign<&ast::TopLevelFunction::kind>(ref<TopLevelGetKw>) +
      assign<&ast::TopLevelFunction::name>(ref<IdentName>);
  static constexpr auto TopLevelSetterSig_ =
      option(assign<&ast::TopLevelFunction::returnType>(ref<Type_>)) +
      assign<&ast::TopLevelFunction::kind>(ref<TopLevelSetKw>) +
      assign<&ast::TopLevelFunction::name>(ref<IdentName>) +
      assign<&ast::TopLevelFunction::params>(ref<ParamListRule>);
  static constexpr auto TopLevelFunctionSig_ =
      option(assign<&ast::TopLevelFunction::returnType>(ref<Type_>)) +
      assign<&ast::TopLevelFunction::name>(ref<IdentName>) +
      option(lit<"<"> +
             append<&ast::TopLevelFunction::typeParameters>(
                 ref<TypeParamRule>) +
             many(lit<","> + append<&ast::TopLevelFunction::typeParameters>(
                                  ref<TypeParamRule>)) +
             lit<">">) +
      assign<&ast::TopLevelFunction::params>(ref<ParamListRule>);

  static constexpr auto TopLevelFunctionBody_ =
      (option(flag<&ast::TopLevelFunction::isAsync>(ref<K_ASYNC>)) +
       lit<"=>"> + assign<&ast::TopLevelFunction::bodyExpr>(ref<Expression>) +
       lit<";">)  //
      | (option((flag<&ast::TopLevelFunction::isAsync>(ref<K_ASYNC>) +
                 option(flag<&ast::TopLevelFunction::isGenerator>(
                     lit<"*">)))  //
                | (flag<&ast::TopLevelFunction::isSync>(ref<K_SYNC>) +
                   flag<&ast::TopLevelFunction::isGenerator>(
                       lit<"*">))) +
         assign<&ast::TopLevelFunction::body>(ref<BlockStmt>));

  static constexpr auto TopLevelFunction_ = create<ast::TopLevelFunction>(
      option(flag<&ast::TopLevelFunction::isExternal>(ref<K_EXTERNAL>)) +
      (TopLevelGetterSig_ | TopLevelSetterSig_ | TopLevelFunctionSig_) +
      (lit<";"> | TopLevelFunctionBody_));

  // The annotation prefix is spliced here, as in `MemberDecl`.
  struct TopLevelDeclRule
      : rule<many(append<&ast::TopLevelDecl::annotations>(ref<MetadataItem>)) +
             (ClassDecl_ | MixinDecl_ | ExtensionDecl_ | EnumDecl_ |
              TypedefDecl_ | TopLevelFunction_ | TopLevelVariable_)> {};

  static constexpr auto LibraryBody =
      option(assign<&ast::CompilationUnit::libraryName>(ref<LibraryName_>)) +
      many(append<&ast::CompilationUnit::imports>(ref<ImportDirective_>) |
           append<&ast::CompilationUnit::exports>(ref<ExportDirective_>)) +
      many(append<&ast::CompilationUnit::parts>(ref<PartDirective_>)) +
      many(append<&ast::CompilationUnit::declarations>(ref<TopLevelDeclRule>));

  struct ScriptTag : terminal<lit<"#!"> + many(cr<R"(^\r\n)">)> {};

  struct CompilationUnitRule
      : rule<create<ast::CompilationUnit>(
            option(lit<"\xEF\xBB\xBF">) +
            option(assign<&ast::CompilationUnit::scriptTag>(ref<ScriptTag>)) +
            (LibraryBody |
            (assign<&ast::CompilationUnit::partOf>(ref<PartOfDirective_>) +
             many(append<&ast::CompilationUnit::declarations>(
                 ref<TopLevelDeclRule>))) |
            assign<&ast::CompilationUnit::exprEntry>(ref<Expression>) |
            assign<&ast::CompilationUnit::stmtEntry>(ref<EntryStatement>)))> {};

  using entry = CompilationUnitRule;
  using ignore = WS;
  using hidden = list<SL_COMMENT, ML_COMMENT>;
};

}  // namespace dart2
