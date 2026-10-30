// The xquery language: its grammar. XQuery 4.0 is XPath 4.0's expression
// grammar plus module structure, FLWOR, constructors and Update Facility 1.0.
#pragma once

#include <cstdint>
#include <charconv>
#include <string>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xquery/core/ast.hpp"
#include "xquery/core/StringLiteral.hpp"

namespace xquery {

template <int Base, std::size_t Prefix = 2>
inline constexpr auto radix_integer = [](std::string_view text) {
  using Result = cursiv::ConversionResult<std::int64_t>;
  if (text.size() < Prefix) return Result::err("Expected an integer literal.");
  std::string digits;
  for (char c : text.substr(Prefix))
    if (c != '_') digits.push_back(c);
  std::int64_t value = 0;
  const auto [end, error] =
      std::from_chars(digits.data(), digits.data() + digits.size(), value, Base);
  if (error != std::errc{} || end != digits.data() + digits.size())
    return Result::err("Integer literal is outside the signed 64-bit range.");
  return Result::ok(value);
};

inline constexpr auto strip_hash = [](std::string_view t) {
  return t.empty() ? std::string_view{} : t.substr(1);
};

struct XQuery : cursiv::GrammarSpec {
  // XQuery's NCName ranges are XML's Name production without the colon,
  // which the QName production spells separately.
  static constexpr auto NameStartChar =
      cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-"
         "󯿿">;
  static constexpr auto NameContChar =
      cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-󯿿0-9.·̀-ͯ‿-"
         "⁀-">;

  // A name's own continuation class, not `w`: `for-each` must lex as one
  // name, so `for` guarded by the default `w` boundary would wrongly claim
  // its prefix.
  static constexpr auto word_boundary = NameContChar;

  static constexpr auto XmlSpace = cr<R"( \t\r\n)">;
  static constexpr auto XmlChar = cr<"\t\r\n\x20-\uD7FF\uE000-\uFFFD\U00010000-\U0010FFFF">;
  static constexpr auto EntityRef = lit<"&lt;"> | lit<"&gt;"> | lit<"&amp;"> | lit<"&quot;"> | lit<"&apos;">;
  static constexpr auto CharRefBody = (lit<"&#"> + some(d) + lit<";">) |
      (lit<"&#x"> + some(cr<"a-fA-F0-9">) + lit<";">);
  static constexpr auto BracedUri = lit<"Q{"> + many(EntityRef | CharRefBody | (!cr<"{}&"> + XmlChar)) + lit<"}">;
  struct WS : terminal<some(XmlSpace)> {};
  struct COMMENT
      : terminal<lit<"(:"> +
                 many(ref<COMMENT> | (!lit<"(:"> + !lit<":)"> + XmlChar)) +
                 lit<":)">> {};

  struct NCName : terminal<NameStartChar + many(NameContChar)> {};
  struct QName : terminal<NameStartChar + many(NameContChar) +
                          option(lit<":"> + NameStartChar + many(NameContChar))> {};
  static constexpr auto CheckedName = [](std::string_view text) -> cursiv::ConversionResult<std::string_view> {
    if (!valid_character_references(text)) return cursiv::ConversionResult<std::string_view>::err("Invalid XML character reference.");
    return cursiv::ConversionResult<std::string_view>::ok(text);
  };
  struct URIQualifiedName : terminal<as<CheckedName>(BracedUri + NameStartChar + many(NameContChar) +
      option(lit<":"> + NameStartChar + many(NameContChar)))> {};
  struct WildcardName : terminal<as<CheckedName>(
      (NameStartChar + many(NameContChar) + lit<":*">) |
      (lit<"*:"> + NameStartChar + many(NameContChar)) | (BracedUri + lit<"*">))> {};

  template <auto Digit>
  static constexpr auto digits = Digit + option(many(Digit | lit<"_">) + Digit);
  struct IntegerLiteral : terminal<as<radix_integer<10, 0>>(digits<d>)> {};
  struct HexIntegerLiteral
      : terminal<as<radix_integer<16>>(lit<"0x"> + digits<cr<"a-fA-F0-9">>)> {};
  struct BinaryIntegerLiteral
      : terminal<as<radix_integer<2>>(lit<"0b"> + digits<cr<"01">>)> {};
  static constexpr auto DecimalBody =
      (lit<"."> + digits<d>) | (digits<d> + lit<"."> + option(digits<d>));
  struct DecimalLiteral : terminal<DecimalBody> {};
  struct DoubleLiteral
      : terminal<((lit<"."> + digits<d>) | (digits<d> + option(lit<"."> + option(digits<d>)))) +
                 cr<"eE"> + option(cr<"+-">) + digits<d>> {};

  static constexpr auto dq_body = lit<"\"\""> | EntityRef | CharRefBody | (!cr<R"("&)"> + XmlChar);
  static constexpr auto sq_body = lit<"''"> | EntityRef | CharRefBody | (!cr<"'&"> + XmlChar);
  struct STRING
      : terminal<as<unquote_str>((lit<"\""> + many(dq_body) + lit<"\"">) |
                                 (lit<"'"> + many(sq_body) + lit<"'">))> {};

  struct StringTemplate
      : terminal<lit<"`"> + many(lit<"{{"> | lit<"}}"> | (!lit<"`"> + XmlChar)) + lit<"`">> {
  };
  struct StringConstructor
      : terminal<lit<"``["> + many(!lit<"]``"> + XmlChar) + lit<"]``">> {};

  struct Pragma
      : terminal<lit<"(#"> + many(XmlSpace) + NameStartChar + many(NameContChar) +
                     option(lit<":"> + NameStartChar + many(NameContChar)) +
                     option(some(XmlSpace) + many(!lit<"#)"> + XmlChar)) +
                     lit<"#)">> {};

  // ---- names -----------------------------------------------------------

  // Keywords can still name nodes/types. Where a constructor/clause is also
  // viable, explicit alternatives let its keyword token complete as a name.
  struct EqName
      : rule<ref<QName> | ref<URIQualifiedName> |
             kw<"after"> | kw<"allowing"> | kw<"ancestor"> | kw<"ancestor-or-self"> | kw<"and"> | kw<"array"> |
      kw<"as"> | kw<"ascending"> | kw<"at"> | kw<"attribute"> | kw<"base-uri"> | kw<"before"> |
      kw<"boundary-space"> | kw<"by"> | kw<"case"> | kw<"cast"> | kw<"castable"> | kw<"catch"> |
      kw<"child"> | kw<"collation"> | kw<"comment"> | kw<"construction"> | kw<"context"> | kw<"copy-namespaces"> |
      kw<"count"> | kw<"decimal-format"> | kw<"decimal-separator"> | kw<"declare"> | kw<"default"> | kw<"descendant"> |
      kw<"descendant-or-self"> | kw<"descending"> | kw<"digit"> | kw<"div"> | kw<"document-node"> | kw<"document"> |
      kw<"element"> | kw<"else"> | kw<"empty-sequence"> | kw<"empty"> | kw<"encoding"> | kw<"end"> |
      kw<"enum"> | kw<"eq"> | kw<"every"> | kw<"except"> | kw<"exponent-separator"> | kw<"external"> |
      kw<"finally"> | kw<"first"> | kw<"fn"> | kw<"following"> | kw<"following-or-self"> | kw<"following-sibling"> |
      kw<"following-sibling-or-self"> | kw<"follows"> | kw<"follows-or-is"> | kw<"for"> | kw<"function"> | kw<"ge"> |
      kw<"gnode"> | kw<"greatest"> | kw<"group"> | kw<"grouping-separator"> | kw<"gt"> | kw<"idiv"> |
      kw<"if"> | kw<"import"> | kw<"in"> | kw<"infinity"> | kw<"inherit"> | kw<"instance"> |
      kw<"intersect"> | kw<"is"> | kw<"is-not"> | kw<"item"> | kw<"jnode"> | kw<"key"> |
      kw<"last"> | kw<"lax"> | kw<"le"> | kw<"least"> | kw<"let"> | kw<"lt"> |
      kw<"map"> | kw<"member"> | kw<"minus-sign"> | kw<"mod"> | kw<"module"> | kw<"NaN"> |
      kw<"namespace"> | kw<"namespace-node"> | kw<"ne"> | kw<"next"> | kw<"no-inherit"> | kw<"no-preserve"> |
      kw<"node"> | kw<"of"> | kw<"only"> | kw<"option"> | kw<"or"> | kw<"order"> |
      kw<"ordered"> | kw<"ordering"> | kw<"otherwise"> | kw<"parent"> | kw<"pattern-separator"> | kw<"percent"> |
      kw<"per-mille"> | kw<"precedes"> | kw<"precedes-or-is"> | kw<"preceding"> | kw<"preceding-or-self"> | kw<"preceding-sibling"> |
      kw<"preceding-sibling-or-self"> | kw<"preserve"> | kw<"previous"> | kw<"processing-instruction"> | kw<"record"> | kw<"return"> |
      kw<"satisfies"> | kw<"schema"> | kw<"schema-attribute"> | kw<"schema-element"> | kw<"self"> | kw<"sliding"> |
      kw<"some"> | kw<"stable"> | kw<"start"> | kw<"strict"> | kw<"strip"> | kw<"switch"> |
      kw<"text"> | kw<"then"> | kw<"to"> | kw<"trace"> | kw<"treat"> | kw<"try"> |
      kw<"tumbling"> | kw<"type"> | kw<"typeswitch"> | kw<"union"> | kw<"unordered"> | kw<"validate"> |
      kw<"value"> | kw<"variable"> | kw<"version"> | kw<"when"> | kw<"where"> | kw<"while"> |
      kw<"window"> | kw<"xquery"> | kw<"zero-digit">> {};

  struct VarName : rule<lit<"$"> + ref<EqName>> {};

  // Forward declarations: the grammar is mutually recursive top to bottom
  // (expressions reach the primary/postfix layer, which reaches every
  // constructor, which reaches expressions again).
  struct SequenceType;
  struct ItemType;
  struct Expr;
  struct ExprSingle;
  struct NumericLiteral;
  struct ParenExpr;
  struct FunctionItemExpr;
  struct MapConstructor;
  struct ArrayConstructor;
  struct PrimaryExpr;
  struct Prolog;
  struct QueryList;
  struct DirElemConstructor;
  struct OrExpr;
  struct VarNameAndType;

  static constexpr auto Occurrence = lit<"?"> | lit<"*"> | lit<"+">;

  static constexpr auto RecordField =
      assign<&ast::FieldDecl::name>(ref<NCName> | ref<STRING>) +
      option(kw<"as"> + assign<&ast::FieldDecl::type>(ref<SequenceType>));
  struct FieldDecl : rule<RecordField> {};
  struct ExtendedFieldDecl
      : rule<option(flag<&ast::FieldDecl::isStatic>(kw<"%static">)) +
             RecordField +
             option(lit<":="> + assign<&ast::FieldDecl::def>(ref<ExprSingle>))> {};

  static constexpr auto NameTestWord = ref<EqName> | ref<WildcardName> | lit<"*">;
  static constexpr auto NodeTypeNameUnion =
      append<&ast::ItemType::names>(NameTestWord) +
      many(lit<"|"> + append<&ast::ItemType::names>(NameTestWord));

  struct ElementNodeType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"element">) + lit<"("> +
            option(NodeTypeNameUnion +
                   option(lit<","> + assign<&ast::ItemType::typeName>(ref<EqName>) +
                          option(flag<&ast::ItemType::nillable>(lit<"?">)))) +
            lit<")">)> {};
  struct AttributeNodeType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"attribute">) + lit<"("> +
            option(NodeTypeNameUnion +
                   option(lit<","> + assign<&ast::ItemType::typeName>(ref<EqName>))) +
            lit<")">)> {};
  struct SchemaElementNodeType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"schema-element">) + lit<"("> +
            append<&ast::ItemType::names>(ref<EqName>) + lit<")">)> {};
  struct DocumentNodeType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"document-node">) + lit<"("> +
            option(append<&ast::ItemType::itemParams>(ref<ElementNodeType> |
                                                     ref<SchemaElementNodeType>) |
                   NodeTypeNameUnion) + lit<")">)> {};
  struct XNodeType
      : rule<ref<DocumentNodeType> | ref<ElementNodeType> |
             ref<AttributeNodeType> | ref<SchemaElementNodeType> |
             create<ast::ItemType>(
                 assign<&ast::ItemType::name>(kw<"schema-attribute">) + lit<"("> +
                 append<&ast::ItemType::names>(ref<EqName>) + lit<")">) |
             create<ast::ItemType>(
                 assign<&ast::ItemType::name>(kw<"processing-instruction">) + lit<"("> +
                 option(append<&ast::ItemType::names>(ref<NCName> | ref<STRING>)) +
                 lit<")">) |
             create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"comment">) +
                                   lit<"("> + lit<")">) |
             create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"text">) +
                                   lit<"("> + lit<")">) |
             create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"namespace-node">) +
                                   lit<"("> + lit<")">) |
             create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"node">) +
                                   lit<"("> + lit<")">)> {};

  struct GNodeType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"gnode">) + lit<"("> +
                                   lit<")">)> {};

  static constexpr auto JRootSelector = lit<"("> + lit<")">;
  // A jnode selector's string/number/name constant is kept as raw spelling
  // (never decoded): jnode/gnode are the grammar's least-used corner.
  struct RawNumber
      : terminal<option(lit<"-">) + some(d) + option(lit<"."> + many(d)) +
                option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  struct JNodeType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"jnode">) + lit<"("> +
            option((flag<&ast::ItemType::wildcard>(lit<"*">) |
                    flag<&ast::ItemType::rootSelector>(JRootSelector) |
                    append<&ast::ItemType::names>(ref<NCName>) |
                    append<&ast::ItemType::names>(ref<STRING> | ref<RawNumber>)) +
                   option(lit<","> +
                          append<&ast::ItemType::params>(ref<SequenceType>))) +
            lit<")">)> {};

  struct TypeTest
      : rule<ref<GNodeType> | ref<XNodeType> | ref<JNodeType>> {};

  struct Selector : rule<NameTestWord> {};

  // A union member: fed both standalone (SimpleNodeTest) and repeated
  // inside a `(...)` union (NodeTestRule) -- one fragment, not two rules, so
  // both write into the SAME NodeTest node rather than nesting one.
  static constexpr auto SimpleNodeTestMember =
      append<&ast::NodeTest::types>(ref<TypeTest>)  //
      | append<&ast::NodeTest::names>(ref<Selector>);

  struct SimpleNodeTest
      : rule<create<ast::NodeTest>(SimpleNodeTestMember)> {};

  struct NodeTestRule
      : rule<create<ast::NodeTest>(lit<"("> + SimpleNodeTestMember +
                                   many(lit<"|"> + SimpleNodeTestMember) + lit<")">)  //
             | ref<SimpleNodeTest>  //
             | create<ast::NodeTest>(
                   lit<"{"> + option(assign<&ast::NodeTest::dynamic>(ref<Expr>)) +
                   lit<"}">)> {};

  static constexpr auto ChoiceItemType =
      lit<"("> + some(append<&ast::ItemType::itemParams>(ref<ItemType>), lit<"|">) + lit<")">;

  struct EnumerationType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"enum">) + lit<"("> +
            some(append<&ast::ItemType::names>(ref<STRING>), lit<",">) + lit<")">)> {};

  struct Literal;
  struct Constant : rule<ref<Literal> |
      create<ast::UnaryExpr>(assign<&ast::UnaryExpr::signs>(lit<"-">) + assign<&ast::UnaryExpr::operand>(ref<NumericLiteral>)) |
      create<ast::BooleanLiteral>((flag<&ast::BooleanLiteral::value>(kw<"true">) | kw<"false">) + lit<"("> + lit<")">)> {};
  struct Annotation : rule<lit<"%"> + assign<&ast::Annotation::name>(ref<EqName>) +
      option(lit<"("> + some(append<&ast::Annotation::args>(ref<Constant>), lit<",">) + lit<")">)> {};

  static constexpr auto Annotations =
      many(append<&ast::ItemType::annotations>(ref<Annotation>));
  static constexpr auto FunctionKw = kw<"function"> | kw<"fn">;

  struct TypedFunctionParam
      : rule<create<ast::Param>(
            option(lit<"$"> + assign<&ast::Param::name>(ref<EqName>) + kw<"as">) +
            assign<&ast::Param::type>(ref<SequenceType>))> {};

  struct FunctionType
      : rule<Annotations +
             (create<ast::ItemType>(assign<&ast::ItemType::name>(FunctionKw) + lit<"("> +
                                    flag<&ast::ItemType::wildcard>(lit<"*">) + lit<")">)  //
              | create<ast::ItemType>(
                    assign<&ast::ItemType::name>(FunctionKw) + lit<"("> +
                    option(some(append<&ast::ItemType::functionParams>(ref<TypedFunctionParam>), lit<",">)) +
                    lit<")"> + kw<"as"> +
                    assign<&ast::ItemType::returnType>(ref<SequenceType>)))> {};

  struct TypedArrayType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
            append<&ast::ItemType::params>(ref<SequenceType>) + lit<")">)> {};
  struct ArrayType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
                                   lit<"*"> + lit<")">)  //
             | ref<TypedArrayType>> {};

  struct TypedMapType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
            append<&ast::ItemType::itemParams>(ref<ItemType>) + lit<","> +
            append<&ast::ItemType::params>(ref<SequenceType>) + lit<")">)> {};
  struct MapType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
                                   lit<"*"> + lit<")">)  //
             | ref<TypedMapType>> {};

  struct TypedRecordType
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"record">) + lit<"("> +
            option(some(append<&ast::ItemType::fields>(ref<FieldDecl>), lit<",">)) +
            lit<")">)> {};
  struct RecordType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"record">) + lit<"("> +
                                   flag<&ast::ItemType::wildcard>(lit<"*">) + lit<")">)  //
             | ref<TypedRecordType>> {};

  struct AnyItemType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"item">) + lit<"("> +
                                   lit<")">)> {};

  struct RegularItemType
      : rule<ref<AnyItemType> | ref<TypeTest> | ref<MapType> |
             ref<ArrayType> | ref<RecordType> | ref<EnumerationType>> {};

  struct TypeName
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(ref<EqName>))> {};

  struct ChoiceItemTypeRule
      : rule<create<ast::ItemType>(ChoiceItemType)> {};

  struct ItemType
      : rule<ref<RegularItemType> | ref<FunctionType> | ref<TypeName> |
             ref<ChoiceItemTypeRule>> {};

  struct SequenceType
      : rule<create<ast::SequenceType>(
                 flag<&ast::SequenceType::emptySequence>(kw<"empty-sequence">) +
                 lit<"("> + lit<")">)  //
             | create<ast::SequenceType>(
                   assign<&ast::SequenceType::itemType>(ref<ItemType>) +
                   option(assign<&ast::SequenceType::occurrence>(Occurrence)))> {
  };

  // `|`-separated: a case clause could not otherwise tell
  // two union members apart.
  static constexpr auto SequenceTypeUnion =
      some(append<&ast::TypeswitchCase::types>(ref<SequenceType>), lit<"|">);

  struct CastTarget
      : rule<ref<TypeName> | ref<ChoiceItemTypeRule> | ref<EnumerationType> |
             ref<TypedArrayType> | ref<TypedMapType> | ref<TypedRecordType>> {
  };

  // ---- numeric / literal expressions -------------------------------------

  struct NumericLiteral
      : rule<create<ast::IntLiteral>(
                 assign<&ast::IntLiteral::value>(ref<IntegerLiteral>))  //
             | create<ast::IntLiteral>(
                   assign<&ast::IntLiteral::value>(ref<HexIntegerLiteral>))  //
             | create<ast::IntLiteral>(
                   assign<&ast::IntLiteral::value>(ref<BinaryIntegerLiteral>))  //
             | create<ast::DecimalLiteral>(
                   assign<&ast::DecimalLiteral::value>(ref<DecimalLiteral>))  //
             | create<ast::DecimalLiteral>(flag<&ast::DecimalLiteral::isDouble>(
                   assign<&ast::DecimalLiteral::value>(ref<DoubleLiteral>)))> {};

  struct QNameLiteral
      : rule<create<ast::QNameLiteral>(lit<"#"> +
                                       assign<&ast::QNameLiteral::name>(ref<EqName>))> {
  };

  struct Literal
      : rule<ref<NumericLiteral>  //
             | create<ast::StringLiteral>(assign<&ast::StringLiteral::value>(ref<STRING>))  //
             | ref<QNameLiteral>> {};

  // ---- operators ----------------------------------------------------------



  static constexpr auto GeneralComp =
      as<ast::BinOp::eq>(lit<"=">) | as<ast::BinOp::ne>(lit<"!=">) | as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::ge>(lit<">=">) | as<ast::BinOp::gt>(lit<">">);
  static constexpr auto ValueComp = as<ast::BinOp::value_eq>(kw<"eq">) | as<ast::BinOp::value_ne>(kw<"ne">) | as<ast::BinOp::value_lt>(kw<"lt">) |
                                    as<ast::BinOp::value_le>(kw<"le">) | as<ast::BinOp::value_gt>(kw<"gt">) | as<ast::BinOp::value_ge>(kw<"ge">);
  static constexpr auto NodeComp = as<ast::BinOp::node_is>(kw<"is">) | as<ast::BinOp::node_is_not>(kw<"is-not">) | as<ast::BinOp::node_precedes>(lit<"<<">) |
                                   as<ast::BinOp::node_precedes>(kw<"precedes">) | as<ast::BinOp::node_follows>(lit<">>">) |
                                   as<ast::BinOp::node_follows>(kw<"follows">) | as<ast::BinOp::node_precedes_or_is>(kw<"precedes-or-is">) |
                                   as<ast::BinOp::node_follows_or_is>(kw<"follows-or-is">);

  // ---- axis --------------------------------------------------------------

  static constexpr auto AxisWord =
      kw<"ancestor-or-self"> | kw<"ancestor"> | kw<"attribute"> | kw<"child"> |
      kw<"descendant-or-self"> | kw<"descendant"> | kw<"following-or-self"> |
      kw<"following-sibling-or-self"> | kw<"following-sibling"> | kw<"following"> |
      kw<"parent"> | kw<"preceding-or-self"> | kw<"preceding-sibling-or-self"> |
      kw<"preceding-sibling"> | kw<"preceding"> | kw<"self">;
  struct AxisWordRule : rule<AxisWord> {};

  static constexpr auto Predicate = lit<"["> + ref<Expr> + lit<"]">;

  struct AxisStep
      : rule<create<ast::AxisStep>(
            (flag<&ast::AxisStep::isParentShortcut>(lit<"..">)  //
             | (flag<&ast::AxisStep::isAttributeShortcut>(lit<"@">) +
                assign<&ast::AxisStep::test>(ref<NodeTestRule>))  //
             | (assign<&ast::AxisStep::axis>(ref<AxisWordRule>) + lit<"::"> +
                assign<&ast::AxisStep::test>(ref<NodeTestRule>))  //
             | assign<&ast::AxisStep::test>(ref<SimpleNodeTest>)) +
            many(lit<"["> + append<&ast::AxisStep::predicates>(ref<Expr>) +
                 lit<"]">))> {};

  // ---- arguments / calls / postfix ---------------------------------------

  struct Argument
      : rule<create<ast::ArgPlaceholder>(lit<"?">) | ref<ExprSingle>> {};

  struct KeywordArgument
      : rule<create<ast::KeywordArg>(
            assign<&ast::KeywordArg::name>(ref<EqName>) + lit<":="> +
            assign<&ast::KeywordArg::value>(ref<Argument>))> {};

  struct PositionalArgumentList
      : rule<create<ast::Arguments>(
            lit<"("> +
            option(some(append<&ast::Arguments::positional>(ref<Argument>), lit<",">)) +
            lit<")">)> {};

  struct ArgumentList
      : rule<create<ast::Arguments>(
            lit<"("> +
            option((some(append<&ast::Arguments::positional>(ref<Argument>), lit<",">) +
                    option(lit<","> +
                           some(append<&ast::Arguments::keyword>(ref<KeywordArgument>), lit<",">)))  //
                   | (some(append<&ast::Arguments::keyword>(ref<KeywordArgument>), lit<",">))) +
            lit<")">)> {};

  // A step position reaches both an unprefixed call (`name(args)`, here) and
  // an abbreviated kind-test step (`element(...)`, AxisStep via
  // SimpleNodeTest): the same span would complete two ways for any of
  // these names. XQuery's own reserved function names keep them disjoint --
  // but only the BARE spelling: `fn:true` is a prefixed QName, not this
  // word, so the guard must fail on `:` too, or `kw<"fn">` alone refuses
  // every call qualified by the ubiquitous `fn:` namespace.
  static constexpr auto NotReservedTail = !(word_boundary | lit<":">);
  static constexpr auto ReservedFunctionName =
      lit<"array", NotReservedTail> | lit<"attribute", NotReservedTail> |
      lit<"comment", NotReservedTail> | lit<"document-node", NotReservedTail> |
      lit<"element", NotReservedTail> | lit<"empty-sequence", NotReservedTail> |
      lit<"fn", NotReservedTail> | lit<"function", NotReservedTail> |
      lit<"gnode", NotReservedTail> | lit<"if", NotReservedTail> |
      lit<"item", NotReservedTail> | lit<"jnode", NotReservedTail> |
      lit<"map", NotReservedTail> | lit<"namespace-node", NotReservedTail> |
      lit<"node", NotReservedTail> |
      lit<"processing-instruction", NotReservedTail> |
      lit<"record", NotReservedTail> |
      lit<"schema-attribute", NotReservedTail> |
      lit<"schema-element", NotReservedTail> | lit<"switch", NotReservedTail> |
      lit<"text", NotReservedTail> | lit<"typeswitch", NotReservedTail>;
  struct NonReservedEqName
      : rule<!ReservedFunctionName + ref<EqName>> {};

  struct FunctionCall
      : rule<create<ast::CallExpr>(
            assign<&ast::CallExpr::name>(ref<NonReservedEqName>) +
            assign<&ast::CallExpr::args>(ref<ArgumentList>))> {};

  struct MarkedNCName : rule<as<strip_hash>(lit<"#"> + ref<QName>)> {};

  struct VarRef
      : rule<create<ast::VarRef>(lit<"$"> + assign<&ast::VarRef::name>(ref<EqName>))> {
  };

  struct KeySpecifier
      : rule<create<ast::Lookup>(assign<&ast::Lookup::key>(ref<QName>))  //
             | create<ast::Lookup>(
                   assign<&ast::Lookup::key>(ref<STRING> | ref<RawNumber>))  //
             | create<ast::Lookup>(assign<&ast::Lookup::key>(lit<".">))  //
             | create<ast::Lookup>(assign<&ast::Lookup::keyExpr>(
                   ref<VarRef> | ref<ParenExpr>))  //
             | create<ast::Lookup>(flag<&ast::Lookup::wildcard>(lit<"*">))> {};

  struct Lookup : rule<lit<"?"> + ref<KeySpecifier>> {};

  struct RestrictedDynamicCall
      : rule<create<ast::PostfixExpr>(
            assign<&ast::PostfixExpr::base>(
                ref<VarRef> | ref<ParenExpr> | ref<FunctionItemExpr> |
                ref<MapConstructor> | ref<ArrayConstructor>) +
            append<&ast::PostfixExpr::suffixes>(ref<PositionalArgumentList>))> {};

  static constexpr auto PostfixSuffixAlt =
      (lit<"["> + append<&ast::PostfixExpr::suffixes>(ref<Expr>) + lit<"]">)  //
      | append<&ast::PostfixExpr::suffixes>(ref<PositionalArgumentList>)  //
      | append<&ast::PostfixExpr::suffixes>(ref<Lookup>)  //
      | append<&ast::PostfixExpr::suffixes>(create<ast::MethodCall>(
            lit<"=?>"> + assign<&ast::MethodCall::name>(ref<QName>) +
            assign<&ast::MethodCall::args>(ref<PositionalArgumentList>)));

  // No suffix: adopt PrimaryExpr's own node as-is, so a bare literal
  // surfaces as itself and not wrapped in an empty PostfixExpr.
  struct PostfixExprRule
      : rule<create<ast::PostfixExpr>(
                 assign<&ast::PostfixExpr::base>(ref<PrimaryExpr>) +
                 some(PostfixSuffixAlt))  //
             | ref<PrimaryExpr>> {};

  // ---- constructors -------------------------------------------------------

  struct QNameLiteralText : rule<as<strip_hash>(lit<"#"> + ref<EqName>)> {};

  struct CompName
      : rule<create<ast::CompName>(
                 assign<&ast::CompName::literal>(ref<QNameLiteralText>))  //
             | create<ast::CompName>(assign<&ast::CompName::literal>(ref<EqName>))  //
             | create<ast::CompName>(lit<"{"> +
                                     assign<&ast::CompName::dynamic>(ref<Expr>) +
                                     lit<"}">)> {};

  struct CompNCName
      : rule<create<ast::CompName>(assign<&ast::CompName::literal>(ref<MarkedNCName>))  //
             | create<ast::CompName>(assign<&ast::CompName::literal>(ref<NCName>))  //
             | create<ast::CompName>(lit<"{"> +
                                     assign<&ast::CompName::dynamic>(ref<Expr>) +
                                     lit<"}">)> {};

  static constexpr auto EnclosedExpr =
      lit<"{"> + option(assign<&ast::ComputedConstructor::content>(ref<Expr>)) + lit<"}">;


  struct ComputedConstructor
      : rule<create<ast::ComputedConstructor>(
                 assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::document>(kw<"document">)) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::element>(kw<"element">)) +
                   assign<&ast::ComputedConstructor::name>(ref<CompName>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::attribute>(kw<"attribute">)) +
                   assign<&ast::ComputedConstructor::name>(ref<CompName>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::namespace_>(kw<"namespace">)) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNCName>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::text>(kw<"text">)) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::comment>(kw<"comment">)) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(as<ast::ConstructorKind::pi>(kw<"processing-instruction">)) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNCName>) +
                   EnclosedExpr)> {};

  // ---- direct constructors --------------------------------------------
  // The element form of a direct constructor:
  // per-state lexing needs no lexer-mode mechanism for it.

  struct TagOpen : terminal<as<strip_hash>(
                       lit<"<"> + NameStartChar + many(NameContChar) +
                       option(lit<":"> + NameStartChar + many(NameContChar)))> {
  };
  struct AttrTextQuot
      : terminal<some(lit<"\"\""> | lit<"{{"> | lit<"}}"> | EntityRef | CharRefBody | (!cr<R"("{}&<)"> + XmlChar))> {};
  struct AttrTextApos
      : terminal<some(lit<"''"> | lit<"{{"> | lit<"}}"> | EntityRef | CharRefBody | (!cr<"'{}&<"> + XmlChar))> {};
  struct ElemText
      : terminal<some(lit<"{{"> | lit<"}}"> | EntityRef | CharRefBody |
                     (!cr<"&<{}"> + XmlChar))> {};
  struct CDataSection : terminal<lit<"<![CDATA["> + many(!lit<"]]>"> + XmlChar) + lit<"]]>">> {};
  struct DirCommentTerm : terminal<lit<"<!--"> + many(!lit<"--"> + XmlChar) + lit<"-->">> {};
  struct DirPITerm : terminal<lit<"<?"> + !ikw<"xml"> + NameStartChar + many(NameContChar) +
      option(some(XmlSpace) + many(!lit<"?>"> + XmlChar)) + lit<"?>">> {};

  // XML text has no trivia. Tag syntax allows XML whitespace; a braced
  // expression restores the query's own trivia only between its braces.
  static constexpr auto AttrStart = assign<&ast::DirAttr::name>(ref<QName>) + lit<"=">;
  static constexpr auto AttrExpr = skip(ignore<WS>, hide<COMMENT>,
      lit<"{"> + option(append<&ast::DirAttr::value>(ref<Expr>)) + lit<"}">);
  struct DirAttr : rule<create<ast::DirAttr>(tight(
      (skip(ignore<WS>, hide<>, AttrStart + lit<"\"">) +
       many(append<&ast::DirAttr::value>(ref<AttrTextQuot>) | AttrExpr) + lit<"\"">) |
      (skip(ignore<WS>, hide<>, AttrStart + lit<"'">) +
       many(append<&ast::DirAttr::value>(ref<AttrTextApos>) | AttrExpr) + lit<"'">)))> {};

  struct TagClose : terminal<as<[](std::string_view text) {
      return text.size() < 2 ? std::string_view{} : text.substr(2);
    }>(lit<"</"> + NameStartChar + many(NameContChar) +
       option(lit<":"> + NameStartChar + many(NameContChar)))> {};
  static constexpr auto DirContentItem =
      append<&ast::DirElemConstructor::content>(ref<ElemText>) |
      append<&ast::DirElemConstructor::content>(ref<CDataSection>) |
      append<&ast::DirElemConstructor::content>(ref<DirCommentTerm>) |
      append<&ast::DirElemConstructor::content>(ref<DirPITerm>) |
      append<&ast::DirElemConstructor::content>(ref<DirElemConstructor>) |
      skip(ignore<WS>, hide<COMMENT>, lit<"{"> +
          option(append<&ast::DirElemConstructor::content>(ref<Expr>)) + lit<"}">);
  struct DirElemConstructor : rule<create<ast::DirElemConstructor>(
      skip(ignore<WS>, hide<>, assign<&ast::DirElemConstructor::name>(ref<TagOpen>) +
          many(gap::has(XmlSpace) + append<&ast::DirElemConstructor::attributes>(ref<DirAttr>)) +
          (flag<&ast::DirElemConstructor::selfClosing>(lit<"/>">) |
           tight(lit<">"> + many(DirContentItem) +
               skip(ignore<WS>, hide<>, assign<&ast::DirElemConstructor::closeName>(ref<TagClose>) + lit<">">)))))> {};

  struct DirCommentConstructor
      : rule<create<ast::DirComment>(assign<&ast::DirComment::text>(ref<DirCommentTerm>))> {
  };
  struct DirPIConstructor
      : rule<create<ast::DirPI>(assign<&ast::DirPI::text>(ref<DirPITerm>))> {};

  struct NodeConstructor
      : rule<ref<DirElemConstructor> | ref<DirCommentConstructor> |
             ref<DirPIConstructor> | ref<ComputedConstructor>> {};

  // ---- map / array / mode / function-item constructors --------------------

  struct MapEntry
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ref<ExprSingle>) +
            option(lit<":"> + assign<&ast::MapEntry::value>(ref<ExprSingle>)))> {};

  struct MapConstructor
      : rule<create<ast::MapConstructor>(
            option(kw<"map">) + lit<"{"> +
            option(some(append<&ast::MapConstructor::entries>(ref<MapEntry>), lit<",">)) +
            lit<"}">)> {};

  struct ArrayConstructor
      : rule<create<ast::ArrayConstructor>(
                 lit<"["> +
                 option(append<&ast::ArrayConstructor::items>(ref<ExprSingle>) +
                        many(lit<","> + append<&ast::ArrayConstructor::items>(
                                            ref<ExprSingle>))) +
                 lit<"]">)  //
             | create<ast::ArrayConstructor>(
                   flag<&ast::ArrayConstructor::curly>(kw<"array">) + lit<"{"> +
                   option(assign<&ast::ArrayConstructor::body>(ref<Expr>)) +
                   lit<"}">)> {};

  struct OrderedExpr
      : rule<create<ast::ModeExpr>(flag<&ast::ModeExpr::ordered>(kw<"ordered">) +
                                   lit<"{"> +
                                   option(assign<&ast::ModeExpr::body>(ref<Expr>)) +
                                   lit<"}">)> {};
  struct UnorderedExpr
      : rule<create<ast::ModeExpr>(kw<"unordered"> + lit<"{"> +
                                   option(assign<&ast::ModeExpr::body>(ref<Expr>)) +
                                   lit<"}">)> {};

  struct ContextItemExpr : rule<create<ast::ContextItemExpr>(lit<".">)> {};

  struct ParenExpr
      : rule<create<ast::ParenExpr>(lit<"("> +
                                    option(assign<&ast::ParenExpr::body>(ref<Expr>)) +
                                    lit<")">)> {};

  struct NamedFunctionRef
      : rule<create<ast::NamedFunctionRef>(
            assign<&ast::NamedFunctionRef::name>(ref<EqName>) + lit<"#"> +
            assign<&ast::NamedFunctionRef::arity>(ref<IntegerLiteral>))> {};

  // A fragment, not a rule<>: it writes into the enclosing InlineFunctionExpr
  // directly, and a rule<> of its own would give that node a second head.
  static constexpr auto FunctionSignature =
      lit<"("> +
      option(some(append<&ast::InlineFunctionExpr::params>(ref<VarNameAndType>), lit<",">)) +
      lit<")"> +
      option(kw<"as"> +
             assign<&ast::InlineFunctionExpr::returnType>(ref<SequenceType>));

  struct InlineFunctionExprRule
      : rule<create<ast::InlineFunctionExpr>(
            many(append<&ast::InlineFunctionExpr::annotations>(ref<Annotation>)) +
            FunctionKw + option(FunctionSignature) + lit<"{"> +
            option(assign<&ast::InlineFunctionExpr::body>(ref<Expr>)) + lit<"}">)> {
  };

  struct FunctionItemExpr
      : rule<ref<NamedFunctionRef> | ref<InlineFunctionExprRule>> {};

  struct StringTemplateExpr
      : rule<create<ast::StringTemplateExpr>(
            assign<&ast::StringTemplateExpr::raw>(ref<StringTemplate>))> {};
  struct StringConstructorExpr
      : rule<create<ast::StringConstructorExpr>(
            assign<&ast::StringConstructorExpr::raw>(ref<StringConstructor>))> {};

  struct UnaryLookup : rule<ref<Lookup>> {};

  struct PrimaryExpr
      : rule<ref<ArrayConstructor>  //
             | ref<Literal>  //
             | ref<ContextItemExpr>  //
             | ref<FunctionCall>  //
             | ref<FunctionItemExpr>  //
             | ref<MapConstructor>  //
             | ref<NodeConstructor>  //
             | ref<OrderedExpr>  //
             | ref<ParenExpr>  //
             | ref<StringTemplateExpr>  //
             | ref<StringConstructorExpr>  //
             | ref<UnaryLookup>  //
             | ref<UnorderedExpr>  //
             | ref<VarRef>> {};

  // ---- precedence ladder ---------------------------------------------------

  struct ValidateExpr
      : rule<create<ast::ValidateExpr>(
            kw<"validate"> +
            option(assign<&ast::ValidateExpr::mode>(kw<"lax"> | kw<"strict">) |
                   (kw<"type"> + assign<&ast::ValidateExpr::typeName>(ref<EqName>))) +
            lit<"{"> + assign<&ast::ValidateExpr::body>(ref<Expr>) + lit<"}">)> {};

  struct ExtensionExpr
      : rule<create<ast::ExtensionExpr>(
            some(append<&ast::ExtensionExpr::pragmas>(ref<Pragma>)) + lit<"{"> +
            option(assign<&ast::ExtensionExpr::body>(ref<Expr>)) + lit<"}">)> {};

  struct StepExpr : rule<ref<PostfixExprRule> | ref<AxisStep>> {};

  struct RelativePathExpr
      : infix<StepExpr, ast::BinaryExpr,
              left(as<ast::BinOp::path_descendant>(lit<"//">) | as<ast::BinOp::path_child>(lit<"/">))> {};

  struct AbsolutePathExpr
      : rule<create<ast::AbsolutePathExpr>(
                 flag<&ast::AbsolutePathExpr::doubleSlash>(lit<"//">) +
                 assign<&ast::AbsolutePathExpr::rest>(ref<RelativePathExpr>))  //
             | create<ast::AbsolutePathExpr>(
                   lit<"/"> + option(assign<&ast::AbsolutePathExpr::rest>(
                                  ref<RelativePathExpr>)))> {};

  struct PathExpr
      : rule<ref<AbsolutePathExpr> | ref<RelativePathExpr>> {};

  struct SimpleMapExpr : infix<PathExpr, ast::BinaryExpr,
                               left(as<ast::BinOp::simple_map>(lit<"!">))> {};

  struct ValueExpr
      : rule<ref<ValidateExpr> | ref<ExtensionExpr> | ref<SimpleMapExpr>> {};

  struct SignRun : rule<some(lit<"+"> | lit<"-">)> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::signs>(ref<SignRun>) +
                                    assign<&ast::UnaryExpr::operand>(ref<ValueExpr>))  //
             | ref<ValueExpr>> {};

  struct ArrowTarget : rule<ref<FunctionCall> | ref<RestrictedDynamicCall>> {};

  struct ArrowStep
      : rule<create<ast::ArrowStep>(lit<"=>"> +
                                    assign<&ast::ArrowStep::target>(ref<ArrowTarget>))  //
             | create<ast::ArrowStep>(
                   flag<&ast::ArrowStep::mapping>(lit<"=!>">) +
                   assign<&ast::ArrowStep::target>(ref<ArrowTarget>))> {};

  // No arrow step: adopt UnaryExpr's own node, not an empty ArrowExpr.
  struct ArrowExprRule
      : rule<create<ast::ArrowExpr>(
                 assign<&ast::ArrowExpr::base>(ref<UnaryExpr>) +
                 some(append<&ast::ArrowExpr::steps>(ref<ArrowStep>)))  //
             | ref<UnaryExpr>> {};

  struct PipelineExpr : infix<ArrowExprRule, ast::BinaryExpr,
                              left(as<ast::BinOp::pipeline>(lit<"->">))> {};

  struct CastExpr
      : rule<create<ast::CastExpr>(
                 assign<&ast::CastExpr::operand>(ref<PipelineExpr>) + kw<"cast"> +
                 kw<"as"> + assign<&ast::CastExpr::target>(ref<CastTarget>) +
                 option(assign<&ast::CastExpr::occurrence>(Occurrence)))  //
             | ref<PipelineExpr>> {};

  struct CastableExpr
      : rule<create<ast::CastableExpr>(
                 assign<&ast::CastableExpr::operand>(ref<CastExpr>) + kw<"castable"> +
                 kw<"as"> + assign<&ast::CastableExpr::target>(ref<CastTarget>) +
                 option(assign<&ast::CastableExpr::occurrence>(Occurrence)))  //
             | ref<CastExpr>> {};

  struct TreatExpr
      : rule<create<ast::TreatExpr>(
                 assign<&ast::TreatExpr::operand>(ref<CastableExpr>) + kw<"treat"> +
                 kw<"as"> + assign<&ast::TreatExpr::type>(ref<SequenceType>))  //
             | ref<CastableExpr>> {};

  struct InstanceOfExpr
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::operand>(ref<TreatExpr>) +
                 kw<"instance"> + kw<"of"> +
                 assign<&ast::InstanceOfExpr::type>(ref<SequenceType>))  //
             | ref<TreatExpr>> {};

  struct RecordPutExpr : infix<InstanceOfExpr, ast::BinaryExpr,
                               left(as<ast::BinOp::record_put>(lit<"+:=">))> {};

  struct IntersectExceptExpr
      : infix<RecordPutExpr, ast::BinaryExpr,
              left(as<ast::BinOp::intersect>(kw<"intersect">) |
                   as<ast::BinOp::except>(kw<"except">))> {};

  struct UnionExpr : infix<IntersectExceptExpr, ast::BinaryExpr,
                           left(as<ast::BinOp::union_>(kw<"union">) |
                                as<ast::BinOp::union_>(lit<"|">))> {};

  struct MultiplicativeExpr
      : infix<UnionExpr, ast::BinaryExpr,
              left(as<ast::BinOp::mul>(lit<"*">) | as<ast::BinOp::mul>(lit<"×">) | as<ast::BinOp::div_>(kw<"div">) | as<ast::BinOp::div_>(lit<"÷">) | as<ast::BinOp::idiv>(kw<"idiv">) |
                   as<ast::BinOp::mod>(kw<"mod">))> {};

  struct AdditiveExpr : infix<MultiplicativeExpr, ast::BinaryExpr,
                              left(as<ast::BinOp::add>(lit<"+">) |
                                   as<ast::BinOp::sub>(lit<"-">))> {};

  struct RangeExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AdditiveExpr>) +
                 assign<&ast::BinaryExpr::op>(as<ast::BinOp::range>(kw<"to">)) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveExpr>))  //
             | ref<AdditiveExpr>> {};

  struct StringConcatExpr : infix<RangeExpr, ast::BinaryExpr,
                                  left(as<ast::BinOp::concat>(lit<"||">))> {};

  struct OtherwiseExpr
      : infix<StringConcatExpr, ast::BinaryExpr,
              left(as<ast::BinOp::otherwise_>(kw<"otherwise">))> {};

  struct ComparisonExpr
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OtherwiseExpr>) +
                 assign<&ast::BinaryExpr::op>(ValueComp | GeneralComp | NodeComp) +
                 assign<&ast::BinaryExpr::right>(ref<OtherwiseExpr>))  //
             | ref<OtherwiseExpr>> {};

  struct AndExpr : infix<ComparisonExpr, ast::BinaryExpr,
                         left(as<ast::BinOp::and_>(kw<"and">))> {};

  struct OrExpr
      : infix<AndExpr, ast::BinaryExpr, left(as<ast::BinOp::or_>(kw<"or">))> {};

  // ---- FLWOR ---------------------------------------------------------------

  struct VarNameAndType
      : rule<create<ast::Param>(lit<"$"> + assign<&ast::Param::name>(ref<EqName>) +
                                option(kw<"as"> +
                                       assign<&ast::Param::type>(ref<SequenceType>)))> {
  };

  struct ForItemBinding
      : rule<create<ast::ForBinding>(
            assign<&ast::ForBinding::name>(ref<VarName>) +
            option(kw<"as"> + assign<&ast::ForBinding::type>(ref<SequenceType>)) +
            option(flag<&ast::ForBinding::allowingEmpty>(kw<"allowing">) +
                   kw<"empty">) +
            option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarName>)) +
            kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingle>))> {};

  struct ForMemberBinding
      : rule<create<ast::ForBinding>(
            flag<&ast::ForBinding::isMember>(kw<"member">) +
            assign<&ast::ForBinding::name>(ref<VarName>) +
            option(kw<"as"> + assign<&ast::ForBinding::type>(ref<SequenceType>)) +
            option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarName>)) +
            kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingle>))> {};

  struct ForEntryBinding
      : rule<create<ast::ForBinding>(
                 flag<&ast::ForBinding::isEntry>(kw<"key">) +
                 assign<&ast::ForBinding::keyName>(ref<VarName>) +
                 option(kw<"as"> +
                        assign<&ast::ForBinding::keyType>(ref<SequenceType>)) +
                 option(kw<"value"> + assign<&ast::ForBinding::name>(ref<VarName>) +
                        option(kw<"as"> + assign<&ast::ForBinding::type>(
                                              ref<SequenceType>))) +
                 option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarName>)) +
                 kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingle>))  //
             | create<ast::ForBinding>(
                   flag<&ast::ForBinding::isEntry>(kw<"value">) +
                   assign<&ast::ForBinding::name>(ref<VarName>) +
                   option(kw<"as"> +
                          assign<&ast::ForBinding::type>(ref<SequenceType>)) +
                   option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarName>)) +
                   kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingle>))> {
  };

  struct ForBinding
      : rule<ref<ForItemBinding> | ref<ForMemberBinding> |
             ref<ForEntryBinding>> {};

  struct ForClause
      : rule<create<ast::ForClause>(
            kw<"for"> + some(append<&ast::ForClause::bindings>(ref<ForBinding>), lit<",">))> {
  };

  struct LetValueBinding
      : rule<create<ast::LetBinding>(
            assign<&ast::LetBinding::name>(ref<VarName>) +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceType>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingle>))> {};

  struct LetSequenceBinding
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"(">) +
            some(append<&ast::LetBinding::names>(ref<VarNameAndType>), lit<",">) +
            lit<")"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceType>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingle>))> {};

  struct LetArrayBinding
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"[">) +
            some(append<&ast::LetBinding::names>(ref<VarNameAndType>), lit<",">) +
            lit<"]"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceType>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingle>))> {};

  struct LetMapBinding
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"{">) +
            some(append<&ast::LetBinding::names>(ref<VarNameAndType>), lit<",">) +
            lit<"}"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceType>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingle>))> {};

  struct LetBinding
      : rule<ref<LetValueBinding> | ref<LetSequenceBinding> |
             ref<LetArrayBinding> | ref<LetMapBinding>> {};

  struct LetClause
      : rule<create<ast::LetClause>(
            kw<"let"> + some(append<&ast::LetClause::bindings>(ref<LetBinding>), lit<",">))> {
  };

  struct WindowVarSpec
      : rule<create<ast::WindowVarSpec>(
            option(assign<&ast::WindowVarSpec::current>(ref<VarName>)) +
            option(kw<"at"> + assign<&ast::WindowVarSpec::posVar>(ref<VarName>)) +
            option(kw<"previous"> +
                   assign<&ast::WindowVarSpec::previous>(ref<VarName>)) +
            option(kw<"next"> + assign<&ast::WindowVarSpec::next>(ref<VarName>)))> {
  };

  struct WindowStartCondition
      : rule<create<ast::WindowCondition>(
            kw<"start"> + assign<&ast::WindowCondition::vars>(ref<WindowVarSpec>) +
            option(kw<"when"> +
                   assign<&ast::WindowCondition::when>(ref<ExprSingle>)))> {};

  struct WindowEndCondition
      : rule<create<ast::WindowCondition>(
            option(flag<&ast::WindowCondition::only>(kw<"only">)) + kw<"end"> +
            assign<&ast::WindowCondition::vars>(ref<WindowVarSpec>) +
            option(kw<"when"> +
                   assign<&ast::WindowCondition::when>(ref<ExprSingle>)))> {};

  struct TumblingWindowClause
      : rule<create<ast::WindowClause>(
            kw<"tumbling"> + kw<"window"> +
            assign<&ast::WindowClause::name>(ref<VarName>) +
            option(kw<"as"> + assign<&ast::WindowClause::type>(ref<SequenceType>)) +
            kw<"in"> + assign<&ast::WindowClause::source>(ref<ExprSingle>) +
            option(assign<&ast::WindowClause::start>(ref<WindowStartCondition>)) +
            option(assign<&ast::WindowClause::end>(ref<WindowEndCondition>)))> {};

  struct SlidingWindowClause
      : rule<create<ast::WindowClause>(
            flag<&ast::WindowClause::sliding>(kw<"sliding">) + kw<"window"> +
            assign<&ast::WindowClause::name>(ref<VarName>) +
            option(kw<"as"> + assign<&ast::WindowClause::type>(ref<SequenceType>)) +
            kw<"in"> + assign<&ast::WindowClause::source>(ref<ExprSingle>) +
            option(assign<&ast::WindowClause::start>(ref<WindowStartCondition>)) +
            assign<&ast::WindowClause::end>(ref<WindowEndCondition>))> {};

  struct WindowClause
      : rule<kw<"for"> +
             (ref<TumblingWindowClause> | ref<SlidingWindowClause>)> {};

  struct WhereClause
      : rule<create<ast::WhereClause>(kw<"where"> +
                                      assign<&ast::WhereClause::cond>(ref<ExprSingle>))> {
  };
  struct WhileClause
      : rule<create<ast::WhileClause>(
            kw<"while"> + assign<&ast::WhileClause::cond>(ref<ExprSingle>))> {};
  struct TraceClause
      : rule<create<ast::TraceClause>(
            kw<"trace"> + assign<&ast::TraceClause::value>(ref<ExprSingle>))> {};
  struct CountClause
      : rule<create<ast::CountClause>(
            kw<"count"> + assign<&ast::CountClause::name>(ref<VarName>))> {};

  struct GroupingSpec
      : rule<create<ast::GroupingSpec>(
            assign<&ast::GroupingSpec::name>(ref<VarName>) +
            option(option(kw<"as"> +
                          assign<&ast::GroupingSpec::type>(ref<SequenceType>)) +
                   lit<":="> + assign<&ast::GroupingSpec::value>(ref<ExprSingle>)) +
            option(kw<"collation"> +
                   assign<&ast::GroupingSpec::collation>(ref<STRING>)))> {};

  struct GroupByClause
      : rule<create<ast::GroupByClause>(
            kw<"group"> + kw<"by"> +
            some(append<&ast::GroupByClause::specs>(ref<GroupingSpec>), lit<",">))> {
  };

  struct OrderSpec
      : rule<create<ast::OrderSpec>(
            assign<&ast::OrderSpec::value>(ref<ExprSingle>) +
            option(assign<&ast::OrderSpec::direction>(kw<"ascending"> | kw<"descending">)) +
            option(kw<"empty"> +
                   (flag<&ast::OrderSpec::emptyGreatest>(kw<"greatest">)  //
                    | flag<&ast::OrderSpec::emptyLeast>(kw<"least">))) +
            option(kw<"collation"> + assign<&ast::OrderSpec::collation>(ref<STRING>)))> {
  };

  struct OrderByClause
      : rule<create<ast::OrderByClause>(
            option(flag<&ast::OrderByClause::stable>(kw<"stable">)) + kw<"order"> +
            kw<"by"> + some(append<&ast::OrderByClause::specs>(ref<OrderSpec>), lit<",">))> {};

  static constexpr auto InitialClause =
      append<&ast::FlworExpr::clauses>(ref<ForClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<LetClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<WindowClause>);
  static constexpr auto IntermediateClause =
      InitialClause  //
      | append<&ast::FlworExpr::clauses>(ref<WhereClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<GroupByClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<OrderByClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<CountClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<WhileClause>)  //
      | append<&ast::FlworExpr::clauses>(ref<TraceClause>);

  struct FlworExpr
      : rule<create<ast::FlworExpr>(
            InitialClause + many(IntermediateClause) + kw<"return"> +
            assign<&ast::FlworExpr::result>(ref<ExprSingle>))> {};

  // ---- quantified / switch / typeswitch / if / try-catch -------------------

  struct QuantifierBinding
      : rule<create<ast::QuantifierBinding>(
            assign<&ast::QuantifierBinding::name>(ref<VarName>) +
            option(kw<"as"> +
                   assign<&ast::QuantifierBinding::type>(ref<SequenceType>)) +
            kw<"in"> + assign<&ast::QuantifierBinding::source>(ref<ExprSingle>))> {
  };

  struct QuantifiedExpr
      : rule<create<ast::QuantifiedExpr>(
            (kw<"some"> | flag<&ast::QuantifiedExpr::every>(kw<"every">)) +
            some(append<&ast::QuantifiedExpr::bindings>(ref<QuantifierBinding>), lit<",">) +
            kw<"satisfies"> +
            assign<&ast::QuantifiedExpr::predicate>(ref<ExprSingle>))> {};

  struct SwitchCaseClause
      : rule<create<ast::SwitchCaseClause>(
            some(kw<"case"> +
                 append<&ast::SwitchCaseClause::operands>(ref<Expr>)) +
            kw<"return"> +
            assign<&ast::SwitchCaseClause::result>(ref<ExprSingle>))> {};

  static constexpr auto SwitchCasesBody =
      some(append<&ast::SwitchExpr::cases>(ref<SwitchCaseClause>)) + kw<"default"> +
      kw<"return"> + assign<&ast::SwitchExpr::defaultResult>(ref<ExprSingle>);

  struct SwitchExpr
      : rule<create<ast::SwitchExpr>(
            kw<"switch"> + lit<"("> +
            option(assign<&ast::SwitchExpr::comparand>(ref<Expr>)) + lit<")"> +
            (SwitchCasesBody | (lit<"{"> + SwitchCasesBody + lit<"}">)))> {};

  struct CaseClause
      : rule<create<ast::TypeswitchCase>(
            kw<"case"> +
            option(assign<&ast::TypeswitchCase::varName>(ref<VarName>) + kw<"as">) +
            SequenceTypeUnion + kw<"return"> +
            assign<&ast::TypeswitchCase::result>(ref<ExprSingle>))> {};

  static constexpr auto TypeswitchCasesBody =
      some(append<&ast::TypeswitchExpr::cases>(ref<CaseClause>)) + kw<"default"> +
      option(assign<&ast::TypeswitchExpr::defaultVarName>(ref<VarName>)) +
      kw<"return"> + assign<&ast::TypeswitchExpr::defaultResult>(ref<ExprSingle>);

  struct TypeswitchExpr
      : rule<create<ast::TypeswitchExpr>(
            kw<"typeswitch"> + lit<"("> +
            assign<&ast::TypeswitchExpr::operand>(ref<Expr>) + lit<")"> +
            (TypeswitchCasesBody | (lit<"{"> + TypeswitchCasesBody + lit<"}">)))> {};

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expr>) + lit<")"> +
            ((kw<"then"> + assign<&ast::IfExpr::thenBranch>(ref<ExprSingle>) +
              kw<"else"> + assign<&ast::IfExpr::elseBranch>(ref<ExprSingle>))  //
             | (lit<"{"> + option(assign<&ast::IfExpr::thenBranch>(ref<Expr>)) +
                lit<"}"> + option(kw<"else"> +
                  (assign<&ast::IfExpr::elseBranch>(ref<IfExpr>) |
                   (lit<"{"> + option(assign<&ast::IfExpr::elseBranch>(ref<Expr>)) + lit<"}">))))))> {};

  struct CatchClause
      : rule<create<ast::CatchClause>(
            kw<"catch"> + append<&ast::CatchClause::tests>(NameTestWord) +
            many(lit<"|"> + append<&ast::CatchClause::tests>(NameTestWord)) + lit<"{"> +
            option(assign<&ast::CatchClause::body>(ref<Expr>)) + lit<"}">)> {};

  struct TryCatchExpr
      : rule<create<ast::TryCatchExpr>(
            kw<"try"> + lit<"{"> +
            option(assign<&ast::TryCatchExpr::tryBody>(ref<Expr>)) + lit<"}"> +
            ((some(append<&ast::TryCatchExpr::catches>(ref<CatchClause>)) +
              option(kw<"finally"> + lit<"{"> +
                     option(assign<&ast::TryCatchExpr::finallyBody>(ref<Expr>)) +
                     lit<"}">))  //
             | (kw<"finally"> + lit<"{"> +
                option(assign<&ast::TryCatchExpr::finallyBody>(ref<Expr>)) +
                lit<"}">)))> {};

  // ---- Update Facility 1.0 --------------------------------------------------

  struct InsertExpr
      : rule<create<ast::InsertExpr>(
            kw<"insert"> + (kw<"node"> | flag<&ast::InsertExpr::nodes>(kw<"nodes">)) +
            assign<&ast::InsertExpr::source>(ref<ExprSingle>) +
            ((option(kw<"as"> +
                     assign<&ast::InsertExpr::position>(kw<"first"> | kw<"last">)) +
              assign<&ast::InsertExpr::target>(kw<"into">))  //
             | assign<&ast::InsertExpr::target>(kw<"after">)  //
             | assign<&ast::InsertExpr::target>(kw<"before">)) +
            assign<&ast::InsertExpr::targetExpr>(ref<ExprSingle>))> {};

  struct DeleteExpr
      : rule<create<ast::DeleteExpr>(
            kw<"delete"> + (kw<"node"> | flag<&ast::DeleteExpr::nodes>(kw<"nodes">)) +
            assign<&ast::DeleteExpr::target>(ref<ExprSingle>))> {};

  struct ReplaceExpr
      : rule<create<ast::ReplaceExpr>(
            kw<"replace"> +
            option(flag<&ast::ReplaceExpr::valueOf>(kw<"value">) + kw<"of">) +
            kw<"node"> + assign<&ast::ReplaceExpr::target>(ref<ExprSingle>) +
            kw<"with"> + assign<&ast::ReplaceExpr::with>(ref<ExprSingle>))> {};

  struct RenameExpr
      : rule<create<ast::RenameExpr>(
            kw<"rename"> + kw<"node"> +
            assign<&ast::RenameExpr::target>(ref<ExprSingle>) + kw<"as"> +
            assign<&ast::RenameExpr::newName>(ref<ExprSingle>))> {};

  struct CopyBinding
      : rule<create<ast::CopyBinding>(
            lit<"$"> + assign<&ast::CopyBinding::name>(ref<EqName>) + lit<":="> +
            assign<&ast::CopyBinding::value>(ref<ExprSingle>))> {};

  struct TransformExpr
      : rule<create<ast::TransformExpr>(
            kw<"copy"> + some(append<&ast::TransformExpr::bindings>(ref<CopyBinding>), lit<",">) +
            kw<"modify"> + assign<&ast::TransformExpr::modify>(ref<ExprSingle>) +
            kw<"return"> + assign<&ast::TransformExpr::result>(ref<ExprSingle>))> {};

  // ---- exprSingle / expr ----------------------------------------------------

  struct ExprSingle
      : rule<ref<FlworExpr> | ref<QuantifiedExpr> | ref<SwitchExpr> |
             ref<TypeswitchExpr> | ref<IfExpr> | ref<TryCatchExpr> |
             ref<InsertExpr> | ref<DeleteExpr> | ref<RenameExpr> |
             ref<ReplaceExpr> | ref<TransformExpr> | ref<OrExpr>> {};

  struct Expr
      : rule<create<ast::SequenceExpr>(
            some(append<&ast::SequenceExpr::items>(ref<ExprSingle>), lit<",">))> {
  };

  // ---- prolog / declarations -------------------------------------------

  struct Setter
      : rule<create<ast::Setter>(kw<"declare"> + assign<&ast::Setter::kind>(kw<"boundary-space">) +
                                 assign<&ast::Setter::value1>(kw<"preserve"> | kw<"strip">))  //
             | create<ast::Setter>(kw<"declare"> + kw<"default"> +
                                   assign<&ast::Setter::kind>(kw<"collation">) +
                                   assign<&ast::Setter::value1>(ref<STRING>))  //
             | create<ast::Setter>(kw<"declare"> + assign<&ast::Setter::kind>(kw<"base-uri">) +
                                   assign<&ast::Setter::value1>(ref<STRING>))  //
             | create<ast::Setter>(kw<"declare"> +
                                   assign<&ast::Setter::kind>(kw<"construction">) +
                                   assign<&ast::Setter::value1>(kw<"strip"> | kw<"preserve">))  //
             | create<ast::Setter>(kw<"declare"> + assign<&ast::Setter::kind>(kw<"ordering">) +
                                   assign<&ast::Setter::value1>(kw<"ordered"> |
                                                                kw<"unordered">))  //
             | create<ast::Setter>(kw<"declare"> + kw<"default"> + kw<"order"> +
                                   assign<&ast::Setter::kind>(kw<"empty">) +
                                   assign<&ast::Setter::value1>(kw<"greatest"> |
                                                                kw<"least">))  //
             | create<ast::Setter>(
                   kw<"declare"> + assign<&ast::Setter::kind>(kw<"copy-namespaces">) +
                   assign<&ast::Setter::value1>(kw<"preserve"> | kw<"no-preserve">) +
                   lit<","> +
                   assign<&ast::Setter::value2>(kw<"inherit"> | kw<"no-inherit">))> {};

  struct DfPropertyName
      : rule<kw<"decimal-separator"> | kw<"grouping-separator"> | kw<"infinity"> |
             kw<"minus-sign"> | kw<"NaN"> | kw<"percent"> | kw<"per-mille"> |
             kw<"zero-digit"> | kw<"digit"> | kw<"pattern-separator"> |
             kw<"exponent-separator">> {};

  struct DecimalFormatProperty
      : rule<create<ast::DecimalFormatProperty>(
            assign<&ast::DecimalFormatProperty::name>(ref<DfPropertyName>) + lit<"="> +
            assign<&ast::DecimalFormatProperty::value>(ref<STRING>))> {};

  struct DecimalFormatDecl
      : rule<create<ast::DecimalFormatDecl>(
                 kw<"declare"> + kw<"decimal-format"> +
                 assign<&ast::DecimalFormatDecl::name>(ref<EqName>) +
                 many(append<&ast::DecimalFormatDecl::properties>(
                     ref<DecimalFormatProperty>)))  //
             | create<ast::DecimalFormatDecl>(
                   kw<"declare"> +
                   flag<&ast::DecimalFormatDecl::isDefault>(kw<"default">) +
                   kw<"decimal-format"> +
                   many(append<&ast::DecimalFormatDecl::properties>(
                       ref<DecimalFormatProperty>)))> {};

  struct NamespaceDecl
      : rule<create<ast::NamespaceDecl>(
            kw<"declare"> + kw<"namespace"> +
            assign<&ast::NamespaceDecl::prefix>(ref<NCName>) + lit<"="> +
            assign<&ast::NamespaceDecl::uri>(ref<STRING>))> {};

  struct DefaultNamespaceDecl
      : rule<create<ast::DefaultNamespaceDecl>(
            kw<"declare"> +
            option(flag<&ast::DefaultNamespaceDecl::fixed>(kw<"fixed">)) +
            kw<"default"> +
            (kw<"element"> |
             flag<&ast::DefaultNamespaceDecl::isFunction>(kw<"function">)) +
            kw<"namespace"> + assign<&ast::DefaultNamespaceDecl::uri>(ref<STRING>))> {};

  struct SchemaImport
      : rule<create<ast::SchemaImport>(
            kw<"import"> + kw<"schema"> +
            option((kw<"namespace"> + assign<&ast::SchemaImport::prefix>(ref<NCName>) +
                    lit<"=">)  //
                   | (option(flag<&ast::SchemaImport::fixed>(kw<"fixed">)) +
                      flag<&ast::SchemaImport::isDefaultElement>(kw<"default">) +
                      kw<"element"> + kw<"namespace">)) +
            assign<&ast::SchemaImport::uri>(ref<STRING>) +
            option(kw<"at"> + some(append<&ast::SchemaImport::locations>(ref<STRING>), lit<",">)))> {};

  struct ModuleImport
      : rule<create<ast::ModuleImport>(
            kw<"import"> + kw<"module"> +
            option(kw<"namespace"> + assign<&ast::ModuleImport::prefix>(ref<NCName>) +
                   lit<"=">) +
            assign<&ast::ModuleImport::uri>(ref<STRING>) +
            option(kw<"at"> + some(append<&ast::ModuleImport::locations>(ref<STRING>), lit<",">)))> {};

  struct ContextValueDecl
      : rule<create<ast::ContextValueDecl>(
            kw<"declare"> + kw<"context"> +
            ((kw<"value"> +
              option(kw<"as"> +
                     assign<&ast::ContextValueDecl::type>(ref<SequenceType>)))  //
             | (flag<&ast::ContextValueDecl::isItem>(kw<"item">) +
                option(kw<"as"> +
                       assign<&ast::ContextValueDecl::type>(create<ast::SequenceType>(
                           assign<&ast::SequenceType::itemType>(ref<ItemType>)))))) +
            ((lit<":="> + assign<&ast::ContextValueDecl::value>(ref<ExprSingle>))  //
             | (flag<&ast::ContextValueDecl::external>(kw<"external">) +
                option(lit<":="> +
                       assign<&ast::ContextValueDecl::value>(ref<ExprSingle>)))))> {
  };

  struct VarDecl
      : rule<create<ast::VarDecl>(
            kw<"declare"> + many(append<&ast::VarDecl::annotations>(ref<Annotation>)) +
            kw<"variable"> + lit<"$"> + assign<&ast::VarDecl::name>(ref<EqName>) +
            option(kw<"as"> + assign<&ast::VarDecl::type>(ref<SequenceType>)) +
            ((lit<":="> + assign<&ast::VarDecl::value>(ref<ExprSingle>))  //
             | (flag<&ast::VarDecl::external>(kw<"external">) +
                option(lit<":="> + assign<&ast::VarDecl::value>(ref<ExprSingle>)))))> {
  };

  struct ParamWithDefault
      : rule<create<ast::Param>(
            lit<"$"> + assign<&ast::Param::name>(ref<EqName>) +
            option(kw<"as"> + assign<&ast::Param::type>(ref<SequenceType>)) +
            option(lit<":="> + assign<&ast::Param::def>(ref<ExprSingle>)))> {};

  struct FunctionDecl
      : rule<create<ast::FunctionDecl>(
            kw<"declare"> +
            many(append<&ast::FunctionDecl::annotations>(ref<Annotation>)) +
            kw<"function"> + assign<&ast::FunctionDecl::name>(ref<EqName>) + lit<"("> +
            option(append<&ast::FunctionDecl::params>(ref<ParamWithDefault>) +
                   many(lit<","> + append<&ast::FunctionDecl::params>(
                                       ref<ParamWithDefault>))) +
            lit<")"> +
            option(kw<"as"> + assign<&ast::FunctionDecl::returnType>(ref<SequenceType>)) +
            ((lit<"{"> + option(assign<&ast::FunctionDecl::body>(ref<Expr>)) + lit<"}">)  //
             | flag<&ast::FunctionDecl::external>(kw<"external">)))> {};

  struct ItemTypeDecl
      : rule<create<ast::ItemTypeDecl>(
            kw<"declare"> +
            many(append<&ast::ItemTypeDecl::annotations>(ref<Annotation>)) + kw<"type"> +
            assign<&ast::ItemTypeDecl::name>(ref<EqName>) + kw<"as"> +
            assign<&ast::ItemTypeDecl::type>(ref<ItemType>))> {};

  struct NamedRecordTypeDecl
      : rule<create<ast::NamedRecordTypeDecl>(
            kw<"declare"> +
            many(append<&ast::NamedRecordTypeDecl::annotations>(ref<Annotation>)) +
            kw<"record"> + assign<&ast::NamedRecordTypeDecl::name>(ref<EqName>) +
            lit<"("> +
            option(append<&ast::NamedRecordTypeDecl::fields>(ref<ExtendedFieldDecl>) +
                   many(lit<","> + append<&ast::NamedRecordTypeDecl::fields>(
                                       ref<ExtendedFieldDecl>))) +
            lit<")">)> {};

  struct OptionDecl
      : rule<create<ast::OptionDecl>(kw<"declare"> + kw<"option"> +
                                     assign<&ast::OptionDecl::name>(ref<EqName>) +
                                     assign<&ast::OptionDecl::value>(ref<STRING>))> {};

  static constexpr auto PrologFirstItem =
      append<&ast::Prolog::items>(ref<DefaultNamespaceDecl>)  //
      | append<&ast::Prolog::items>(ref<Setter>)  //
      | append<&ast::Prolog::items>(ref<DecimalFormatDecl>)  //
      | append<&ast::Prolog::items>(ref<NamespaceDecl>)  //
      | append<&ast::Prolog::items>(ref<SchemaImport>)  //
      | append<&ast::Prolog::items>(ref<ModuleImport>);
  static constexpr auto PrologSecondItem =
      append<&ast::Prolog::items>(ref<ContextValueDecl>)  //
      | append<&ast::Prolog::items>(ref<VarDecl>)  //
      | append<&ast::Prolog::items>(ref<FunctionDecl>)  //
      | append<&ast::Prolog::items>(ref<ItemTypeDecl>)  //
      | append<&ast::Prolog::items>(ref<NamedRecordTypeDecl>)  //
      | append<&ast::Prolog::items>(ref<OptionDecl>);

  struct Prolog
      : rule<create<ast::Prolog>(many(PrologFirstItem + lit<";">) +
                                 many(PrologSecondItem + lit<";">))> {};

  // ---- module / query list --------------------------------------------

  struct LibraryModule
      : rule<create<ast::LibraryModule>(
            kw<"module"> + kw<"namespace"> +
            assign<&ast::LibraryModule::prefix>(ref<NCName>) + lit<"="> +
            assign<&ast::LibraryModule::uri>(ref<STRING>) + lit<";"> +
            assign<&ast::LibraryModule::prolog>(ref<Prolog>))> {};

  struct MainModule
      : rule<create<ast::MainModule>(
            assign<&ast::MainModule::prolog>(ref<Prolog>) +
            assign<&ast::MainModule::body>(ref<Expr>))> {};

  struct Module
      : rule<create<ast::Module>(
            option(kw<"xquery"> +
                   ((kw<"encoding"> + assign<&ast::Module::encoding>(ref<STRING>))  //
                    | (kw<"version"> + assign<&ast::Module::version>(ref<STRING>) +
                       option(kw<"encoding"> + assign<&ast::Module::encoding>(ref<STRING>)))) +
                   lit<";">) +
            (assign<&ast::Module::library>(ref<LibraryModule>)  //
             | assign<&ast::Module::main>(ref<MainModule>)))> {};

  struct QueryList
      : rule<create<ast::QueryList>(
            append<&ast::QueryList::modules>(ref<Module>) +
            many((lit<";"> | lit<"%%%">) +
                 option(append<&ast::QueryList::modules>(ref<Module>))))> {};

  using entry = QueryList;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace xquery
