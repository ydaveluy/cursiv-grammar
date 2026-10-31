// The hexpat language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace hexpat::ast {

// The expression family. Empty by design: every operand member holds an
// `Expr*`, so the builder's family check is against this base and any
// alternative fits any operand slot.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

// Declaration-only array initialization, not a general brace expression.
struct ArrayInitializer : cursiv::Extends<Expr> {
  vector<Expr*> values;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// `$`, the current cursor offset in the pattern.
struct DollarExpr : cursiv::Extends<Expr> {};

// `+x`, `-x`, `~x`, `!x`. The operator's text, so a consumer does not have to
// know the set.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `a.b` and `a::b`: `scoped` tells the two apart.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
  boolean scoped;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct AddressofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// `sizeof(x)` or `sizeof(u32)`: exactly one of the two is set.
struct SizeofExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view primType;
};

struct TypenameofExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view primType;
};

// `be u32(offset)`: a read of a primitive type at an expression address.
struct CastCallExpr : cursiv::Extends<Expr> {
  string_view endian;
  string_view type;
  Expr* operand;
};

// Every binary level of the operator ladder. `op` holds the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

// Right-associative; the target is a unary-tier operand, which keeps
// `a + b = c` out of the language the way the assignment target in most
// C-family grammars does.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// `(ns::)*name`, `name` alone, or a primitive keyword's spelling.
struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> path;
  string_view name;
};

struct TypeArg;

// `auto? typeName typeArguments?`, or a bare expression -- a literal is
// already an expression, so that redundant alternative folds into this one.
struct TypeArg : cursiv::Extends<cursiv::AstNode> {
  boolean isAuto;
  TypeRef* type;
  vector<TypeArg*> typeArgs;
  Expr* expr;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  boolean isAuto;
  TypeRef* type;
};

struct AttrItem : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;   // set for the call-shaped form, name(...)
  vector<Expr*> args;
  string_view name;  // set for the bare-identifier form
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  vector<AttrItem*> items;
};

// `@ offsetExpression` with an optional `in sectionExpression`.
struct At : cursiv::Extends<cursiv::AstNode> {
  Expr* expr;
  Expr* section;
};

// `expr` or `expr ... expr` (a range), used by enum values and match
// patterns.
struct PatternItem : cursiv::Extends<cursiv::AstNode> {
  Expr* from;
  Expr* to;
};

struct PatternElem : cursiv::Extends<cursiv::AstNode> {
  vector<PatternItem*> alts;
};

// Base of anything a struct/union/bitfield/block body, a namespace, or the
// document itself may hold. The grammar, not this type, keeps a function
// definition out of a block.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Import : cursiv::Extends<Item> {
  vector<string_view> parts;
  StringLit* source;
  string_view alias;
  boolean all;
};

// `using Name<T, auto N> = Type;`, or a forward declaration without `=`.
struct UsingAlias : cursiv::Extends<Item> {
  string_view name;
  vector<TypeParam*> typeParams;
  boolean isRef;
  string_view endian;
  TypeRef* type;
  vector<TypeArg*> typeArgs;
  Attribute* attr;
};

struct Namespace : cursiv::Extends<Item> {
  string_view name;
  boolean auto_;
  vector<Item*> items;
};

struct Block;

struct Struct : cursiv::Extends<Item> {
  string_view name;
  vector<TypeParam*> typeParams;
  TypeRef* base;
  Block* body;
  Attribute* attr;
};

struct Union : cursiv::Extends<Item> {
  string_view name;
  vector<TypeParam*> typeParams;
  Block* body;
  Attribute* attr;
};

struct Bitfield : cursiv::Extends<Item> {
  string_view name;
  vector<TypeParam*> typeParams;
  Block* body;
  Attribute* attr;
};

struct EnumEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  PatternItem* value;
};

struct Enum : cursiv::Extends<Item> {
  string_view name;
  TypeRef* type;
  vector<EnumEntry*> entries;
  Attribute* attr;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean isRef;
  string_view endian;
  TypeRef* type;
  vector<TypeArg*> typeArgs;
  string_view name;
  Expr* defaultValue;
  boolean isPack;
};

struct Fn : cursiv::Extends<Item> {
  string_view name;
  vector<Param*> params;
  Block* body;
};

// A brace-delimited body: a struct/union/bitfield/function block, and a
// nested `{ ... }` anywhere inside one. One class for all four contexts --
// the grammar rule that recurses into it is what keeps a function body out
// of a struct.
struct Block : cursiv::Extends<Item> {
  vector<Item*> items;
};

struct StructMemberDeclarator : cursiv::Extends<cursiv::AstNode> {
  boolean isPointer;
  string_view name;
  boolean hasBrackets;  // `[` was written at all -- `size`/`sizeWhile` alone
                        // cannot tell `foo[]` (unknown length) from a scalar
  Expr* size;
  Expr* sizeWhile;
  TypeRef* colonType;
  Expr* init;
  At* at;
  Attribute* attr;
};

struct StructMember : cursiv::Extends<Item> {
  boolean isConst;
  string_view endian;
  TypeRef* type;
  vector<TypeArg*> typeArgs;
  vector<StructMemberDeclarator*> declarators;
  string_view io;
};

struct UnionMember : cursiv::Extends<Item> {
  TypeRef* type;
  string_view name;
  boolean hasBrackets;
  Expr* size;
  Expr* sizeWhile;
  Expr* init;
  At* at;
  Attribute* attr;
};

// `pointedType* name : sizeType` -- a union member read through a pointer.
struct UnionPtrMember : cursiv::Extends<Item> {
  TypeRef* pointedType;
  string_view name;
  TypeRef* sizeType;
  Attribute* attr;
};

// `padding: n` is `isPadding` with `name` empty; the ordinary field form
// sets `name` (and optionally `type`, `init`, `width`).
struct BitfieldMember : cursiv::Extends<Item> {
  boolean isPadding;
  TypeRef* type;
  string_view name;
  Expr* init;
  Expr* width;
  Attribute* attr;
};

// `padding[n];` or `padding[while(cond)];`.
struct Padding : cursiv::Extends<Item> {
  Expr* size;
  Expr* whileCond;
};

// One `IfStmt`/`MatchStmt`/`Block`/... class reused across the struct,
// union, bitfield and function-body grammar contexts: four parallel grammar
// rules each recurse into their own context's statement rule, so the branch
// a document actually takes is still the grammar's to say, not this type's.
struct IfStmt : cursiv::Extends<Item> {
  Expr* cond;
  Item* thenBranch;
  Item* elseBranch;
};

struct WhileStmt : cursiv::Extends<Item> {
  Expr* cond;
  Item* body;
};

struct VarDecl;

struct ForStmt : cursiv::Extends<Item> {
  VarDecl* initDecl;
  vector<Expr*> initExprs;
  Expr* cond;
  vector<Expr*> update;
  Item* body;
};

struct MatchCase : cursiv::Extends<cursiv::AstNode> {
  vector<PatternElem*> patterns;
  Item* body;
};

struct MatchStmt : cursiv::Extends<Item> {
  vector<Expr*> subjects;
  vector<MatchCase*> cases;
};

struct BreakStmt : cursiv::Extends<Item> {};
struct ContinueStmt : cursiv::Extends<Item> {};

// `return;` (struct/union/bitfield bodies) leaves `value` null; function
// bodies may set it.
struct ReturnStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct TryCatchStmt : cursiv::Extends<Item> {
  Block* tryBlock;
  Block* catchBlock;
};

struct VarDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isArray;
  Expr* size;
  Expr* sizeWhile;
  At* at;
  Expr* init;
};

struct VarDecl : cursiv::Extends<Item> {
  boolean isConst;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
};

// The document root: every top-level import, using, namespace, type
// declaration, member declaration and statement, in source order.
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace hexpat::ast
