// The less language: its AST. LESS is CSS with variables, nesting, mixins
// and interpolation -- the tree mirrors that: a Stylesheet of Statements, a
// Block holding a mix of Property/Statement/MixinRef, and Name/Interp
// carrying a possibly-interpolated identifier apart wherever CSS allows one.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace less::ast {

enum class MathOp : std::uint8_t { times, plus, div, minus, mod };
enum class CompareOp : std::uint8_t { eq, lt, gt, ge, le };
enum class AttribRelate : std::uint8_t { equals, includes, dashmatch };
enum class SelectorPrefix : std::uint8_t { child, sibling, general_sibling };
enum class ImportOption : std::uint8_t {
  reference,
  inline_,
  less,
  css,
  once,
  multiple,
};

// `@{name}`: one interpolated piece of an otherwise-literal name.
struct Interp : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// A name that may be spliced from literal text and interpolations:
// `@{prefix}-suffix`, a bare identifier, a property or selector name.
struct Name : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<string_view, Interp *>> parts;
};

// `@x` (leaf: `name` set) or `@@x` (indirection: `inner` set) -- a variable
// used as a name: a declaration's key or a mixin parameter.
struct VariableName : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  VariableName *inner = nullptr;
};

// The value-expression family: one node per literal shape `expression`
// accepts, plus the two ways a name can be used as a value.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct CommandStatement;
struct ParenExpr : cursiv::Extends<Expr> {
  CommandStatement *value;
};

struct Measurement : cursiv::Extends<Expr> {
  float64 value;
  string_view unit;
};

struct ColorLit : cursiv::Extends<Expr> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
  boolean escaped; // Less ~"...": emit the value without its quotes.
};

struct UrlLit : cursiv::Extends<Expr> {
  string_view value;
};

struct IdentValue : cursiv::Extends<Expr> {
  Name *name = nullptr;
  boolean important;
};

struct FuncCall : cursiv::Extends<Expr> {
  Name *name = nullptr;
  vector<CommandStatement *> args;
};

struct VarValue : cursiv::Extends<Expr> {
  VariableName *name = nullptr;
  boolean important;
};

// One multiplicative term: a juxtaposed run of values, with the `*`/`/`/`%`
// that introduced it (empty for a chain's first term).
struct MulTerm : cursiv::Extends<cursiv::AstNode> {
  optional<MathOp> op;
  vector<Expr *> items;
};

// A left-associative `*`/`/`/`%` chain: binds tighter than `+`/`-`.
struct MulExpr : cursiv::Extends<cursiv::AstNode> {
  vector<MulTerm *> terms;
};

// One additive term: a multiplicative chain, with the `+`/`-` that
// introduced it (empty for a chain's first term).
struct AddTerm : cursiv::Extends<cursiv::AstNode> {
  optional<MathOp> op;
  MulExpr *operand = nullptr;
};

// A value expression: a left-associative `+`/`-` chain of multiplicative
// terms.
struct CommandStatement : cursiv::Extends<cursiv::AstNode> {
  vector<AddTerm *> terms;
};

struct ValueList : cursiv::Extends<cursiv::AstNode> {
  vector<CommandStatement *> items;
};

struct ConditionStatement : cursiv::Extends<cursiv::AstNode> {
  CommandStatement *left = nullptr;
  optional<CompareOp> op;
  CommandStatement *right = nullptr;
};

struct Condition : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  ConditionStatement *test = nullptr;
};

struct Attrib : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<AttribRelate> relate;
  string_view value;
};

// The selector family: one class per `element` alternative, so a tool can
// tell a class/tag name from a combinator or a pseudo-selector.
struct Element : cursiv::Extends<cursiv::AstNode> {};

struct Selector;

struct NamedElement : cursiv::Extends<Element> {
  optional<SelectorPrefix> prefix;
  Name *name = nullptr;
};

struct IdElement : cursiv::Extends<Element> {
  Name *name = nullptr;
};

struct ParentRefElement : cursiv::Extends<Element> {};
struct WildcardElement : cursiv::Extends<Element> {};

struct Pseudo : cursiv::Extends<Element> {
  boolean isClass; // `::` rather than `:`
  string_view name;
};

struct Negation : cursiv::Extends<Element> {
  vector<Selector *> alternatives;
};

struct Selector : cursiv::Extends<cursiv::AstNode> {
  vector<Element *> elements;
  vector<Attrib *> attribs;
  Pseudo *trailingPseudo = nullptr;
};

struct Selectors : cursiv::Extends<cursiv::AstNode> {
  vector<Selector *> items;
};

// A statement of a stylesheet or a mixin body: import, nested ruleset,
// variable declaration, mixin definition.
struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct Block;
struct AtRule : cursiv::Extends<Statement> {
  string_view name;
  string_view prelude;
  Block *body;
};

struct Import : cursiv::Extends<Statement> {
  vector<ImportOption> options;
  string_view url;
  vector<string_view> mediaTypes;
};

struct VariableDecl : cursiv::Extends<Statement> {
  VariableName *name = nullptr;
  ValueList *value = nullptr;
};

struct Property : cursiv::Extends<cursiv::AstNode> {
  Name *name = nullptr;
  ValueList *value = nullptr;
};

struct MixinRef : cursiv::Extends<cursiv::AstNode> {
  Selector *target = nullptr;
  vector<CommandStatement *> args;
  boolean important;
};

// A block's body, in source order: a declaration, a nested statement, or a
// mixin call -- the three things `block` interleaves.
struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Property *, Statement *, MixinRef *>> items;
};

struct Ruleset : cursiv::Extends<Statement> {
  Selectors *selectors = nullptr;
  Block *body = nullptr;
};

struct MixinDef : cursiv::Extends<Statement> {
  Selectors *selectors = nullptr;
  vector<std::variant<VariableName *, VariableDecl *>> params;
  boolean variadic;
  vector<Condition *> guard;
  Block *body = nullptr;
};

struct Stylesheet : cursiv::Extends<cursiv::AstNode> {
  vector<Statement *> statements;
};

} // namespace less::ast
