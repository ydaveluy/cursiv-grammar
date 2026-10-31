// The scss language: its AST. `ComponentValue` is the shared value family
// (CSS component values plus Sass variables, interpolation, lists and maps);
// `Statement` is the shared block-item family (rulesets, at-rules,
// declarations). A `Block` is reused for every `{ ... }` body, including a
// @function's, since a @function body is a strict superset of an ordinary
// block.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace scss::ast {

enum class AttribOp : std::uint8_t {
  equals,
  includes,
  dashmatch,
  prefix,
  suffix,
  substring,
};

enum class Combinator : std::uint8_t {
  descendant,
  child,
  adjacent_sibling,
  general_sibling,
};

// `!important` / `!default`, trailing almost every value-bearing construct.
enum class Prio : std::uint8_t { none, important, default_ };

struct ComponentValue : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  float64 magnitude;
};

struct PercentageLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view text;
};

// Length/time/frequency/resolution/angle units are kept fused with the
// digits, one text span (`10px`, `4s`): the unit alone decides nothing a
// tool would branch on, so one class covers all of `unit`'s dimensioned
// alternatives.
struct DimensionLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view text;
};

struct StringLit : cursiv::Extends<ComponentValue> {
  string_view value;
};

struct BooleanLit : cursiv::Extends<ComponentValue> {
  boolean value;
};

struct NullLit : cursiv::Extends<ComponentValue> {};

// Also covers a bare measurement unit used as a value (`$x: px`) and a
// vendor-prefixed name (`-webkit-transform`): both are ordinary identifier
// spellings once lexed per state.
struct IdentValue : cursiv::Extends<ComponentValue> {
  string_view name;
};

struct HexColorLit : cursiv::Extends<ComponentValue> {
  string_view text;
};

// `$name`, `ns.$name`, a signed `+$name`/`-$name`, or a custom property
// (`--name`, `isCustomProperty`).
struct VariableRef : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view nsPrefix;
  string_view name;
  boolean isCustomProperty;
};

// `#{ ... }`; `inner` is the interpolated expression, `&` or an `@if`
// expression included (all three share `ComponentValue`). `trailingUnit` is
// a unit glued immediately after the closing brace (`#{$v}px`), read as an
// ordinary identifier rather than through a dedicated unit keyword.
// Text Sass passes through unevaluated: an unknown at-rule's value.
struct RawText : cursiv::Extends<ComponentValue> {
  string_view text;
};

// A custom property's unevaluated source, with Sass interpolations retained.
struct RawCustomValue : cursiv::Extends<ComponentValue> {
  vector<ComponentValue*> parts;
};

struct RawCustomGroup : cursiv::Extends<ComponentValue> {
  string_view open;
  vector<ComponentValue*> parts;
  string_view close;
};

struct Interpolation : cursiv::Extends<ComponentValue> {
  string_view nsPrefix;
  string_view innerNsPrefix;
  ComponentValue* inner = nullptr;
  string_view trailingUnit;
};

struct UrlRef : cursiv::Extends<ComponentValue> {
  string_view value;
};

struct FormatRef : cursiv::Extends<ComponentValue> {
  string_view value;
};

// `&`, the parent selector reference -- a selector item and an ordinary
// value both (interpolation, @if conditions).
struct ParentRef : cursiv::Extends<ComponentValue> {};

// `($n - 1) * $gutter-width`; multiplicative > additive > relational >
// equality > and > or.
enum class Op : std::uint8_t {
  divide,
  times,
  minus,
  plus,
  greater,
  greater_eq,
  less,
  less_eq,
  eq,  // `==`, or a bare `=` (folded)
  not_equal,
  and_,
  or_,
  modulo,
};

struct BinaryExpr : cursiv::Extends<ComponentValue> {
  ComponentValue* left = nullptr;
  Op op;
  ComponentValue* right = nullptr;
};

// A leading `not` on an operand (`not $flag`, `if(not $x, ...)`- style
// terms); wraps only when the word is actually present.
struct NotExpr : cursiv::Extends<ComponentValue> {
  ComponentValue* operand = nullptr;
};

// A signed, possibly-empty parenthesised group (`(400px - 200px)`, the bare
// `()` Sass also allows).
struct ParenGroup : cursiv::Extends<ComponentValue> {
  boolean negative;
  ComponentValue* inner = nullptr;
};

// A bare `!important`/`!default` used as one of `if()`'s three arguments --
// the only spot it is reachable from (otherwise indistinguishable from the
// trailing `prio?` that follows almost every value-bearing construct).
struct PrioValue : cursiv::Extends<ComponentValue> {
  boolean isDefault;
};

// `if(cond, then, else)` used as a value.
struct IfExpr : cursiv::Extends<ComponentValue> {
  ComponentValue* condition = nullptr;
  ComponentValue* thenValue = nullptr;
  ComponentValue* elseValue = nullptr;
  string_view trailingUnit;
  Prio prio = Prio::none;
};

struct CalcExpr : cursiv::Extends<ComponentValue> {
  ComponentValue* sum = nullptr;
};

// rgba() keeps its own class: unlike a generic call, its arguments may run
// with no separating comma (modern `rgba(r g b / a)` syntax).
struct RgbaCall : cursiv::Extends<ComponentValue> {
  vector<ComponentValue*> args;
};

struct RepeatCall : cursiv::Extends<ComponentValue> {
  ComponentValue* value = nullptr;
  ComponentValue* count = nullptr;
  string_view freqUnit;
};

struct Parameter;

// A generic call: `namespace.name(args)`. Also covers `rotate()`/`var()`,
// which add no shape of their own once per-state lexing removes the need
// for merged lookahead tokens. Args share
// `Parameter`'s shape with a signature's (`color.scale($base-color,
// $lightness: -10%)`): a call site and a signature parse the same shape.
struct FunctionCall : cursiv::Extends<ComponentValue> {
  string_view nsPrefix;
  string_view name;
  vector<Parameter*> args;
};

// A `[...]`/`(...)`/bare run of items, comma- and/or space-separated
// (`listCommaSeparated`, `listSpaceSeparated`, `listBracketed` unified: the
// reference already lets a `listElement` mix both separators).
struct ListValue : cursiv::Extends<ComponentValue> {
  boolean bracketed;
  boolean commaSeparated;
  vector<ComponentValue*> items;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  ComponentValue* key = nullptr;
  ComponentValue* value = nullptr;
};

struct MapValue : cursiv::Extends<ComponentValue> {
  vector<MapEntry*> entries;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

// `expr;` used as a statement -- legal anywhere a block is, though the
// reference only reaches it inside a @function body.
struct ExprStatement : cursiv::Extends<Statement> {
  ComponentValue* expr = nullptr;
};

struct ReturnStmt : cursiv::Extends<Statement> {
  vector<ComponentValue*> values;
};

// Every `{ ... }` body: a ruleset's, a mixin's, a @function's, an @if
// branch's. Returns remain in the statement sequence.
struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

// A signature or call-site argument: `$image`, `$x: 50%`, `$rest...`.
// `defaultValue` is set only by the named-default form; a bare name's
// value/reference lives in `value` alone.
struct Parameter : cursiv::Extends<cursiv::AstNode> {
  ComponentValue* value = nullptr;
  ComponentValue* defaultValue = nullptr;
  boolean isRest;
  Prio prio = Prio::none;
};

enum class ImportKind : std::uint8_t { import_, require, use, forward };
enum class ForwardVisibility : std::uint8_t { all, show, hide };

struct MediaQuery;

struct ImportDeclaration : cursiv::Extends<Statement> {
  ImportKind kind;
  ComponentValue* path = nullptr;
  vector<ComponentValue*> morePaths;  // `@import "a", "b"`
  vector<ComponentValue*> modifiers;  // `@import "a" layer(base)`
  vector<MediaQuery*> media;  // `@import "a" screen`
  boolean hasAs;
  boolean asModule;  // `as *`
  string_view asName;  // @use namespace, or @forward prefix without `*`
  ForwardVisibility forwardVisibility = ForwardVisibility::all;
  vector<string_view> forwardedNames;
  vector<string_view> forwardedVariables;  // names without `$`
  vector<Parameter*> withArgs;
  boolean hasWith;
};

struct VariableDeclaration : cursiv::Extends<Statement> {
  boolean negative;
  boolean isDefault;
  boolean isGlobal;
  string_view nsPrefix;
  string_view name;
  boolean isCustomProperty;
  ComponentValue* value = nullptr;
  Prio prio = Prio::none;
};

struct PropertyDeclaration : cursiv::Extends<Statement> {
  string_view property;
  vector<ComponentValue*> suffix;  // interpolations and `-` glued after it
  vector<ComponentValue*> value;
  Prio prio = Prio::none;
  // `margin: 5px { top: 2px; }`, the nested-property shorthand.
  Block* nestedBlock = nullptr;
};

struct InterpolationDeclaration : cursiv::Extends<Statement> {
  boolean leadingDash;  // `-#{$x}`
  Interpolation* key = nullptr;
  vector<ComponentValue*> suffix;  // interpolations and `-` glued after it
  vector<ComponentValue*> value;
  Prio prio = Prio::none;
  Block* nestedBlock = nullptr;
};

struct MediaExpression : cursiv::Extends<ComponentValue> {
  string_view feature;
  ComponentValue* value = nullptr;  // null when the `: value` clause is absent
};

struct MediaQuery : cursiv::Extends<cursiv::AstNode> {
  boolean isOnly;
  boolean isNot;
  ComponentValue* typeOrValue = nullptr;  // name or interpolation, may be null
  // Features in parens and interpolations, joined by `and` unless
  // `joinedByOr`; `conditionNot` is `screen and not (...)`.
  vector<ComponentValue*> expressions;
  boolean conditionNot;
  boolean joinedByOr;
};

struct MediaDeclaration : cursiv::Extends<Statement> {
  vector<MediaQuery*> queries;
  Block* body = nullptr;
};

struct MixinDeclaration : cursiv::Extends<Statement> {
  string_view name;
  boolean hasParams;
  vector<Parameter*> params;
  Block* body = nullptr;
};

struct ContentDeclaration : cursiv::Extends<Statement> {
  boolean hasArgs;
  vector<Parameter*> args;
};

struct FunctionDeclaration : cursiv::Extends<Statement> {
  string_view nsPrefix;
  string_view name;
  vector<Parameter*> params;
  Block* body = nullptr;
};

struct FontFaceDeclaration : cursiv::Extends<Statement> {
  Block* body = nullptr;
};

struct PercentageStatement : cursiv::Extends<Statement> {
  PercentageLit* percentage = nullptr;
  Block* body = nullptr;
};

struct KeyframesDeclaration : cursiv::Extends<Statement> {
  string_view name;
  Block* body = nullptr;
};

struct IncludeDeclaration : cursiv::Extends<Statement> {
  string_view nsPrefix;
  string_view name;
  boolean hasArgs;
  vector<Parameter*> args;
  boolean hasUsing;
  vector<Parameter*> usingParams;
  Block* body = nullptr;
};

// Every selector fragment shares `ComponentValue`: a functional pseudo's
// argument list (`:not([disabled])`, `:nth-child(2n+1)`) then needs no
// separate shape from an ordinary value's, and `:not(...)` is just a
// `PseudoSelector` whose args happen to be selector fragments.
struct TypeRef : cursiv::Extends<ComponentValue> {
  boolean hasNsPrefix;
  string_view nsPrefix;  // may be "*"
  boolean isPlaceholder;  // leading `%`
  boolean hasParentRefPrefix;  // leading `&`
  boolean universal;
  ComponentValue* name = nullptr;  // IdentValue or VariableRef; null when universal
};

struct ClassSelector : cursiv::Extends<ComponentValue> {
  // Literal runs (IdentValue) mixed with interpolated ones
  // (`.foo-#{$x}-bar`).
  vector<ComponentValue*> parts;
};

struct IdSelector : cursiv::Extends<ComponentValue> {
  string_view name;  // leading `#` included, like HexColorLit::text
};

struct AttribSelector : cursiv::Extends<ComponentValue> {
  boolean hasNsPrefix;
  string_view nsPrefix;
  string_view name;
  optional<AttribOp> op;
  string_view value;
};

struct PseudoSelector : cursiv::Extends<ComponentValue> {
  boolean isElement;  // `::` rather than `:`
  string_view name;
  Interpolation* interp = nullptr;  // set instead of `name` when interpolated
  boolean isFunctional;
  vector<ComponentValue*> args;
};

struct SimpleSelectorSequence : cursiv::Extends<cursiv::AstNode> {
  Combinator combinator = Combinator::descendant;
  TypeRef* type = nullptr;
  vector<ComponentValue*> items;
};

struct Selector : cursiv::Extends<cursiv::AstNode> {
  vector<SimpleSelectorSequence*> sequences;
};

struct SelectorGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Selector*> selectors;
};

struct Ruleset : cursiv::Extends<Statement> {
  SelectorGroup* selectors = nullptr;
  Block* body = nullptr;
};

struct ExtendDeclaration : cursiv::Extends<Statement> {
  boolean isOptional;
  boolean isPlaceholder;  // leading `%`
  boolean hasParentRefPrefix;
  vector<ComponentValue*> targets;
  SelectorGroup* targetGroup = nullptr;  // selector-list fallback; targets stays legacy-first
};

struct WarnDeclaration : cursiv::Extends<Statement> {
  ComponentValue* message = nullptr;
};

struct ErrorDeclaration : cursiv::Extends<Statement> {
  ComponentValue* message = nullptr;
};

// Any `@name value? { ... }` or `@name value?;` the grammar has no
// dedicated shape for: its value is raw text and interpolations.
struct AtStatementDeclaration : cursiv::Extends<Statement> {
  string_view name;
  vector<ComponentValue*> value;
  Block* body = nullptr;
};

struct AtRootDeclaration : cursiv::Extends<Statement> {
  string_view queryName;  // `with`/`without`, empty without a query
  vector<ComponentValue*> queryValue;
  Block* body = nullptr;
  Ruleset* rule = nullptr;  // `@at-root .a { ... }`
};

struct DebugDeclaration : cursiv::Extends<Statement> {
  ComponentValue* value = nullptr;
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  ComponentValue* condition = nullptr;
  Block* body = nullptr;
};

struct IfDeclaration : cursiv::Extends<Statement> {
  ComponentValue* condition = nullptr;
  Block* body = nullptr;
  vector<ElseIfClause*> elseIfs;
  Block* elseBody = nullptr;
};

struct ForDeclaration : cursiv::Extends<Statement> {
  string_view varName;
  ComponentValue* from = nullptr;
  boolean throughInclusive;  // `through` rather than `to`
  ComponentValue* to = nullptr;
  Block* body = nullptr;
};

struct WhileDeclaration : cursiv::Extends<Statement> {
  ComponentValue* condition = nullptr;
  Block* body = nullptr;
};

struct EachDeclaration : cursiv::Extends<Statement> {
  vector<string_view> varNames;
  ComponentValue* list = nullptr;  // a ListValue or MapValue
  Block* body = nullptr;
};

struct Stylesheet : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace scss::ast
