// The scss language: its grammar. `ComponentValueAlt` is the CSS/Sass value
// family (also selector fragments, which share it -- see ast.hpp); `Value`
// is the operator-chain built over it. `Block` is one production reused for
// every `{ ... }` body, function bodies included.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "scss/core/ast.hpp"

namespace scss {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

inline constexpr auto drop_parent = [](std::string_view t) {
  return t.substr(1);
};
using AfterParent = cursiv::text_conv<drop_parent>;

struct Scss : cursiv::GrammarSpec {
  // `-` is a legal identifier byte throughout CSS/Sass (`font-size`,
  // `$my-var`), so the keyword boundary must include it.
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct ML_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct SL_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct SPACE : terminal<some(cr<R"( \t\r\n\f)">)> {};

  static constexpr auto IdentEscape = lit<"\\"> + any;
  static constexpr auto IdentStart =
      ~cr<R"(\x00-@[-^`{-\x7f)"> | IdentEscape;
  static constexpr auto IdentCont =
      ~cr<R"(\x00-,.-/:-@[-^`{-\x7f)"> |
      IdentEscape;
  struct IDENT : terminal<option(lit<"-">) + IdentStart + many(IdentCont)> {};

  // A custom property's name run: an identifier byte, or an interpolated
  // segment (`--#{$prefix}-color`), narrow enough (`$?` then plain `Ident`,
  // no arbitrary value) to stay inside one terminal.
  static constexpr auto CustomPropFragment =
      lit<"#"> + lit<"{"> + option(lit<"$">) + IdentStart + many(IdentCont) +
      lit<"}">;
  struct CUSTOM_PROP_NAME
      : terminal<(CustomPropFragment | IdentStart) +
                 many(CustomPropFragment | IdentCont)> {};

  static constexpr auto NumberBody =
      (some(d) | (many(d) + lit<"."> + some(d))) +
      option(cr<"eE"> + option(cr<R"(+-)">) + some(d));
  struct NUMBER : terminal<NumberBody, double> {};

  // A unit is any identifier glued to the number (`1in`, `2dvh`, `1x`): never
  // `--` first, never `e` before a digit or a sign (an exponent), and a `-`
  // before a digit or `.` ends it (`1px-2px`).
  static constexpr auto UnitStart =
      ~cr<R"(\x00-@E[-^`e{-\x7f)"> |
      IdentEscape | lit<"e", !cr<R"(0-9+-)">> | lit<"E", !cr<R"(0-9+-)">>;
  static constexpr auto UnitCont =
      ~cr<R"(\x00-/:-@[-^`{-\x7f)"> |
      IdentEscape | lit<"-", !cr<".0-9">>;
  struct DIMENSION
      : terminal<NumberBody + option(lit<"-">) + UnitStart + many(UnitCont)> {};
  struct PERCENTAGE : terminal<NumberBody + lit<"%">> {};

  // `#fff`, `#container`, an interpolation's opener: one fused terminal, so
  // a hex-only class never competes with a wider identifier class for the
  // same `#` prefix and loses the shorter match on a name like `#container`.
  struct HASH : terminal<lit<"#"> + some(IdentCont)> {};

  // `&` and its suffix are one token: the suffix is any identifier body
  // (`&-2`, `&--on`, `&-` before `#{`), and `& b` stays a descendant.
  struct PARENT_SUFFIX : terminal<lit<"&"> + some(IdentCont), AfterParent> {};

  static constexpr auto StringBody1 =
      many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">);
  static constexpr auto StringBody2 =
      many((lit<"\\"> + any) | cr<R"(^'\\\r\n)">);
  struct STRING
      : terminal<(lit<"\""> + StringBody1 + lit<"\"">) |
                     (lit<"'"> + StringBody2 + lit<"'">),
                 Unquoted> {};

  // Unquoted `url(...)` content: admits embedded whitespace, needed for
  // `url(hello world)`; narrowed to ASCII, an unquoted non-ASCII URL being
  // vanishingly rare.
  struct URL_RAW
      : terminal<some(cr<R"(!#$%&*-~ \t\r\n\f)"> | IdentEscape)> {};

  // An unknown at-rule's value, raw up to its block or its end: strings,
  // escapes and loud comments whole, `#{` and `//` left to the parser, no
  // blank at either end.
  static constexpr auto AtValueChar =
      cr<R"(^{};#"'/\\ \t\r\n\f)"> | IdentEscape |
      (lit<"\""> + StringBody1 + lit<"\"">) |
      (lit<"'"> + StringBody2 + lit<"'">) | lit<"#", !cr<"{">> |
      lit<"/", !cr<"/*">> | through(lit<"/*">, lit<"*/">);
  struct AT_VALUE
      : terminal<AtValueChar + many(many(cr<R"( \t\r\n\f)">) + AtValueChar)> {};

  struct IMPORTANT_TOK
      : terminal<lit<"!"> + many(cr<R"( \t\r\n\f)"> | through(lit<"/*">, lit<"*/">)) +
                     kw<"important">,
                 value<ast::Prio::important>> {};
  struct DEFAULT_TOK
      : terminal<lit<"!default">, value<ast::Prio::default_>> {};
  static constexpr auto PrioAlt = ref<IMPORTANT_TOK> | ref<DEFAULT_TOK>;

  struct TRUE_TOK : terminal<kw<"true">, value<true>> {};
  struct FALSE_TOK : terminal<kw<"false">, value<false>> {};

  struct OP_GE : terminal<lit<">=">, value<ast::Op::greater_eq>> {};
  struct OP_GT : terminal<lit<">">, value<ast::Op::greater>> {};
  struct OP_LE : terminal<lit<"<=">, value<ast::Op::less_eq>> {};
  struct OP_LT : terminal<lit<"<">, value<ast::Op::less>> {};
  struct OP_EQ
      : terminal<lit<"=="> | lit<"=">, value<ast::Op::eq>> {};
  struct OP_NEQ : terminal<lit<"!=">, value<ast::Op::not_equal>> {};
  struct OP_DIV : terminal<lit<"/">, value<ast::Op::divide>> {};
  struct OP_TIMES : terminal<lit<"*">, value<ast::Op::times>> {};
  struct OP_MINUS : terminal<lit<"-">, value<ast::Op::minus>> {};
  struct OP_PLUS : terminal<lit<"+">, value<ast::Op::plus>> {};
  struct OP_AND : terminal<kw<"and">, value<ast::Op::and_>> {};
  struct OP_OR : terminal<kw<"or">, value<ast::Op::or_>> {};

  struct PREFIX_MATCH : terminal<lit<"^=">, value<ast::AttribOp::prefix>> {};
  struct SUFFIX_MATCH : terminal<lit<"$=">, value<ast::AttribOp::suffix>> {};
  struct SUBSTRING_MATCH
      : terminal<lit<"*=">, value<ast::AttribOp::substring>> {};
  struct ATTR_INCLUDES
      : terminal<lit<"~=">, value<ast::AttribOp::includes>> {};
  struct ATTR_DASHMATCH
      : terminal<lit<"|=">, value<ast::AttribOp::dashmatch>> {};
  struct ATTR_EQ : terminal<lit<"=">, value<ast::AttribOp::equals>> {};
  static constexpr auto AttribOpAlt =
      ref<PREFIX_MATCH> | ref<SUFFIX_MATCH> | ref<SUBSTRING_MATCH> |
      ref<ATTR_INCLUDES> | ref<ATTR_DASHMATCH> | ref<ATTR_EQ>;

  struct COMBINE_CHILD
      : terminal<lit<">">, value<ast::Combinator::child>> {};
  struct COMBINE_ADJ
      : terminal<lit<"+">, value<ast::Combinator::adjacent_sibling>> {};
  struct COMBINE_SIB
      : terminal<lit<"~">, value<ast::Combinator::general_sibling>> {};
  static constexpr auto CombinatorAlt =
      ref<COMBINE_CHILD> | ref<COMBINE_ADJ> | ref<COMBINE_SIB>;

  struct IMPORT_KW
      : terminal<lit<"@import">, value<ast::ImportKind::import_>> {};
  struct REQUIRE_KW
      : terminal<lit<"@require">, value<ast::ImportKind::require>> {};
  struct USE_KW : terminal<lit<"@use">, value<ast::ImportKind::use>> {};
  struct FORWARD_KW
      : terminal<lit<"@forward">, value<ast::ImportKind::forward>> {};

  // A name the language also uses as a fixed keyword elsewhere (`@for`'s
  // `from`/`to`): spelled once, as the soft-keyword idiom asks.
  static constexpr auto IdentLike = ref<IDENT> | kw<"from"> | kw<"to">;

  // Forward declarations: the value/selector/statement families are
  // mutually recursive (a function call argument is a value, a value can be
  // a call; a block holds statements, a statement can hold a block).
  struct ComponentValueAlt;
  struct ExpressionRule;
  struct InterpolationRule;
  struct IfExprRule;
  struct FunctionCallRule;
  struct ListValueRule;
  struct NestedListItem;
  struct MapDeclarationRule;
  struct ParameterRule;
  struct TypeRefRule;
  struct PseudoSelectorRule;
  struct ClassSelectorRule;
  struct AttribSelectorRule;
  struct IdSelectorRule;
  struct BlockRule;
  struct RulesetRule;
  struct MediaQueryRule;
  struct FunctionBlockRule;
  struct StatementAlt;

  static constexpr auto Value = ref<ExpressionRule>;

  // A value slot that may also be a bare list or a map literal: a
  // comma-separated argument may itself be a space-separated run
  // (`linear-gradient(to bottom right, ...)`), a variable's value, a
  // parameter's default, a map's own key/value.
  static constexpr auto ValueOrListOrMap =
      ref<MapDeclarationRule> | ref<ListValueRule> | Value;


  // -- Leaf values ------------------------------------------------------

  struct NumberLitRule
      : rule<option(flag<&ast::NumberLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::NumberLit::magnitude>(ref<NUMBER>)> {};

  struct PercentageLitRule
      : rule<option(flag<&ast::PercentageLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::PercentageLit::text>(ref<PERCENTAGE>)> {};

  struct DimensionLitRule
      : rule<option(flag<&ast::DimensionLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::DimensionLit::text>(ref<DIMENSION>)> {};

  struct StringLitRule : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct BooleanLitRule
      : rule<assign<&ast::BooleanLit::value>(ref<TRUE_TOK> |
                                              ref<FALSE_TOK>)> {};

  struct IdentValueRule : rule<assign<&ast::IdentValue::name>(IdentLike)> {};

  struct HexColorLitRule
      : rule<assign<&ast::HexColorLit::text>(ref<HASH>)> {};

  // `$name`, `ns.$name`, a signed `+$name`/`-$name`, or `--custom-prop`.
  struct VariableRefRule
      : rule<(flag<&ast::VariableRef::isCustomProperty>(lit<"--">) +
              assign<&ast::VariableRef::name>(ref<CUSTOM_PROP_NAME>)) |
             (option(flag<&ast::VariableRef::negative>(lit<"-">) |
                     lit<"+">) +
              option(assign<&ast::VariableRef::nsPrefix>(ref<IDENT>) + lit<".">) +
              lit<"$"> + assign<&ast::VariableRef::name>(IdentLike))> {};

  struct ParentRefRule : rule<create<ast::ParentRef>(lit<"&">)> {};

  // A leading `not` on one operand; wraps only when the word is present.
  struct NotExprRule
      : rule<create<ast::NotExpr>(
            kw<"not"> +
            assign<&ast::NotExpr::operand>(ref<ComponentValueAlt>))> {};

  // An item after a blank never opens on a sign the item before takes as
  // its operator: `+` never, `-` only before a name, or before a number with
  // a blank in front of it (dart-sass, `_expression`).
  static constexpr auto NameFollow =
      ~cr<R"(\x00-,.-@\[\]^`{-\x7f)">;
  static constexpr auto SpaceNext =
      !lit<"+"> + ((!lit<"-", !NameFollow>) |
                   (!gap::none + &lit<"-", !cr<"^.0-9">>));

  static constexpr auto ListComma =
      flag<&ast::ListValue::commaSeparated>(lit<",">);
  static constexpr auto ListItem =
      append<&ast::ListValue::items>(ref<NestedListItem>);
  static constexpr auto NextListItem =
      (ListComma + ListItem) | (SpaceNext + ListItem);
  static constexpr auto ListTail = many(NextListItem) + option(ListComma);
  static constexpr auto BracketList = create<ast::ListValue>(
      flag<&ast::ListValue::bracketed>(lit<"[">) + ListItem + ListTail +
      lit<"]">);
  // `(a)` is a value in parens: a list there has a comma or a second item.
  static constexpr auto ParenList = create<ast::ListValue>(
      lit<"("> + ListItem +
      ((ListComma + option(ListItem + ListTail)) |
       (SpaceNext + ListItem + ListTail)) +
      lit<")">);

  struct ParenGroupRule
      : rule<create<ast::ParenGroup>(
            option(flag<&ast::ParenGroup::negative>(lit<"-">) |
                   lit<"+">) +
            lit<"("> +
            option(assign<&ast::ParenGroup::inner>(Value | BracketList |
                                                   ParenList)) +
            lit<")">)> {};

  // -- Calls --------------------------------------------------------------

  struct CalcExprRule
      : rule<create<ast::CalcExpr>(
            kw<"calc"> + lit<"("> + assign<&ast::CalcExpr::sum>(Value) +
            lit<")">)> {};

  // rgba() keeps its own shape: args may run with no separating comma
  // (`rgba(200 200 200 / 0.3)`), which a generic call never allows.
  struct RgbaCallRule
      : rule<create<ast::RgbaCall>(
            kw<"rgba"> + lit<"("> + append<&ast::RgbaCall::args>(Value) +
            many(option(lit<",">) +
                 append<&ast::RgbaCall::args>(Value)) +
            lit<")">)> {};

  struct RepeatCallRule
      : rule<create<ast::RepeatCall>(
            kw<"repeat"> + lit<"("> +
            assign<&ast::RepeatCall::value>(Value) + lit<","> +
            assign<&ast::RepeatCall::count>(ref<NumberLitRule>) +
            assign<&ast::RepeatCall::freqUnit>(kw<"hz"> | kw<"khz"> |
                                                kw<"fr">) +
            lit<")">)> {};

  struct UrlRefRule
      : rule<create<ast::UrlRef>(
            kw<"url"> + lit<"("> +
            assign<&ast::UrlRef::value>(ref<STRING> | ref<URL_RAW>) +
            lit<")">)> {};

  struct FormatRefRule
      : rule<create<ast::FormatRef>(
            kw<"format"> + lit<"("> +
            assign<&ast::FormatRef::value>(ref<STRING>) + lit<")">)> {};

  // A generic call: `ns.name(args)`. Also covers `var()`/`rotate()`, whose
  // arguments already fit this shape once per-state lexing removes the need
  // for merged lookahead tokens.
  struct FunctionCallRule
      : rule<option(assign<&ast::FunctionCall::nsPrefix>(ref<IDENT>) + lit<".">) +
             assign<&ast::FunctionCall::name>(IdentLike) + gap::none +
             lit<"("> + many(append<&ast::FunctionCall::args>(ref<ParameterRule>) + option(lit<",">)) +
             lit<")">> {};

  struct PrioValueRule
      : rule<create<ast::PrioValue>(
            flag<&ast::PrioValue::isDefault>(ref<DEFAULT_TOK>) |
            ref<IMPORTANT_TOK>)> {};

  static constexpr auto ValueOrPrio = Value | ref<PrioValueRule>;

  struct IfExprRule
      : rule<create<ast::IfExpr>(
            kw<"if"> + lit<"("> +
            assign<&ast::IfExpr::condition>(ValueOrPrio) + lit<","> +
            assign<&ast::IfExpr::thenValue>(ValueOrPrio) + lit<","> +
            assign<&ast::IfExpr::elseValue>(ValueOrPrio) + lit<")"> +
            option(assign<&ast::IfExpr::trailingUnit>(ref<IDENT>)) +
            option(assign<&ast::IfExpr::prio>(PrioAlt)))> {};

  // `#{ ... }`; a unit or a bare `%` glued right after `}` (`#{$v}px`,
  // `#{$i * 10}%` -- the keyframe-step idiom) reads as an ordinary
  // identifier or a literal `%` rather than through a dedicated unit
  // keyword, since that spot never competes with one.
  // `gap::none`: the gluing is the point -- a space leaves it a separate
  // token instead.
  struct InterpolationRule
      : rule<option(assign<&ast::Interpolation::nsPrefix>(ref<IDENT>) + lit<".">) +
             lit<"#"> + lit<"{"> +
             option(assign<&ast::Interpolation::innerNsPrefix>(ref<IDENT>) + lit<".">) +
             assign<&ast::Interpolation::inner>(Value) + lit<"}"> +
             option(gap::none +
                    assign<&ast::Interpolation::trailingUnit>(ref<IDENT> |
                                                               lit<"%">))> {
  };

  // A name glued to `(` is a call: read as a name then a group, it never
  // wins.
  struct ComponentValueAlt
      : rule<ref<NotExprRule> | ref<NumberLitRule> | ref<PercentageLitRule> |
             ref<DimensionLitRule> | ref<BooleanLitRule> |
             ref<HexColorLitRule> | ref<VariableRefRule> |
             ref<InterpolationRule> | ref<CalcExprRule> | ref<RgbaCallRule> |
             ref<RepeatCallRule> | ref<UrlRefRule> | ref<FormatRefRule> |
             ref<IfExprRule> | ref<FunctionCallRule> | ref<StringLitRule> |
             ref<ParenGroupRule> | ref<ParentRefRule> |
             (ref<IdentValueRule> + !(gap::none + &lit<"(">))> {};

  // `a -1` is a list of two: behind a blank, a `-` glued to a number signs it.
  struct MinusOp
      : rule<(gap::none | !lit<"-", !cr<"^.0-9">>) + ref<OP_MINUS>,
             value<ast::Op::minus>> {};

  // multiplicative > additive > relational > equality > and > or (Sass,
  // "Operators", order of operations) -- unary `not`/sign stay ad hoc
  // per-rule (`Not? expressionPart`, `VariableRefRule`, `ParenGroupRule`),
  // matching the source grammar, which has no general unary tier either.
  struct ExpressionRule
      : infix<ComponentValueAlt, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<OP_DIV> | ref<OP_TIMES>) >>
               left(ref<MinusOp> | ref<OP_PLUS>) >>
               left(ref<OP_GE> | ref<OP_GT> | ref<OP_LE> | ref<OP_LT>) >>
               left(ref<OP_EQ> | ref<OP_NEQ>) >>
               left(ref<OP_AND>) >>
               left(ref<OP_OR>))> {};

  // -- Lists and maps -------------------------------------------------

  // An item inside a list: a nested bracketed/parenthesised list, or a
  // plain value. Excludes the bare (unwrapped) list form to keep this
  // left-recursion-free -- only the outermost list may be unwrapped.
  struct NestedListItem : rule<BracketList | ParenList | Value> {};

  // A bare, unwrapped list needs 2+ items: with exactly one, `Value` alone
  // already matches, and requiring a second item here keeps the two from
  // ever competing over the same input.
  struct BareListValueRule
      : rule<create<ast::ListValue>(ListItem + some(NextListItem))> {};

  struct ListValueRule
      : rule<BracketList | ParenList | ref<BareListValueRule>> {};

  // `ValueOrListOrMap`, not `NestedListItem`: a key or a value here may be a
  // single plain value, a bare space-separated run (`border: 1px solid
  // $color`), or a nested map/list -- and `MapEntryRule` is never called
  // from inside `ListValueRule`'s own item loop, so its bare fallback
  // carries no recursion risk here.
  struct MapEntryRule
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ValueOrListOrMap) + lit<":"> +
            assign<&ast::MapEntry::value>(ValueOrListOrMap))> {};

  struct MapDeclarationRule
      : rule<create<ast::MapValue>(
            lit<"("> + append<&ast::MapValue::entries>(ref<MapEntryRule>) +
            many(lit<","> + append<&ast::MapValue::entries>(ref<MapEntryRule>)) +
            option(lit<",">) + lit<")">)> {};

  // -- Selectors ----------------------------------------------------------
  // Every fragment below shares `ComponentValue` (ast.hpp), so a functional
  // pseudo's argument list can hold either an ordinary value
  // (`:nth-child(2n+1)`) or a selector fragment (`:not([disabled])`) with no
  // separate shape needed for either.

  struct IdSelectorRule
      : rule<assign<&ast::IdSelector::name>(ref<HASH>)> {};

  // A lone `-` before an interpolation of one name (`.#{$a}-#{$b}`).
  static constexpr auto DashName =
      create<ast::IdentValue>(assign<&ast::IdentValue::name>(lit<"-">));

  static constexpr auto ClassNamePart =
      ref<IdentValueRule> | ref<InterpolationRule> | DashName;

  struct ClassSelectorRule
      : rule<create<ast::ClassSelector>(
            lit<"."> + append<&ast::ClassSelector::parts>(ClassNamePart) +
            many(gap::none +
                 append<&ast::ClassSelector::parts>(ClassNamePart)))> {};

  struct AttribSelectorRule
      : rule<create<ast::AttribSelector>(
            lit<"["> +
            option(option(assign<&ast::AttribSelector::nsPrefix>(
                              IdentLike | lit<"*">)) +
                   flag<&ast::AttribSelector::hasNsPrefix>(lit<"|">)) +
            assign<&ast::AttribSelector::name>(IdentLike) +
            option(assign<&ast::AttribSelector::op>(AttribOpAlt) +
                   assign<&ast::AttribSelector::value>(ref<IDENT> |
                                                        ref<STRING>)) +
            lit<"]">)> {};

  static constexpr auto IdentOrVarAlt =
      ref<IdentValueRule> | ref<VariableRefRule>;

  struct TypeRefRule
      : rule<create<ast::TypeRef>(
            option(option(assign<&ast::TypeRef::nsPrefix>(IdentLike |
                                                            lit<"*">)) +
                   flag<&ast::TypeRef::hasNsPrefix>(lit<"|">)) +
            ((option(flag<&ast::TypeRef::isPlaceholder>(lit<"%">) |
                     (flag<&ast::TypeRef::hasParentRefPrefix>(lit<"&">) +
                      gap::none)) +
              (flag<&ast::TypeRef::universal>(lit<"*">) |
               assign<&ast::TypeRef::name>(IdentOrVarAlt))) |
             flag<&ast::TypeRef::hasParentRefPrefix>(
                 assign<&ast::TypeRef::name>(create<ast::IdentValue>(
                     assign<&ast::IdentValue::name>(ref<PARENT_SUFFIX>))))))> {};

  static constexpr auto PseudoArgAlt =
      ref<AttribSelectorRule> | ref<TypeRefRule> | ref<IdSelectorRule> |
      ref<PseudoSelectorRule> | Value;

  // A class reads the interpolations glued to it, any other argument keeps
  // them as its own: an argument glued to the one before never opens on `#{`.
  static constexpr auto PseudoArg =
      append<&ast::PseudoSelector::args>(ref<ClassSelectorRule>) |
      (append<&ast::PseudoSelector::args>(PseudoArgAlt) +
       many(gap::none +
            append<&ast::PseudoSelector::args>(ref<InterpolationRule>)));
  static constexpr auto PseudoArgSeparator =
      lit<","> | !gap::none | !lit<"#"> | &lit<"#", !cr<"{">>;

  // Glued to its colon: `a: b` is never a pseudo-class.
  struct PseudoSelectorRule
      : rule<create<ast::PseudoSelector>(
            lit<":"> +
            option(gap::none +
                   flag<&ast::PseudoSelector::isElement>(lit<":">)) +
            gap::none +
            (assign<&ast::PseudoSelector::interp>(ref<InterpolationRule>) |
             (assign<&ast::PseudoSelector::name>(ref<IDENT>) +
              option(flag<&ast::PseudoSelector::isFunctional>(
                         gap::none + lit<"(">) +
                     PseudoArg + many(PseudoArgSeparator + PseudoArg) +
                     option(lit<",">) + lit<")">))))> {};

  // `VariableRefRule` here: a bare `$name` glued right after another item
  // with no separator (`#{$sel}$y`) is its own sequence item, not folded
  // into the interpolation before it.
  static constexpr auto SelectorItemAlt =
      ref<IdSelectorRule> | ref<AttribSelectorRule> | ref<PseudoSelectorRule> |
      ref<ParentRefRule> | ref<VariableRefRule>;

  // A name's interpolations and lone `-` glued after an item. A class reads
  // its own, so none starts the item after a class: `.a#{$b}` is one class.
  static constexpr auto NameFragments =
      many(gap::none + append<&ast::SimpleSelectorSequence::items>(
                           ref<InterpolationRule> | DashName));

  static constexpr auto SelectorUnit =
      append<&ast::SimpleSelectorSequence::items>(ref<ClassSelectorRule>) |
      (append<&ast::SimpleSelectorSequence::items>(SelectorItemAlt) +
       NameFragments);

  // A gap (whitespace) between a leading type and the next item, or between
  // two items, starts a NEW sequence (implicit descendant combinator): this
  // is what tells `body.cls` (one compound selector) apart from `body .cls`
  // (two).
  struct SimpleSelectorSequenceRule
      : rule<create<ast::SimpleSelectorSequence>(
            option(assign<&ast::SimpleSelectorSequence::combinator>(
                CombinatorAlt)) +
            ((assign<&ast::SimpleSelectorSequence::type>(ref<TypeRefRule>) +
              NameFragments) |
             (option(append<&ast::SimpleSelectorSequence::items>(DashName) +
                     gap::none) +
              append<&ast::SimpleSelectorSequence::items>(
                  ref<InterpolationRule>) +
              NameFragments) |
             SelectorUnit) +
            many(gap::none + SelectorUnit))> {};

  // A sequence glued to the one before it is never a descendant.
  struct SelectorRule
      : rule<create<ast::Selector>(
            append<&ast::Selector::sequences>(ref<SimpleSelectorSequenceRule>) +
            many(((!gap::none) | &cr<">+~">) +
                 append<&ast::Selector::sequences>(
                     ref<SimpleSelectorSequenceRule>)))> {};

  struct SelectorGroupRule
      : rule<create<ast::SelectorGroup>(
            append<&ast::SelectorGroup::selectors>(ref<SelectorRule>) +
            many(lit<","> +
                 append<&ast::SelectorGroup::selectors>(ref<SelectorRule>)))> {
  };

  // -- Parameters -----------------------------------------------------

  // A signature or call-site argument: `$image`, `$x: 50%`, `$rest...`.
  struct ParameterRule
      : rule<create<ast::Parameter>(
            assign<&ast::Parameter::value>(ValueOrListOrMap) +
            option(lit<":"> +
                   assign<&ast::Parameter::defaultValue>(ValueOrListOrMap)) +
            option(flag<&ast::Parameter::isRest>(lit<"...">)) +
            option(assign<&ast::Parameter::prio>(PrioAlt)))> {};

  // -- Declarations -----------------------------------------------------

  // A statement without a block ends at `;`, implied before `}` or at the
  // end: never optional, or `a:b, c {}` reads as a declaration then a rule,
  // and an empty statement `;` as the end of the one before.
  static constexpr auto StatementEnd = implied(lit<";">, &lit<"}"> | eof);
  // Looser for `@import` and `@include`, which the corpus ends at a line break.
  static constexpr auto LineStatementEnd =
      implied(lit<";">, gap::line | &lit<"}"> | eof);

  // Runs and interpolations glued after a declaration's first fragment
  // (`margin-#{$a}-#{$b}`).
  template <auto SuffixMember>
  static constexpr auto NameSuffix =
      many(gap::none +
           append<SuffixMember>(ref<InterpolationRule> | DashName));

  struct VariableDeclarationRule
      : rule<((flag<&ast::VariableDeclaration::isCustomProperty>(
                   lit<"--">) +
               assign<&ast::VariableDeclaration::name>(
                   ref<CUSTOM_PROP_NAME>)) |
              (option(flag<&ast::VariableDeclaration::negative>(
                          lit<"-">) |
                      lit<"+">) +
               option(assign<&ast::VariableDeclaration::nsPrefix>(ref<IDENT>) + lit<".">) +
               lit<"$"> +
               assign<&ast::VariableDeclaration::name>(IdentLike))) +
             lit<":"> +
             assign<&ast::VariableDeclaration::value>(ValueOrListOrMap) +
             option(assign<&ast::VariableDeclaration::prio>(PrioAlt)) +
             StatementEnd> {};

  // A value glued to its `:` that opens like a name could be a pseudo-class
  // (`a:hover`): that declaration takes no nested block, so `a:hover {` is a
  // style rule. The two conditions are each other's complement.
  static constexpr auto NameStart = IdentStart | (lit<"-"> + IdentStart);
  static constexpr auto GluedName =
      gap::none + (&NameStart | (&lit<"#"> + !lit<"#", !cr<"{">>));
  static constexpr auto NotGluedName =
      (!gap::none) | (!NameStart + ((!lit<"#">) | &lit<"#", !cr<"{">>));

  template <auto ValueMember>
  static constexpr auto ValueRun =
      append<ValueMember>(Value) +
      many((lit<","> + append<ValueMember>(Value)) |
           (SpaceNext + append<ValueMember>(Value))) +
      option(lit<",">);

  template <auto ValueMember, auto PrioMember, auto NestedMember>
  static constexpr auto DeclarationBody =
      (GluedName + ValueRun<ValueMember> +
       option(assign<PrioMember>(PrioAlt)) + StatementEnd) |
      (NotGluedName + ValueRun<ValueMember> +
       option(assign<PrioMember>(PrioAlt)) +
       (assign<NestedMember>(ref<BlockRule>) | StatementEnd)) |
      assign<NestedMember>(ref<BlockRule>) |
      (option(assign<PrioMember>(PrioAlt)) + StatementEnd);

  struct PropertyDeclarationRule
      : rule<assign<&ast::PropertyDeclaration::property>(IdentLike) +
             NameSuffix<&ast::PropertyDeclaration::suffix> + lit<":"> +
             DeclarationBody<&ast::PropertyDeclaration::value,
                             &ast::PropertyDeclaration::prio,
                             &ast::PropertyDeclaration::nestedBlock>> {};

  struct InterpolationDeclarationRule
      : rule<option(flag<&ast::InterpolationDeclaration::leadingDash>(
                        lit<"-">) +
                    gap::none) +
             assign<&ast::InterpolationDeclaration::key>(
                 ref<InterpolationRule>) +
             NameSuffix<&ast::InterpolationDeclaration::suffix> + lit<":"> +
             DeclarationBody<&ast::InterpolationDeclaration::value,
                             &ast::InterpolationDeclaration::prio,
                             &ast::InterpolationDeclaration::nestedBlock>> {};

  // -- Import family ----------------------------------------------------

  static constexpr auto ImportMedia =
      some(append<&ast::ImportDeclaration::media>(ref<MediaQueryRule>),
           lit<",">);

  struct ImportDeclarationRule
      : rule<assign<&ast::ImportDeclaration::kind>(
                 ref<IMPORT_KW> | ref<REQUIRE_KW> | ref<USE_KW> |
                 ref<FORWARD_KW>) +
             assign<&ast::ImportDeclaration::path>(ref<UrlRefRule> |
                                                    ref<StringLitRule>) +
             many(lit<","> + append<&ast::ImportDeclaration::morePaths>(
                                 ref<UrlRefRule> | ref<StringLitRule>)) +
             option(!gap::line +
                    ((some(append<&ast::ImportDeclaration::modifiers>(
                          ref<FunctionCallRule>)) +
                      option(ImportMedia)) |
                     ImportMedia)) +
             option(flag<&ast::ImportDeclaration::hasAs>(kw<"as">) +
                    (flag<&ast::ImportDeclaration::asModule>(
                         lit<"*">) |
                     assign<&ast::ImportDeclaration::asName>(IdentLike))) +
             option(flag<&ast::ImportDeclaration::hasWith>(kw<"with">) +
                    lit<"("> +
                    many(append<&ast::ImportDeclaration::withArgs>(ref<ParameterRule>) + option(lit<",">)) +
                    lit<")">) +
             LineStatementEnd> {};

  // -- Media --------------------------------------------------------------

  struct MediaExpressionRule
      : rule<create<ast::MediaExpression>(
            lit<"("> + assign<&ast::MediaExpression::feature>(IdentLike) +
            option(lit<":"> +
                   assign<&ast::MediaExpression::value>(Value)) +
            lit<")">)> {};

  static constexpr auto MediaCondition =
      append<&ast::MediaQuery::expressions>(ref<MediaExpressionRule>) |
      append<&ast::MediaQuery::expressions>(ref<InterpolationRule>);

  // A media type is a name or an interpolation: `not`/`only` flag the query
  // and `and`/`or` join its conditions, never as Sass operators.
  struct MediaQueryRule
      : rule<create<ast::MediaQuery>(
            (option(flag<&ast::MediaQuery::isOnly>(kw<"only">) |
                    flag<&ast::MediaQuery::isNot>(kw<"not">)) +
             assign<&ast::MediaQuery::typeOrValue>(ref<IdentValueRule> |
                                                   ref<InterpolationRule>) +
             option(kw<"and"> +
                    ((flag<&ast::MediaQuery::conditionNot>(kw<"not">) +
                      MediaCondition) |
                     some(MediaCondition, kw<"and">)))) |
            (option(flag<&ast::MediaQuery::isNot>(kw<"not">)) +
             append<&ast::MediaQuery::expressions>(ref<MediaExpressionRule>) +
             many(kw<"and"> + MediaCondition)) |
            (append<&ast::MediaQuery::expressions>(ref<MediaExpressionRule>) +
             some(flag<&ast::MediaQuery::joinedByOr>(ref<OP_OR>) +
                  MediaCondition)))> {};

  struct MediaDeclarationRule
      : rule<kw<"@media"> +
             many(append<&ast::MediaDeclaration::queries>(ref<MediaQueryRule>),
                  lit<",">) +
             assign<&ast::MediaDeclaration::body>(ref<BlockRule>)> {};

  // -- Mixins, functions --------------------------------------------------

  struct MixinDeclarationRule
      : rule<kw<"@mixin"> +
             assign<&ast::MixinDeclaration::name>(IdentLike) +
             option(flag<&ast::MixinDeclaration::hasParams>(lit<"(">) +
                    many(append<&ast::MixinDeclaration::params>(ref<ParameterRule>) + option(lit<",">)) +
                    lit<")">) +
             assign<&ast::MixinDeclaration::body>(ref<BlockRule>)> {};

  struct ContentDeclarationRule
      : rule<kw<"@content"> +
             option(flag<&ast::ContentDeclaration::hasArgs>(lit<"(">) +
                    many(append<&ast::ContentDeclaration::args>(ref<ParameterRule>) + option(lit<",">)) +
                    lit<")">) +
             StatementEnd> {};

  struct FunctionDeclarationRule
      : rule<kw<"@function"> +
             option(option(assign<&ast::FunctionDeclaration::nsPrefix>(ref<IDENT>) + lit<".">) +
                    assign<&ast::FunctionDeclaration::name>(IdentLike)) +
             lit<"("> +
             many(append<&ast::FunctionDeclaration::params>(ref<ParameterRule>) + option(lit<",">)) +
             lit<")"> +
             assign<&ast::FunctionDeclaration::body>(ref<FunctionBlockRule>)> {
  };

  // -- Other at-rules -------------------------------------------------

  struct FontFaceDeclarationRule
      : rule<kw<"@font-face"> +
             assign<&ast::FontFaceDeclaration::body>(ref<BlockRule>)> {};

  struct PercentageStatementRule
      : rule<assign<&ast::PercentageStatement::percentage>(
                 ref<PercentageLitRule>) +
             assign<&ast::PercentageStatement::body>(ref<BlockRule>)> {};

  struct KeyframesDeclarationRule
      : rule<kw<"@keyframes"> +
             option(assign<&ast::KeyframesDeclaration::name>(IdentLike)) +
             assign<&ast::KeyframesDeclaration::body>(ref<BlockRule>)> {};

  struct IncludeDeclarationRule
      : rule<kw<"@include"> +
             option(assign<&ast::IncludeDeclaration::nsPrefix>(ref<IDENT>) + lit<".">) +
             assign<&ast::IncludeDeclaration::name>(IdentLike) +
             option(flag<&ast::IncludeDeclaration::hasArgs>(lit<"(">) +
                    many(append<&ast::IncludeDeclaration::args>(ref<ParameterRule>) + option(lit<",">)) +
                    lit<")">) +
             option(flag<&ast::IncludeDeclaration::hasUsing>(
                        kw<"using">) +
                    lit<"("> +
                    many(append<&ast::IncludeDeclaration::usingParams>(ref<ParameterRule>) + option(lit<",">)) +
                    lit<")">) +
             (assign<&ast::IncludeDeclaration::body>(ref<BlockRule>) |
              LineStatementEnd)> {};

  static constexpr auto ExtendTargetAlt =
      ref<IdSelectorRule> | ref<TypeRefRule> | ref<ClassSelectorRule> |
      ref<AttribSelectorRule> | ref<PseudoSelectorRule> |
      ref<InterpolationRule> | ref<ParentRefRule>;

  struct ExtendDeclarationRule
      : rule<kw<"@extend"> +
             option(flag<&ast::ExtendDeclaration::isPlaceholder>(
                        lit<"%">) |
                    flag<&ast::ExtendDeclaration::hasParentRefPrefix>(
                        lit<"&">)) +
             some(append<&ast::ExtendDeclaration::targets>(ExtendTargetAlt)) +
             StatementEnd> {};

  struct WarnDeclarationRule
      : rule<kw<"@warn"> +
             assign<&ast::WarnDeclaration::message>(ref<STRING>) + StatementEnd> {
  };

  struct ErrorDeclarationRule
      : rule<kw<"@error"> +
             assign<&ast::ErrorDeclaration::message>(ref<STRING>) + StatementEnd> {
  };

  struct RawTextRule : rule<assign<&ast::RawText::text>(ref<AT_VALUE>)> {};

  // No unit glued after `}`: what follows is the value's next text.
  struct AtValueInterpolationRule
      : rule<create<ast::Interpolation>(
            lit<"#"> + lit<"{"> + assign<&ast::Interpolation::inner>(Value) +
            lit<"}">)> {};

  struct AtStatementDeclarationRule
      : rule<lit<"@"> + assign<&ast::AtStatementDeclaration::name>(IdentLike) +
             many(append<&ast::AtStatementDeclaration::value>(
                 ref<RawTextRule> | ref<AtValueInterpolationRule>)) +
             (assign<&ast::AtStatementDeclaration::body>(ref<BlockRule>) |
              StatementEnd)> {};

  struct AtRootDeclarationRule
      : rule<kw<"@at-root"> +
             ((option(lit<"("> +
                      assign<&ast::AtRootDeclaration::queryName>(IdentLike) +
                      lit<":"> +
                      some(append<&ast::AtRootDeclaration::queryValue>(Value)) +
                      lit<")">) +
               assign<&ast::AtRootDeclaration::body>(ref<BlockRule>)) |
              assign<&ast::AtRootDeclaration::rule>(ref<RulesetRule>))> {};

  struct DebugDeclarationRule
      : rule<kw<"@debug"> +
             assign<&ast::DebugDeclaration::value>(ValueOrListOrMap) +
             StatementEnd> {};

  // -- Control flow -----------------------------------------------------

  struct ElseIfClauseRule
      : rule<create<ast::ElseIfClause>(
            kw<"@else"> + kw<"if"> +
            assign<&ast::ElseIfClause::condition>(Value) +
            assign<&ast::ElseIfClause::body>(ref<BlockRule>))> {};

  struct IfDeclarationRule
      : rule<kw<"@if"> + assign<&ast::IfDeclaration::condition>(Value) +
             assign<&ast::IfDeclaration::body>(ref<BlockRule>) +
             many(append<&ast::IfDeclaration::elseIfs>(ref<ElseIfClauseRule>)) +
             option(kw<"@else"> +
                    assign<&ast::IfDeclaration::elseBody>(ref<BlockRule>))> {};

  struct PlainNumberLitRule
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::magnitude>(ref<NUMBER>))> {};

  struct ForDeclarationRule
      : rule<kw<"@for"> + lit<"$"> +
             assign<&ast::ForDeclaration::varName>(IdentLike) + kw<"from"> +
             assign<&ast::ForDeclaration::from>(Value) +
             (kw<"to"> | flag<&ast::ForDeclaration::throughInclusive>(
                             kw<"through">)) +
             assign<&ast::ForDeclaration::to>(Value) +
             assign<&ast::ForDeclaration::body>(ref<BlockRule>)> {};

  struct WhileDeclarationRule
      : rule<kw<"@while"> + assign<&ast::WhileDeclaration::condition>(Value) +
             assign<&ast::WhileDeclaration::body>(ref<BlockRule>)> {};

  struct EachDeclarationRule
      : rule<kw<"@each"> +
             some(lit<"$"> +
                      append<&ast::EachDeclaration::varNames>(IdentLike),
                  lit<",">) +
             kw<"in"> + assign<&ast::EachDeclaration::list>(ValueOrListOrMap) +
             assign<&ast::EachDeclaration::body>(ref<BlockRule>)> {};

  // -- Rulesets -------------------------------------------------------

  struct RulesetRule
      : rule<assign<&ast::Ruleset::selectors>(ref<SelectorGroupRule>) +
             assign<&ast::Ruleset::body>(ref<BlockRule>)> {};

  // -- Statements, blocks -------------------------------------------------

  struct ExprStatementRule
      : rule<create<ast::ExprStatement>(
            assign<&ast::ExprStatement::expr>(Value) + lit<";">)> {};

  struct ReturnStmtRule
      : rule<create<ast::ReturnStmt>(
            kw<"@return"> +
            some(append<&ast::ReturnStmt::values>(Value), lit<",">) +
            StatementEnd)> {};

  struct BlockRule
      : rule<create<ast::Block>(
            lit<"{"> +
            many(append<&ast::Block::statements>(ref<StatementAlt>) |
                 lit<";">) +
            lit<"}">)> {};

  // `expr;` is legal only inside a @function body: admitting it into every
  // block would make a keyword like `repeat`/`calc`, reserved for the
  // dedicated call shapes below, compete with the SAME spelling used as an
  // ordinary property name in that block (per-state lexing picks one
  // reading for the whole state, not per alternative).
  struct FunctionBlockRule
      : rule<create<ast::Block>(
            lit<"{"> +
            many(append<&ast::Block::statements>(ref<StatementAlt> |
                                                 ref<ExprStatementRule>) |
                 lit<";">) +
            lit<"}">)> {};

  struct StatementAlt
      : rule<ref<ReturnStmtRule> | ref<ImportDeclarationRule> |
             ref<VariableDeclarationRule> | ref<InterpolationDeclarationRule> |
             ref<PropertyDeclarationRule> | ref<MediaDeclarationRule> |
             ref<MixinDeclarationRule> | ref<ContentDeclarationRule> |
             ref<FunctionDeclarationRule> | ref<IfDeclarationRule> |
             ref<ForDeclarationRule> | ref<WhileDeclarationRule> |
             ref<EachDeclarationRule> | ref<FontFaceDeclarationRule> |
             ref<KeyframesDeclarationRule> | ref<IncludeDeclarationRule> |
             ref<ExtendDeclarationRule> | ref<WarnDeclarationRule> |
             ref<ErrorDeclarationRule> | ref<RulesetRule> |
             ref<AtRootDeclarationRule> | ref<DebugDeclarationRule> |
             ref<AtStatementDeclarationRule> | ref<PercentageStatementRule>> {};

  // `@charset` builds nothing, as dart-sass drops it from the stylesheet.
  struct StylesheetRule
      : rule<create<ast::Stylesheet>(
            many(append<&ast::Stylesheet::statements>(ref<StatementAlt>) |
                 lit<";"> | (kw<"@charset"> + ref<STRING>)))> {
  };

  using entry = StylesheetRule;
  using ignore = list<SPACE, SL_COMMENT, ML_COMMENT>;
};

}  // namespace scss
