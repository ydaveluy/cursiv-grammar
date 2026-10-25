// The css3 language: its AST. A Stylesheet of Statements (at-rules and
// rulesets); declaration values, selector attribute values, media features,
// calc() operands and an unrecognized construct's generic fallback all
// share one ComponentValue hierarchy -- CSS itself treats them as the same
// kind of thing (a component value), so the tree does too.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace css3::ast {

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

enum class CalcOp : std::uint8_t { add, subtract, multiply, divide };

// The value family: one class per literal shape a component value can take.
struct ComponentValue : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  float64 magnitude;
};

// Percentage/dimension/unknown-dimension keep their unit fused with the
// digits in one token (`10px`, `50%`): the raw spelling is kept whole rather
// than split into magnitude/unit fields, one node per literal shape suffices.
struct PercentageLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view text;
};

struct DimensionLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view text;
};

struct UnknownDimensionLit : cursiv::Extends<ComponentValue> {
  boolean negative;
  string_view text;
};

struct StringLit : cursiv::Extends<ComponentValue> {
  string_view value;
};

struct IdentValue : cursiv::Extends<ComponentValue> {
  string_view name;
};

struct UnicodeRangeLit : cursiv::Extends<ComponentValue> {
  string_view text;
};

// `#rgb`/`#rrggbb` as a value (hexcolor), an id selector, or a bare token
// inside an unrecognized construct -- one shape, the three call sites read
// it the same way.
struct HashRef : cursiv::Extends<ComponentValue> {
  string_view text;
};

struct UrlRef : cursiv::Extends<ComponentValue> {
  string_view value;
};

struct VarRef : cursiv::Extends<ComponentValue> {
  string_view name;
  boolean hasFallback;
  vector<ComponentValue*> fallback;
};

struct CalcProduct : cursiv::Extends<cursiv::AstNode> {
  vector<ComponentValue*> factors;
  vector<CalcOp> ops;  // one fewer than factors, applied left to right
};

struct CalcSum : cursiv::Extends<cursiv::AstNode> {
  vector<CalcProduct*> terms;
  vector<CalcOp> ops;  // one fewer than terms, applied left to right
};

struct CalcExpr : cursiv::Extends<ComponentValue> {
  CalcSum* sum = nullptr;
};

struct FunctionCall : cursiv::Extends<ComponentValue> {
  string_view name;
  vector<ComponentValue*> args;
};

// A stray delimiter kept as a value: a bare `:`, `~=` or `|=` inside an
// unrecognized construct, or an nth-child argument's bare `+`/`-` (not
// arithmetic).
struct PunctValue : cursiv::Extends<ComponentValue> {
  string_view text;
};

// `[...]`/`(...)` read generically, as a bracket/paren group of items: an
// unrecognized declaration's value, or an unknown at-rule's prelude.
struct GroupValue : cursiv::Extends<ComponentValue> {
  boolean bracketed;  // '[' ']' rather than '(' ')'
  vector<ComponentValue*> items;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct Declaration : cursiv::Extends<cursiv::AstNode> {
  boolean iePrefixStar;  // `*color` (IE hack); a leading `_`/`--` already
                         // reads as ordinary identifier bytes, no flag needed
  string_view property;
  vector<ComponentValue*> value;
  boolean important;
};

// A generic `{ ... }` region: an unrecognized at-rule's body, or the rare
// brace nested inside a malformed declaration value. Kept flat rather than
// as separately ordered runs of declarations, nested statements and bare
// tokens.
struct RawBlock : cursiv::Extends<ComponentValue> {
  vector<std::variant<Declaration*, Statement*, ComponentValue*>> items;
};

struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  boolean hasNsPrefix;
  string_view nsPrefix;  // may be empty (default namespace) or "*"
  boolean universal;     // '*' as the element name
  string_view name;      // set when !universal
};

struct ClassSelector : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct IdSelector : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct AttribSelector : cursiv::Extends<cursiv::AstNode> {
  boolean hasNsPrefix;
  string_view nsPrefix;
  string_view name;
  optional<AttribOp> op;
  string_view value;
  string_view modifier;
};

struct Selector;

// `args` is empty for a plain `:name`; non-empty means `:name(...)` --
// `expression` demands at least one item, so emptiness is an unambiguous tag.
struct PseudoSelector : cursiv::Extends<cursiv::AstNode> {
  boolean isElement;  // '::' rather than ':'
  string_view name;
  vector<ComponentValue*> args;
};

using SelectorItem = std::variant<IdSelector*, ClassSelector*,
                                   AttribSelector*, PseudoSelector*>;

// `combinator` names the relation to the PRECEDING sequence (meaningless on
// a selector's first sequence) -- defaulted to `descendant`, the relation a
// bare gap between two sequences denotes when no `>`/`+`/`~` is written.
struct SimpleSelectorSequence : cursiv::Extends<cursiv::AstNode> {
  Combinator combinator = Combinator::descendant;
  TypeRef* type = nullptr;  // leading type/universal selector, if any
  vector<SelectorItem> items;
};

struct Selector : cursiv::Extends<cursiv::AstNode> {
  vector<SimpleSelectorSequence*> sequences;
};

struct SelectorGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Selector*> selectors;
};

struct Ruleset : cursiv::Extends<Statement> {
  SelectorGroup* selectors = nullptr;
  vector<Declaration*> declarations;
};

// A prelude that is not a selector at all: empty, or opening with a value
// no selector can start with (a number, a string, a function call...).
struct UnknownRuleset : cursiv::Extends<Statement> {
  vector<ComponentValue*> prelude;
  vector<Declaration*> declarations;
};

struct MediaExpression : cursiv::Extends<cursiv::AstNode> {
  string_view feature;
  vector<ComponentValue*> value;  // empty when the ':' clause is absent
};

struct MediaQuery : cursiv::Extends<cursiv::AstNode> {
  boolean isOnly;
  boolean isNot;
  string_view mediaType;  // empty when the query is expression-only
  vector<MediaExpression*> expressions;
};

struct MediaQueryList : cursiv::Extends<cursiv::AstNode> {
  vector<MediaQuery*> queries;
};

struct AtMedia : cursiv::Extends<Statement> {
  MediaQueryList* queries = nullptr;
  vector<Statement*> body;
};

struct AtPage : cursiv::Extends<Statement> {
  string_view pseudo;  // empty when absent, e.g. ":first"
  vector<Declaration*> declarations;
};

struct AtFontFace : cursiv::Extends<Statement> {
  vector<Declaration*> declarations;
};

struct KeyframeSelector : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> stops;  // "from", "to", "50%"
};

struct KeyframeBlock : cursiv::Extends<cursiv::AstNode> {
  KeyframeSelector* selector = nullptr;
  vector<Declaration*> declarations;
};

struct AtKeyframes : cursiv::Extends<Statement> {
  string_view name;
  vector<KeyframeBlock*> blocks;
};

// The condition family: `supportsCondition`'s alternatives.
struct SupportsCondition : cursiv::Extends<cursiv::AstNode> {};

struct SupportsDeclaration : cursiv::Extends<SupportsCondition> {
  Declaration* declaration = nullptr;
};

struct SupportsGeneral : cursiv::Extends<SupportsCondition> {
  vector<ComponentValue*> items;
};

struct SupportsParen : cursiv::Extends<SupportsCondition> {
  SupportsCondition* inner = nullptr;
};

struct SupportsNot : cursiv::Extends<SupportsCondition> {
  SupportsCondition* operand = nullptr;
};

struct SupportsAnd : cursiv::Extends<SupportsCondition> {
  vector<SupportsCondition*> operands;
};

struct SupportsOr : cursiv::Extends<SupportsCondition> {
  vector<SupportsCondition*> operands;
};

struct AtSupports : cursiv::Extends<Statement> {
  SupportsCondition* condition = nullptr;
  vector<Statement*> body;
};

struct AtViewport : cursiv::Extends<Statement> {
  vector<Declaration*> declarations;
};

struct AtCounterStyle : cursiv::Extends<Statement> {
  string_view name;
  vector<Declaration*> declarations;
};

struct FeatureValueDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<NumberLit*> values;
};

struct FeatureValueBlock : cursiv::Extends<cursiv::AstNode> {
  string_view type;  // the AtKeyword spelling, e.g. "@styleset"
  vector<FeatureValueDef*> defs;
};

// A family name is a quoted string (one word) or a run of bare identifiers
// (`Times New Roman`) -- the words are kept separate, not concatenated.
struct FontFamilyName : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> words;
};

struct AtFontFeatureValues : cursiv::Extends<Statement> {
  vector<FontFamilyName*> familyNames;
  vector<FeatureValueBlock*> blocks;
};

struct AtCharset : cursiv::Extends<Statement> {
  string_view value;
};

struct AtImport : cursiv::Extends<Statement> {
  string_view url;
  MediaQueryList* media = nullptr;
};

struct AtNamespace : cursiv::Extends<Statement> {
  string_view prefix;  // empty for the default namespace
  string_view url;
};

// Any at-rule the grammar does not name specifically: a vendor extension, or
// one this port has no dedicated shape for.
struct UnknownAtRule : cursiv::Extends<Statement> {
  string_view name;
  vector<ComponentValue*> prelude;
  RawBlock* body = nullptr;
};

// CSS orders statements by kind -- @charset, then @import, then
// @namespace, then general rules -- as four separate runs; this port keeps
// one flat ordered list instead: position
// among statements is preserved, the grouping is not.
struct Stylesheet : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace css3::ast
