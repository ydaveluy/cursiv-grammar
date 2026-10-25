// The sysmlv2 language: its grammar -- the OMG SysML v2 textual notation.
// ~90 definition/usage/relationship keywords share one shape (see
// ast::Element: a run of prefixes, an optional identification, specs, a
// value, a body); this grammar is one rule for that shape plus the
// expression language. Shared shapes and deviations are documented inline.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sysmlv2/core/ast.hpp"

namespace sysmlv2 {

struct SysmlV2 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // A `//` not immediately followed by `*` -- OMG's `//* ... */`
  // block-comment-out form belongs to REGULAR_COMMENT, not a line comment.
  struct SL_COMMENT : terminal<lit<"//", !lit<"*">> + many(cr<R"(^\r\n)">)> {};

  struct IDENTIFIER : terminal<cr<"a-zA-Z_"> + many(w)> {};
  struct STRING
      : terminal<lit<"'"> + many((lit<"\\"> + any) | cr<R"(^'\\)">) + lit<"'">> {
  };
  struct DOUBLE_STRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                 lit<"\"">> {};
  struct INTEGER : terminal<some(d)> {};
  struct REAL
      : terminal<(many(d) + lit<"."> + some(d) +
                  option(cr<"eE"> + option(cr<"+-">) + some(d)))  //
                 | (some(d) + cr<"eE"> + option(cr<"+-">) + some(d))> {};

  // A block comment is a real terminal, not trivia: `comment`, `doc` and
  // `rep` elements consume it explicitly, and so may an expression (a
  // comment used as a placeholder).
  struct REGULAR_COMMENT
      : terminal<through(lit<"/*">, lit<"*/">) | through(lit<"//*">, lit<"*/">)> {};

  // Soft keywords: legal names where a state reads names, keywords where it
  // reads one of these words. `metadata` is added past SysML v2's own
  // grammar so `x.metadata` parses through the ordinary member-access
  // postfix instead of needing its own ambiguous rule.
  static constexpr auto UnreservedKw =
      kw<"type"> | kw<"multiplicity"> | kw<"var"> | kw<"language"> |
      kw<"locale"> | kw<"crosses"> | kw<"step"> | kw<"feature"> |
      kw<"behavior"> | kw<"function"> | kw<"member"> | kw<"predicate"> |
      kw<"interaction"> | kw<"metaclass"> | kw<"metadata">;

  struct Name : rule<ref<IDENTIFIER> | ref<STRING> | UnreservedKw> {};

  struct QualifiedName
      : rule<option(lit<"$"> + lit<"::">) + some(ref<Name>, lit<"::">)> {};

  // `qualifiedName (DOT qualifiedName)*` -- a feature chain -- covers every
  // dotted reference SysML v2 uses: generalType, specificType,
  // ownedSubsetting/Redefinition/ReferenceSubsetting/CrossSubsetting,
  // ownedConjugation/Disjoining, unioning/intersecting/differencing,
  // featureChainMember, connector/flow ends, import/dependency targets.
  struct Chain : rule<some(ref<QualifiedName>, lit<".">)> {};

  // ===== Expressions =====

  struct OwnedExpression;
  struct BodyItem;
  struct Element;

  struct LiteralText
      : rule<kw<"true"> | kw<"false"> | ref<DOUBLE_STRING> | ref<REAL> |
             ref<INTEGER> | lit<"*">> {};

  struct EmptyParens : rule<lit<"("> + lit<")">> {};

  static constexpr auto ElementBody =
      lit<";"> |
      (lit<"{"> + many(append<&ast::Element::members>(ref<BodyItem>)) +
       option(assign<&ast::Element::result>(ref<OwnedExpression>)) +
       lit<"}">);

  // A `{ ... }` used as an expression (a function/calc/case body read as a
  // value): items, then an optional trailing result expression.
  struct FunctionBodyPart
      : rule<many(append<&ast::Body::items>(ref<BodyItem>)) +
             option(assign<&ast::Body::result>(ref<OwnedExpression>))> {};

  struct BaseExpression
      : rule<create<ast::Literal>(assign<&ast::Literal::text>(
                 kw<"null">))  //
             | create<ast::Literal>(
                   assign<&ast::Literal::text>(ref<EmptyParens>))  //
             | create<ast::Literal>(
                   assign<&ast::Literal::text>(ref<REGULAR_COMMENT>))  //
             | create<ast::Literal>(
                   assign<&ast::Literal::text>(ref<LiteralText>))  //
             | create<ast::Postfix>(
                   lit<"("> + assign<&ast::Postfix::op>(kw<"as">) +
                   assign<&ast::Postfix::name>(ref<Chain>) + lit<")">)  //
             | create<ast::Postfix>(
                   assign<&ast::Postfix::op>(kw<"new">) +
                   assign<&ast::Postfix::name>(ref<Chain>) + lit<"("> +
                   option(some(append<&ast::Postfix::args>(
                                   ref<OwnedExpression>),
                               lit<",">)) +
                   lit<")">)  //
             | (lit<"{"> + ref<FunctionBodyPart> + lit<"}">)  //
             | create<ast::Seq>(
                   lit<"("> +
                   some(append<&ast::Seq::items>(ref<OwnedExpression>),
                        lit<",">) +
                   lit<")">)  //
             | create<ast::NameRef>(
                   assign<&ast::NameRef::name>(ref<Chain>))> {};

  struct PrimaryExpression
      : rule<ref<BaseExpression> +
             many(nest<ast::Postfix, &ast::Postfix::base>(
                      assign<&ast::Postfix::op>(lit<".">) +
                      assign<&ast::Postfix::name>(ref<QualifiedName>))  //
                  | nest<ast::Postfix, &ast::Postfix::base>(
                        assign<&ast::Postfix::op>(lit<".?">) + lit<"{"> +
                        assign<&ast::Postfix::body>(ref<FunctionBodyPart>) +
                        lit<"}">)  //
                  | nest<ast::Postfix, &ast::Postfix::base>(
                        assign<&ast::Postfix::op>(lit<"->">) +
                        assign<&ast::Postfix::name>(ref<QualifiedName>) +
                        ((lit<"{"> +
                          assign<&ast::Postfix::body>(
                              ref<FunctionBodyPart>) +
                          lit<"}">) |
                         (lit<"("> +
                          option(some(append<&ast::Postfix::args>(
                                          ref<OwnedExpression>),
                                      lit<",">)) +
                          lit<")">)))  //
                  | nest<ast::Postfix, &ast::Postfix::base>(
                        assign<&ast::Postfix::op>(lit<"[">) +
                        option(some(append<&ast::Postfix::args>(
                                        ref<OwnedExpression>),
                                    lit<",">)) +
                        lit<"]">)  //
                  | nest<ast::Postfix, &ast::Postfix::base>(
                        assign<&ast::Postfix::op>(lit<"#">) + lit<"("> +
                        option(some(append<&ast::Postfix::args>(
                                        ref<OwnedExpression>),
                                    lit<",">)) +
                        lit<")">)  //
                  | nest<ast::Postfix, &ast::Postfix::base>(
                        assign<&ast::Postfix::op>(lit<"(">) +
                        option(some(append<&ast::Postfix::args>(
                                        ref<OwnedExpression>),
                                    lit<",">)) +
                        lit<")">))> {};

  static constexpr auto OpUnary = lit<"+"> | lit<"-"> | lit<"~"> | kw<"not">;

  struct UnaryExpression
      : rule<create<ast::Unary>(
                 assign<&ast::Unary::op>(OpUnary) +
                 assign<&ast::Unary::operand>(ref<UnaryExpression>))  //
             | create<ast::Unary>(
                   assign<&ast::Unary::op>(lit<"@@"> | lit<"@">) +
                   assign<&ast::Unary::operand>(create<ast::NameRef>(
                       assign<&ast::NameRef::name>(ref<Chain>))))  //
             | create<ast::Unary>(
                   assign<&ast::Unary::op>(kw<"all">) +
                   assign<&ast::Unary::operand>(create<ast::NameRef>(
                       assign<&ast::NameRef::name>(ref<Chain>))))  //
             | ref<PrimaryExpression>> {};

  // POW right-assoc, tightest; MUL, ADD, RANGE, CMP, TYPECHECK (widened: the
  // right operand accepts a full expression rather than only a type
  // reference -- over-acceptance), EQ, BITAND, XOR, BITOR,
  // IMPLIES, COALESCE, loosest.
  struct OperatorExpression
      : infix<UnaryExpression, &ast::Binary::left, &ast::Binary::op,
              &ast::Binary::right,
              (right(lit<"**"> | lit<"^">) >>
               left(lit<"*"> | lit<"/"> | lit<"%">) >>
               left(lit<"+"> | lit<"-">) >> left(lit<"..">) >>
               left(lit<"<="> | lit<">="> | lit<"<"> | lit<">">) >>
               left(kw<"istype"> | kw<"hastype"> | lit<"@@"> | lit<"@"> |
                    kw<"as"> | kw<"meta">) >>
               left(lit<"==="> | lit<"!=="> | lit<"=="> | lit<"!=">) >>
               left(lit<"&"> | kw<"and">) >> left(kw<"xor">) >>
               left(lit<"|"> | kw<"or">) >> left(kw<"implies">) >>
               left(lit<"??">))> {};

  struct OwnedExpression
      : rule<create<ast::Ternary>(
                 kw<"if"> +
                 assign<&ast::Ternary::cond>(ref<OwnedExpression>) +
                 lit<"?"> +
                 assign<&ast::Ternary::then_>(ref<OwnedExpression>) +
                 kw<"else"> +
                 assign<&ast::Ternary::else_>(ref<OwnedExpression>))  //
             | ref<OperatorExpression>> {};

  // ===== The generic declaration shape =====

  static constexpr auto Flag =
      kw<"public"> | kw<"private"> | kw<"protected"> | kw<"abstract"> |
      kw<"variation"> | kw<"in"> | kw<"out"> | kw<"inout"> | kw<"derived"> |
      kw<"composite"> | kw<"portion"> | kw<"var"> | kw<"const"> |
      kw<"constant"> | kw<"ref"> | kw<"individual"> | kw<"snapshot"> |
      kw<"timeslice"> | kw<"standard"> | kw<"all"> | kw<"nonunique"> |
      kw<"ordered"> | kw<"end">;

  struct UseCaseKw : rule<kw<"use"> + kw<"case">> {};
  struct AssocStructKw : rule<kw<"assoc"> + kw<"struct">> {};
  struct SuccessionFlowKw : rule<kw<"succession"> + kw<"flow">> {};
  // A handful of keywords open with an optional second keyword naming the
  // shortcut's target kind (`perform action`, `require constraint`): kept
  // as its own text rule so `Kind` stays one assign.
  struct PerformKw : rule<kw<"perform"> + option(kw<"action">)> {};
  struct ExhibitKw : rule<kw<"exhibit"> + option(kw<"state">)> {};
  struct IncludeKw : rule<kw<"include"> + option(ref<UseCaseKw>)> {};
  struct AssertKw
      : rule<kw<"assert"> + option(kw<"not">) + option(kw<"constraint">)> {};
  struct SatisfyKw
      : rule<option(kw<"assert"> + option(kw<"not">)) + kw<"satisfy"> +
             option(kw<"requirement">)> {};
  struct RequireKw
      : rule<(kw<"assume"> | kw<"require">) + option(kw<"constraint">)> {};
  struct FrameKw : rule<kw<"frame"> + option(kw<"concern">)> {};
  struct VerifyKw : rule<kw<"verify"> + option(kw<"requirement">)> {};
  struct RenderKw : rule<kw<"render"> + option(kw<"rendering">)> {};

  // Every leading keyword of a SysML definition/usage/relationship-with-a-
  // body/action node/state action -- one flat set, since each is reached
  // through a distinct token and none double as a `Flag` or `Name`.
  static constexpr auto Kind =
      kw<"package"> | ref<UseCaseKw> | ref<AssocStructKw> |
      ref<SuccessionFlowKw> | kw<"type"> | kw<"classifier"> | kw<"class"> |
      kw<"struct"> | kw<"datatype"> | kw<"assoc"> | kw<"behavior"> |
      kw<"interaction"> | kw<"metaclass"> | kw<"attribute"> | kw<"enum"> |
      kw<"occurrence"> | kw<"item"> | kw<"part"> | kw<"port"> |
      kw<"connection"> | kw<"interface"> | kw<"allocation"> | kw<"flow"> |
      kw<"message"> | kw<"action"> | kw<"calc"> | kw<"state"> |
      kw<"constraint"> | ref<RequireKw> | kw<"requirement"> | kw<"concern"> |
      kw<"case"> | kw<"analysis"> | kw<"verification"> | kw<"view"> |
      kw<"viewpoint"> | kw<"rendering"> | kw<"metadata"> | lit<"@"> |
      kw<"connector"> | kw<"binding"> | kw<"succession"> |
      kw<"multiplicity"> | ref<PerformKw> | ref<ExhibitKw> |
      ref<IncludeKw> | ref<AssertKw> | ref<SatisfyKw> | kw<"variant"> |
      kw<"subject"> | kw<"actor"> | kw<"stakeholder"> | kw<"objective"> |
      ref<FrameKw> | ref<VerifyKw> | ref<RenderKw> |
      kw<"merge"> | kw<"decide"> | kw<"join"> | kw<"fork"> | kw<"accept"> |
      kw<"send"> | kw<"assign"> | kw<"terminate"> | kw<"transition"> |
      kw<"entry"> | kw<"do"> | kw<"exit"> | kw<"individual"> |
      kw<"return">;

  struct TypedByKw : rule<kw<"typed"> + kw<"by">> {};
  struct DefinedByKw : rule<kw<"defined"> + kw<"by">> {};

  // A single specialization/subsetting/typing/redefinition/conjugation
  // edge. `:>` covers both specialization and subsetting depending on
  // context (type vs. feature position); the AST does not distinguish them
  // -- a deliberate simplification.
  struct SpecItem
      : rule<create<ast::Spec>(
            assign<&ast::Spec::kind>(
                lit<":>>"> | lit<"::>"> | lit<":>"> | lit<"~"> |
                lit<"=>"> | lit<":"> | kw<"specializes"> | kw<"subsets"> |
                kw<"references"> | kw<"crosses"> | kw<"redefines"> |
                kw<"conjugates"> | ref<TypedByKw> | ref<DefinedByKw>) +
            assign<&ast::Spec::target>(ref<Chain>))> {};

  struct MultiplicityBoundExpr : rule<ref<LiteralText> | ref<Chain>> {};
  struct MultiplicityBounds
      : rule<lit<"["> + ref<MultiplicityBoundExpr> +
             option(lit<".."> + ref<MultiplicityBoundExpr>) + lit<"]">> {};

  // SysML v2's own grammar restricts both `identification` and a shortcut
  // usage's target (`perform X::Y`, `include X::Y`) to a bare name and a
  // dotted feature chain respectively; folded into one `name` slot here
  // that accepts a chain everywhere -- over-acceptance.
  static constexpr auto Identification =
      (lit<"<"> + assign<&ast::Element::shortName>(ref<Name>) + lit<">"> +
       option(assign<&ast::Element::name>(ref<Chain>)))  //
      | assign<&ast::Element::name>(ref<Chain>);

  static constexpr auto ValuePart =
      (lit<":="> | lit<"="> |
       (kw<"default"> + option(lit<":="> | lit<"=">))) +
      assign<&ast::Element::value>(ref<OwnedExpression>);

  static constexpr auto ChainCSV =
      some(append<&ast::Element::ends>(ref<Chain>), lit<",">);
  struct ConnectorEnd : rule<option(flag<&ast::Endpoint::isEnd>(kw<"end">)) +
                             assign<&ast::Endpoint::name>(ref<Chain>) +
                             many(append<&ast::Endpoint::specs>(ref<SpecItem>))> {};
  static constexpr auto ConnectorEnds =
      (append<&ast::Element::connectorEnds>(ref<ConnectorEnd>) + kw<"to"> +
       append<&ast::Element::connectorEnds>(ref<ConnectorEnd>)) |
      (lit<"("> +
       some(append<&ast::Element::connectorEnds>(ref<ConnectorEnd>), lit<",">) +
       lit<")">);
  static constexpr auto EndsPart =
      (kw<"from"> + ChainCSV + option(kw<"to"> + ChainCSV)) | (kw<"to"> + ChainCSV) |
      ConnectorEnds                                       //
      | ((kw<"connect"> | kw<"allocate">)+ConnectorEnds)  //
      | (kw<"about"> + ChainCSV)                          //
      // `accept`'s/`send`'s trigger and receiver, folded to a chain rather
      // than the full expression SysML v2's own grammar allows there.
      | (kw<"via"> + append<&ast::Element::ends>(ref<Chain>) +
         option(kw<"to"> + append<&ast::Element::ends>(ref<Chain>)))  //
      // `satisfy ... by X`.
      | (kw<"by"> + ChainCSV);

  struct ActionClause
      : rule<(assign<&ast::ActionClause::kind>(kw<"first"> | kw<"then"> | kw<"accept"> |
                                               kw<"send"> | kw<"via"> | kw<"to">) +
              assign<&ast::ActionClause::endpoint>(ref<ConnectorEnd>)) |
             (assign<&ast::ActionClause::kind>(kw<"if"> | kw<"do">) +
              assign<&ast::ActionClause::expression>(ref<OwnedExpression>))> {};

  // `typings`/`subsettings`/`redefinitions` let a comma-separated list
  // continue ONE relation without repeating its operator (`:> A, B`
  // subsets both): each continuation reads as its own `Spec`, tagged `,`
  // rather than replaying the operator it continues.
  static constexpr auto SpecWithContinuations =
      append<&ast::Element::specs>(ref<SpecItem>) +
      many(append<&ast::Element::specs>(create<ast::Spec>(
          assign<&ast::Spec::kind>(lit<",">) +
          assign<&ast::Spec::target>(ref<Chain>))));

  // The shape every definition, usage, action node, state action and
  // in-body relationship shares: prefixes, an optional keyword pair
  // (`kind`, then `def`), an optional identification, specializations and
  // an optional multiplicity in either order, a value, endpoints, a body.
  struct DeclItem
      : rule<create<ast::Element>(
            many(append<&ast::Element::flags>(Flag)) +
            many(lit<"#"> + append<&ast::Element::metaPrefixes>(ref<Chain>)) +
            !(kw<"connect"> |
              kw<"allocate">)+option(assign<&ast::Element::kind>(Kind)) +
            option(flag<&ast::Element::isDef>(kw<"def">)) +
            option(Identification) + many(SpecWithContinuations) +
            option(assign<&ast::Element::multiplicity>(ref<MultiplicityBounds>) +
                   many(SpecWithContinuations)) +
            many(append<&ast::Element::flags>(kw<"ordered"> | kw<"nonunique">)) +
            option(ValuePart) + option(EndsPart) +
            many(append<&ast::Element::clauses>(ref<ActionClause>)) + ElementBody)> {};

  struct ConnectItem : rule<create<ast::Element>(
                           many(append<&ast::Element::flags>(Flag)) +
                           assign<&ast::Element::kind>(kw<"connect"> | kw<"allocate">) +
                           ConnectorEnds + ElementBody)> {};

  // `bind a = b;`: the shortcut form of a binding connector, its two ends
  // joined by `=` instead of the usual identification.
  struct BindItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"bind">) +
            append<&ast::Element::ends>(ref<Chain>) + lit<"="> +
            append<&ast::Element::ends>(ref<Chain>) + ElementBody)> {};

  static constexpr auto VisibilityFlags =
      kw<"public"> | kw<"private"> | kw<"protected">;

  struct ImportTarget
      : rule<ref<Chain> +
             option(lit<"::"> +
                    (lit<"**"> | (lit<"*"> + option(lit<"::"> + lit<"**">))))> {
  };

  struct ImportItem
      : rule<create<ast::Element>(
            many(append<&ast::Element::flags>(VisibilityFlags)) +
            assign<&ast::Element::kind>(kw<"import">) +
            option(append<&ast::Element::flags>(kw<"all">)) +
            append<&ast::Element::ends>(ref<ImportTarget>) + ElementBody)> {
  };

  // `expose` shares an import target's shape (`Ns::*`, `Ns::**`).
  struct ExposeItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"expose">) +
            append<&ast::Element::ends>(ref<ImportTarget>) + ElementBody)> {
  };

  // `library package Foo { }`: kind stays "package" (a library package is a
  // package), `library`/`standard` become flags, so a reader filtering by
  // `kind == "package"` sees both alike.
  struct LibraryPackageItem
      : rule<create<ast::Element>(
            option(append<&ast::Element::flags>(kw<"standard">)) +
            append<&ast::Element::flags>(kw<"library">) +
            many(lit<"#"> +
                 append<&ast::Element::metaPrefixes>(ref<Chain>)) +
            assign<&ast::Element::kind>(kw<"package">) +
            option(Identification) + ElementBody)> {};

  // The nine standalone relationship statements (`specialization S subtype
  // A :> B;`, `conjugation ... conjugate A ~ B;`, `disjoining ... disjoint
  // A from B;`, `subclassification`, `subsetting`, the standalone
  // `featureTyping`, `typeFeaturing`, `featureInverting`): `kind` is the
  // verb (subtype/subclassifier/subset/redefinition/typing/conjugate/
  // disjoint/featuring/inverse), `name` the first chain, and the second
  // chain lands in `specs` when SysML v2 joins it with an operator
  // `SpecItem` already reads (`:>`, `specializes`, `~`,
  // `conjugates`, `subsets`, `redefines`, `:`, `typed by`), in `ends`
  // when it joins with `from`/`by`/`of` instead. The relationship's own
  // optional identification (rare) lands in `shortName`, since `name`
  // already holds the first chain.
  struct RelationshipItem
      : rule<create<ast::Element>(
            option((kw<"specialization"> | kw<"conjugation"> |
                    kw<"disjoining"> | kw<"inverting">) +
                   option(assign<&ast::Element::shortName>(ref<Name>))) +
            ((assign<&ast::Element::kind>(kw<"subtype">) +
              assign<&ast::Element::name>(ref<Chain>) +
              append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"subclassifier">) +
                assign<&ast::Element::name>(ref<Chain>) +
                append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"subset">) +
                assign<&ast::Element::name>(ref<Chain>) +
                append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"redefinition">) +
                assign<&ast::Element::name>(ref<Chain>) +
                append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"typing">) +
                assign<&ast::Element::name>(ref<Chain>) +
                append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"conjugate">) +
                assign<&ast::Element::name>(ref<Chain>) +
                append<&ast::Element::specs>(ref<SpecItem>))  //
             | (assign<&ast::Element::kind>(kw<"disjoint">) +
                assign<&ast::Element::name>(ref<Chain>) + kw<"from"> +
                append<&ast::Element::ends>(ref<Chain>))  //
             | (assign<&ast::Element::kind>(kw<"featuring">) +
                option(assign<&ast::Element::name>(ref<Chain>) +
                       kw<"of">) +
                append<&ast::Element::ends>(ref<Chain>) + kw<"by"> +
                append<&ast::Element::ends>(ref<Chain>))  //
             | (assign<&ast::Element::kind>(kw<"inverse">) +
                assign<&ast::Element::name>(ref<Chain>) + kw<"of"> +
                append<&ast::Element::ends>(ref<Chain>))) +
            ElementBody)> {};

  struct AliasItem
      : rule<create<ast::Element>(
            many(append<&ast::Element::flags>(VisibilityFlags)) +
            assign<&ast::Element::kind>(kw<"alias">) +
            option(lit<"<"> + assign<&ast::Element::shortName>(ref<Name>) +
                   lit<">">) +
            option(assign<&ast::Element::name>(ref<Name>)) + kw<"for"> +
            append<&ast::Element::ends>(ref<Chain>) + ElementBody)> {};

  struct DependencyItem
      : rule<create<ast::Element>(
            many(lit<"#"> +
                 append<&ast::Element::metaPrefixes>(ref<Chain>)) +
            assign<&ast::Element::kind>(kw<"dependency">) +
            option(Identification) + option(kw<"from">) + ChainCSV +
            kw<"to"> + ChainCSV + ElementBody)> {};

  static constexpr auto About =
      option(kw<"about"> + ChainCSV);

  struct CommentItem
      : rule<create<ast::Element>(
            option(assign<&ast::Element::kind>(kw<"comment">) +
                   option(assign<&ast::Element::name>(ref<Name>)) +
                   About) +
            option(kw<"locale"> +
                   assign<&ast::Element::lang>(ref<DOUBLE_STRING>)) +
            assign<&ast::Element::text>(ref<REGULAR_COMMENT>))> {};

  struct DocItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"doc">) +
            option(assign<&ast::Element::name>(ref<Name>)) +
            option(kw<"locale"> +
                   assign<&ast::Element::lang>(ref<DOUBLE_STRING>)) +
            assign<&ast::Element::text>(ref<REGULAR_COMMENT>))> {};

  struct RepItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"rep">) +
            option(assign<&ast::Element::name>(ref<Name>)) +
            kw<"language"> +
            assign<&ast::Element::lang>(ref<DOUBLE_STRING>) +
            assign<&ast::Element::text>(ref<REGULAR_COMMENT>))> {};

  // `first x;` names an action's initial node; `then y;` a target
  // succession from the (here implicit) node before it -- both terminated
  // like any other member, never combined into one statement here (the
  // combined `first a then b;` form is a documented gap).
  struct FirstItem : rule<create<ast::Element>(
                         assign<&ast::Element::kind>(kw<"first">) +
                         append<&ast::Element::ends>(ref<Chain>) +
                         option(kw<"then"> + append<&ast::Element::ends>(ref<Chain>)) +
                         ElementBody)> {};
  struct ThenItem
      : rule<create<ast::Element>(assign<&ast::Element::kind>(kw<"then">) +
                                  append<&ast::Element::ends>(ref<Chain>) +
                                  ElementBody)> {};

  // `if`/`while`/`for` carry a condition (or loop variable) the generic
  // `DeclItem` shape has no slot for, so each gets its own small rule.
  // `loop` (a condition-less `while`) and a guarded `until` clause are not
  // ported.
  struct IfItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"if">) +
            assign<&ast::Element::value>(ref<OwnedExpression>) + lit<"{"> +
            many(append<&ast::Element::members>(ref<BodyItem>)) + lit<"}"> +
            option(kw<"else"> +
                   (assign<&ast::Element::alt>(ref<IfItem>) |
                    assign<&ast::Element::alt>(create<ast::Element>(
                        lit<"{"> +
                        many(append<&ast::Element::members>(
                            ref<BodyItem>)) +
                        lit<"}">)))))> {};

  struct WhileItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"while">) +
            assign<&ast::Element::value>(ref<OwnedExpression>) + lit<"{"> +
            many(append<&ast::Element::members>(ref<BodyItem>)) +
            lit<"}">)> {};

  struct ForItem
      : rule<create<ast::Element>(
            assign<&ast::Element::kind>(kw<"for">) +
            assign<&ast::Element::name>(ref<Name>) + kw<"in"> +
            assign<&ast::Element::value>(ref<OwnedExpression>) + lit<"{"> +
            many(append<&ast::Element::members>(ref<BodyItem>)) +
            lit<"}">)> {};

  struct BodyItem
      : rule<ref<ImportItem> | ref<ExposeItem> | ref<AliasItem> | ref<DependencyItem> |
             ref<DocItem> | ref<RepItem> | ref<CommentItem> | ref<FirstItem> |
             ref<ThenItem> | ref<IfItem> | ref<WhileItem> | ref<ForItem> |
             ref<LibraryPackageItem> | ref<RelationshipItem> | ref<BindItem> |
             ref<ConnectItem> | ref<DeclItem>> {};

  struct RootNamespace
      : rule<create<ast::RootNamespace>(
            many(append<&ast::RootNamespace::members>(ref<BodyItem>)))> {};

  using entry = RootNamespace;
  using ignore = WS;
  using hidden = list<SL_COMMENT>;
};

}  // namespace sysmlv2
