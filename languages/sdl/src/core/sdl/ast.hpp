// The sdl language: its AST. `Item` is the base of anything a module body or
// an aggregate body may hold, `TypeRef` of a field's declared type, `Expr` of
// the SDL expression language (`.`/`^`/`:` are location-counter atoms).
// A clause that may repeat (SDL's DUPCONATT) keeps each value in its list.
#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace sdl::ast {

enum class Modifier { signed_, unsigned_, mask_, varying_, value_, descriptor_, rtlStrDesc_, in_, out_, list_, optional_, reference_, returnVoid, variable_, fill_, align_, noalign_, common_, global_, typedef_, based_ };

template <class T>
bool has_modifier(const std::vector<T>& values, T wanted) {
  for (const auto value : values)
    if (value == wanted) return true;
  return false;
}

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Expr> {
  int64 value;
};

struct LocalRef : cursiv::Extends<Expr> {
  string_view name;  // includes the leading '#'
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct StringRef : cursiv::Extends<Expr> {
  string text;
};

// '.', '^' or ':' used as a bare expression atom (a location-counter form).
struct LocationRef : cursiv::Extends<Expr> {
  string_view symbol;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

enum class BinOp : std::uint8_t { multiply, divide, add, subtract, at, bang, amp };

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

// A dimension bound: a plain expression, a `lo:hi` range, or `*`.
struct DimSpec : cursiv::Extends<cursiv::AstNode> {
  Expr* lo;
  Expr* hi;
  boolean star;
};

struct TypeRef;

// A base alignment: a type to align on, or a parenthesised size.
struct BaseAlign : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  Expr* size;
};

struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

// The parenthesised content after ADDRESS/POINTER*: either a type (with an
// optional dimension) or a nested `ENTRY` signature -- `ADDRESS(ENTRY ...)`.
struct EntryOpts;
struct ObjectSpec : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  DimSpec* dimension;
  EntryOpts* entry;
  BaseAlign* basealign;
};

// A built-in type: one keyword plus whichever of its options apply.
// `length`/`lengthStar` serve both BITFIELD's LENGTH (a plain expression) and
// CHARACTER's LENGTH (which may be `*`); which applies is read off `keyword`.
struct PrimitiveType : cursiv::Extends<TypeRef> {
  vector<Modifier> modifiers;
  string_view keyword;
  bool signed_() const { return has_modifier(modifiers, Modifier::signed_); }
  bool unsigned_() const { return has_modifier(modifiers, Modifier::unsigned_); }
  boolean complex_;
  bool mask_() const { return has_modifier(modifiers, Modifier::mask_); }
  bool varying_() const { return has_modifier(modifiers, Modifier::varying_); }
  Expr* precision;
  Expr* scale;
  Expr* length;
  boolean lengthStar;
  ObjectSpec* object;
};

// A named (user-defined) type, optionally sized by `SIZEOF`.
struct NamedType : cursiv::Extends<TypeRef> {
  string name;
  TypeRef* sizeofType;
  Expr* sizeofExpr;
};

// A parameter: a declared type, or the bare `ANY` keyword, plus its flags.
struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Modifier> modifiers;
  boolean any_;
  TypeRef* type;
  bool value_() const { return has_modifier(modifiers, Modifier::value_); }
  bool descriptor_() const { return has_modifier(modifiers, Modifier::descriptor_); }
  bool rtlStrDesc_() const { return has_modifier(modifiers, Modifier::rtlStrDesc_); }
  vector<DimSpec*> dimensions;
  bool in_() const { return has_modifier(modifiers, Modifier::in_); }
  bool out_() const { return has_modifier(modifiers, Modifier::out_); }
  bool list_() const { return has_modifier(modifiers, Modifier::list_); }
  vector<string> named;
  vector<Expr*> defaults;
  bool optional_() const { return has_modifier(modifiers, Modifier::optional_); }
  bool reference_() const { return has_modifier(modifiers, Modifier::reference_); }
  vector<string> typeNames;
};

// An entry's signature: standalone (an `ENTRY` statement) or nested inside
// `ADDRESS(ENTRY ...)`.
struct EntryOpts : cursiv::Extends<cursiv::AstNode> {
  vector<Modifier> modifiers;
  vector<Param*> params;
  vector<TypeRef*> returnTypes;
  bool returnVoid() const { return has_modifier(modifiers, Modifier::returnVoid); }
  vector<string> returnNames;
  bool variable_() const { return has_modifier(modifiers, Modifier::variable_); }
  vector<string> typeNames;
  vector<string> aliases;
  vector<string> linkages;
};

struct EntryDecl : cursiv::Extends<Item> {
  string name;
  EntryOpts* opts;
};

// A plain field of an aggregate.
struct Member : cursiv::Extends<Item> {
  vector<Modifier> modifiers;
  string name;
  TypeRef* type;
  vector<string> prefixes;
  vector<string> tags;
  bool fill_() const { return has_modifier(modifiers, Modifier::fill_); }
  bool align_() const { return has_modifier(modifiers, Modifier::align_); }
  bool noalign_() const { return has_modifier(modifiers, Modifier::noalign_); }
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
};

// A nested `STRUCTURE`/`UNION`, in place of a plain field.
struct NestedAggregate : cursiv::Extends<Item> {
  vector<Modifier> modifiers;
  string name;
  string_view kind;
  TypeRef* implType;
  vector<string> prefixes;
  vector<string> tags;
  bool fill_() const { return has_modifier(modifiers, Modifier::fill_); }
  bool align_() const { return has_modifier(modifiers, Modifier::align_); }
  bool noalign_() const { return has_modifier(modifiers, Modifier::noalign_); }
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
  vector<Item*> body;
  string endName;
};

struct AggregateStmt : cursiv::Extends<Item> {
  vector<Modifier> modifiers;
  string name;
  string_view kind;
  TypeRef* implType;
  bool common_() const { return has_modifier(modifiers, Modifier::common_); }
  bool global_() const { return has_modifier(modifiers, Modifier::global_); }
  bool typedef_() const { return has_modifier(modifiers, Modifier::typedef_); }
  bool align_() const { return has_modifier(modifiers, Modifier::align_); }
  bool noalign_() const { return has_modifier(modifiers, Modifier::noalign_); }
  vector<BaseAlign*> basealigns;
  vector<string> prefixes;
  vector<string> tags;
  vector<DimSpec*> dimensions;
  vector<string> origins;
  bool based_() const { return has_modifier(modifiers, Modifier::based_); }
  vector<string> basedNames;
  vector<string> markers;
  bool fill_() const { return has_modifier(modifiers, Modifier::fill_); }
  vector<Item*> body;
  string endName;
};

struct ItemDecl : cursiv::Extends<Item> {
  vector<Modifier> modifiers;
  string name;
  TypeRef* type;
  bool common_() const { return has_modifier(modifiers, Modifier::common_); }
  bool global_() const { return has_modifier(modifiers, Modifier::global_); }
  bool typedef_() const { return has_modifier(modifiers, Modifier::typedef_); }
  vector<string> prefixes;
  vector<string> tags;
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
};

struct DeclareStmt : cursiv::Extends<Item> {
  string name;
  TypeRef* sizeofType;
  Expr* sizeofExpr;
  string prefix;
  string tag;
};

struct LocalAssignment : cursiv::Extends<Item> {
  string_view name;
  Expr* value;
};

struct ConstantPhrase : cursiv::Extends<cursiv::AstNode> {
  vector<string> names;
  boolean isString;
  Expr* value;
  string stringValue;
  vector<Expr*> increments;
  vector<string> prefixes;
  vector<string> tags;
  vector<string_view> counters;
  vector<string> typeNames;
};

struct ConstantStmt : cursiv::Extends<Item> {
  vector<ConstantPhrase*> phrases;
};

struct ElseIfBranch : cursiv::Extends<cursiv::AstNode> {
  string symbol;
  vector<Item*> body;
};

// A symbol conditional's branches are all `vector<Item*>`, so one class
// covers it both at module level and nested inside an aggregate.
struct IfSymbolStmt : cursiv::Extends<Item> {
  string symbol;
  vector<Item*> thenBody;
  vector<ElseIfBranch*> elseIfs;
  boolean hasElse;
  vector<Item*> elseBody;
};

struct ConditionalStmt : cursiv::Extends<Item> {
  vector<string> languages;
  vector<Item*> body;
  vector<string> endLanguages;
};

struct LiteralStmt : cursiv::Extends<Item> {
  string_view text;  // exact pass-through source between LITERAL; and END_LITERAL
};

struct ReadStmt : cursiv::Extends<Item> {
  string path;
};

struct IncludeStmt : cursiv::Extends<Item> {
  string path;
};

struct Module : cursiv::Extends<Item> {
  string name;
  string ident;
  vector<Item*> body;
  string endName;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace sdl::ast
