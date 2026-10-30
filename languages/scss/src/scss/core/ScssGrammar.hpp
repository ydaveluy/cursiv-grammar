// The scss language: its grammar. `ComponentValueAlt` is the CSS/Sass value
// family (also selector fragments, which share it -- see ast.hpp); `Value`
// is the operator-chain built over it. `Block` is one production reused for
// every `{ ... }` body, function bodies included.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "scss/core/ExtendOptional.hpp"
#include "scss/core/ast.hpp"

namespace scss {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto trim_url = [](std::string_view text) {
  const auto blank = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f'; };
  while (!text.empty() && blank(text.front())) text.remove_prefix(1);
  while (!text.empty() && blank(text.back())) text.remove_suffix(1);
  return text;
};
inline constexpr auto quoted_url = [](std::string_view text) {
  std::size_t start = 0;
  while (start < text.size()) {
    if (text.substr(start).starts_with("/*")) { start = text.find("*/", start + 2) + 2; continue; }
    if (text[start] == '\'' || text[start] == '"') break;
    ++start;
  }
  const char quote = text[start];
  std::size_t end = start + 1;
  while (end < text.size()) {
    if (text[end] == '\\') { end += 2; continue; }
    if (text[end++] == quote) break;
  }
  return unquote(text.substr(start, end - start));
};

inline constexpr auto drop_parent = [](std::string_view t) {
  return t.substr(1);
};

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
  struct NUMBER : terminal<as<double>(NumberBody)> {};

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
  struct PARENT_SUFFIX : terminal<as<drop_parent>(lit<"&"> + some(IdentCont))> {
  };

  static constexpr auto StringBody1 =
      many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">);
  static constexpr auto StringBody2 =
      many((lit<"\\"> + any) | cr<R"(^'\\\r\n)">);
  struct STRING : terminal<as<unquote>((lit<"\""> + StringBody1 + lit<"\"">) |
                                       (lit<"'"> + StringBody2 + lit<"'">))> {};

  // Native unquoted URLs preserve Unicode and embedded whitespace.
  struct URL_RAW
      : terminal<as<trim_url>(some(cr<R"(!#$%&*-~ \t\r\n\f)"> |
                     cr<R"(\u0080-\u{10FFFF})"> | IdentEscape))> {};
  struct URL_QUOTED
      : terminal<as<quoted_url>(many(ref<SPACE> | ref<ML_COMMENT>) + ref<STRING> +
                               many(ref<SPACE> | ref<ML_COMMENT>))> {};

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
      : terminal<as<ast::Prio::important>(
            lit<"!"> +
            many(cr<R"( \t\r\n\f)"> | through(lit<"/*">, lit<"*/">)) +
            ikw<"important">)> {};
  static constexpr auto PrioAlt = ref<IMPORTANT_TOK> | as<ast::Prio::default_>(lit<"!default">);

  struct OP_EQ : terminal<as<ast::Op::eq>(lit<"=="> | lit<"=">)> {};

  static constexpr auto AttribOpAlt =
      as<ast::AttribOp::prefix>(lit<"^=">) | as<ast::AttribOp::suffix>(lit<"$=">) | as<ast::AttribOp::substring>(lit<"*=">) |
      as<ast::AttribOp::includes>(lit<"~=">) | as<ast::AttribOp::dashmatch>(lit<"|=">) | as<ast::AttribOp::equals>(lit<"=">);

  static constexpr auto CombinatorAlt =
      as<ast::Combinator::child>(lit<">">) | as<ast::Combinator::adjacent_sibling>(lit<"+">) | as<ast::Combinator::general_sibling>(lit<"~">);


  // A name the language also uses as a fixed keyword elsewhere (`@for`'s
  // `from`/`to`): spelled once, as the soft-keyword idiom asks.
  static constexpr auto IdentLike = ref<IDENT> | kw<"from"> | kw<"to"> |
      kw<"calc"> | kw<"rgba"> | kw<"repeat"> | kw<"url"> | kw<"format"> | kw<"if">;

  // Forward declarations: the value/selector/statement families are
  // mutually recursive (a function call argument is a value, a value can be
  // a call; a block holds statements, a statement can hold a block).
  struct ComponentValueAlt;
  struct Expression;
  struct Interpolation;
  struct IfExpr;
  struct FunctionCall;
  struct ListValue;
  struct NestedListItem;
  struct MapDeclaration;
  struct Parameter;
  struct TypeRef;
  struct PseudoSelector;
  struct ClassSelector;
  struct AttribSelector;
  struct IdSelector;
  struct BlockRule;
  struct Ruleset;
  struct MediaQuery;
  struct FunctionBlock;
  struct StatementAlt;

  static constexpr auto Value = ref<Expression>;

  // A value slot that may also be a bare list or a map literal: a
  // comma-separated argument may itself be a space-separated run
  // (`linear-gradient(to bottom right, ...)`), a variable's value, a
  // parameter's default, a map's own key/value.
  static constexpr auto ValueOrListOrMap =
      ref<MapDeclaration> | ref<ListValue> | Value;


  // -- Leaf values ------------------------------------------------------

  struct NumberLit
      : rule<option(flag<&ast::NumberLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::NumberLit::magnitude>(ref<NUMBER>)> {};

  struct PercentageLit
      : rule<option(flag<&ast::PercentageLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::PercentageLit::text>(ref<PERCENTAGE>)> {};

  struct DimensionLit
      : rule<option(flag<&ast::DimensionLit::negative>(lit<"-">) |
                    lit<"+">) +
             assign<&ast::DimensionLit::text>(ref<DIMENSION>)> {};

  struct StringLit : rule<assign<&ast::StringLit::value>(ref<STRING>)> {};

  struct BooleanLit
      : rule<assign<&ast::BooleanLit::value>(as<true>(kw<"true">) |
                                              as<false>(kw<"false">))> {};

  struct NullLit : rule<create<ast::NullLit> + kw<"null">> {};

  struct IdentValue : rule<assign<&ast::IdentValue::name>(IdentLike)> {};

  struct HexColorLit
      : rule<assign<&ast::HexColorLit::text>(ref<HASH>)> {};

  // `$name`, `ns.$name`, a signed `+$name`/`-$name`, or `--custom-prop`.
  struct VariableRef
      : rule<(flag<&ast::VariableRef::isCustomProperty>(lit<"--">) +
              assign<&ast::VariableRef::name>(ref<CUSTOM_PROP_NAME>)) |
             (option(flag<&ast::VariableRef::negative>(lit<"-">) |
                     lit<"+">) +
              option(assign<&ast::VariableRef::nsPrefix>(ref<IDENT>) + lit<".">) +
              lit<"$"> + assign<&ast::VariableRef::name>(IdentLike))> {};

  struct ParentRef : rule<create<ast::ParentRef>(lit<"&">)> {};

  // A leading `not` on one operand; wraps only when the word is present.
  struct NotExpr
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

  struct ParenGroup
      : rule<create<ast::ParenGroup>(
            option(flag<&ast::ParenGroup::negative>(lit<"-">) |
                   lit<"+">) +
            lit<"("> +
            option(assign<&ast::ParenGroup::inner>(Value | BracketList |
                                                   ParenList)) +
            lit<")">)> {};

  // -- Calls --------------------------------------------------------------

  struct CalcExpr
      : rule<create<ast::CalcExpr>(
            kw<"calc"> + gap::none + lit<"("> + assign<&ast::CalcExpr::sum>(Value) +
            lit<")">)> {};

  // rgba() keeps its own shape: args may run with no separating comma
  // (`rgba(200 200 200 / 0.3)`), which a generic call never allows.
  struct RgbaCall
      : rule<create<ast::RgbaCall>(
            kw<"rgba"> + gap::none + lit<"("> + append<&ast::RgbaCall::args>(Value) +
            many(option(lit<",">) +
                 append<&ast::RgbaCall::args>(Value)) +
            lit<")">)> {};

  struct RepeatCall
      : rule<create<ast::RepeatCall>(
            kw<"repeat"> + gap::none + lit<"("> +
            assign<&ast::RepeatCall::value>(Value) + lit<","> +
            assign<&ast::RepeatCall::count>(ref<NumberLit>) +
            assign<&ast::RepeatCall::freqUnit>(kw<"hz"> | kw<"khz"> |
                                                kw<"fr">) +
            lit<")">)> {};

  struct UrlRef
      : rule<create<ast::UrlRef>(
            kw<"url"> + gap::none + tight(lit<"("> +
            option(assign<&ast::UrlRef::value>(ref<URL_QUOTED> | ref<URL_RAW>)) +
            lit<")">))> {};

  struct FormatRef
      : rule<create<ast::FormatRef>(
            kw<"format"> + gap::none + lit<"("> +
            assign<&ast::FormatRef::value>(ref<STRING>) + lit<")">)> {};

  // A generic call: `ns.name(args)`. Also covers `var()`/`rotate()`, whose
  // arguments already fit this shape once per-state lexing removes the need
  // for merged lookahead tokens.
  struct FunctionCall
      : rule<option(assign<&ast::FunctionCall::nsPrefix>(ref<IDENT>) + lit<".">) +
             assign<&ast::FunctionCall::name>(IdentLike) + gap::none +
             lit<"("> + many(append<&ast::FunctionCall::args>(ref<Parameter>) + option(lit<",">)) +
             lit<")">> {};

  struct PrioValue
      : rule<create<ast::PrioValue>(
            flag<&ast::PrioValue::isDefault>(as<ast::Prio::default_>(lit<"!default">)) |
            ref<IMPORTANT_TOK>)> {};

  static constexpr auto ValueOrPrio = Value | ref<PrioValue>;

  struct IfExpr
      : rule<create<ast::IfExpr>(
            kw<"if"> + gap::none + lit<"("> +
            assign<&ast::IfExpr::condition>(ValueOrPrio) + lit<","> +
            assign<&ast::IfExpr::thenValue>(ValueOrPrio) + lit<","> +
            assign<&ast::IfExpr::elseValue>(ValueOrPrio) + lit<")"> +
            option(assign<&ast::IfExpr::trailingUnit>(ref<IDENT>)) +
            // !default belongs to the surrounding declaration/configuration.
            option(assign<&ast::IfExpr::prio>(ref<IMPORTANT_TOK>)))> {};

  // `#{ ... }`; a unit or a bare `%` glued right after `}` (`#{$v}px`,
  // `#{$i * 10}%` -- the keyframe-step idiom) reads as an ordinary
  // identifier or a literal `%` rather than through a dedicated unit
  // keyword, since that spot never competes with one.
  // `gap::none`: the gluing is the point -- a space leaves it a separate
  // token instead.
  struct Interpolation
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
      : rule<ref<NotExpr> | ref<NumberLit> | ref<PercentageLit> |
             ref<DimensionLit> | ref<BooleanLit> | ref<NullLit> |
             ref<HexColorLit> | ref<VariableRef> |
             ref<Interpolation> | ref<CalcExpr> | ref<RgbaCall> |
             ref<RepeatCall> | ref<UrlRef> | ref<FormatRef> |
             ref<IfExpr> | ref<FunctionCall> | ref<StringLit> |
             ref<ParenGroup> | ref<ParentRef> |
             (ref<IdentValue> + !(gap::none + &lit<"(">))> {};

  // `a -1` is a list of two: behind a blank, a `-` glued to a number signs it.

  // multiplicative > additive > relational > equality > and > or (Sass,
  // "Operators", order of operations) -- unary `not`/sign stay ad hoc
  // per-rule (`Not? expressionPart`, `VariableRef`, `ParenGroup`),
  // matching the source grammar, which has no general unary tier either.
  struct Expression
      : infix<ComponentValueAlt, ast::BinaryExpr,
              levels(left(as<ast::Op::divide>(lit<"/">) | as<ast::Op::times>(lit<"*">) | as<ast::Op::modulo>(lit<"%">)),
                     left(as<ast::Op::minus>((gap::none | !lit<"-", !cr<"^.0-9">>) + as<ast::Op::minus>(lit<"-">)) | as<ast::Op::plus>(lit<"+">)),
                     left(as<ast::Op::greater_eq>(lit<">=">) | as<ast::Op::greater>(lit<">">) | as<ast::Op::less_eq>(lit<"<=">) | as<ast::Op::less>(lit<"<">)),
                     left(ref<OP_EQ> | as<ast::Op::not_equal>(lit<"!=">)),
                     left(as<ast::Op::and_>(kw<"and">)),
                     left(as<ast::Op::or_>(kw<"or">)))> {};

  // -- Lists and maps -------------------------------------------------

  // An item inside a list: a nested bracketed/parenthesised list, or a
  // plain value. Excludes the bare (unwrapped) list form to keep this
  // left-recursion-free -- only the outermost list may be unwrapped.
  struct NestedListItem : rule<BracketList | ParenList | Value> {};

  // A bare, unwrapped list needs 2+ items: with exactly one, `Value` alone
  // already matches, and requiring a second item here keeps the two from
  // ever competing over the same input.
  struct BareListValue
      : rule<create<ast::ListValue>(ListItem + some(NextListItem))> {};

  struct ListValue
      : rule<BracketList | ParenList | ref<BareListValue>> {};

  // `ValueOrListOrMap`, not `NestedListItem`: a key or a value here may be a
  // single plain value, a bare space-separated run (`border: 1px solid
  // $color`), or a nested map/list -- and `MapEntry` is never called
  // from inside `ListValue`'s own item loop, so its bare fallback
  // carries no recursion risk here.
  struct MapEntry
      : rule<create<ast::MapEntry>(
            assign<&ast::MapEntry::key>(ValueOrListOrMap) + lit<":"> +
            assign<&ast::MapEntry::value>(ValueOrListOrMap))> {};

  struct MapDeclaration
      : rule<create<ast::MapValue>(
            lit<"("> + append<&ast::MapValue::entries>(ref<MapEntry>) +
            many(lit<","> + append<&ast::MapValue::entries>(ref<MapEntry>)) +
            option(lit<",">) + lit<")">)> {};

  // -- Selectors ----------------------------------------------------------
  // Every fragment below shares `ComponentValue` (ast.hpp), so a functional
  // pseudo's argument list can hold either an ordinary value
  // (`:nth-child(2n+1)`) or a selector fragment (`:not([disabled])`) with no
  // separate shape needed for either.

  struct IdSelector
      : rule<assign<&ast::IdSelector::name>(ref<HASH>)> {};

  // A lone `-` before an interpolation of one name (`.#{$a}-#{$b}`).
  static constexpr auto DashName =
      create<ast::IdentValue>(assign<&ast::IdentValue::name>(lit<"-">));

  static constexpr auto ClassNamePart =
      ref<IdentValue> | ref<Interpolation> | DashName;

  struct ClassSelector
      : rule<create<ast::ClassSelector>(
            lit<"."> + append<&ast::ClassSelector::parts>(ClassNamePart) +
            many(gap::none +
                 append<&ast::ClassSelector::parts>(ClassNamePart)))> {};

  struct AttribSelector
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
      ref<IdentValue> | ref<VariableRef>;

  struct TypeRef
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
      ref<AttribSelector> | ref<TypeRef> | ref<IdSelector> |
      ref<PseudoSelector> | Value;

  // A class reads the interpolations glued to it, any other argument keeps
  // them as its own: an argument glued to the one before never opens on `#{`.
  static constexpr auto PseudoArg =
      append<&ast::PseudoSelector::args>(ref<ClassSelector>) |
      (append<&ast::PseudoSelector::args>(PseudoArgAlt) +
       many(gap::none +
            append<&ast::PseudoSelector::args>(ref<Interpolation>)));
  static constexpr auto PseudoArgSeparator =
      lit<","> | !gap::none | !lit<"#"> | &lit<"#", !cr<"{">>;

  // These words can also start the competing declaration value. Keep
  // their keyword tokens available as pseudo names, so `a:not(...)`
  // stays a selector rather than a nested property declaration.
  static constexpr auto PseudoName =
      IdentLike | kw<"not"> | kw<"true"> | kw<"false"> | kw<"null">;

  // Glued to its colon: `a: b` is never a pseudo-class.
  struct PseudoSelector
      : rule<create<ast::PseudoSelector>(
            lit<":"> +
            option(gap::none +
                   flag<&ast::PseudoSelector::isElement>(lit<":">)) +
            gap::none +
            (assign<&ast::PseudoSelector::interp>(ref<Interpolation>) |
             (assign<&ast::PseudoSelector::name>(PseudoName) +
              option(flag<&ast::PseudoSelector::isFunctional>(
                         gap::none + lit<"(">) +
                     PseudoArg + many(PseudoArgSeparator + PseudoArg) +
                     option(lit<",">) + lit<")">))))> {};

  // `VariableRef` here: a bare `$name` glued right after another item
  // with no separator (`#{$sel}$y`) is its own sequence item, not folded
  // into the interpolation before it.
  static constexpr auto SelectorItemAlt =
      ref<IdSelector> | ref<AttribSelector> | ref<PseudoSelector> |
      ref<ParentRef> | ref<VariableRef>;

  // A name's interpolations and lone `-` glued after an item. A class reads
  // its own, so none starts the item after a class: `.a#{$b}` is one class.
  static constexpr auto NameFragments =
      many(gap::none + append<&ast::SimpleSelectorSequence::items>(
                           ref<Interpolation> | DashName));

  static constexpr auto SelectorUnit =
      append<&ast::SimpleSelectorSequence::items>(ref<ClassSelector>) |
      (append<&ast::SimpleSelectorSequence::items>(SelectorItemAlt) +
       NameFragments);

  // A gap (whitespace) between a leading type and the next item, or between
  // two items, starts a NEW sequence (implicit descendant combinator): this
  // is what tells `body.cls` (one compound selector) apart from `body .cls`
  // (two).
  struct SimpleSelectorSequence
      : rule<create<ast::SimpleSelectorSequence>(
            option(assign<&ast::SimpleSelectorSequence::combinator>(
                CombinatorAlt)) +
            ((assign<&ast::SimpleSelectorSequence::type>(ref<TypeRef>) +
              NameFragments) |
             (option(append<&ast::SimpleSelectorSequence::items>(DashName) +
                     gap::none) +
              append<&ast::SimpleSelectorSequence::items>(
                  ref<Interpolation>) +
              NameFragments) |
             SelectorUnit) +
            many(gap::none + SelectorUnit))> {};

  // A sequence glued to the one before it is never a descendant.
  struct Selector
      : rule<create<ast::Selector>(
            append<&ast::Selector::sequences>(ref<SimpleSelectorSequence>) +
            many(((!gap::none) | &cr<">+~">) +
                 append<&ast::Selector::sequences>(
                     ref<SimpleSelectorSequence>)))> {};

  struct SelectorGroup
      : rule<create<ast::SelectorGroup>(
            append<&ast::SelectorGroup::selectors>(ref<Selector>) +
            many(lit<","> +
                 append<&ast::SelectorGroup::selectors>(ref<Selector>)))> {
  };

  // -- Parameters -----------------------------------------------------

  // A signature or call-site argument: `$image`, `$x: 50%`, `$rest...`.
  struct Parameter
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
           append<SuffixMember>(ref<Interpolation> | DashName));

  struct VariableDeclaration
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
             many(flag<&ast::VariableDeclaration::isDefault>(lit<"!default">) |
                  flag<&ast::VariableDeclaration::isGlobal>(lit<"!global">)) +
             option(assign<&ast::VariableDeclaration::prio>(ref<IMPORTANT_TOK>)) +
             StatementEnd> {};

  // Test the adjacent bytes in a literal guard: a rule condition would
  // compare this prefix with a whole token such as `not` or `hover`.
  // A glued name can start a pseudo-class and takes no nested property body.
  static constexpr auto PseudoStart =
      IdentStart | (lit<"-"> + IdentStart) | lit<"#{">;

  template <auto ValueMember>
  static constexpr auto ValueRun =
      append<ValueMember>(Value) +
      many((lit<","> + append<ValueMember>(Value)) |
           (SpaceNext + append<ValueMember>(Value))) +
      option(lit<",">);

  template <auto ValueMember, auto PrioMember, auto NestedMember>
  static constexpr auto DeclarationBody =
      (lit<":", &PseudoStart> + ValueRun<ValueMember> +
       option(assign<PrioMember>(PrioAlt)) + StatementEnd) |
      (lit<":", !PseudoStart> + ValueRun<ValueMember> +
       option(assign<PrioMember>(PrioAlt)) +
       (assign<NestedMember>(ref<BlockRule>) | StatementEnd)) |
      (lit<":"> +
       (assign<NestedMember>(ref<BlockRule>) |
        (option(assign<PrioMember>(PrioAlt)) + StatementEnd)));

  struct PropertyDeclaration
      : rule<assign<&ast::PropertyDeclaration::property>(IdentLike) +
             NameSuffix<&ast::PropertyDeclaration::suffix> +
             DeclarationBody<&ast::PropertyDeclaration::value,
                             &ast::PropertyDeclaration::prio,
                             &ast::PropertyDeclaration::nestedBlock>> {};

  struct InterpolationDeclaration
      : rule<option(flag<&ast::InterpolationDeclaration::leadingDash>(
                        lit<"-">) +
                    gap::none) +
             assign<&ast::InterpolationDeclaration::key>(
                 ref<Interpolation>) +
             NameSuffix<&ast::InterpolationDeclaration::suffix> +
             DeclarationBody<&ast::InterpolationDeclaration::value,
                             &ast::InterpolationDeclaration::prio,
                             &ast::InterpolationDeclaration::nestedBlock>> {};

  // -- Import family ----------------------------------------------------

  static constexpr auto ImportMedia =
      some(append<&ast::ImportDeclaration::media>(ref<MediaQuery>),
           lit<",">);

  // A configuration's comma separates variables. Lists may still contain
  // commas inside brackets or parentheses, or spaces at the top level.
  struct ForwardImportantValue : rule<create<ast::PrioValue>(ref<IMPORTANT_TOK>)> {};
  static constexpr auto ForwardValueItem =
      ref<ForwardImportantValue> | ref<NestedListItem>;
  static constexpr auto ForwardListItem =
      append<&ast::ListValue::items>(ForwardValueItem);
  struct ForwardSpaceList
      : rule<create<ast::ListValue>(
            ForwardListItem + some(SpaceNext + ForwardListItem))> {};

  struct ForwardConfigurationValue
      : rule<ref<MapDeclaration> | ref<ForwardSpaceList> |
             ForwardValueItem> {};

  struct ForwardConfiguredVariable
      : rule<create<ast::Parameter>(
            assign<&ast::Parameter::value>(create<ast::VariableRef>(
                lit<"$"> + gap::none +
                assign<&ast::VariableRef::name>(IdentLike))) +
            lit<":"> +
            assign<&ast::Parameter::defaultValue>(
                ref<ForwardConfigurationValue>) +
            option(assign<&ast::Parameter::prio>(
                as<ast::Prio::default_>(lit<"!default">))))> {};

  static constexpr auto ForwardMember =
      append<&ast::ImportDeclaration::forwardedNames>(IdentLike) |
      (lit<"$"> + gap::none +
       append<&ast::ImportDeclaration::forwardedVariables>(IdentLike));

  static constexpr auto ForwardImport =
      assign<&ast::ImportDeclaration::kind>(
          as<ast::ImportKind::forward>(kw<"@forward">)) +
      assign<&ast::ImportDeclaration::path>(ref<StringLit>) +
      option(flag<&ast::ImportDeclaration::hasAs>(ikw<"as">) +
             assign<&ast::ImportDeclaration::asName>(IdentLike) +
             gap::none + lit<"*">) +
      option(assign<&ast::ImportDeclaration::forwardVisibility>(
                 as<ast::ForwardVisibility::show>(ikw<"show">) |
                 as<ast::ForwardVisibility::hide>(ikw<"hide">)) +
             some(ForwardMember, lit<",">)) +
      option(flag<&ast::ImportDeclaration::hasWith>(ikw<"with">) +
             lit<"("> +
             append<&ast::ImportDeclaration::withArgs>(
                 ref<ForwardConfiguredVariable>) +
             many(lit<","> + append<&ast::ImportDeclaration::withArgs>(
                                 ref<ForwardConfiguredVariable>)) +
             option(lit<",">) + lit<")">) +
      StatementEnd;

  struct ImportDeclaration
      : rule<ForwardImport |
             (assign<&ast::ImportDeclaration::kind>(
                 as<ast::ImportKind::import_>(lit<"@import">) | as<ast::ImportKind::require>(lit<"@require">) | as<ast::ImportKind::use>(lit<"@use">)) +
             assign<&ast::ImportDeclaration::path>(ref<UrlRef> |
                                                    ref<StringLit>) +
             many(lit<","> + append<&ast::ImportDeclaration::morePaths>(
                                 ref<UrlRef> | ref<StringLit>)) +
             option(!gap::line +
                    ((some(append<&ast::ImportDeclaration::modifiers>(
                          ref<FunctionCall>)) +
                      option(ImportMedia)) |
                     ImportMedia)) +
             option(flag<&ast::ImportDeclaration::hasAs>(kw<"as">) +
                    (flag<&ast::ImportDeclaration::asModule>(
                         lit<"*">) |
                     assign<&ast::ImportDeclaration::asName>(IdentLike))) +
             option(flag<&ast::ImportDeclaration::hasWith>(kw<"with">) +
                    lit<"("> +
                    many(append<&ast::ImportDeclaration::withArgs>(ref<Parameter>) + option(lit<",">)) +
                    lit<")">) +
             LineStatementEnd)> {};

  // -- Media --------------------------------------------------------------

  struct MediaExpression
      : rule<create<ast::MediaExpression>(
            lit<"("> + assign<&ast::MediaExpression::feature>(IdentLike) +
            option(lit<":"> +
                   assign<&ast::MediaExpression::value>(Value)) +
            lit<")">)> {};

  static constexpr auto MediaCondition =
      append<&ast::MediaQuery::expressions>(ref<MediaExpression>) |
      append<&ast::MediaQuery::expressions>(ref<Interpolation>);

  // A media type is a name or an interpolation: `not`/`only` flag the query
  // and `and`/`or` join its conditions, never as Sass operators.
  struct MediaQuery
      : rule<create<ast::MediaQuery>(
            (option(flag<&ast::MediaQuery::isOnly>(kw<"only">) |
                    flag<&ast::MediaQuery::isNot>(kw<"not">)) +
             assign<&ast::MediaQuery::typeOrValue>(ref<IdentValue> |
                                                   ref<Interpolation>) +
             option(kw<"and"> +
                    ((flag<&ast::MediaQuery::conditionNot>(kw<"not">) +
                      MediaCondition) |
                     some(MediaCondition, kw<"and">)))) |
            (option(flag<&ast::MediaQuery::isNot>(kw<"not">)) +
             append<&ast::MediaQuery::expressions>(ref<MediaExpression>) +
             many(kw<"and"> + MediaCondition)) |
            (append<&ast::MediaQuery::expressions>(ref<MediaExpression>) +
             some(flag<&ast::MediaQuery::joinedByOr>(as<ast::Op::or_>(kw<"or">)) +
                  MediaCondition)))> {};

  struct MediaDeclaration
      : rule<kw<"@media"> +
             many(append<&ast::MediaDeclaration::queries>(ref<MediaQuery>),
                  lit<",">) +
             assign<&ast::MediaDeclaration::body>(ref<BlockRule>)> {};

  // -- Mixins, functions --------------------------------------------------

  struct MixinDeclaration
      : rule<kw<"@mixin"> +
             assign<&ast::MixinDeclaration::name>(IdentLike) +
             option(flag<&ast::MixinDeclaration::hasParams>(lit<"(">) +
                    many(append<&ast::MixinDeclaration::params>(ref<Parameter>) + option(lit<",">)) +
                    lit<")">) +
             assign<&ast::MixinDeclaration::body>(ref<BlockRule>)> {};

  struct ContentDeclaration
      : rule<kw<"@content"> +
             option(flag<&ast::ContentDeclaration::hasArgs>(lit<"(">) +
                    many(append<&ast::ContentDeclaration::args>(ref<Parameter>) + option(lit<",">)) +
                    lit<")">) +
             StatementEnd> {};

  struct FunctionDeclaration
      : rule<kw<"@function"> +
             option(option(assign<&ast::FunctionDeclaration::nsPrefix>(ref<IDENT>) + lit<".">) +
                    assign<&ast::FunctionDeclaration::name>(IdentLike)) +
             lit<"("> +
             many(append<&ast::FunctionDeclaration::params>(ref<Parameter>) + option(lit<",">)) +
             lit<")"> +
             assign<&ast::FunctionDeclaration::body>(ref<FunctionBlock>)> {
  };

  // -- Other at-rules -------------------------------------------------

  struct FontFaceDeclaration
      : rule<kw<"@font-face"> +
             assign<&ast::FontFaceDeclaration::body>(ref<BlockRule>)> {};

  struct PercentageStatement
      : rule<assign<&ast::PercentageStatement::percentage>(
                 ref<PercentageLit>) +
             assign<&ast::PercentageStatement::body>(ref<BlockRule>)> {};

  struct KeyframesDeclaration
      : rule<kw<"@keyframes"> +
             option(assign<&ast::KeyframesDeclaration::name>(IdentLike)) +
             assign<&ast::KeyframesDeclaration::body>(ref<BlockRule>)> {};

  struct IncludeDeclaration
      : rule<kw<"@include"> +
             option(assign<&ast::IncludeDeclaration::nsPrefix>(ref<IDENT>) + lit<".">) +
             assign<&ast::IncludeDeclaration::name>(IdentLike) +
             option(flag<&ast::IncludeDeclaration::hasArgs>(lit<"(">) +
                    many(append<&ast::IncludeDeclaration::args>(ref<Parameter>) + option(lit<",">)) +
                    lit<")">) +
             option(flag<&ast::IncludeDeclaration::hasUsing>(
                        kw<"using">) +
                    lit<"("> +
                    many(append<&ast::IncludeDeclaration::usingParams>(ref<Parameter>) + option(lit<",">)) +
                    lit<")">) +
             (assign<&ast::IncludeDeclaration::body>(ref<BlockRule>) |
              LineStatementEnd)> {};

  // This one keyword uses the existing CSS identifier boundary, including
  // non-ASCII and escaped continuations, when excluding its raw fallback.
  static constexpr auto ExtendKeyword = lit<"@extend", !IdentCont>;
  static constexpr auto OptionalEscape = lit<"\\"> +
      ((repeat<1, 6>(cr<"0-9a-fA-F">) + option(cr<R"( \t\r\n\f)">)) |
       (!cr<"0-9a-fA-F"> + ~cr<R"(\r\n\f)">));
  struct EXTEND_OPTIONAL : terminal<as<extend_optional>(
      lit<"!"> + (OptionalEscape | (!lit<"\\"> + IdentStart)) +
      many(OptionalEscape | (!lit<"\\"> + IdentCont)))> {};

  static constexpr auto ExtendTargetAlt =
      ref<IdSelector> | ref<TypeRef> | ref<ClassSelector> |
      ref<AttribSelector> | ref<PseudoSelector> |
      ref<Interpolation> | ref<ParentRef>;

  // Reuse selectors; Sass's selector-list reader tolerates empty comma
  // segments after its first selector and a final comma.
  struct ExtendSelectorGroup : rule<create<ast::SelectorGroup>(
      append<&ast::SelectorGroup::selectors>(ref<Selector>) +
      many(some(lit<",">) + append<&ast::SelectorGroup::selectors>(ref<Selector>)) +
      option(some(lit<",">)))> {};

  struct ExtendDeclaration
      : rule<ExtendKeyword +
             ((option(flag<&ast::ExtendDeclaration::isPlaceholder>(lit<"%">) |
                      flag<&ast::ExtendDeclaration::hasParentRefPrefix>(lit<"&">)) +
               some(append<&ast::ExtendDeclaration::targets>(ExtendTargetAlt))) |
              assign<&ast::ExtendDeclaration::targetGroup>(ref<ExtendSelectorGroup>)) +
             option(assign<&ast::ExtendDeclaration::isOptional>(ref<EXTEND_OPTIONAL>)) +
             StatementEnd> {};

  struct WarnDeclaration
      : rule<kw<"@warn"> +
             assign<&ast::WarnDeclaration::message>(ValueOrListOrMap) + StatementEnd> {
  };

  struct ErrorDeclaration
      : rule<kw<"@error"> +
             assign<&ast::ErrorDeclaration::message>(ValueOrListOrMap) + StatementEnd> {
  };

  struct RawText : rule<assign<&ast::RawText::text>(ref<AT_VALUE>)> {};

  // No unit glued after `}`: what follows is the value's next text.
  struct AtValueInterpolation
      : rule<create<ast::Interpolation>(
            lit<"#"> + lit<"{"> + assign<&ast::Interpolation::inner>(Value) +
            lit<"}">)> {};

  struct AtStatementDeclaration
      : rule<!kw<"@forward"> + !ExtendKeyword + lit<"@"> +
             assign<&ast::AtStatementDeclaration::name>(IdentLike) +
             many(append<&ast::AtStatementDeclaration::value>(
                 ref<RawText> | ref<AtValueInterpolation>)) +
             (assign<&ast::AtStatementDeclaration::body>(ref<BlockRule>) |
              StatementEnd)> {};

  struct AtRootDeclaration
      : rule<kw<"@at-root"> +
             ((option(lit<"("> +
                      assign<&ast::AtRootDeclaration::queryName>(IdentLike) +
                      lit<":"> +
                      some(append<&ast::AtRootDeclaration::queryValue>(Value)) +
                      lit<")">) +
               assign<&ast::AtRootDeclaration::body>(ref<BlockRule>)) |
              assign<&ast::AtRootDeclaration::rule>(ref<Ruleset>))> {};

  struct DebugDeclaration
      : rule<kw<"@debug"> +
             assign<&ast::DebugDeclaration::value>(ValueOrListOrMap) +
             StatementEnd> {};

  // -- Control flow -----------------------------------------------------

  struct ElseIfClause
      : rule<create<ast::ElseIfClause>(
            kw<"@else"> + kw<"if"> +
            assign<&ast::ElseIfClause::condition>(Value) +
            assign<&ast::ElseIfClause::body>(ref<BlockRule>))> {};

  struct IfDeclaration
      : rule<kw<"@if"> + assign<&ast::IfDeclaration::condition>(Value) +
             assign<&ast::IfDeclaration::body>(ref<BlockRule>) +
             many(append<&ast::IfDeclaration::elseIfs>(ref<ElseIfClause>)) +
             option(kw<"@else"> +
                    assign<&ast::IfDeclaration::elseBody>(ref<BlockRule>))> {};

  struct PlainNumberLit
      : rule<create<ast::NumberLit>(
            assign<&ast::NumberLit::magnitude>(ref<NUMBER>))> {};

  struct ForDeclaration
      : rule<kw<"@for"> + lit<"$"> +
             assign<&ast::ForDeclaration::varName>(IdentLike) + kw<"from"> +
             assign<&ast::ForDeclaration::from>(Value) +
             (kw<"to"> | flag<&ast::ForDeclaration::throughInclusive>(
                             kw<"through">)) +
             assign<&ast::ForDeclaration::to>(Value) +
             assign<&ast::ForDeclaration::body>(ref<BlockRule>)> {};

  struct WhileDeclaration
      : rule<kw<"@while"> + assign<&ast::WhileDeclaration::condition>(Value) +
             assign<&ast::WhileDeclaration::body>(ref<BlockRule>)> {};

  struct EachDeclaration
      : rule<kw<"@each"> +
             some(lit<"$"> +
                      append<&ast::EachDeclaration::varNames>(IdentLike),
                  lit<",">) +
             kw<"in"> + assign<&ast::EachDeclaration::list>(ValueOrListOrMap) +
             assign<&ast::EachDeclaration::body>(ref<BlockRule>)> {};

  // -- Rulesets -------------------------------------------------------

  struct Ruleset
      : rule<assign<&ast::Ruleset::selectors>(ref<SelectorGroup>) +
             assign<&ast::Ruleset::body>(ref<BlockRule>)> {};

  // -- Statements, blocks -------------------------------------------------

  struct ExprStatement
      : rule<create<ast::ExprStatement>(
            assign<&ast::ExprStatement::expr>(Value) + lit<";">)> {};

  struct ReturnStmt
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
  struct FunctionBlock
      : rule<create<ast::Block>(
            lit<"{"> +
            many(append<&ast::Block::statements>(ref<StatementAlt> |
                                                 ref<ExprStatement>) |
                 lit<";">) +
            lit<"}">)> {};

  struct StatementAlt
      : rule<ref<ReturnStmt> | ref<ImportDeclaration> |
             ref<VariableDeclaration> | ref<InterpolationDeclaration> |
             ref<PropertyDeclaration> | ref<MediaDeclaration> |
             ref<MixinDeclaration> | ref<ContentDeclaration> |
             ref<FunctionDeclaration> | ref<IfDeclaration> |
             ref<ForDeclaration> | ref<WhileDeclaration> |
             ref<EachDeclaration> | ref<FontFaceDeclaration> |
             ref<KeyframesDeclaration> | ref<IncludeDeclaration> |
             ref<ExtendDeclaration> | ref<WarnDeclaration> |
             ref<ErrorDeclaration> | ref<Ruleset> |
             ref<AtRootDeclaration> | ref<DebugDeclaration> |
             ref<AtStatementDeclaration> | ref<PercentageStatement>> {};

  // `@charset` builds nothing, as dart-sass drops it from the stylesheet.
  struct Stylesheet
      : rule<create<ast::Stylesheet>(
            many(append<&ast::Stylesheet::statements>(ref<StatementAlt>) |
                 lit<";"> | (kw<"@charset"> + ref<STRING>)))> {
  };

  using entry = Stylesheet;
  using ignored = list<SPACE, SL_COMMENT, ML_COMMENT>;
};

}  // namespace scss
