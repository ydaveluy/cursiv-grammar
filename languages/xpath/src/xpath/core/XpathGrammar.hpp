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
  if (t.size() < 2) return std::string_view{};
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

struct Xpath : cursiv::GrammarSpec {
  static constexpr auto Scalar = cr<R"(\x00-\u{10FFFF})">;
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
                 many(ref<COMMENT> | (!lit<"(:"> + !lit<":)"> + Scalar)) +
                 lit<":)">> {};

  struct NCName : terminal<NameStartChar + many(NameContChar)> {};
  struct QName : terminal<NameStartChar + many(NameContChar) +
                          option(lit<":"> + NameStartChar + many(NameContChar))> {};
  static constexpr auto BracedURI =
      lit<"Q"> + lit<"{"> + many(~(cr<"{}"> | ~Scalar)) + lit<"}">;
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

  static constexpr auto dq_body = lit<"\"\""> | ~(cr<R"(")"> | ~Scalar);
  static constexpr auto sq_body = lit<"''"> | ~(cr<"'"> | ~Scalar);
  struct STRING
      : terminal<as<unquote_str>((lit<"\""> + many(dq_body) + lit<"\"">) |
                                 (lit<"'"> + many(sq_body) + lit<"'">))> {};

  // The recursive core: expressions and their arguments reach path steps,
  // which reach postfix expressions, which reach primary expressions, which
  // reach expressions again through parentheses, calls and constructors.
  struct Expr;
  struct ExprSingle;
  struct ParenExpr;

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

  struct ItemType;

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

  struct KindTest
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

  struct SequenceType;

  struct AnyFunctionTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"function">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedFunctionTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"function">) + lit<"("> +
            option(some(append<&ast::ItemType::params>(ref<SequenceType>), lit<",">)) +
            lit<")"> + kw<"as"> +
            assign<&ast::ItemType::returnType>(ref<SequenceType>))> {};
  struct FunctionTest : rule<ref<TypedFunctionTest> | ref<AnyFunctionTest>> {};

  struct AnyMapTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"map">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedMapTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"map">) + lit<"("> +
            assign<&ast::ItemType::typeName>(ref<EqName>) + lit<","> +
            append<&ast::ItemType::params>(ref<SequenceType>) + lit<")">)> {
  };
  struct MapTest : rule<ref<TypedMapTest> | ref<AnyMapTest>> {};

  struct AnyArrayTest
      : rule<create<ast::ItemType>(assign<&ast::ItemType::name>(kw<"array">) +
                                   lit<"("> + lit<"*"> + lit<")">)> {};
  struct TypedArrayTest
      : rule<create<ast::ItemType>(
            assign<&ast::ItemType::name>(kw<"array">) + lit<"("> +
            append<&ast::ItemType::params>(ref<SequenceType>) + lit<")">)> {
  };
  struct ArrayTest : rule<ref<TypedArrayTest> | ref<AnyArrayTest>> {};

  struct ParenthesizedItemType
      : rule<create<ast::ItemType>(lit<"("> +
                                   assign<&ast::ItemType::inner>(
                                       ref<ItemType>) +
                                   lit<")">)> {};

  struct ItemType
      : rule<ref<KindTest> | ref<AnyItemTest> | ref<FunctionTest> |
             ref<MapTest> | ref<ArrayTest> | ref<AtomicOrUnionType> |
             ref<ParenthesizedItemType>> {};

  struct SequenceType
      : rule<create<ast::SequenceType>(
                 flag<&ast::SequenceType::emptySequence>(
                     kw<"empty-sequence">) +
                 lit<"("> + lit<")">) |
             create<ast::SequenceType>(
                 assign<&ast::SequenceType::itemType>(ref<ItemType>) +
                 option(assign<&ast::SequenceType::occurrence>(
                     ref<OccurrenceIndicator>)))> {};

  struct SingleType
      : rule<create<ast::SingleType>(
            assign<&ast::SingleType::name>(ref<EqName>) +
            option(flag<&ast::SingleType::optional>(lit<"?">)))> {};

  // --------------------------------------------------------- node tests

  struct NameTest
      : rule<ref<EqName> | lit<"*"> | (ref<NCName> + lit<":*">) |
             (lit<"*:"> + ref<NCName>) | (ref<BracedURILiteral> + lit<"*">)> {
  };

  struct NodeTest
      : rule<create<ast::NodeTest>(assign<&ast::NodeTest::kind>(
                ref<KindTest>)) |
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
      many(lit<"["> + append<&ast::AxisStep::predicates>(ref<Expr>) +
           lit<"]">);

  struct AxisStep
      : rule<create<ast::AxisStep>(
                 assign<&ast::AxisStep::axis>(ref<ForwardAxisWord>) +
                 lit<"::"> +
                 assign<&ast::AxisStep::test>(ref<NodeTest>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(
                 option(assign<&ast::AxisStep::axis>(lit<"@">)) +
                 assign<&ast::AxisStep::test>(ref<NodeTest>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(
                 assign<&ast::AxisStep::axis>(ref<ReverseAxisWord>) +
                 lit<"::"> +
                 assign<&ast::AxisStep::test>(ref<NodeTest>) +
                 PredicateListFrag) |
             create<ast::AxisStep>(assign<&ast::AxisStep::axis>(lit<"..">) +
                                   PredicateListFrag)> {};

  // ------------------------------------------------------------ postfix

  struct ArgPlaceholder : rule<create<ast::ArgPlaceholder> + lit<"?">> {};
  struct Argument : rule<ref<ArgPlaceholder> | ref<ExprSingle>> {};
  struct Arguments
      : rule<create<ast::Arguments>(
            lit<"("> +
            option(some(append<&ast::Arguments::positional>(ref<Argument>), lit<",">)) +
            lit<")">)> {};

  struct KeySpecifier
      : rule<create<ast::Lookup>(assign<&ast::Lookup::key>(ref<NCName>)) |
             create<ast::Lookup>(assign<&ast::Lookup::key>(ref<UINT_TEXT>)) |
             create<ast::Lookup>(
                 assign<&ast::Lookup::keyExpr>(ref<ParenExpr>)) |
             create<ast::Lookup>(
                 flag<&ast::Lookup::wildcard>(lit<"*">))> {};
  struct Lookup : rule<lit<"?"> + ref<KeySpecifier>> {};

  struct VarRef
      : rule<create<ast::VarRef>(lit<"$"> +
                                 assign<&ast::VarRef::name>(ref<EqName>))> {};

  struct ParenExpr
      : rule<create<ast::ParenExpr>(
            lit<"("> + option(assign<&ast::ParenExpr::body>(ref<Expr>)) +
            lit<")">)> {};

  struct ContextItemExpr : rule<create<ast::ContextItemExpr> + lit<".">> {
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

  struct FunctionCall
      : rule<create<ast::CallExpr>(
            !ReservedFunctionName + assign<&ast::CallExpr::name>(ref<EqName>) +
            assign<&ast::CallExpr::args>(ref<Arguments>))> {};

  struct NamedFunctionRef
      : rule<create<ast::NamedFunctionRef>(
            assign<&ast::NamedFunctionRef::name>(ref<EqName>) + lit<"#"> +
            assign<&ast::NamedFunctionRef::arity>(ref<IntegerLiteral>))> {};

  struct Param
      : rule<create<ast::Param>(
            lit<"$"> + assign<&ast::Param::name>(ref<EqName>) +
            option(kw<"as"> +
                   assign<&ast::Param::type>(ref<SequenceType>)))> {};

  struct InlineFunctionExpr
      : rule<create<ast::InlineFunctionExpr>(
            kw<"function"> + lit<"("> +
            option(append<&ast::InlineFunctionExpr::params>(ref<Param>) +
                   many(lit<","> +
                        append<&ast::InlineFunctionExpr::params>(
                            ref<Param>))) +
            lit<")"> +
            option(kw<"as"> +
                   assign<&ast::InlineFunctionExpr::returnType>(
                       ref<SequenceType>)) +
            lit<"{"> +
            option(assign<&ast::InlineFunctionExpr::body>(ref<Expr>)) +
            lit<"}">)> {};

  struct FunctionItemExpr
      : rule<ref<NamedFunctionRef> | ref<InlineFunctionExpr>> {};

  struct MapEntry
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ref<ExprSingle>) + lit<":"> +
            assign<&ast::MapEntry::value>(ref<ExprSingle>))> {};
  struct MapConstructor
      : rule<create<ast::MapConstructor>(
            kw<"map"> + lit<"{"> +
            option(append<&ast::MapConstructor::entries>(ref<MapEntry>) +
                   many(lit<","> +
                        append<&ast::MapConstructor::entries>(
                            ref<MapEntry>))) +
            lit<"}">)> {};

  struct ArrayConstructor
      : rule<create<ast::ArrayConstructor>(
                 lit<"["> +
                 option(append<&ast::ArrayConstructor::items>(
                            ref<ExprSingle>) +
                        many(lit<","> +
                             append<&ast::ArrayConstructor::items>(
                                 ref<ExprSingle>))) +
                 lit<"]">) |
             create<ast::ArrayConstructor>(
                 flag<&ast::ArrayConstructor::curly>(kw<"array">) +
                 lit<"{"> +
                 option(assign<&ast::ArrayConstructor::body>(ref<Expr>)) +
                 lit<"}">)> {};

  struct NumericLiteral
      : rule<create<ast::IntegerLit>(
                 assign<&ast::IntegerLit::raw>(ref<IntegerLiteral>)) |
             create<ast::DecimalLit>(
                 assign<&ast::DecimalLit::raw>(ref<DecimalLiteral>)) |
             create<ast::DoubleLit>(
                 assign<&ast::DoubleLit::raw>(ref<DoubleLiteral>))> {};
  struct Literal
      : rule<ref<NumericLiteral> |
             create<ast::StringLit>(
                 assign<&ast::StringLit::value>(ref<STRING>))> {};

  struct PrimaryExpr
      : rule<ref<Literal> | ref<VarRef> | ref<ParenExpr> |
             ref<ContextItemExpr> | ref<FunctionCall> |
             ref<FunctionItemExpr> | ref<MapConstructor> |
             ref<ArrayConstructor> | ref<Lookup>> {};

  static constexpr auto PostfixSuffixAlt =
      (lit<"["> + append<&ast::PostfixExpr::suffixes>(ref<Expr>) +
       lit<"]">) |
      append<&ast::PostfixExpr::suffixes>(ref<Arguments>) |
      append<&ast::PostfixExpr::suffixes>(ref<Lookup>);

  // No suffix: adopt PrimaryExpr's own node as-is, so a bare literal
  // surfaces as itself and not wrapped in an empty PostfixExpr.
  struct PostfixExprRule
      : rule<create<ast::PostfixExpr>(
                 assign<&ast::PostfixExpr::base>(ref<PrimaryExpr>) +
                 some(PostfixSuffixAlt)) |
             ref<PrimaryExpr>> {};

  struct StepExpr : rule<ref<PostfixExprRule> | ref<AxisStep>> {};

  // ---------------------------------------------------------------- paths


  struct RelativePathExpr
      : infix<StepExpr, ast::BinaryExpr,
              left(as<ast::BinOp::path_child>(lit<"/">) | as<ast::BinOp::path_descendant>(lit<"//">))> {};

  struct PathExpr
      : rule<create<ast::AbsolutePathExpr>(
                 flag<&ast::AbsolutePathExpr::doubleSlash>(lit<"//">) +
                 assign<&ast::AbsolutePathExpr::rest>(
                     ref<RelativePathExpr>)) |
             create<ast::AbsolutePathExpr>(
                 lit<"/"> +
                 option(assign<&ast::AbsolutePathExpr::rest>(
                     ref<RelativePathExpr>))) |
             ref<RelativePathExpr>> {};

  struct SimpleMapExpr : infix<PathExpr, ast::BinaryExpr,
                               left(as<ast::BinOp::simple_map>(lit<"!">))> {};

  struct UnaryExpr
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"-"> | lit<"+">) +
                 assign<&ast::UnaryExpr::operand>(ref<UnaryExpr>)) |
             ref<SimpleMapExpr>> {};

  // ---------------------------------------------------------------- arrow

  struct ArrowStep
      : rule<create<ast::ArrowStep>(
            lit<"=>"> +
            (assign<&ast::ArrowStep::name>(ref<EqName>) |
             assign<&ast::ArrowStep::target>(ref<VarRef> |
                                             ref<ParenExpr>)) +
            assign<&ast::ArrowStep::args>(ref<Arguments>))> {};

  struct ArrowExpr
      : rule<create<ast::ArrowExpr>(
                 assign<&ast::ArrowExpr::base>(ref<UnaryExpr>) +
                 some(append<&ast::ArrowExpr::steps>(ref<ArrowStep>))) |
             ref<UnaryExpr>> {};

  // --------------------------------------------------- the precedence ladder

  struct CastExpr
      : rule<ref<ArrowExpr> +
             option(nest<ast::CastExpr, &ast::CastExpr::operand>(
                 kw<"cast"> + kw<"as"> +
                 assign<&ast::CastExpr::type>(ref<SingleType>)))> {};

  struct CastableExpr
      : rule<ref<CastExpr> +
             option(nest<ast::CastableExpr, &ast::CastableExpr::operand>(
                 kw<"castable"> + kw<"as"> +
                 assign<&ast::CastableExpr::type>(ref<SingleType>)))> {};

  struct TreatExpr
      : rule<ref<CastableExpr> +
             option(nest<ast::TreatExpr, &ast::TreatExpr::operand>(
                 kw<"treat"> + kw<"as"> +
                 assign<&ast::TreatExpr::type>(ref<SequenceType>)))> {};

  struct InstanceOfExpr
      : rule<ref<TreatExpr> +
             option(nest<ast::InstanceOfExpr, &ast::InstanceOfExpr::operand>(
                 kw<"instance"> + kw<"of"> +
                 assign<&ast::InstanceOfExpr::type>(
                     ref<SequenceType>)))> {};



  static constexpr auto CompareOp =
      as<ast::BinOp::value_eq>(kw<"eq">) | as<ast::BinOp::value_ne>(kw<"ne">) | as<ast::BinOp::value_lt>(kw<"lt">) | as<ast::BinOp::value_le>(kw<"le">) |
      as<ast::BinOp::value_gt>(kw<"gt">) | as<ast::BinOp::value_ge>(kw<"ge">) | as<ast::BinOp::eq>(lit<"=">) | as<ast::BinOp::ne>(lit<"!=">) |
      as<ast::BinOp::le>(lit<"<=">) | as<ast::BinOp::lt>(lit<"<">) | as<ast::BinOp::ge>(lit<">=">) |
      as<ast::BinOp::gt>(lit<">">) | as<ast::BinOp::node_is>(kw<"is">) | as<ast::BinOp::node_precedes>(lit<"<<">) |
      as<ast::BinOp::node_follows>(lit<">>">);

  struct AdditiveExpr
      : infix<InstanceOfExpr, ast::BinaryExpr,
              levels(left(as<ast::BinOp::intersect>(kw<"intersect">) | as<ast::BinOp::except>(kw<"except">)),
                     left(as<ast::BinOp::union_>(lit<"|">) | as<ast::BinOp::union_>(kw<"union">)),
                     left(as<ast::BinOp::mul>(lit<"*">) | as<ast::BinOp::div_>(kw<"div">) | as<ast::BinOp::idiv>(kw<"idiv">) |
                          as<ast::BinOp::mod>(kw<"mod">)),
                     left(as<ast::BinOp::add>(lit<"+">) | as<ast::BinOp::sub>(lit<"-">)))> {};
  struct RangeExpr
      : rule<ref<AdditiveExpr> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(as<ast::BinOp::to>(kw<"to">)) +
                 assign<&ast::BinaryExpr::right>(ref<AdditiveExpr>)))> {};
  struct StringConcatExpr : infix<RangeExpr, ast::BinaryExpr,
                                  left(as<ast::BinOp::concat>(lit<"||">))> {};
  struct ComparisonExpr
      : rule<ref<StringConcatExpr> +
             option(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(CompareOp) +
                 assign<&ast::BinaryExpr::right>(ref<StringConcatExpr>)))> {};
  struct OrExpr
      : infix<ComparisonExpr, ast::BinaryExpr,
              levels(left(as<ast::BinOp::and_>(kw<"and">)), left(as<ast::BinOp::or_>(kw<"or">)))> {};

  // ------------------------------------------------------------ flow control

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> + assign<&ast::IfExpr::cond>(ref<Expr>) +
            lit<")"> + kw<"then"> +
            assign<&ast::IfExpr::thenBranch>(ref<ExprSingle>) +
            kw<"else"> +
            assign<&ast::IfExpr::elseBranch>(ref<ExprSingle>))> {};

  struct QuantifierBinding
      : rule<create<ast::QuantifierBinding>(
            lit<"$"> + assign<&ast::QuantifierBinding::name>(ref<EqName>) +
            kw<"in"> +
            assign<&ast::QuantifierBinding::source>(ref<ExprSingle>))> {};
  struct QuantifiedExpr
      : rule<create<ast::QuantifiedExpr>(
            (flag<&ast::QuantifiedExpr::every>(kw<"every">) |
             kw<"some">) +
            append<&ast::QuantifiedExpr::bindings>(ref<QuantifierBinding>) +
            many(lit<","> +
                 append<&ast::QuantifiedExpr::bindings>(
                     ref<QuantifierBinding>)) +
            kw<"satisfies"> +
            assign<&ast::QuantifiedExpr::predicate>(ref<ExprSingle>))> {};

  struct LetBinding
      : rule<create<ast::LetBinding>(
            lit<"$"> + assign<&ast::LetBinding::name>(ref<EqName>) +
            lit<":="> +
            assign<&ast::LetBinding::value>(ref<ExprSingle>))> {};
  struct LetExpr
      : rule<create<ast::LetExpr>(
            kw<"let"> + some(append<&ast::LetExpr::bindings>(ref<LetBinding>), lit<",">) +
            kw<"return"> +
            assign<&ast::LetExpr::result>(ref<ExprSingle>))> {};

  struct ForBinding
      : rule<create<ast::ForBinding>(
            lit<"$"> + assign<&ast::ForBinding::name>(ref<EqName>) +
            kw<"in"> +
            assign<&ast::ForBinding::source>(ref<ExprSingle>))> {};
  struct ForExpr
      : rule<create<ast::ForExpr>(
            kw<"for"> + some(append<&ast::ForExpr::bindings>(ref<ForBinding>), lit<",">) +
            kw<"return"> +
            assign<&ast::ForExpr::result>(ref<ExprSingle>))> {};

  struct ExprSingle
      : rule<ref<ForExpr> | ref<LetExpr> | ref<QuantifiedExpr> |
             ref<IfExpr> | ref<OrExpr>> {};

  struct Expr
      : rule<create<ast::SequenceExpr>(
            some(append<&ast::SequenceExpr::items>(ref<ExprSingle>), lit<",">))> {};

  // -------------------------------------------------------------- document

  // `;`-separated so the corpus's own multi-expression test files parse,
  // the trailing `;` optional so a single bare expression -- the spec's own
  // language -- parses too.
  struct Document
      : rule<create<ast::Document>(
            some(append<&ast::Document::expressions>(ref<Expr>), lit<";">) +
            option(lit<";">))> {};

  using entry = Document;
  using ignored = list<WS, COMMENT>;
};

}  // namespace xpath
