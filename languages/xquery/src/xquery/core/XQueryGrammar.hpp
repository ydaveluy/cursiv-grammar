// The xquery language: its grammar. XQuery 4.0 is XPath 4.0's expression
// grammar plus module structure, FLWOR, constructors and Update Facility 1.0.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xquery/core/ast.hpp"

namespace xquery {

inline constexpr auto unquote_str = [](std::string_view t,
                                       std::pmr::memory_resource* mem) {
  const char q = t.front();
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find(q) == std::string_view::npos) return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == q) ++i;
  }
  return std::string_view(room.data(), n);
};
using UnquoteStr = cursiv::text_conv<unquote_str>;

inline constexpr auto parse_hex = [](std::string_view t) -> std::int64_t {
  std::int64_t v = 0;
  std::from_chars(t.data() + 2, t.data() + t.size(), v, 16);
  return v;
};
using ParseHex = cursiv::text_conv<parse_hex>;

inline constexpr auto parse_bin = [](std::string_view t) -> std::int64_t {
  std::int64_t v = 0;
  for (std::size_t i = 2; i < t.size(); ++i)
    if (t[i] != '_') v = v * 2 + (t[i] - '0');
  return v;
};
using ParseBin = cursiv::text_conv<parse_bin>;

inline constexpr auto strip_hash = [](std::string_view t) {
  return t.substr(1);
};
using StripHash = cursiv::text_conv<strip_hash>;

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

  struct WS : terminal<some(s)> {};
  struct COMMENT
      : terminal<lit<"(:"> +
                 many(ref<COMMENT> | (!lit<"(:"> + !lit<":)"> + any)) +
                 lit<":)">> {};

  struct NCName : terminal<NameStartChar + many(NameContChar)> {};
  struct QName : terminal<NameStartChar + many(NameContChar) +
                          option(lit<":"> + NameStartChar + many(NameContChar))> {};
  struct URIQualifiedName
      : terminal<lit<"Q{"> + many(cr<"^{}">) + lit<"}"> + NameStartChar +
                many(NameContChar) +
                option(lit<":"> + NameStartChar + many(NameContChar))> {};

  struct IntegerLiteral : terminal<some(d), std::int64_t> {};
  struct HexIntegerLiteral
      : terminal<lit<"0x"> + some(cr<"a-fA-F0-9">), ParseHex> {};
  struct BinaryIntegerLiteral
      : terminal<lit<"0b"> + cr<"01"> + many(cr<"01_">), ParseBin> {};
  static constexpr auto DecimalBody =
      (lit<"."> + some(d)) | (some(d) + lit<"."> + many(d));
  struct DecimalLiteral : terminal<DecimalBody> {};
  struct DoubleLiteral
      : terminal<((lit<"."> + some(d)) | (some(d) + option(lit<"."> + many(d)))) +
                 cr<"eE"> + option(cr<"+-">) + some(d)> {};

  static constexpr auto dq_body = lit<"\"\""> | cr<R"(^")">;
  static constexpr auto sq_body = lit<"''"> | cr<"^'">;
  struct STRING
      : terminal<(lit<"\""> + many(dq_body) + lit<"\"">) |
                     (lit<"'"> + many(sq_body) + lit<"'">),
                 UnquoteStr> {};

  struct StringTemplate
      : terminal<lit<"`"> + many(lit<"{{"> | lit<"}}"> | cr<"^`">) + lit<"`">> {
  };
  struct StringConstructor
      : terminal<through(lit<"``[">, lit<"]``">)> {};

  struct Pragma
      : terminal<lit<"(#"> + many(s) + NameStartChar + many(NameContChar) +
                     option(lit<":"> + NameStartChar + many(NameContChar)) +
                     option(some(s) + many(cr<"^#"> | (lit<"#"> + !lit<")">))) +
                     lit<"#)">> {};

  // ---- names -----------------------------------------------------------

  struct EqNameRule
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

  struct VarNameRule : rule<lit<"$"> + ref<EqNameRule>> {};

  // Forward declarations: the grammar is mutually recursive top to bottom
  // (expressions reach the primary/postfix layer, which reaches every
  // constructor, which reaches expressions again).
  struct SequenceTypeRule;
  struct ItemTypeRule;
  struct ExprRule;
  struct ExprSingleRule;
  struct NumericLiteralRule;
  struct ParenExprRule;
  struct FunctionItemExprRule;
  struct MapConstructorRule;
  struct ArrayConstructorRule;
  struct PrimaryExprRule;
  struct PrologRule;
  struct QueryListRule;
  struct DirElemConstructorRule;
  struct OrExprRule;
  struct VarNameAndTypeRule;

  static constexpr auto Occurrence = lit<"?"> | lit<"*"> | lit<"+">;

  struct FieldDeclRule
      : rule<assign<&ast::FieldDecl::name>(ref<EqNameRule> | ref<STRING>) +
             option(kw<"as"> + assign<&ast::FieldDecl::type>(ref<SequenceTypeRule>))> {
  };
  struct ExtendedFieldDeclRule
      : rule<assign<&ast::FieldDecl::name>(ref<EqNameRule> | ref<STRING>) +
             option(kw<"as"> + assign<&ast::FieldDecl::type>(ref<SequenceTypeRule>)) +
             option(lit<":="> + assign<&ast::FieldDecl::def>(ref<ExprSingleRule>))> {
  };

  static constexpr auto NameTestWord = ref<EqNameRule> | lit<"*">;

  struct XNodeTypeRule
      : rule<create<ast::ItemType>(
                 assign<&ast::ItemType::name>(kw<"document-node">) + lit<"("> +
                 option(append<&ast::ItemType::names>(NameTestWord) |
                        append<&ast::ItemType::names>(ref<STRING>)) +
                 lit<")">)  //
             | create<ast::ItemType>(
                   assign<&ast::ItemType::name>(kw<"element">) + lit<"("> +
                   option(append<&ast::ItemType::names>(NameTestWord) +
                          many(lit<"|"> + append<&ast::ItemType::names>(NameTestWord)) +
                          option(lit<","> +
                                 append<&ast::ItemType::names>(ref<EqNameRule>) +
                                 option(lit<"?">))) +
                   lit<")">)  //
             | create<ast::ItemType>(
                   assign<&ast::ItemType::name>(kw<"attribute">) + lit<"("> +
                   option(append<&ast::ItemType::names>(NameTestWord) +
                          many(lit<"|"> + append<&ast::ItemType::names>(NameTestWord)) +
                          option(lit<","> + append<&ast::ItemType::names>(ref<EqNameRule>))) +
                   lit<")">)  //
             | create<ast::ItemType>(
                   assign<&ast::ItemType::name>(kw<"schema-element">) + lit<"("> +
                   append<&ast::ItemType::names>(ref<EqNameRule>) + lit<")">)  //
             | create<ast::ItemType>(
                   assign<&ast::ItemType::name>(kw<"schema-attribute">) + lit<"("> +
                   append<&ast::ItemType::names>(ref<EqNameRule>) + lit<")">)  //
             | create<ast::ItemType>(
                   assign<&ast::ItemType::name>(kw<"processing-instruction">) + lit<"("> +
                   option(append<&ast::ItemType::names>(ref<QName> | ref<STRING>)) +
                   lit<")">)  //
             | create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"comment">) +
                                     lit<"("> + lit<")">)  //
             | create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"text">) + lit<"("> +
                                     lit<")">)  //
             | create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"namespace-node">) +
                                     lit<"("> + lit<")">)  //
             | create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"node">) + lit<"("> +
                                     lit<")">)> {};

  struct GNodeTypeRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"gnode">) + lit<"("> +
                                   lit<")">)> {};

  static constexpr auto JRootSelector = lit<"("> + lit<")">;
  // A jnode selector's string/number/name constant is kept as raw spelling
  // (never decoded): jnode/gnode are the grammar's least-used corner.
  struct RawNumber
      : terminal<option(lit<"-">) + some(d) + option(lit<"."> + many(d)) +
                option(cr<"eE"> + option(cr<"+-">) + some(d))> {};
  struct JNodeTypeRule
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"jnode">) + lit<"("> +
            option((lit<"*"> | JRootSelector | append<&ast::ItemType::names>(ref<QName>) |
                    append<&ast::ItemType::names>(ref<STRING> | ref<RawNumber>)) +
                   option(lit<","> +
                          append<&ast::ItemType::params>(ref<SequenceTypeRule>))) +
            lit<")">)> {};

  struct TypeTestRule
      : rule<ref<GNodeTypeRule> | ref<XNodeTypeRule> | ref<JNodeTypeRule>> {};

  struct SelectorRule : rule<NameTestWord> {};

  // A union member: fed both standalone (SimpleNodeTestRule) and repeated
  // inside a `(...)` union (NodeTestRule) -- one fragment, not two rules, so
  // both write into the SAME NodeTest node rather than nesting one.
  static constexpr auto SimpleNodeTestMember =
      append<&ast::NodeTest::types>(ref<TypeTestRule>)  //
      | append<&ast::NodeTest::names>(ref<SelectorRule>);

  struct SimpleNodeTestRule
      : rule<create<ast::NodeTest>(SimpleNodeTestMember)> {};

  struct NodeTestRule
      : rule<create<ast::NodeTest>(lit<"("> + SimpleNodeTestMember +
                                   many(lit<"|"> + SimpleNodeTestMember) + lit<")">)  //
             | ref<SimpleNodeTestRule>  //
             | create<ast::NodeTest>(
                   lit<"{"> + option(assign<&ast::NodeTest::dynamic>(ref<ExprRule>)) +
                   lit<"}">)> {};

  static constexpr auto ChoiceItemType =
      lit<"("> + append<&ast::ItemType::itemParams>(ref<ItemTypeRule>) +
      many(lit<"|"> + append<&ast::ItemType::itemParams>(ref<ItemTypeRule>)) + lit<")">;

  struct EnumerationTypeRule
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"enum">) + lit<"("> +
            append<&ast::ItemType::names>(ref<STRING>) +
            many(lit<","> + append<&ast::ItemType::names>(ref<STRING>)) + lit<")">)> {};

  struct SignedRawNumber : rule<option(lit<"-">) + ref<RawNumber>> {};

  struct AnnotationRule
      : rule<lit<"%"> + assign<&ast::Annotation::name>(ref<EqNameRule>) +
             option(lit<"("> +
                    some(append<&ast::Annotation::args>(
                             ref<STRING> | ref<SignedRawNumber> | ref<EqNameRule>),
                         lit<",">) +
                    lit<")">)> {};

  static constexpr auto Annotations =
      many(append<&ast::ItemType::annotations>(ref<AnnotationRule>));
  static constexpr auto FunctionKw = kw<"function"> | kw<"fn">;

  struct TypedFunctionParam
      : rule<option(lit<"$"> + ref<EqNameRule> + kw<"as">) + ref<SequenceTypeRule>> {};

  struct FunctionTypeRule
      : rule<Annotations +
             (create<ast::ItemType>(assign<&ast::ItemType::name>(FunctionKw) + lit<"("> +
                                    lit<"*"> + lit<")">)  //
              | create<ast::ItemType>(
                    assign<&ast::ItemType::name>(FunctionKw) + lit<"("> +
                    option(append<&ast::ItemType::params>(ref<TypedFunctionParam>) +
                           many(lit<","> +
                                append<&ast::ItemType::params>(ref<TypedFunctionParam>))) +
                    lit<")"> + kw<"as"> +
                    assign<&ast::ItemType::returnType>(ref<SequenceTypeRule>)))> {};

  struct TypedArrayTypeRule
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
            append<&ast::ItemType::params>(ref<SequenceTypeRule>) + lit<")">)> {};
  struct ArrayTypeRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
                                   lit<"*"> + lit<")">)  //
             | ref<TypedArrayTypeRule>> {};

  struct TypedMapTypeRule
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
            append<&ast::ItemType::itemParams>(ref<ItemTypeRule>) + lit<","> +
            append<&ast::ItemType::params>(ref<SequenceTypeRule>) + lit<")">)> {};
  struct MapTypeRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
                                   lit<"*"> + lit<")">)  //
             | ref<TypedMapTypeRule>> {};

  struct TypedRecordTypeRule
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"record">) + lit<"("> +
            option(append<&ast::ItemType::fields>(ref<FieldDeclRule>) +
                   many(lit<","> + append<&ast::ItemType::fields>(ref<FieldDeclRule>))) +
            lit<")">)> {};
  struct RecordTypeRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"record">) + lit<"("> +
                                   lit<"*"> + lit<")">)  //
             | ref<TypedRecordTypeRule>> {};

  struct AnyItemTypeRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"item">) + lit<"("> +
                                   lit<")">)> {};

  struct RegularItemTypeRule
      : rule<ref<AnyItemTypeRule> | ref<TypeTestRule> | ref<MapTypeRule> |
             ref<ArrayTypeRule> | ref<RecordTypeRule> | ref<EnumerationTypeRule>> {};

  struct TypeNameRule
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(ref<EqNameRule>))> {};

  struct ChoiceItemTypeRule
      : rule<create<ast::ItemType>(ChoiceItemType)> {};

  struct ItemTypeRule
      : rule<ref<RegularItemTypeRule> | ref<FunctionTypeRule> | ref<TypeNameRule> |
             ref<ChoiceItemTypeRule>> {};

  struct SequenceTypeRule
      : rule<create<ast::SequenceType>(
                 flag<&ast::SequenceType::emptySequence>(kw<"empty-sequence">) +
                 lit<"("> + lit<")">)  //
             | create<ast::SequenceType>(
                   assign<&ast::SequenceType::itemType>(ref<ItemTypeRule>) +
                   option(assign<&ast::SequenceType::occurrence>(Occurrence)))> {
  };

  // `|`-separated: a case clause could not otherwise tell
  // two union members apart.
  static constexpr auto SequenceTypeUnion =
      append<&ast::TypeswitchCase::types>(ref<SequenceTypeRule>) +
      many(lit<"|"> + append<&ast::TypeswitchCase::types>(ref<SequenceTypeRule>));

  struct CastTargetRule
      : rule<ref<TypeNameRule> | ref<ChoiceItemTypeRule> | ref<EnumerationTypeRule> |
             ref<TypedArrayTypeRule> | ref<TypedMapTypeRule> | ref<TypedRecordTypeRule>> {
  };

  // ---- numeric / literal expressions -------------------------------------

  struct NumericLiteralRule
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

  struct QNameLiteralRule
      : rule<create<ast::QNameLiteral>(lit<"#"> +
                                       assign<&ast::QNameLiteral::name>(ref<EqNameRule>))> {
  };

  struct LiteralRule
      : rule<ref<NumericLiteralRule>  //
             | create<ast::StringLiteral>(assign<&ast::StringLiteral::value>(ref<STRING>))  //
             | ref<QNameLiteralRule>> {};

  // ---- operators ----------------------------------------------------------

  struct OpOr : terminal<kw<"or">, value<ast::BinOp::or_>> {};
  struct OpAnd : terminal<kw<"and">, value<ast::BinOp::and_>> {};
  struct OpOtherwise : terminal<kw<"otherwise">, value<ast::BinOp::otherwise_>> {};
  struct OpConcat : terminal<lit<"||">, value<ast::BinOp::concat>> {};
  struct OpTo : terminal<kw<"to">, value<ast::BinOp::range>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::BinOp::sub>> {};
  struct OpMul : terminal<lit<"*">, value<ast::BinOp::mul>> {};
  struct OpMulAlt : terminal<lit<"×">, value<ast::BinOp::mul>> {};
  struct OpDiv : terminal<kw<"div">, value<ast::BinOp::div_>> {};
  struct OpDivAlt : terminal<lit<"÷">, value<ast::BinOp::div_>> {};
  struct OpIdiv : terminal<kw<"idiv">, value<ast::BinOp::idiv>> {};
  struct OpMod : terminal<kw<"mod">, value<ast::BinOp::mod>> {};
  struct OpUnion : terminal<kw<"union">, value<ast::BinOp::union_>> {};
  struct OpUnionAlt : terminal<lit<"|">, value<ast::BinOp::union_>> {};
  struct OpIntersect : terminal<kw<"intersect">, value<ast::BinOp::intersect>> {};
  struct OpExcept : terminal<kw<"except">, value<ast::BinOp::except>> {};
  struct OpSimpleMap : terminal<lit<"!">, value<ast::BinOp::simple_map>> {};
  struct OpRecordPut : terminal<lit<"+:=">, value<ast::BinOp::record_put>> {};
  struct OpPipeline : terminal<lit<"->">, value<ast::BinOp::pipeline>> {};
  struct OpChildSlash : terminal<lit<"/">, value<ast::BinOp::path_child>> {};
  struct OpDescendantSlash : terminal<lit<"//">, value<ast::BinOp::path_descendant>> {};

  struct OpLe : terminal<lit<"<=">, value<ast::BinOp::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::BinOp::ge>> {};
  struct OpNe : terminal<lit<"!=">, value<ast::BinOp::ne>> {};
  struct OpLt : terminal<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::BinOp::gt>> {};
  struct OpEq : terminal<lit<"=">, value<ast::BinOp::eq>> {};
  struct OpValEq : terminal<kw<"eq">, value<ast::BinOp::value_eq>> {};
  struct OpValNe : terminal<kw<"ne">, value<ast::BinOp::value_ne>> {};
  struct OpValLt : terminal<kw<"lt">, value<ast::BinOp::value_lt>> {};
  struct OpValLe : terminal<kw<"le">, value<ast::BinOp::value_le>> {};
  struct OpValGt : terminal<kw<"gt">, value<ast::BinOp::value_gt>> {};
  struct OpValGe : terminal<kw<"ge">, value<ast::BinOp::value_ge>> {};
  struct OpPrecedesOp : terminal<lit<"<<">, value<ast::BinOp::node_precedes>> {};
  struct OpPrecedesKw : terminal<kw<"precedes">, value<ast::BinOp::node_precedes>> {};
  struct OpFollowsOp : terminal<lit<">>">, value<ast::BinOp::node_follows>> {};
  struct OpFollowsKw : terminal<kw<"follows">, value<ast::BinOp::node_follows>> {};
  struct OpPrecedesOrIs
      : terminal<kw<"precedes-or-is">, value<ast::BinOp::node_precedes_or_is>> {};
  struct OpFollowsOrIs
      : terminal<kw<"follows-or-is">, value<ast::BinOp::node_follows_or_is>> {};
  struct OpIs : terminal<kw<"is">, value<ast::BinOp::node_is>> {};
  struct OpIsNot : terminal<kw<"is-not">, value<ast::BinOp::node_is_not>> {};

  static constexpr auto GeneralComp =
      ref<OpEq> | ref<OpNe> | ref<OpLe> | ref<OpLt> | ref<OpGe> | ref<OpGt>;
  static constexpr auto ValueComp = ref<OpValEq> | ref<OpValNe> | ref<OpValLt> |
                                    ref<OpValLe> | ref<OpValGt> | ref<OpValGe>;
  static constexpr auto NodeComp = ref<OpIs> | ref<OpIsNot> | ref<OpPrecedesOp> |
                                   ref<OpPrecedesKw> | ref<OpFollowsOp> |
                                   ref<OpFollowsKw> | ref<OpPrecedesOrIs> |
                                   ref<OpFollowsOrIs>;

  // ---- axis --------------------------------------------------------------

  static constexpr auto AxisWord =
      kw<"ancestor-or-self"> | kw<"ancestor"> | kw<"attribute"> | kw<"child"> |
      kw<"descendant-or-self"> | kw<"descendant"> | kw<"following-or-self"> |
      kw<"following-sibling-or-self"> | kw<"following-sibling"> | kw<"following"> |
      kw<"parent"> | kw<"preceding-or-self"> | kw<"preceding-sibling-or-self"> |
      kw<"preceding-sibling"> | kw<"preceding"> | kw<"self">;
  struct AxisWordRule : rule<AxisWord> {};

  static constexpr auto Predicate = lit<"["> + ref<ExprRule> + lit<"]">;

  struct AxisStepRule
      : rule<create<ast::AxisStep>(
            (flag<&ast::AxisStep::isParentShortcut>(lit<"..">)  //
             | (flag<&ast::AxisStep::isAttributeShortcut>(lit<"@">) +
                assign<&ast::AxisStep::test>(ref<NodeTestRule>))  //
             | (assign<&ast::AxisStep::axis>(ref<AxisWordRule>) + lit<"::"> +
                assign<&ast::AxisStep::test>(ref<NodeTestRule>))  //
             | assign<&ast::AxisStep::test>(ref<SimpleNodeTestRule>)) +
            many(lit<"["> + append<&ast::AxisStep::predicates>(ref<ExprRule>) +
                 lit<"]">))> {};

  // ---- arguments / calls / postfix ---------------------------------------

  struct ArgumentRule
      : rule<create<ast::ArgPlaceholder>(lit<"?">) | ref<ExprSingleRule>> {};

  struct KeywordArgumentRule
      : rule<create<ast::KeywordArg>(
            assign<&ast::KeywordArg::name>(ref<EqNameRule>) + lit<":="> +
            assign<&ast::KeywordArg::value>(ref<ArgumentRule>))> {};

  struct PositionalArgumentListRule
      : rule<create<ast::Arguments>(
            lit<"("> +
            option(append<&ast::Arguments::positional>(ref<ArgumentRule>) +
                   many(lit<","> + append<&ast::Arguments::positional>(ref<ArgumentRule>))) +
            lit<")">)> {};

  struct ArgumentListRule
      : rule<create<ast::Arguments>(
            lit<"("> +
            option((append<&ast::Arguments::positional>(ref<ArgumentRule>) +
                    many(lit<","> +
                         append<&ast::Arguments::positional>(ref<ArgumentRule>)) +
                    option(lit<","> +
                           append<&ast::Arguments::keyword>(ref<KeywordArgumentRule>) +
                           many(lit<","> +
                                append<&ast::Arguments::keyword>(ref<KeywordArgumentRule>))))  //
                   | (append<&ast::Arguments::keyword>(ref<KeywordArgumentRule>) +
                      many(lit<","> +
                           append<&ast::Arguments::keyword>(ref<KeywordArgumentRule>)))) +
            lit<")">)> {};

  // A step position reaches both an unprefixed call (`name(args)`, here) and
  // an abbreviated kind-test step (`element(...)`, AxisStepRule via
  // SimpleNodeTestRule): the same span would complete two ways for any of
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
  struct NonReservedEqNameRule
      : rule<!ReservedFunctionName + ref<EqNameRule>> {};

  struct FunctionCallRule
      : rule<create<ast::CallExpr>(
            assign<&ast::CallExpr::name>(ref<NonReservedEqNameRule>) +
            assign<&ast::CallExpr::args>(ref<ArgumentListRule>))> {};

  struct MarkedNCNameRule : rule<lit<"#"> + ref<QName>, StripHash> {};

  struct VarRefRule
      : rule<create<ast::VarRef>(lit<"$"> + assign<&ast::VarRef::name>(ref<EqNameRule>))> {
  };

  struct KeySpecifierRule
      : rule<create<ast::Lookup>(assign<&ast::Lookup::key>(ref<QName>))  //
             | create<ast::Lookup>(
                   assign<&ast::Lookup::key>(ref<STRING> | ref<RawNumber>))  //
             | create<ast::Lookup>(assign<&ast::Lookup::key>(lit<".">))  //
             | create<ast::Lookup>(assign<&ast::Lookup::keyExpr>(
                   ref<VarRefRule> | ref<ParenExprRule>))  //
             | create<ast::Lookup>(flag<&ast::Lookup::wildcard>(lit<"*">))> {};

  struct LookupRule : rule<lit<"?"> + ref<KeySpecifierRule>> {};

  struct RestrictedDynamicCallRule
      : rule<create<ast::PostfixExpr>(
            assign<&ast::PostfixExpr::base>(
                ref<VarRefRule> | ref<ParenExprRule> | ref<FunctionItemExprRule> |
                ref<MapConstructorRule> | ref<ArrayConstructorRule>) +
            append<&ast::PostfixExpr::suffixes>(ref<PositionalArgumentListRule>))> {};

  static constexpr auto PostfixSuffixAlt =
      (lit<"["> + append<&ast::PostfixExpr::suffixes>(ref<ExprRule>) + lit<"]">)  //
      | append<&ast::PostfixExpr::suffixes>(ref<PositionalArgumentListRule>)  //
      | append<&ast::PostfixExpr::suffixes>(ref<LookupRule>)  //
      | append<&ast::PostfixExpr::suffixes>(create<ast::MethodCall>(
            lit<"=?>"> + assign<&ast::MethodCall::name>(ref<QName>) +
            assign<&ast::MethodCall::args>(ref<PositionalArgumentListRule>)));

  // No suffix: adopt PrimaryExprRule's own node as-is, so a bare literal
  // surfaces as itself and not wrapped in an empty PostfixExpr.
  struct PostfixExprRule
      : rule<create<ast::PostfixExpr>(
                 assign<&ast::PostfixExpr::base>(ref<PrimaryExprRule>) +
                 some(PostfixSuffixAlt))  //
             | ref<PrimaryExprRule>> {};

  // ---- constructors -------------------------------------------------------

  struct QNameLiteralTextRule : rule<lit<"#"> + ref<EqNameRule>, StripHash> {};

  struct CompNameRule
      : rule<create<ast::CompName>(
                 assign<&ast::CompName::literal>(ref<QNameLiteralTextRule>))  //
             | create<ast::CompName>(assign<&ast::CompName::literal>(ref<EqNameRule>))  //
             | create<ast::CompName>(lit<"{"> +
                                     assign<&ast::CompName::dynamic>(ref<ExprRule>) +
                                     lit<"}">)> {};

  struct CompNCNameRule
      : rule<create<ast::CompName>(assign<&ast::CompName::literal>(ref<MarkedNCNameRule>))  //
             | create<ast::CompName>(assign<&ast::CompName::literal>(ref<NCName>))  //
             | create<ast::CompName>(lit<"{"> +
                                     assign<&ast::CompName::dynamic>(ref<ExprRule>) +
                                     lit<"}">)> {};

  static constexpr auto EnclosedExpr =
      lit<"{"> + option(assign<&ast::ComputedConstructor::content>(ref<ExprRule>)) + lit<"}">;

  struct KindDocument : rule<kw<"document">, value<ast::ConstructorKind::document>> {};
  struct KindElement : rule<kw<"element">, value<ast::ConstructorKind::element>> {};
  struct KindAttribute : rule<kw<"attribute">, value<ast::ConstructorKind::attribute>> {};
  struct KindNamespace : rule<kw<"namespace">, value<ast::ConstructorKind::namespace_>> {};
  struct KindText : rule<kw<"text">, value<ast::ConstructorKind::text>> {};
  struct KindComment : rule<kw<"comment">, value<ast::ConstructorKind::comment>> {};
  struct KindPI
      : rule<kw<"processing-instruction">, value<ast::ConstructorKind::pi>> {};

  struct ComputedConstructorRule
      : rule<create<ast::ComputedConstructor>(
                 assign<&ast::ComputedConstructor::kind>(ref<KindDocument>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindElement>) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNameRule>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindAttribute>) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNameRule>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindNamespace>) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNCNameRule>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindText>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindComment>) + EnclosedExpr)  //
             | create<ast::ComputedConstructor>(
                   assign<&ast::ComputedConstructor::kind>(ref<KindPI>) +
                   assign<&ast::ComputedConstructor::name>(ref<CompNCNameRule>) +
                   EnclosedExpr)> {};

  // ---- direct constructors --------------------------------------------
  // The element form of a direct constructor:
  // per-state lexing needs no lexer-mode mechanism for it.

  static constexpr auto EntityRef = lit<"&lt;"> | lit<"&gt;"> | lit<"&amp;"> |
                                    lit<"&quot;"> | lit<"&apos;">;
  static constexpr auto CharRefBody =
      (lit<"&#"> + some(d) + lit<";">) |
      (lit<"&#x"> + some(cr<"a-fA-F0-9">) + lit<";">);

  struct TagOpen
      : terminal<lit<"<"> + NameStartChar + many(NameContChar) +
                option(lit<":"> + NameStartChar + many(NameContChar)),
                StripHash> {};
  struct AttrTextQuot
      : terminal<some(lit<"\"\""> | EntityRef | CharRefBody | cr<R"(^"{&<)">)> {};
  struct AttrTextApos
      : terminal<some(lit<"''"> | EntityRef | CharRefBody | cr<"^'{&<">)> {};
  struct ElemText
      : terminal<some(lit<"{{"> | lit<"}}"> | EntityRef | CharRefBody |
                     cr<"^&<{}">)> {};
  struct CDataSection : terminal<through(lit<"<![CDATA[">, lit<"]]>">)> {};
  struct DirCommentTerm : terminal<through(lit<"<!--">, lit<"-->">)> {};
  struct DirPITerm : terminal<through(lit<"<?">, lit<"?>">)> {};

  // No skip() island: a region only NARROWS, and can never widen back for an
  // embedded `{expr}`'s own operators, which need the ambient skipper same as
  // anywhere else. Instead this stays under the ambient `ignore = WS`
  // everywhere, and a text run wins the whitespace it touches over the
  // ambient skipper by the longer match (mirrors xml's SEA_WS/TEXT: a
  // whitespace-only run still loses to trivia on the tie, which is the one
  // accepted gap).
  struct DirAttrRule
      : rule<create<ast::DirAttr>(
            assign<&ast::DirAttr::name>(ref<QName>) + lit<"="> +
            ((lit<"\""> +
              many(append<&ast::DirAttr::value>(ref<AttrTextQuot>) |
                   (lit<"{"> + append<&ast::DirAttr::value>(ref<ExprRule>) + lit<"}">)) +
              lit<"\"">)  //
             | (lit<"'"> +
                many(append<&ast::DirAttr::value>(ref<AttrTextApos>) |
                     (lit<"{"> + append<&ast::DirAttr::value>(ref<ExprRule>) +
                      lit<"}">)) +
                lit<"'">)))> {};

  static constexpr auto DirContentItem =
      append<&ast::DirElemConstructor::content>(ref<ElemText>)  //
      | append<&ast::DirElemConstructor::content>(ref<CDataSection>)  //
      | append<&ast::DirElemConstructor::content>(ref<DirCommentTerm>)  //
      | append<&ast::DirElemConstructor::content>(ref<DirPITerm>)  //
      | append<&ast::DirElemConstructor::content>(ref<DirElemConstructorRule>)  //
      | (lit<"{"> + append<&ast::DirElemConstructor::content>(ref<ExprRule>) + lit<"}">);

  struct DirElemConstructorRule
      : rule<create<ast::DirElemConstructor>(
            assign<&ast::DirElemConstructor::name>(ref<TagOpen>) +
            many(append<&ast::DirElemConstructor::attributes>(ref<DirAttrRule>)) +
            (flag<&ast::DirElemConstructor::selfClosing>(lit<"/>">)  //
             | (lit<">"> + many(DirContentItem) + lit<"</"> +
                assign<&ast::DirElemConstructor::closeName>(ref<QName>) + lit<">">)))> {
  };

  struct DirCommentConstructorRule
      : rule<create<ast::DirComment>(assign<&ast::DirComment::text>(ref<DirCommentTerm>))> {
  };
  struct DirPIConstructorRule
      : rule<create<ast::DirPI>(assign<&ast::DirPI::text>(ref<DirPITerm>))> {};

  struct NodeConstructorRule
      : rule<ref<DirElemConstructorRule> | ref<DirCommentConstructorRule> |
             ref<DirPIConstructorRule> | ref<ComputedConstructorRule>> {};

  // ---- map / array / mode / function-item constructors --------------------

  struct MapEntryRule
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ref<ExprSingleRule>) +
            option(lit<":"> + assign<&ast::MapEntry::value>(ref<ExprSingleRule>)))> {};

  struct MapConstructorRule
      : rule<create<ast::MapConstructor>(
            option(kw<"map">) + lit<"{"> +
            option(append<&ast::MapConstructor::entries>(ref<MapEntryRule>) +
                   many(lit<","> +
                        append<&ast::MapConstructor::entries>(ref<MapEntryRule>))) +
            lit<"}">)> {};

  struct ArrayConstructorRule
      : rule<create<ast::ArrayConstructor>(
                 lit<"["> +
                 option(append<&ast::ArrayConstructor::items>(ref<ExprSingleRule>) +
                        many(lit<","> + append<&ast::ArrayConstructor::items>(
                                            ref<ExprSingleRule>))) +
                 lit<"]">)  //
             | create<ast::ArrayConstructor>(
                   flag<&ast::ArrayConstructor::curly>(kw<"array">) + lit<"{"> +
                   option(assign<&ast::ArrayConstructor::body>(ref<ExprRule>)) +
                   lit<"}">)> {};

  struct OrderedExprRule
      : rule<create<ast::ModeExpr>(flag<&ast::ModeExpr::ordered>(kw<"ordered">) +
                                   lit<"{"> +
                                   option(assign<&ast::ModeExpr::body>(ref<ExprRule>)) +
                                   lit<"}">)> {};
  struct UnorderedExprRule
      : rule<create<ast::ModeExpr>(kw<"unordered"> + lit<"{"> +
                                   option(assign<&ast::ModeExpr::body>(ref<ExprRule>)) +
                                   lit<"}">)> {};

  struct ContextItemExprRule : rule<create<ast::ContextItemExpr>(lit<".">)> {};

  struct ParenExprRule
      : rule<create<ast::ParenExpr>(lit<"("> +
                                    option(assign<&ast::ParenExpr::body>(ref<ExprRule>)) +
                                    lit<")">)> {};

  struct NamedFunctionRefRule
      : rule<create<ast::NamedFunctionRef>(
            assign<&ast::NamedFunctionRef::name>(ref<EqNameRule>) + lit<"#"> +
            assign<&ast::NamedFunctionRef::arity>(ref<IntegerLiteral>))> {};

  // A fragment, not a rule<>: it writes into the enclosing InlineFunctionExpr
  // directly, and a rule<> of its own would give that node a second head.
  static constexpr auto FunctionSignature =
      lit<"("> +
      option(append<&ast::InlineFunctionExpr::params>(ref<VarNameAndTypeRule>) +
             many(lit<","> +
                  append<&ast::InlineFunctionExpr::params>(ref<VarNameAndTypeRule>))) +
      lit<")"> +
      option(kw<"as"> +
             assign<&ast::InlineFunctionExpr::returnType>(ref<SequenceTypeRule>));

  struct InlineFunctionExprRule
      : rule<create<ast::InlineFunctionExpr>(
            many(append<&ast::InlineFunctionExpr::annotations>(ref<AnnotationRule>)) +
            FunctionKw + option(FunctionSignature) + lit<"{"> +
            option(assign<&ast::InlineFunctionExpr::body>(ref<ExprRule>)) + lit<"}">)> {
  };

  struct FunctionItemExprRule
      : rule<ref<NamedFunctionRefRule> | ref<InlineFunctionExprRule>> {};

  struct StringTemplateExprRule
      : rule<create<ast::StringTemplateExpr>(
            assign<&ast::StringTemplateExpr::raw>(ref<StringTemplate>))> {};
  struct StringConstructorExprRule
      : rule<create<ast::StringConstructorExpr>(
            assign<&ast::StringConstructorExpr::raw>(ref<StringConstructor>))> {};

  struct UnaryLookupRule : rule<ref<LookupRule>> {};

  struct PrimaryExprRule
      : rule<ref<ArrayConstructorRule>  //
             | ref<LiteralRule>  //
             | ref<ContextItemExprRule>  //
             | ref<FunctionCallRule>  //
             | ref<FunctionItemExprRule>  //
             | ref<MapConstructorRule>  //
             | ref<NodeConstructorRule>  //
             | ref<OrderedExprRule>  //
             | ref<ParenExprRule>  //
             | ref<StringTemplateExprRule>  //
             | ref<StringConstructorExprRule>  //
             | ref<UnaryLookupRule>  //
             | ref<UnorderedExprRule>  //
             | ref<VarRefRule>> {};

  // ---- precedence ladder ---------------------------------------------------

  struct ValidateExprRule
      : rule<create<ast::ValidateExpr>(
            kw<"validate"> +
            option(assign<&ast::ValidateExpr::mode>(kw<"lax"> | kw<"strict">) |
                   (kw<"type"> + assign<&ast::ValidateExpr::typeName>(ref<EqNameRule>))) +
            lit<"{"> + assign<&ast::ValidateExpr::body>(ref<ExprRule>) + lit<"}">)> {};

  struct ExtensionExprRule
      : rule<create<ast::ExtensionExpr>(
            some(append<&ast::ExtensionExpr::pragmas>(ref<Pragma>)) + lit<"{"> +
            option(assign<&ast::ExtensionExpr::body>(ref<ExprRule>)) + lit<"}">)> {};

  struct StepExprRule : rule<ref<PostfixExprRule> | ref<AxisStepRule>> {};

  struct RelativePathExprRule
      : infix<StepExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<OpDescendantSlash> | ref<OpChildSlash>)> {};

  struct AbsolutePathExprRule
      : rule<create<ast::AbsolutePathExpr>(
                 flag<&ast::AbsolutePathExpr::doubleSlash>(lit<"//">) +
                 assign<&ast::AbsolutePathExpr::rest>(ref<RelativePathExprRule>))  //
             | create<ast::AbsolutePathExpr>(
                   lit<"/"> + option(assign<&ast::AbsolutePathExpr::rest>(
                                  ref<RelativePathExprRule>)))> {};

  struct PathExprRule
      : rule<ref<AbsolutePathExprRule> | ref<RelativePathExprRule>> {};

  struct SimpleMapExprRule
      : infix<PathExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpSimpleMap>)> {};

  struct ValueExprRule
      : rule<ref<ValidateExprRule> | ref<ExtensionExprRule> | ref<SimpleMapExprRule>> {};

  struct SignRun : terminal<some(cr<"+-">)> {};

  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(assign<&ast::UnaryExpr::signs>(ref<SignRun>) +
                                    assign<&ast::UnaryExpr::operand>(ref<ValueExprRule>))  //
             | ref<ValueExprRule>> {};

  struct ArrowTargetRule : rule<ref<FunctionCallRule> | ref<RestrictedDynamicCallRule>> {};

  struct ArrowStepRule
      : rule<create<ast::ArrowStep>(lit<"=>"> +
                                    assign<&ast::ArrowStep::target>(ref<ArrowTargetRule>))  //
             | create<ast::ArrowStep>(
                   flag<&ast::ArrowStep::mapping>(lit<"=!>">) +
                   assign<&ast::ArrowStep::target>(ref<ArrowTargetRule>))> {};

  // No arrow step: adopt UnaryExprRule's own node, not an empty ArrowExpr.
  struct ArrowExprRule
      : rule<create<ast::ArrowExpr>(
                 assign<&ast::ArrowExpr::base>(ref<UnaryExprRule>) +
                 some(append<&ast::ArrowExpr::steps>(ref<ArrowStepRule>)))  //
             | ref<UnaryExprRule>> {};

  struct PipelineExprRule
      : infix<ArrowExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpPipeline>)> {};

  struct CastExprRule
      : rule<create<ast::CastExpr>(
                 assign<&ast::CastExpr::operand>(ref<PipelineExprRule>) + kw<"cast"> +
                 kw<"as"> + assign<&ast::CastExpr::target>(ref<CastTargetRule>) +
                 option(assign<&ast::CastExpr::occurrence>(Occurrence)))  //
             | ref<PipelineExprRule>> {};

  struct CastableExprRule
      : rule<create<ast::CastableExpr>(
                 assign<&ast::CastableExpr::operand>(ref<CastExprRule>) + kw<"castable"> +
                 kw<"as"> + assign<&ast::CastableExpr::target>(ref<CastTargetRule>) +
                 option(assign<&ast::CastableExpr::occurrence>(Occurrence)))  //
             | ref<CastExprRule>> {};

  struct TreatExprRule
      : rule<create<ast::TreatExpr>(
                 assign<&ast::TreatExpr::operand>(ref<CastableExprRule>) + kw<"treat"> +
                 kw<"as"> + assign<&ast::TreatExpr::type>(ref<SequenceTypeRule>))  //
             | ref<CastableExprRule>> {};

  struct InstanceOfExprRule
      : rule<create<ast::InstanceOfExpr>(
                 assign<&ast::InstanceOfExpr::operand>(ref<TreatExprRule>) +
                 kw<"instance"> + kw<"of"> +
                 assign<&ast::InstanceOfExpr::type>(ref<SequenceTypeRule>))  //
             | ref<TreatExprRule>> {};

  struct RecordPutExprRule
      : infix<InstanceOfExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpRecordPut>)> {};

  struct IntersectExceptExprRule
      : infix<RecordPutExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpIntersect> | ref<OpExcept>)> {};

  struct UnionExprRule
      : infix<IntersectExceptExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpUnion> | ref<OpUnionAlt>)> {};

  struct MultiplicativeExprRule
      : infix<UnionExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<OpMul> | ref<OpMulAlt> | ref<OpDiv> | ref<OpDivAlt> | ref<OpIdiv> |
                   ref<OpMod>)> {};

  struct AdditiveExprRule
      : infix<MultiplicativeExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpAdd> | ref<OpSub>)> {};

  struct RangeExprRule
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<AdditiveExprRule>) +
                 assign<&ast::BinaryExpr::op>(ref<OpTo>) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveExprRule>))  //
             | ref<AdditiveExprRule>> {};

  struct StringConcatExprRule
      : infix<RangeExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpConcat>)> {};

  struct OtherwiseExprRule
      : infix<StringConcatExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpOtherwise>)> {};

  struct ComparisonExprRule
      : rule<create<ast::BinaryExpr>(
                 assign<&ast::BinaryExpr::left>(ref<OtherwiseExprRule>) +
                 assign<&ast::BinaryExpr::op>(ValueComp | GeneralComp | NodeComp) +
                 assign<&ast::BinaryExpr::right>(ref<OtherwiseExprRule>))  //
             | ref<OtherwiseExprRule>> {};

  struct AndExprRule
      : infix<ComparisonExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpAnd>)> {};

  struct OrExprRule
      : infix<AndExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpOr>)> {};

  // ---- FLWOR ---------------------------------------------------------------

  struct VarNameAndTypeRule
      : rule<create<ast::Param>(lit<"$"> + assign<&ast::Param::name>(ref<EqNameRule>) +
                                option(kw<"as"> +
                                       assign<&ast::Param::type>(ref<SequenceTypeRule>)))> {
  };

  struct ForItemBindingRule
      : rule<create<ast::ForBinding>(
            assign<&ast::ForBinding::name>(ref<VarNameRule>) +
            option(kw<"as"> + assign<&ast::ForBinding::type>(ref<SequenceTypeRule>)) +
            option(flag<&ast::ForBinding::allowingEmpty>(kw<"allowing">) +
                   kw<"empty">) +
            option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarNameRule>)) +
            kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingleRule>))> {};

  struct ForMemberBindingRule
      : rule<create<ast::ForBinding>(
            flag<&ast::ForBinding::isMember>(kw<"member">) +
            assign<&ast::ForBinding::name>(ref<VarNameRule>) +
            option(kw<"as"> + assign<&ast::ForBinding::type>(ref<SequenceTypeRule>)) +
            option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarNameRule>)) +
            kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingleRule>))> {};

  struct ForEntryBindingRule
      : rule<create<ast::ForBinding>(
                 flag<&ast::ForBinding::isEntry>(kw<"key">) +
                 assign<&ast::ForBinding::keyName>(ref<VarNameRule>) +
                 option(kw<"as"> +
                        assign<&ast::ForBinding::keyType>(ref<SequenceTypeRule>)) +
                 option(kw<"value"> + assign<&ast::ForBinding::name>(ref<VarNameRule>) +
                        option(kw<"as"> + assign<&ast::ForBinding::type>(
                                              ref<SequenceTypeRule>))) +
                 option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarNameRule>)) +
                 kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingleRule>))  //
             | create<ast::ForBinding>(
                   flag<&ast::ForBinding::isEntry>(kw<"value">) +
                   assign<&ast::ForBinding::name>(ref<VarNameRule>) +
                   option(kw<"as"> +
                          assign<&ast::ForBinding::type>(ref<SequenceTypeRule>)) +
                   option(kw<"at"> + assign<&ast::ForBinding::posVar>(ref<VarNameRule>)) +
                   kw<"in"> + assign<&ast::ForBinding::source>(ref<ExprSingleRule>))> {
  };

  struct ForBindingRule
      : rule<ref<ForItemBindingRule> | ref<ForMemberBindingRule> |
             ref<ForEntryBindingRule>> {};

  struct ForClauseRule
      : rule<create<ast::ForClause>(
            kw<"for"> + append<&ast::ForClause::bindings>(ref<ForBindingRule>) +
            many(lit<","> + append<&ast::ForClause::bindings>(ref<ForBindingRule>)))> {
  };

  struct LetValueBindingRule
      : rule<create<ast::LetBinding>(
            assign<&ast::LetBinding::name>(ref<VarNameRule>) +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceTypeRule>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingleRule>))> {};

  struct LetSequenceBindingRule
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"(">) +
            append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>) +
            many(lit<","> + append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>)) +
            lit<")"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceTypeRule>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingleRule>))> {};

  struct LetArrayBindingRule
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"[">) +
            append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>) +
            many(lit<","> + append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>)) +
            lit<"]"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceTypeRule>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingleRule>))> {};

  struct LetMapBindingRule
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::destructuring>(lit<"{">) +
            append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>) +
            many(lit<","> + append<&ast::LetBinding::names>(ref<VarNameAndTypeRule>)) +
            lit<"}"> +
            option(kw<"as"> + assign<&ast::LetBinding::type>(ref<SequenceTypeRule>)) +
            lit<":="> + assign<&ast::LetBinding::value>(ref<ExprSingleRule>))> {};

  struct LetBindingRule
      : rule<ref<LetValueBindingRule> | ref<LetSequenceBindingRule> |
             ref<LetArrayBindingRule> | ref<LetMapBindingRule>> {};

  struct LetClauseRule
      : rule<create<ast::LetClause>(
            kw<"let"> + append<&ast::LetClause::bindings>(ref<LetBindingRule>) +
            many(lit<","> + append<&ast::LetClause::bindings>(ref<LetBindingRule>)))> {
  };

  struct WindowVarSpecRule
      : rule<create<ast::WindowVarSpec>(
            option(assign<&ast::WindowVarSpec::current>(ref<VarNameRule>)) +
            option(kw<"at"> + assign<&ast::WindowVarSpec::posVar>(ref<VarNameRule>)) +
            option(kw<"previous"> +
                   assign<&ast::WindowVarSpec::previous>(ref<VarNameRule>)) +
            option(kw<"next"> + assign<&ast::WindowVarSpec::next>(ref<VarNameRule>)))> {
  };

  struct WindowStartConditionRule
      : rule<create<ast::WindowCondition>(
            kw<"start"> + assign<&ast::WindowCondition::vars>(ref<WindowVarSpecRule>) +
            option(kw<"when"> +
                   assign<&ast::WindowCondition::when>(ref<ExprSingleRule>)))> {};

  struct WindowEndConditionRule
      : rule<create<ast::WindowCondition>(
            option(flag<&ast::WindowCondition::only>(kw<"only">)) + kw<"end"> +
            assign<&ast::WindowCondition::vars>(ref<WindowVarSpecRule>) +
            option(kw<"when"> +
                   assign<&ast::WindowCondition::when>(ref<ExprSingleRule>)))> {};

  struct TumblingWindowClauseRule
      : rule<create<ast::WindowClause>(
            kw<"tumbling"> + kw<"window"> +
            assign<&ast::WindowClause::name>(ref<VarNameRule>) +
            option(kw<"as"> + assign<&ast::WindowClause::type>(ref<SequenceTypeRule>)) +
            kw<"in"> + assign<&ast::WindowClause::source>(ref<ExprSingleRule>) +
            option(assign<&ast::WindowClause::start>(ref<WindowStartConditionRule>)) +
            option(assign<&ast::WindowClause::end>(ref<WindowEndConditionRule>)))> {};

  struct SlidingWindowClauseRule
      : rule<create<ast::WindowClause>(
            flag<&ast::WindowClause::sliding>(kw<"sliding">) + kw<"window"> +
            assign<&ast::WindowClause::name>(ref<VarNameRule>) +
            option(kw<"as"> + assign<&ast::WindowClause::type>(ref<SequenceTypeRule>)) +
            kw<"in"> + assign<&ast::WindowClause::source>(ref<ExprSingleRule>) +
            option(assign<&ast::WindowClause::start>(ref<WindowStartConditionRule>)) +
            assign<&ast::WindowClause::end>(ref<WindowEndConditionRule>))> {};

  struct WindowClauseRule
      : rule<kw<"for"> +
             (ref<TumblingWindowClauseRule> | ref<SlidingWindowClauseRule>)> {};

  struct WhereClauseRule
      : rule<create<ast::WhereClause>(kw<"where"> +
                                      assign<&ast::WhereClause::cond>(ref<ExprSingleRule>))> {
  };
  struct WhileClauseRule
      : rule<create<ast::WhileClause>(
            kw<"while"> + assign<&ast::WhileClause::cond>(ref<ExprSingleRule>))> {};
  struct TraceClauseRule
      : rule<create<ast::TraceClause>(
            kw<"trace"> + assign<&ast::TraceClause::value>(ref<ExprSingleRule>))> {};
  struct CountClauseRule
      : rule<create<ast::CountClause>(
            kw<"count"> + assign<&ast::CountClause::name>(ref<VarNameRule>))> {};

  struct GroupingSpecRule
      : rule<create<ast::GroupingSpec>(
            assign<&ast::GroupingSpec::name>(ref<VarNameRule>) +
            option(option(kw<"as"> +
                          assign<&ast::GroupingSpec::type>(ref<SequenceTypeRule>)) +
                   lit<":="> + assign<&ast::GroupingSpec::value>(ref<ExprSingleRule>)) +
            option(kw<"collation"> +
                   assign<&ast::GroupingSpec::collation>(ref<STRING>)))> {};

  struct GroupByClauseRule
      : rule<create<ast::GroupByClause>(
            kw<"group"> + kw<"by"> +
            append<&ast::GroupByClause::specs>(ref<GroupingSpecRule>) +
            many(lit<","> + append<&ast::GroupByClause::specs>(ref<GroupingSpecRule>)))> {
  };

  struct OrderSpecRule
      : rule<create<ast::OrderSpec>(
            assign<&ast::OrderSpec::value>(ref<ExprSingleRule>) +
            option(assign<&ast::OrderSpec::direction>(kw<"ascending"> | kw<"descending">)) +
            option(kw<"empty"> +
                   (flag<&ast::OrderSpec::emptyGreatest>(kw<"greatest">)  //
                    | flag<&ast::OrderSpec::emptyLeast>(kw<"least">))) +
            option(kw<"collation"> + assign<&ast::OrderSpec::collation>(ref<STRING>)))> {
  };

  struct OrderByClauseRule
      : rule<create<ast::OrderByClause>(
            option(flag<&ast::OrderByClause::stable>(kw<"stable">)) + kw<"order"> +
            kw<"by"> + append<&ast::OrderByClause::specs>(ref<OrderSpecRule>) +
            many(lit<","> + append<&ast::OrderByClause::specs>(ref<OrderSpecRule>)))> {};

  static constexpr auto InitialClause =
      append<&ast::FlworExpr::clauses>(ref<ForClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<LetClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<WindowClauseRule>);
  static constexpr auto IntermediateClause =
      InitialClause  //
      | append<&ast::FlworExpr::clauses>(ref<WhereClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<GroupByClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<OrderByClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<CountClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<WhileClauseRule>)  //
      | append<&ast::FlworExpr::clauses>(ref<TraceClauseRule>);

  struct FlworExprRule
      : rule<create<ast::FlworExpr>(
            InitialClause + many(IntermediateClause) + kw<"return"> +
            assign<&ast::FlworExpr::result>(ref<ExprSingleRule>))> {};

  // ---- quantified / switch / typeswitch / if / try-catch -------------------

  struct QuantifierBindingRule
      : rule<create<ast::QuantifierBinding>(
            assign<&ast::QuantifierBinding::name>(ref<VarNameRule>) +
            option(kw<"as"> +
                   assign<&ast::QuantifierBinding::type>(ref<SequenceTypeRule>)) +
            kw<"in"> + assign<&ast::QuantifierBinding::source>(ref<ExprSingleRule>))> {
  };

  struct QuantifiedExprRule
      : rule<create<ast::QuantifiedExpr>(
            (kw<"some"> | flag<&ast::QuantifiedExpr::every>(kw<"every">)) +
            append<&ast::QuantifiedExpr::bindings>(ref<QuantifierBindingRule>) +
            many(lit<","> +
                 append<&ast::QuantifiedExpr::bindings>(ref<QuantifierBindingRule>)) +
            kw<"satisfies"> +
            assign<&ast::QuantifiedExpr::predicate>(ref<ExprSingleRule>))> {};

  struct SwitchCaseClauseRule
      : rule<create<ast::SwitchCaseClause>(
            some(kw<"case"> +
                 append<&ast::SwitchCaseClause::operands>(ref<ExprRule>)) +
            kw<"return"> +
            assign<&ast::SwitchCaseClause::result>(ref<ExprSingleRule>))> {};

  static constexpr auto SwitchCasesBody =
      some(append<&ast::SwitchExpr::cases>(ref<SwitchCaseClauseRule>)) + kw<"default"> +
      kw<"return"> + assign<&ast::SwitchExpr::defaultResult>(ref<ExprSingleRule>);

  struct SwitchExprRule
      : rule<create<ast::SwitchExpr>(
            kw<"switch"> + lit<"("> +
            option(assign<&ast::SwitchExpr::comparand>(ref<ExprRule>)) + lit<")"> +
            (SwitchCasesBody | (lit<"{"> + SwitchCasesBody + lit<"}">)))> {};

  struct CaseClauseRule
      : rule<create<ast::TypeswitchCase>(
            kw<"case"> +
            option(assign<&ast::TypeswitchCase::varName>(ref<VarNameRule>) + kw<"as">) +
            SequenceTypeUnion + kw<"return"> +
            assign<&ast::TypeswitchCase::result>(ref<ExprSingleRule>))> {};

  static constexpr auto TypeswitchCasesBody =
      some(append<&ast::TypeswitchExpr::cases>(ref<CaseClauseRule>)) + kw<"default"> +
      option(assign<&ast::TypeswitchExpr::defaultVarName>(ref<VarNameRule>)) +
      kw<"return"> + assign<&ast::TypeswitchExpr::defaultResult>(ref<ExprSingleRule>);

  struct TypeswitchExprRule
      : rule<create<ast::TypeswitchExpr>(
            kw<"typeswitch"> + lit<"("> +
            assign<&ast::TypeswitchExpr::operand>(ref<ExprRule>) + lit<")"> +
            (TypeswitchCasesBody | (lit<"{"> + TypeswitchCasesBody + lit<"}">)))> {};

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<ExprRule>) + lit<")"> +
            ((kw<"then"> + assign<&ast::IfExpr::thenBranch>(ref<ExprSingleRule>) +
              kw<"else"> + assign<&ast::IfExpr::elseBranch>(ref<ExprSingleRule>))  //
             | (lit<"{"> + option(assign<&ast::IfExpr::thenBranch>(ref<ExprRule>)) +
                lit<"}">)))> {};

  struct CatchClauseRule
      : rule<create<ast::CatchClause>(
            kw<"catch"> + append<&ast::CatchClause::tests>(NameTestWord) +
            many(lit<"|"> + append<&ast::CatchClause::tests>(NameTestWord)) + lit<"{"> +
            option(assign<&ast::CatchClause::body>(ref<ExprRule>)) + lit<"}">)> {};

  struct TryCatchExprRule
      : rule<create<ast::TryCatchExpr>(
            kw<"try"> + lit<"{"> +
            option(assign<&ast::TryCatchExpr::tryBody>(ref<ExprRule>)) + lit<"}"> +
            ((some(append<&ast::TryCatchExpr::catches>(ref<CatchClauseRule>)) +
              option(kw<"finally"> + lit<"{"> +
                     option(assign<&ast::TryCatchExpr::finallyBody>(ref<ExprRule>)) +
                     lit<"}">))  //
             | (kw<"finally"> + lit<"{"> +
                option(assign<&ast::TryCatchExpr::finallyBody>(ref<ExprRule>)) +
                lit<"}">)))> {};

  // ---- Update Facility 1.0 --------------------------------------------------

  struct InsertExprRule
      : rule<create<ast::InsertExpr>(
            kw<"insert"> + (kw<"node"> | flag<&ast::InsertExpr::nodes>(kw<"nodes">)) +
            assign<&ast::InsertExpr::source>(ref<ExprSingleRule>) +
            ((option(kw<"as"> +
                     assign<&ast::InsertExpr::position>(kw<"first"> | kw<"last">)) +
              assign<&ast::InsertExpr::target>(kw<"into">))  //
             | assign<&ast::InsertExpr::target>(kw<"after">)  //
             | assign<&ast::InsertExpr::target>(kw<"before">)) +
            assign<&ast::InsertExpr::targetExpr>(ref<ExprSingleRule>))> {};

  struct DeleteExprRule
      : rule<create<ast::DeleteExpr>(
            kw<"delete"> + (kw<"node"> | flag<&ast::DeleteExpr::nodes>(kw<"nodes">)) +
            assign<&ast::DeleteExpr::target>(ref<ExprSingleRule>))> {};

  struct ReplaceExprRule
      : rule<create<ast::ReplaceExpr>(
            kw<"replace"> +
            option(flag<&ast::ReplaceExpr::valueOf>(kw<"value">) + kw<"of">) +
            kw<"node"> + assign<&ast::ReplaceExpr::target>(ref<ExprSingleRule>) +
            kw<"with"> + assign<&ast::ReplaceExpr::with>(ref<ExprSingleRule>))> {};

  struct RenameExprRule
      : rule<create<ast::RenameExpr>(
            kw<"rename"> + kw<"node"> +
            assign<&ast::RenameExpr::target>(ref<ExprSingleRule>) + kw<"as"> +
            assign<&ast::RenameExpr::newName>(ref<ExprSingleRule>))> {};

  struct CopyBindingRule
      : rule<create<ast::CopyBinding>(
            lit<"$"> + assign<&ast::CopyBinding::name>(ref<EqNameRule>) + lit<":="> +
            assign<&ast::CopyBinding::value>(ref<ExprSingleRule>))> {};

  struct TransformExprRule
      : rule<create<ast::TransformExpr>(
            kw<"copy"> + append<&ast::TransformExpr::bindings>(ref<CopyBindingRule>) +
            many(lit<","> + append<&ast::TransformExpr::bindings>(ref<CopyBindingRule>)) +
            kw<"modify"> + assign<&ast::TransformExpr::modify>(ref<ExprSingleRule>) +
            kw<"return"> + assign<&ast::TransformExpr::result>(ref<ExprSingleRule>))> {};

  // ---- exprSingle / expr ----------------------------------------------------

  struct ExprSingleRule
      : rule<ref<FlworExprRule> | ref<QuantifiedExprRule> | ref<SwitchExprRule> |
             ref<TypeswitchExprRule> | ref<IfExprRule> | ref<TryCatchExprRule> |
             ref<InsertExprRule> | ref<DeleteExprRule> | ref<RenameExprRule> |
             ref<ReplaceExprRule> | ref<TransformExprRule> | ref<OrExprRule>> {};

  struct ExprRule
      : rule<create<ast::SequenceExpr>(
            append<&ast::SequenceExpr::items>(ref<ExprSingleRule>) +
            many(lit<","> + append<&ast::SequenceExpr::items>(ref<ExprSingleRule>)))> {
  };

  // ---- prolog / declarations -------------------------------------------

  struct SetterRule
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

  struct DfPropertyNameRule
      : rule<kw<"decimal-separator"> | kw<"grouping-separator"> | kw<"infinity"> |
             kw<"minus-sign"> | kw<"NaN"> | kw<"percent"> | kw<"per-mille"> |
             kw<"zero-digit"> | kw<"digit"> | kw<"pattern-separator"> |
             kw<"exponent-separator">> {};

  struct DecimalFormatPropertyRule
      : rule<create<ast::DecimalFormatProperty>(
            assign<&ast::DecimalFormatProperty::name>(ref<DfPropertyNameRule>) + lit<"="> +
            assign<&ast::DecimalFormatProperty::value>(ref<STRING>))> {};

  struct DecimalFormatDeclRule
      : rule<create<ast::DecimalFormatDecl>(
                 kw<"declare"> + kw<"decimal-format"> +
                 assign<&ast::DecimalFormatDecl::name>(ref<EqNameRule>) +
                 many(append<&ast::DecimalFormatDecl::properties>(
                     ref<DecimalFormatPropertyRule>)))  //
             | create<ast::DecimalFormatDecl>(
                   kw<"declare"> +
                   flag<&ast::DecimalFormatDecl::isDefault>(kw<"default">) +
                   kw<"decimal-format"> +
                   many(append<&ast::DecimalFormatDecl::properties>(
                       ref<DecimalFormatPropertyRule>)))> {};

  struct NamespaceDeclRule
      : rule<create<ast::NamespaceDecl>(
            kw<"declare"> + kw<"namespace"> +
            assign<&ast::NamespaceDecl::prefix>(ref<QName>) + lit<"="> +
            assign<&ast::NamespaceDecl::uri>(ref<STRING>))> {};

  struct DefaultNamespaceDeclRule
      : rule<create<ast::DefaultNamespaceDecl>(
            kw<"declare"> +
            option(flag<&ast::DefaultNamespaceDecl::fixed>(kw<"fixed">)) +
            kw<"default"> +
            (kw<"element"> |
             flag<&ast::DefaultNamespaceDecl::isFunction>(kw<"function">)) +
            kw<"namespace"> + assign<&ast::DefaultNamespaceDecl::uri>(ref<STRING>))> {};

  struct SchemaImportRule
      : rule<create<ast::SchemaImport>(
            kw<"import"> + kw<"schema"> +
            option((kw<"namespace"> + assign<&ast::SchemaImport::prefix>(ref<QName>) +
                    lit<"=">)  //
                   | (option(flag<&ast::SchemaImport::fixed>(kw<"fixed">)) +
                      flag<&ast::SchemaImport::isDefaultElement>(kw<"default">) +
                      kw<"element"> + kw<"namespace">)) +
            assign<&ast::SchemaImport::uri>(ref<STRING>) +
            option(kw<"at"> + append<&ast::SchemaImport::locations>(ref<STRING>) +
                   many(lit<","> +
                        append<&ast::SchemaImport::locations>(ref<STRING>))))> {};

  struct ModuleImportRule
      : rule<create<ast::ModuleImport>(
            kw<"import"> + kw<"module"> +
            option(kw<"namespace"> + assign<&ast::ModuleImport::prefix>(ref<QName>) +
                   lit<"=">) +
            assign<&ast::ModuleImport::uri>(ref<STRING>) +
            option(kw<"at"> + append<&ast::ModuleImport::locations>(ref<STRING>) +
                   many(lit<","> +
                        append<&ast::ModuleImport::locations>(ref<STRING>))))> {};

  struct ContextValueDeclRule
      : rule<create<ast::ContextValueDecl>(
            kw<"declare"> + kw<"context"> +
            ((kw<"value"> +
              option(kw<"as"> +
                     assign<&ast::ContextValueDecl::type>(ref<SequenceTypeRule>)))  //
             | (flag<&ast::ContextValueDecl::isItem>(kw<"item">) +
                option(kw<"as"> +
                       assign<&ast::ContextValueDecl::type>(create<ast::SequenceType>(
                           assign<&ast::SequenceType::itemType>(ref<ItemTypeRule>)))))) +
            ((lit<":="> + assign<&ast::ContextValueDecl::value>(ref<ExprSingleRule>))  //
             | (flag<&ast::ContextValueDecl::external>(kw<"external">) +
                option(lit<":="> +
                       assign<&ast::ContextValueDecl::value>(ref<ExprSingleRule>)))))> {
  };

  struct VarDeclRule
      : rule<create<ast::VarDecl>(
            kw<"declare"> + many(append<&ast::VarDecl::annotations>(ref<AnnotationRule>)) +
            kw<"variable"> + lit<"$"> + assign<&ast::VarDecl::name>(ref<EqNameRule>) +
            option(kw<"as"> + assign<&ast::VarDecl::type>(ref<SequenceTypeRule>)) +
            ((lit<":="> + assign<&ast::VarDecl::value>(ref<ExprSingleRule>))  //
             | (flag<&ast::VarDecl::external>(kw<"external">) +
                option(lit<":="> + assign<&ast::VarDecl::value>(ref<ExprSingleRule>)))))> {
  };

  struct ParamWithDefaultRule
      : rule<create<ast::Param>(
            lit<"$"> + assign<&ast::Param::name>(ref<EqNameRule>) +
            option(kw<"as"> + assign<&ast::Param::type>(ref<SequenceTypeRule>)) +
            option(lit<":="> + assign<&ast::Param::def>(ref<ExprSingleRule>)))> {};

  struct FunctionDeclRule
      : rule<create<ast::FunctionDecl>(
            kw<"declare"> +
            many(append<&ast::FunctionDecl::annotations>(ref<AnnotationRule>)) +
            kw<"function"> + assign<&ast::FunctionDecl::name>(ref<EqNameRule>) + lit<"("> +
            option(append<&ast::FunctionDecl::params>(ref<ParamWithDefaultRule>) +
                   many(lit<","> + append<&ast::FunctionDecl::params>(
                                       ref<ParamWithDefaultRule>))) +
            lit<")"> +
            option(kw<"as"> + assign<&ast::FunctionDecl::returnType>(ref<SequenceTypeRule>)) +
            ((lit<"{"> + option(assign<&ast::FunctionDecl::body>(ref<ExprRule>)) + lit<"}">)  //
             | flag<&ast::FunctionDecl::external>(kw<"external">)))> {};

  struct ItemTypeDeclRule
      : rule<create<ast::ItemTypeDecl>(
            kw<"declare"> +
            many(append<&ast::ItemTypeDecl::annotations>(ref<AnnotationRule>)) + kw<"type"> +
            assign<&ast::ItemTypeDecl::name>(ref<EqNameRule>) + kw<"as"> +
            assign<&ast::ItemTypeDecl::type>(ref<ItemTypeRule>))> {};

  struct NamedRecordTypeDeclRule
      : rule<create<ast::NamedRecordTypeDecl>(
            kw<"declare"> +
            many(append<&ast::NamedRecordTypeDecl::annotations>(ref<AnnotationRule>)) +
            kw<"record"> + assign<&ast::NamedRecordTypeDecl::name>(ref<EqNameRule>) +
            lit<"("> +
            option(append<&ast::NamedRecordTypeDecl::fields>(ref<ExtendedFieldDeclRule>) +
                   many(lit<","> + append<&ast::NamedRecordTypeDecl::fields>(
                                       ref<ExtendedFieldDeclRule>))) +
            lit<")">)> {};

  struct OptionDeclRule
      : rule<create<ast::OptionDecl>(kw<"declare"> + kw<"option"> +
                                     assign<&ast::OptionDecl::name>(ref<EqNameRule>) +
                                     assign<&ast::OptionDecl::value>(ref<STRING>))> {};

  static constexpr auto PrologFirstItem =
      append<&ast::Prolog::items>(ref<DefaultNamespaceDeclRule>)  //
      | append<&ast::Prolog::items>(ref<SetterRule>)  //
      | append<&ast::Prolog::items>(ref<DecimalFormatDeclRule>)  //
      | append<&ast::Prolog::items>(ref<NamespaceDeclRule>)  //
      | append<&ast::Prolog::items>(ref<SchemaImportRule>)  //
      | append<&ast::Prolog::items>(ref<ModuleImportRule>);
  static constexpr auto PrologSecondItem =
      append<&ast::Prolog::items>(ref<ContextValueDeclRule>)  //
      | append<&ast::Prolog::items>(ref<VarDeclRule>)  //
      | append<&ast::Prolog::items>(ref<FunctionDeclRule>)  //
      | append<&ast::Prolog::items>(ref<ItemTypeDeclRule>)  //
      | append<&ast::Prolog::items>(ref<NamedRecordTypeDeclRule>)  //
      | append<&ast::Prolog::items>(ref<OptionDeclRule>);

  struct PrologRule
      : rule<create<ast::Prolog>(many(PrologFirstItem + lit<";">) +
                                 many(PrologSecondItem + lit<";">))> {};

  // ---- module / query list --------------------------------------------

  struct LibraryModuleRule
      : rule<create<ast::LibraryModule>(
            kw<"module"> + kw<"namespace"> +
            assign<&ast::LibraryModule::prefix>(ref<QName>) + lit<"="> +
            assign<&ast::LibraryModule::uri>(ref<STRING>) + lit<";"> +
            assign<&ast::LibraryModule::prolog>(ref<PrologRule>))> {};

  struct MainModuleRule
      : rule<create<ast::MainModule>(
            assign<&ast::MainModule::prolog>(ref<PrologRule>) +
            assign<&ast::MainModule::body>(ref<ExprRule>))> {};

  struct ModuleRule
      : rule<create<ast::Module>(
            option(kw<"xquery"> +
                   ((kw<"encoding"> + assign<&ast::Module::version>(ref<STRING>))  //
                    | (kw<"version"> + assign<&ast::Module::version>(ref<STRING>) +
                       option(kw<"encoding"> + ref<STRING>))) +
                   lit<";">) +
            (assign<&ast::Module::library>(ref<LibraryModuleRule>)  //
             | assign<&ast::Module::main>(ref<MainModuleRule>)))> {};

  struct QueryListRule
      : rule<create<ast::QueryList>(
            append<&ast::QueryList::modules>(ref<ModuleRule>) +
            many((lit<";"> | lit<"%%%">) +
                 option(append<&ast::QueryList::modules>(ref<ModuleRule>))))> {};

  using entry = QueryListRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace xquery
