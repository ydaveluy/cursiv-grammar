// The zig language: its AST. Value expressions and type expressions share
// one grammar chain, so both build Expr -- there is no separate
// type-expression family. Anything that is only ever a statement
// extends AstNode directly and lists its body as AstNode* (a Block, an if, a
// loop or a bare expression all fit there without a union).
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace zig::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// --- Container level ---------------------------------------------------

struct ContainerMember : cursiv::Extends<cursiv::AstNode> {};

struct ContainerMembers : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> docComment;
  vector<ContainerMember*> members;
};

struct Block;

struct TestDecl : cursiv::Extends<ContainerMember> {
  optional<string_view> name;  // a string literal or an identifier
  Block* body;
};

struct ComptimeDecl : cursiv::Extends<ContainerMember> {
  Block* body;
};

struct ParamDecl : cursiv::Extends<cursiv::AstNode> {
  boolean variadic;  // '...', every other field then unused
  optional<string_view> docComment;
  boolean noalias;
  boolean comptime;
  optional<string_view> name;
  boolean anytype;
  Expr* type;  // null when anytype
};

// Also usable as an anonymous function type, so it extends Expr rather than
// plain AstNode.
struct FnProto : cursiv::Extends<Expr> {
  optional<string_view> name;
  vector<ParamDecl*> params;
  Expr* alignExpr;
  Expr* addrSpaceExpr;
  Expr* linkSectionExpr;
  Expr* callConvExpr;
  boolean errorUnion;  // a bare '!' before the return type
  Expr* returnType;
};

struct VarDeclProto : cursiv::Extends<cursiv::AstNode> {
  boolean isConst;  // false for 'var'
  string_view name;
  Expr* type;
  Expr* alignExpr;
  Expr* addrSpaceExpr;
  Expr* linkSectionExpr;
};

struct GlobalVarDecl : cursiv::Extends<cursiv::AstNode> {
  VarDeclProto* proto;
  Expr* value;
};

// One class for both branches of `decl`: a function or a global variable,
// each with the export/extern/inline/noinline modifier that precedes it.
struct Decl : cursiv::Extends<ContainerMember> {
  optional<string_view> docComment;
  boolean pub;
  optional<string_view> modifier;  // export, extern, inline, noinline
  optional<string_view> externLib;
  boolean threadlocal;  // only meaningful for the var form
  FnProto* proto;        // set for the fn form
  Block* body;            // set for the fn form; null when ';' closed it
  GlobalVarDecl* varDecl;  // set for the var form
};

struct ContainerField : cursiv::Extends<ContainerMember> {
  optional<string_view> docComment;
  boolean comptime;
  optional<string_view> name;
  Expr* type;
  Expr* alignExpr;
  Expr* value;
};

// --- Control-flow prefixes, reused across statement/expr/type positions ---

struct IfPrefix : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  optional<string_view> payload;
  boolean payloadPtr;
};

// Covers every if-shaped construct, statement, expression or type: the
// then-branch is always an expression (a block or an assign-expression),
// the else-branch is whatever followed -- a statement, an expression, or
// nothing.
struct IfExpr : cursiv::Extends<Expr> {
  IfPrefix* prefix;
  Expr* thenBranch;
  optional<string_view> elsePayload;
  cursiv::AstNode* elseBranch;
};

struct PtrListItem : cursiv::Extends<cursiv::AstNode> {
  boolean ptr;  // a leading '*'
  string_view name;
};

struct WhilePrefix : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  optional<string_view> payload;
  boolean payloadPtr;
  Expr* continueExpr;
};

struct ForItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean isRange;  // '..' was present
  Expr* rangeEnd;   // null for an unbounded range ('0..')
};

struct ForPrefix : cursiv::Extends<cursiv::AstNode> {
  vector<ForItem*> items;
  vector<PtrListItem*> payload;
};

// Covers every for/while-shaped construct, statement, expression or type:
// exactly one of forPrefix/whilePrefix is set.
struct Loop : cursiv::Extends<Expr> {
  optional<string_view> label;
  boolean inlineLoop;
  ForPrefix* forPrefix;
  WhilePrefix* whilePrefix;
  Expr* thenBranch;
  optional<string_view> elsePayload;
  cursiv::AstNode* elseBranch;
};

struct SwitchItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean isRange;
  Expr* rangeEnd;
};

struct SwitchProng : cursiv::Extends<cursiv::AstNode> {
  boolean inlineProng;
  boolean isElse;
  vector<SwitchItem*> items;  // empty when isElse
  vector<PtrListItem*> payload;
  Expr* value;
};

struct SwitchExpr : cursiv::Extends<Expr> {
  optional<string_view> label;
  Expr* value;
  vector<SwitchProng*> prongs;
};

// A block also carries the label a statement may have prefixed it with --
// folded in here rather than a wrapper node.
struct Block : cursiv::Extends<Expr> {
  optional<string_view> label;
  vector<cursiv::AstNode*> statements;
};

struct ComptimeStmt : cursiv::Extends<cursiv::AstNode> {
  cursiv::AstNode* body;  // a block, or a var-decl/expression statement
};

struct NosuspendStmt : cursiv::Extends<cursiv::AstNode> {
  Expr* body;
};

struct SuspendStmt : cursiv::Extends<cursiv::AstNode> {
  Expr* body;
};

struct DeferStmt : cursiv::Extends<cursiv::AstNode> {
  Expr* body;
};

struct ErrdeferStmt : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> payload;
  Expr* body;
};

// Every plain-expression statement, assignment and destructure: one class.
// `lhs` holds one or more VarDeclProto*/Expr* (a comma-destructure mixes
// both); a bare `expr;` leaves op and value unset.
struct VarDeclExprStmt : cursiv::Extends<cursiv::AstNode> {
  vector<cursiv::AstNode*> lhs;
  optional<string_view> op;
  Expr* value;
};

// --- Expression ladder ---------------------------------------------------

// Every binary level (bool_or/bool_and/bitwise/bit_shift/addition/multiply)
// and the non-chaining compare level share this shape; `op` is the matched
// operator text, including a `catch`'s optional `|payload|` verbatim (an
// infix operator hands over text, never a node).
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BreakExpr : cursiv::Extends<Expr> {
  optional<string_view> label;
  Expr* value;
};

struct ContinueExpr : cursiv::Extends<Expr> {
  optional<string_view> label;
  Expr* value;
};

struct ComptimeExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct NosuspendExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct ResumeExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct ReturnExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct FieldInit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

// '{' ... '}': either field inits or bare values, or neither.
struct InitList : cursiv::Extends<Expr> {
  vector<FieldInit*> fields;
  vector<Expr*> values;
};

// A type followed by `{...}`; the plain type alone passes through with no
// wrapper.
struct TypedInit : cursiv::Extends<Expr> {
  Expr* type;
  InitList* list;
};

struct BuiltinCall : cursiv::Extends<Expr> {
  string_view name;  // includes the leading '@'
  vector<Expr*> args;
};

struct CharLiteral : cursiv::Extends<Expr> {
  string_view text;
};

struct NumberLiteral : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLiteral : cursiv::Extends<Expr> {
  string_view text;
};

struct Identifier : cursiv::Extends<Expr> {
  string_view name;
};

struct EnumLiteral : cursiv::Extends<Expr> {
  string_view name;  // the name after the dot
};

struct ErrorValue : cursiv::Extends<Expr> {
  string_view name;  // the name after `error.`
};

struct GroupedExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct AnyframeLit : cursiv::Extends<Expr> {};

struct UnreachableLit : cursiv::Extends<Expr> {};

struct IdentifierListItem : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> docComment;
  string_view name;
};

struct ErrorSetDecl : cursiv::Extends<Expr> {
  vector<IdentifierListItem*> errors;
};

// (extern|packed)? (struct|opaque|enum|union), with the backing/tag type an
// optional parenthesised expression, and union's own optional 'enum' tag.
struct ContainerDecl : cursiv::Extends<Expr> {
  optional<string_view> extPacked;
  string_view kind;  // struct, opaque, enum, union
  Expr* arg;          // struct(T)/enum(T)/union(T)
  boolean unionEnumTag;
  Expr* unionEnumArg;  // union(enum(T))'s inner T
  ContainerMembers* body;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
};

struct SliceExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* start;
  Expr* end;       // null: '[a..]'
  Expr* sentinel;  // null unless ':' expr closed the slice
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct DerefExpr : cursiv::Extends<Expr> {
  Expr* object;
};

struct OptionalUnwrapExpr : cursiv::Extends<Expr> {
  Expr* object;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct ErrorUnionExpr : cursiv::Extends<Expr> {
  Expr* errorSet;
  Expr* payload;
};

struct TypeModifier : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // align, addrspace, const, volatile, allowzero
  Expr* arg;          // align/addrspace's parenthesised expression
  Expr* bitStart;      // align(x:a:b)'s bit-range form, ptr type only
  Expr* bitEnd;
};

// The seven type-prefix shapes get each their own class (an infix operator
// hands over text, but these are plain prefixes, so a class per shape reads
// better than a discriminator field with nothing to capture it from).
struct OptionalType : cursiv::Extends<Expr> {
  Expr* base;
};

struct AnyframeType : cursiv::Extends<Expr> {
  Expr* base;
};

struct SliceType : cursiv::Extends<Expr> {
  Expr* sentinel;
  vector<TypeModifier*> modifiers;
  Expr* base;
};

struct ManyPtrType : cursiv::Extends<Expr> {
  boolean sentinelC;  // '[*c]'
  Expr* sentinel;
  vector<TypeModifier*> modifiers;
  Expr* base;
};

struct PtrType : cursiv::Extends<Expr> {
  vector<TypeModifier*> modifiers;
  Expr* base;
};

struct ArrayType : cursiv::Extends<Expr> {
  Expr* size;
  Expr* sentinel;
  Expr* base;
};

struct AssignExpr : cursiv::Extends<Expr> {
  vector<Expr*> lhs;
  optional<string_view> op;
  Expr* value;
};

struct AsmOutputItem : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  string_view constraint;
  Expr* type;  // set for the `-> type` form, writing a fresh binding
  string_view name;  // set for the bare-name form, writing an existing one
};

struct AsmInputItem : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  string_view constraint;
  Expr* value;
};

struct AsmExpr : cursiv::Extends<Expr> {
  boolean isVolatile;
  Expr* code;
  vector<AsmOutputItem*> outputs;
  vector<AsmInputItem*> inputs;
  Expr* clobbers;
};

}  // namespace zig::ast
