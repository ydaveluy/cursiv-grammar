// The sdl language: its AST. `Item` is the base of anything a module body or
// an aggregate body may hold, `TypeRef` of a field's declared type, `Expr` of
// the SDL expression language (`.`/`^`/`:` are location-counter atoms).
// A clause that may repeat (SDL's DUPCONATT) keeps each value in its list.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sdl::ast {

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
  string_view text;
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
  string_view keyword;
  boolean signed_;
  boolean unsigned_;
  boolean complex_;
  boolean mask_;
  boolean varying_;
  Expr* precision;
  Expr* scale;
  Expr* length;
  boolean lengthStar;
  ObjectSpec* object;
};

// A named (user-defined) type, optionally sized by `SIZEOF`.
struct NamedType : cursiv::Extends<TypeRef> {
  string_view name;
  TypeRef* sizeofType;
  Expr* sizeofExpr;
};

// A parameter: a declared type, or the bare `ANY` keyword, plus its flags.
struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean any_;
  TypeRef* type;
  boolean value_;
  boolean descriptor_;
  boolean rtlStrDesc_;
  vector<DimSpec*> dimensions;
  boolean in_;
  boolean out_;
  boolean list_;
  vector<string_view> named;
  vector<Expr*> defaults;
  boolean optional_;
  boolean reference_;
  vector<string_view> typeNames;
};

// An entry's signature: standalone (an `ENTRY` statement) or nested inside
// `ADDRESS(ENTRY ...)`.
struct EntryOpts : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  vector<TypeRef*> returnTypes;
  boolean returnVoid;
  vector<string_view> returnNames;
  boolean variable_;
  vector<string_view> typeNames;
  vector<string_view> aliases;
  vector<string_view> linkages;
};

struct EntryDecl : cursiv::Extends<Item> {
  string_view name;
  EntryOpts* opts;
};

// A plain field of an aggregate.
struct Member : cursiv::Extends<Item> {
  string_view name;
  TypeRef* type;
  vector<string_view> prefixes;
  vector<string_view> tags;
  boolean fill_;
  boolean align_;
  boolean noalign_;
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
};

// A nested `STRUCTURE`/`UNION`, in place of a plain field.
struct NestedAggregate : cursiv::Extends<Item> {
  string_view name;
  string_view kind;
  TypeRef* implType;
  vector<string_view> prefixes;
  vector<string_view> tags;
  boolean fill_;
  boolean align_;
  boolean noalign_;
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
  vector<Item*> body;
  string_view endName;
};

struct AggregateStmt : cursiv::Extends<Item> {
  string_view name;
  string_view kind;
  TypeRef* implType;
  boolean common_;
  boolean global_;
  boolean typedef_;
  boolean align_;
  boolean noalign_;
  vector<BaseAlign*> basealigns;
  vector<string_view> prefixes;
  vector<string_view> tags;
  vector<DimSpec*> dimensions;
  vector<string_view> origins;
  boolean based_;
  vector<string_view> basedNames;
  vector<string_view> markers;
  boolean fill_;
  vector<Item*> body;
  string_view endName;
};

struct ItemDecl : cursiv::Extends<Item> {
  string_view name;
  TypeRef* type;
  boolean common_;
  boolean global_;
  boolean typedef_;
  vector<string_view> prefixes;
  vector<string_view> tags;
  vector<DimSpec*> dimensions;
  vector<BaseAlign*> basealigns;
};

struct DeclareStmt : cursiv::Extends<Item> {
  string_view name;
  TypeRef* sizeofType;
  Expr* sizeofExpr;
  string_view prefix;
  string_view tag;
};

struct LocalAssignment : cursiv::Extends<Item> {
  string_view name;
  Expr* value;
};

struct ConstantPhrase : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  boolean isString;
  Expr* value;
  string_view stringValue;
  vector<Expr*> increments;
  vector<string_view> prefixes;
  vector<string_view> tags;
  vector<string_view> counters;
  vector<string_view> typeNames;
};

struct ConstantStmt : cursiv::Extends<Item> {
  vector<ConstantPhrase*> phrases;
};

struct ElseIfBranch : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  vector<Item*> body;
};

// A symbol conditional's branches are all `vector<Item*>`, so one class
// covers it both at module level and nested inside an aggregate.
struct IfSymbolStmt : cursiv::Extends<Item> {
  string_view symbol;
  vector<Item*> thenBody;
  vector<ElseIfBranch*> elseIfs;
  boolean hasElse;
  vector<Item*> elseBody;
};

struct ConditionalStmt : cursiv::Extends<Item> {
  vector<string_view> languages;
  vector<Item*> body;
  vector<string_view> endLanguages;
};

struct LiteralStmt : cursiv::Extends<Item> {
  vector<string_view> strings;
};

struct ReadStmt : cursiv::Extends<Item> {
  string_view path;
};

struct IncludeStmt : cursiv::Extends<Item> {
  string_view path;
};

struct Module : cursiv::Extends<Item> {
  string_view name;
  string_view ident;
  vector<Item*> body;
  string_view endName;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace sdl::ast
