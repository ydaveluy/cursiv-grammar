// The xpath language: its grammar. XPath 3.1 is one expression grammar --
// no modules, no prolog, just `expr`. A document here is one or more
// `expr;`.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "xpath/core/ast.hpp"

namespace xpath {

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

struct Xpath : cursiv::GrammarSpec {
  // Xpath's NCName is XML's Name production without the colon, which QName
  // spells separately.
  static constexpr auto NameStartChar = cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-󯿿">;
  static constexpr auto NameContChar = cr<"A-Z_a-zÀ-ÖØ-öø-˿Ͱ-ͽͿ-῿‌-‍⁰-↏Ⰰ-⿯、-퟿豈-﷏ﷰ-�𐀀-󯿿0-9.·̀-ͯ‿-⁀-">;

  // A name's own continuation class, not `w`: `following-sibling` must lex
  // as one name, so a keyword guarded by the default `w` boundary would
  // wrongly claim its hyphenated prefix.
  static constexpr auto word_boundary = NameContChar;

  struct WS : terminal<some(s)> {};
  struct COMMENT
      : terminal<lit<"(:"> +
                 many(ref<COMMENT> | (!lit<"(:"> + !lit<":)"> + any)) +
                 lit<":)">> {};

  struct NCName : terminal<NameStartChar + many(NameContChar)> {};
  struct QName : terminal<NameStartChar + many(NameContChar) +
                          option(lit<":"> + NameStartChar + many(NameContChar))> {};
  static constexpr auto BracedURI =
      lit<"Q"> + lit<"{"> + many(cr<"^{}">) + lit<"}">;
  struct BracedURILiteral : terminal<BracedURI> {};
  struct URIQualifiedName
      : terminal<BracedURI + NameStartChar + many(NameContChar) +
                option(lit<":"> + NameStartChar + many(NameContChar))> {};

  struct IntegerLiteral : terminal<some(d)> {};
  struct UINT_TEXT : terminal<some(d)> {};
  static constexpr auto DecimalBody =
      (lit<"."> + some(d)) | (some(d) + lit<"."> + many(d));
  struct DecimalLiteral : terminal<DecimalBody> {};
  struct DoubleLiteral
      : terminal<((lit<"."> + some(d)) |
                 (some(d) + option(lit<"."> + many(d)))) +
                     cr<"eE"> + option(cr<"+-">) + some(d)> {};

  static constexpr auto dq_body = lit<"\"\""> | cr<R"(^")">;
  static constexpr auto sq_body = lit<"''"> | cr<"^'">;
  struct STRING
      : terminal<(lit<"\""> + many(dq_body) + lit<"\"">) |
                     (lit<"'"> + many(sq_body) + lit<"'">),
                 UnquoteStr> {};

  // The recursive core: expressions and their arguments reach path steps,
  // which reach postfix expressions, which reach primary expressions, which
  // reach expressions again through parentheses, calls and constructors.
  struct ExprRule;
  struct ExprSingleRule;
  struct ParenExprRule;

  // ---------------------------------------------------------------- eqname

  // EQName also admits every keyword below: the spec's own grammar omits
  // them, but the spec text (and every implementation) reads them as names.
  struct EqName
      : rule<ref<QName> | ref<URIQualifiedName> |
             kw<"ancestor"> | kw<"ancestor-or-self"> | kw<"and"> | kw<"array"> |
             kw<"as"> | kw<"attribute"> | kw<"cast"> | kw<"castable"> |
             kw<"child"> | kw<"comment"> | kw<"descendant"> |
             kw<"descendant-or-self"> | kw<"div"> | kw<"document-node"> |
             kw<"element"> | kw<"else"> | kw<"empty-sequence"> | kw<"eq"> |
             kw<"every"> | kw<"except"> | kw<"following"> |
             kw<"following-sibling"> | kw<"for"> | kw<"function"> | kw<"ge"> |
             kw<"gt"> | kw<"idiv"> | kw<"if"> | kw<"in"> | kw<"instance"> |
             kw<"intersect"> | kw<"is"> | kw<"item"> | kw<"le"> | kw<"let"> |
             kw<"lt"> | kw<"map"> | kw<"mod"> | kw<"namespace"> |
             kw<"namespace-node"> | kw<"ne"> | kw<"node"> | kw<"of"> | kw<"or"> |
             kw<"parent"> | kw<"preceding"> | kw<"preceding-sibling"> |
             kw<"processing-instruction"> | kw<"return"> | kw<"satisfies"> |
             kw<"schema-attribute"> | kw<"schema-element"> | kw<"self"> |
             kw<"some"> | kw<"text"> | kw<"then"> | kw<"treat"> |
             kw<"union">> {};

  // ----------------------------------------------------------------- types

  struct ItemTypeRule;

  struct OccurrenceIndicator : rule<lit<"?"> | lit<"*"> | lit<"+">> {};

  struct AnyKindTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"node">) +
                                   lit<"("> + lit<")">)> {};
  struct TextTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"text">) +
                                   lit<"("> + lit<")">)> {};
  struct CommentTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"comment">) + lit<"("> + lit<")">)> {
  };
  struct NamespaceNodeTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"namespace-node">) + lit<"("> +
            lit<")">)> {};
  struct PITest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"processing-instruction">) +
            lit<"("> +
            option(append<&ast::ItemType::names>(ref<NCName> | ref<STRING>)) +
            lit<")">)> {};

  struct AttribNameOrWildcard : rule<ref<EqName> | lit<"*">> {};
  struct AttributeTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"attribute">) + lit<"("> +
            option(append<&ast::ItemType::names>(ref<AttribNameOrWildcard>) +
                   option(lit<","> +
                          assign<&ast::ItemType::typeName>(ref<EqName>))) +
            lit<")">)> {};
  struct SchemaAttributeTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"schema-attribute">) + lit<"("> +
            append<&ast::ItemType::names>(ref<EqName>) + lit<")">)> {};

  struct ElementNameOrWildcard : rule<ref<EqName> | lit<"*">> {};
  struct ElementTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"element">) + lit<"("> +
            option(append<&ast::ItemType::names>(ref<ElementNameOrWildcard>) +
                   option(lit<","> +
                          assign<&ast::ItemType::typeName>(ref<EqName>) +
                          option(flag<&ast::ItemType::nillable>(
                              lit<"?">)))) +
            lit<")">)> {};
  struct SchemaElementTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"schema-element">) + lit<"("> +
            append<&ast::ItemType::names>(ref<EqName>) + lit<")">)> {};

  struct DocumentTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"document-node">) + lit<"("> +
            option(assign<&ast::ItemType::inner>(
                ref<ElementTest> | ref<SchemaElementTest>)) +
            lit<")">)> {};

  struct KindTestRule
      : rule<ref<DocumentTest> | ref<ElementTest> | ref<AttributeTest> |
             ref<SchemaElementTest> | ref<SchemaAttributeTest> | ref<PITest> |
             ref<CommentTest> | ref<TextTest> | ref<NamespaceNodeTest> |
             ref<AnyKindTest>> {};

  struct AnyItemTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"item">) +
                                   lit<"("> + lit<")">)> {};
  struct AtomicOrUnionType
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(ref<EqName>))> {
  };

  struct SequenceTypeRule;

  struct AnyFunctionTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"function">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedFunctionTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"function">) + lit<"("> +
            option(append<&ast::ItemType::params>(ref<SequenceTypeRule>) +
                   many(lit<","> +
                        append<&ast::ItemType::params>(ref<SequenceTypeRule>))) +
            lit<")"> + kw<"as"> +
            assign<&ast::ItemType::returnType>(ref<SequenceTypeRule>))> {};
  struct FunctionTest : rule<ref<TypedFunctionTest> | ref<AnyFunctionTest>> {};

  struct AnyMapTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"map">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedMapTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
            assign<&ast::ItemType::typeName>(ref<EqName>) + lit<","> +
            append<&ast::ItemType::params>(ref<SequenceTypeRule>) + lit<")">)> {
  };
  struct MapTest : rule<ref<TypedMapTest> | ref<AnyMapTest>> {};

  struct AnyArrayTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"array">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedArrayTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
            append<&ast::ItemType::params>(ref<SequenceTypeRule>) + lit<")">)> {
  };
  struct ArrayTest : rule<ref<TypedArrayTest> | ref<AnyArrayTest>> {};

  struct ParenthesizedItemType
      : rule<create<ast::ItemType>(lit<"("> +
                                   assign<&ast::ItemType::inner>(
                                       ref<ItemTypeRule>) +
                                   lit<")">)> {};

  struct ItemTypeRule
      : rule<ref<KindTestRule> | ref<AnyItemTest> | ref<FunctionTest> |
             ref<MapTest> | ref<ArrayTest> | ref<AtomicOrUnionType> |
             ref<ParenthesizedItemType>> {};

  struct SequenceTypeRule
      : rule<create<ast::SequenceType>(
                 flag<&ast::SequenceType::emptySequence>(
                     kw<"empty-sequence">) +
                 lit<"("> + lit<")">) |
             create<ast::SequenceType>(
                 assign<&ast::SequenceType::itemType>(ref<ItemTypeRule>) +
                 option(assign<&ast::SequenceType::occurrence>(
                     ref<OccurrenceIndicator>)))> {};

  struct SingleTypeRule
      : rule<create<ast::SingleType>(
            assign<&ast::SingleType::name>(ref<EqName>) +
            option(flag<&ast::SingleType::optional>(lit<"?">)))> {};

  // --------------------------------------------------------- node tests

  struct NameTest
      : rule<ref<EqName> | lit<"*"> | (ref<NCName> + lit<":*">) |
             (lit<"*:"> + ref<NCName>) | (ref<BracedURILiteral> + lit<"*">)> {
  };

  struct NodeTestRule
      : rule<create<ast::NodeTest>(assign<&ast::NodeTest::kind>(
                ref<KindTestRule>)) |
             create<ast::NodeTest>(
                 assign<&ast::NodeTest::name>(ref<NameTest>))> {};

  struct ForwardAxisWord
      : rule<kw<"child"> | kw<"descendant-or-self"> | kw<"descendant"> |
             kw<"attribute"> | kw<"self"> | kw<"following-sibling"> |
             kw<"following"> | kw<"namespace">> {};
  struct ReverseAxisWord
      : rule<kw<"parent"> | kw<"ancestor-or-self"> | kw<"ancestor"> |
             kw<"preceding-sibling"> | kw<"preceding">> {};

  static constexpr auto PredicateListFrag =
      many(lit<"["> + append<&ast::AxisStep::predicates>(ref<ExprRule>) +
           lit<"]">);

  struct AxisStepRule
      : rule<create<ast::AxisStep>(
                 assign<&ast::AxisStep::axis>(ref<ForwardAxisWord>) +
                 lit<"::"> +
                 assign<&ast::AxisStep::test>(ref<NodeTestRule>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(
                 option(assign<&ast::AxisStep::axis>(lit<"@">)) +
                 assign<&ast::AxisStep::test>(ref<NodeTestRule>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(
                 assign<&ast::AxisStep::axis>(ref<ReverseAxisWord>) +
                 lit<"::"> +
                 assign<&ast::AxisStep::test>(ref<NodeTestRule>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(assign<&ast::AxisStep::axis>(lit<"..">) +
                                   PredicateListFrag)> {};

  // ------------------------------------------------------------ postfix

  struct ArgPlaceholder : rule<create<ast::ArgPlaceholder> + lit<"?">> {};
  struct ArgumentRule : rule<ref<ArgPlaceholder> | ref<ExprSingleRule>> {};
  struct ArgumentsRule
      : rule<create<ast::Arguments>(
            lit<"("> +
            option(append<&ast::Arguments::positional>(ref<ArgumentRule>) +
                   many(lit<","> +
                        append<&ast::Arguments::positional>(ref<ArgumentRule>))) +
            lit<")">)> {};

  struct KeySpecifierRule
      : rule<create<ast::Lookup>(assign<&ast::Lookup::key>(ref<NCName>)) |
             create<ast::Lookup>(assign<&ast::Lookup::key>(ref<UINT_TEXT>)) |
             create<ast::Lookup>(
                 assign<&ast::Lookup::keyExpr>(ref<ParenExprRule>)) |
             create<ast::Lookup>(
                 flag<&ast::Lookup::wildcard>(lit<"*">))> {};
  struct LookupRule : rule<lit<"?"> + ref<KeySpecifierRule>> {};

  struct VarRefRule
      : rule<create<ast::VarRef>(lit<"$"> +
                                 assign<&ast::VarRef::name>(ref<EqName>))> {};

  struct ParenExprRule
      : rule<create<ast::ParenExpr>(
            lit<"("> + option(assign<&ast::ParenExpr::body>(ref<ExprRule>)) +
            lit<")">)> {};

  struct ContextItemExprRule : rule<create<ast::ContextItemExpr> + lit<".">> {
  };

  // A name this grammar may not read as a call: elsewhere in this same step
  // position it introduces a kind test, `if`, or a constructor, and a call
  // to a function of that name would be indistinguishable from it.
  static constexpr auto ReservedFunctionName =
      kw<"array"> | kw<"attribute"> | kw<"comment"> | kw<"document-node"> |
      kw<"element"> | kw<"empty-sequence"> | kw<"function"> | kw<"if"> |
      kw<"item"> | kw<"map"> | kw<"namespace-node"> | kw<"node"> |
      kw<"processing-instruction"> | kw<"schema-attribute"> |
      kw<"schema-element"> | kw<"text">;

  struct FunctionCallRule
      : rule<create<ast::CallExpr>(
            !ReservedFunctionName + assign<&ast::CallExpr::name>(ref<EqName>) +
            assign<&ast::CallExpr::args>(ref<ArgumentsRule>))> {};

  struct NamedFunctionRefRule
      : rule<create<ast::NamedFunctionRef>(
            assign<&ast::NamedFunctionRef::name>(ref<EqName>) + lit<"#"> +
            assign<&ast::NamedFunctionRef::arity>(ref<IntegerLiteral>))> {};

  struct ParamRule
      : rule<create<ast::Param>(
            lit<"$"> + assign<&ast::Param::name>(ref<EqName>) +
            option(kw<"as"> +
                   assign<&ast::Param::type>(ref<SequenceTypeRule>)))> {};

  struct InlineFunctionExprRule
      : rule<create<ast::InlineFunctionExpr>(
            kw<"function"> + lit<"("> +
            option(append<&ast::InlineFunctionExpr::params>(ref<ParamRule>) +
                   many(lit<","> +
                        append<&ast::InlineFunctionExpr::params>(
                            ref<ParamRule>))) +
            lit<")"> +
            option(kw<"as"> +
                   assign<&ast::InlineFunctionExpr::returnType>(
                       ref<SequenceTypeRule>)) +
            lit<"{"> +
            option(assign<&ast::InlineFunctionExpr::body>(ref<ExprRule>)) +
            lit<"}">)> {};

  struct FunctionItemExprRule
      : rule<ref<NamedFunctionRefRule> | ref<InlineFunctionExprRule>> {};

  struct MapEntryRule
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ref<ExprSingleRule>) + lit<":"> +
            assign<&ast::MapEntry::value>(ref<ExprSingleRule>))> {};
  struct MapConstructorRule
      : rule<create<ast::MapConstructor>(
            kw<"map"> + lit<"{"> +
            option(append<&ast::MapConstructor::entries>(ref<MapEntryRule>) +
                   many(lit<","> +
                        append<&ast::MapConstructor::entries>(
                            ref<MapEntryRule>))) +
            lit<"}">)> {};

  struct ArrayConstructorRule
      : rule<create<ast::ArrayConstructor>(
                 lit<"["> +
                 option(append<&ast::ArrayConstructor::items>(
                            ref<ExprSingleRule>) +
                        many(lit<","> +
                             append<&ast::ArrayConstructor::items>(
                                 ref<ExprSingleRule>))) +
                 lit<"]">) |
             create<ast::ArrayConstructor>(
                 flag<&ast::ArrayConstructor::curly>(kw<"array">) +
                 lit<"{"> +
                 option(assign<&ast::ArrayConstructor::body>(ref<ExprRule>)) +
                 lit<"}">)> {};

  struct NumericLiteralRule
      : rule<create<ast::IntegerLit>(
                 assign<&ast::IntegerLit::raw>(ref<IntegerLiteral>)) |
             create<ast::DecimalLit>(
                 assign<&ast::DecimalLit::raw>(ref<DecimalLiteral>)) |
             create<ast::DoubleLit>(
                 assign<&ast::DoubleLit::raw>(ref<DoubleLiteral>))> {};
  struct LiteralRule
      : rule<ref<NumericLiteralRule> |
             create<ast::StringLit>(
                 assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct PrimaryExprRule
      : rule<ref<LiteralRule> | ref<VarRefRule> | ref<ParenExprRule> |
             ref<ContextItemExprRule> | ref<FunctionCallRule> |
             ref<FunctionItemExprRule> | ref<MapConstructorRule> |
             ref<ArrayConstructorRule> | ref<LookupRule>> {};

  static constexpr auto PostfixSuffixAlt =
      (lit<"["> + append<&ast::PostfixExpr::suffixes>(ref<ExprRule>) +
       lit<"]">) |
      append<&ast::PostfixExpr::suffixes>(ref<ArgumentsRule>) |
      append<&ast::PostfixExpr::suffixes>(ref<LookupRule>);

  // No suffix: adopt PrimaryExprRule's own node as-is, so a bare literal
  // surfaces as itself and not wrapped in an empty PostfixExpr.
  struct PostfixExprRule
      : rule<create<ast::PostfixExpr>(
                 assign<&ast::PostfixExpr::base>(ref<PrimaryExprRule>) +
                 some(PostfixSuffixAlt)) |
             ref<PrimaryExprRule>> {};

  struct StepExprRule : rule<ref<PostfixExprRule> | ref<AxisStepRule>> {};

  // ---------------------------------------------------------------- paths

  struct OpChildSlash : rule<lit<"/">, value<ast::BinOp::path_child>> {};
  struct OpDescendantSlash
      : rule<lit<"//">, value<ast::BinOp::path_descendant>> {};

  struct RelativePathExprRule
      : infix<StepExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              left(ref<OpChildSlash> | ref<OpDescendantSlash>)> {};

  struct PathExprRule
      : rule<create<ast::AbsolutePathExpr>(
                 flag<&ast::AbsolutePathExpr::doubleSlash>(lit<"//">) +
                 assign<&ast::AbsolutePathExpr::rest>(
                     ref<RelativePathExprRule>)) |
             create<ast::AbsolutePathExpr>(
                 lit<"/"> +
                 option(assign<&ast::AbsolutePathExpr::rest>(
                     ref<RelativePathExprRule>))) |
             ref<RelativePathExprRule>> {};

  struct OpSimpleMap : rule<lit<"!">, value<ast::BinOp::simple_map>> {};
  struct SimpleMapExprRule
      : infix<PathExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpSimpleMap>)> {};

  struct UnaryExprRule
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExprRule>)) |
             ref<SimpleMapExprRule>> {};

  // ---------------------------------------------------------------- arrow

  struct ArrowStepRule
      : rule<create<ast::ArrowStep>(
            lit<"=>"> +
            (assign<&ast::ArrowStep::name>(ref<EqName>) |
             assign<&ast::ArrowStep::target>(ref<VarRefRule> |
                                             ref<ParenExprRule>)) +
            assign<&ast::ArrowStep::args>(ref<ArgumentsRule>))> {};

  struct ArrowExprRule
      : rule<create<ast::ArrowExpr>(
                 assign<&ast::ArrowExpr::base>(ref<UnaryExprRule>) +
                 some(append<&ast::ArrowExpr::steps>(ref<ArrowStepRule>))) |
             ref<UnaryExprRule>> {};

  // --------------------------------------------------- the precedence ladder

  struct CastExprRule
      : rule<ref<ArrowExprRule> +
             option(nest<ast::CastExpr, &ast::CastExpr::operand>(
                 kw<"cast"> + kw<"as"> +
                 assign<&ast::CastExpr::type>(ref<SingleTypeRule>)))> {};

  struct CastableExprRule
      : rule<ref<CastExprRule> +
             option(nest<ast::CastableExpr, &ast::CastableExpr::operand>(
                 kw<"castable"> + kw<"as"> +
                 assign<&ast::CastableExpr::type>(ref<SingleTypeRule>)))> {};

  struct TreatExprRule
      : rule<ref<CastableExprRule> +
             option(nest<ast::TreatExpr, &ast::TreatExpr::operand>(
                 kw<"treat"> + kw<"as"> +
                 assign<&ast::TreatExpr::type>(ref<SequenceTypeRule>)))> {};

  struct InstanceOfExprRule
      : rule<ref<TreatExprRule> +
             option(nest<ast::InstanceOfExpr, &ast::InstanceOfExpr::operand>(
                 kw<"instance"> + kw<"of"> +
                 assign<&ast::InstanceOfExpr::type>(
                     ref<SequenceTypeRule>)))> {};

  struct OpIntersect : rule<kw<"intersect">, value<ast::BinOp::intersect>> {};
  struct OpExcept : rule<kw<"except">, value<ast::BinOp::except>> {};
  struct OpUnionSym : rule<lit<"|">, value<ast::BinOp::union_>> {};
  struct OpUnionKw : rule<kw<"union">, value<ast::BinOp::union_>> {};
  struct OpMulSym : rule<lit<"*">, value<ast::BinOp::mul>> {};
  struct OpDivKw : rule<kw<"div">, value<ast::BinOp::div_>> {};
  struct OpIDivKw : rule<kw<"idiv">, value<ast::BinOp::idiv>> {};
  struct OpModKw : rule<kw<"mod">, value<ast::BinOp::mod>> {};
  struct OpAddSym : rule<lit<"+">, value<ast::BinOp::add>> {};
  struct OpSubSym : rule<lit<"-">, value<ast::BinOp::sub>> {};

  struct OpTo : rule<kw<"to">, value<ast::BinOp::to>> {};
  struct OpConcat : rule<lit<"||">, value<ast::BinOp::concat>> {};

  struct OpValueEq : rule<kw<"eq">, value<ast::BinOp::value_eq>> {};
  struct OpValueNe : rule<kw<"ne">, value<ast::BinOp::value_ne>> {};
  struct OpValueLt : rule<kw<"lt">, value<ast::BinOp::value_lt>> {};
  struct OpValueLe : rule<kw<"le">, value<ast::BinOp::value_le>> {};
  struct OpValueGt : rule<kw<"gt">, value<ast::BinOp::value_gt>> {};
  struct OpValueGe : rule<kw<"ge">, value<ast::BinOp::value_ge>> {};
  struct OpGeneralEq : rule<lit<"=">, value<ast::BinOp::eq>> {};
  struct OpGeneralNe : rule<lit<"!=">, value<ast::BinOp::ne>> {};
  struct OpGeneralLe : rule<lit<"<=">, value<ast::BinOp::le>> {};
  struct OpGeneralLt : rule<lit<"<">, value<ast::BinOp::lt>> {};
  struct OpGeneralGe : rule<lit<">=">, value<ast::BinOp::ge>> {};
  struct OpGeneralGt : rule<lit<">">, value<ast::BinOp::gt>> {};
  struct OpNodeIs : rule<kw<"is">, value<ast::BinOp::node_is>> {};
  struct OpNodePrecedes : rule<lit<"<<">, value<ast::BinOp::node_precedes>> {};
  struct OpNodeFollows : rule<lit<">>">, value<ast::BinOp::node_follows>> {};
  static constexpr auto CompareOp =
      ref<OpValueEq> | ref<OpValueNe> | ref<OpValueLt> | ref<OpValueLe> |
      ref<OpValueGt> | ref<OpValueGe> | ref<OpGeneralEq> | ref<OpGeneralNe> |
      ref<OpGeneralLe> | ref<OpGeneralLt> | ref<OpGeneralGe> |
      ref<OpGeneralGt> | ref<OpNodeIs> | ref<OpNodePrecedes> |
      ref<OpNodeFollows>;

  struct AdditiveExprRule
      : infix<InstanceOfExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OpIntersect> | ref<OpExcept>) >>
               left(ref<OpUnionSym> | ref<OpUnionKw>) >>
               left(ref<OpMulSym> | ref<OpDivKw> | ref<OpIDivKw> | ref<OpModKw>) >>
               left(ref<OpAddSym> | ref<OpSubSym>))> {};
  struct RangeExprRule
      : rule<ref<AdditiveExprRule> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(ref<OpTo>) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveExprRule>)))> {};
  struct StringConcatExprRule
      : infix<RangeExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, left(ref<OpConcat>)> {};
  struct ComparisonExprRule
      : rule<ref<StringConcatExprRule> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(CompareOp) +
                 assign<&ast::BinaryExpr::right>(ref<StringConcatExprRule>)))> {};
  struct OpAnd : rule<kw<"and">, value<ast::BinOp::and_>> {};
  struct OpOr : rule<kw<"or">, value<ast::BinOp::or_>> {};
  struct OrExprRule
      : infix<ComparisonExprRule, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right, (left(ref<OpAnd>) >> left(ref<OpOr>))> {};

  // ------------------------------------------------------------ flow control

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<ExprRule>) +
            lit<")"> + kw<"then"> +
            assign<&ast::IfExpr::thenBranch>(ref<ExprSingleRule>) +
            kw<"else"> +
            assign<&ast::IfExpr::elseBranch>(ref<ExprSingleRule>))> {};

  struct QuantifierBindingRule
      : rule<create<ast::QuantifierBinding>(
            lit<"$"> + assign<&ast::QuantifierBinding::name>(ref<EqName>) +
            kw<"in"> +
            assign<&ast::QuantifierBinding::source>(ref<ExprSingleRule>))> {};
  struct QuantifiedExprRule
      : rule<create<ast::QuantifiedExpr>(
            (flag<&ast::QuantifiedExpr::every>(kw<"every">) |
             kw<"some">) +
            append<&ast::QuantifiedExpr::bindings>(ref<QuantifierBindingRule>) +
            many(lit<","> +
                 append<&ast::QuantifiedExpr::bindings>(
                     ref<QuantifierBindingRule>)) +
            kw<"satisfies"> +
            assign<&ast::QuantifiedExpr::predicate>(ref<ExprSingleRule>))> {};

  struct LetBindingRule
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::name>(ref<EqName>) +
            lit<":="> +
            assign<&ast::LetBinding::value>(ref<ExprSingleRule>))> {};
  struct LetExprRule
      : rule<create<ast::LetExpr>(
            kw<"let"> + append<&ast::LetExpr::bindings>(ref<LetBindingRule>) +
            many(lit<","> +
                 append<&ast::LetExpr::bindings>(ref<LetBindingRule>)) +
            kw<"return"> +
            assign<&ast::LetExpr::result>(ref<ExprSingleRule>))> {};

  struct ForBindingRule
      : rule<create<ast::ForBinding>(
            lit<"$"> + assign<&ast::ForBinding::name>(ref<EqName>) +
            kw<"in"> +
            assign<&ast::ForBinding::source>(ref<ExprSingleRule>))> {};
  struct ForExprRule
      : rule<create<ast::ForExpr>(
            kw<"for"> + append<&ast::ForExpr::bindings>(ref<ForBindingRule>) +
            many(lit<","> +
                 append<&ast::ForExpr::bindings>(ref<ForBindingRule>)) +
            kw<"return"> +
            assign<&ast::ForExpr::result>(ref<ExprSingleRule>))> {};

  struct ExprSingleRule
      : rule<ref<ForExprRule> | ref<LetExprRule> | ref<QuantifiedExprRule> |
             ref<IfExprRule> | ref<OrExprRule>> {};

  struct ExprRule
      : rule<create<ast::SequenceExpr>(
            append<&ast::SequenceExpr::items>(ref<ExprSingleRule>) +
            many(lit<","> +
                 append<&ast::SequenceExpr::items>(ref<ExprSingleRule>)))> {};

  // -------------------------------------------------------------- document

  // `;`-separated so the corpus's own multi-expression test files parse,
  // the trailing `;` optional so a single bare expression -- the spec's own
  // language -- parses too.
  struct Document
      : rule<create<ast::Document>(
            some(append<&ast::Document::expressions>(ref<ExprRule>), lit<";">) +
            option(lit<";">))> {};

  using entry = Document;
  using ignore = list<WS, COMMENT>;
};

}  // namespace xpath
