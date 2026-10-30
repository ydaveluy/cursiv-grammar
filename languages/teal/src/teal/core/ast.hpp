// The teal language: its AST. A block is one class shared by the document,
// every loop/if/do/function body; a type is one small family alongside it.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace teal::ast {

struct Stat : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};
struct NewType : cursiv::Extends<cursiv::AstNode> {
  boolean legacy = false;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Stat*> statements;
};

// The suffixed-expression family, one flattened chain: a bare NAME or a
// parenthesised expression, followed by any run of index/field/call
// suffixes. `Var` separates the shapes a validator would accept as an
// assignment target from the two only it can rule out (a call, a bare
// parenthesised expression).
struct Var : cursiv::Extends<Expr> {};
struct NameExpr : cursiv::Extends<Var> {
  string_view name;
};
struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};
struct IndexExpr : cursiv::Extends<Var> {
  Expr* object;
  Expr* index;
};
struct FieldExpr : cursiv::Extends<Var> {
  Expr* object;
  string_view field;
};
struct Call : cursiv::Extends<Expr> {
  vector<Expr*> args;
};
struct CallExpr : cursiv::Extends<Call> {
  Expr* callee;
};
struct MethodCallExpr : cursiv::Extends<Call> {
  Expr* object;
  string_view method;
};

struct NilExpr : cursiv::Extends<Expr> {};
struct BoolExpr : cursiv::Extends<Expr> {
  boolean value;
};
struct VarargExpr : cursiv::Extends<Expr> {};
struct NumberExpr : cursiv::Extends<Expr> {
  string_view text;
};
struct StringExpr : cursiv::Extends<Expr> {
  string_view value;
};

struct TableField : cursiv::Extends<cursiv::AstNode> {
  Expr* key;         // `[exp]`, or null
  string_view name;  // `NAME`, or empty
  Type* fieldType;   // `: typ` on the `NAME = exp` form, or null
  Expr* value;       // the `exp` value, or null on the `NAME = newtype` form
  NewType* newType;  // `NAME = newtype`, or null
};
struct TableExpr : cursiv::Extends<Expr> {
  vector<TableField*> fields;
};

// One shape for `partype`, `parname` and a record's `NAME ':' typ` field:
// a name (required by `parname`/the field, optional in `partype`) and a
// type.
struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
};
struct TypeArgs : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
};
struct RetList : cursiv::Extends<cursiv::AstNode> {
  vector<Type*> types;
  boolean vararg;
};
struct FuncBody : cursiv::Extends<cursiv::AstNode> {
  TypeArgs* typeParams;  // null when absent
  vector<Param*> params;
  boolean vararg;
  Type* varargType;  // `...: typ`, or null
  RetList* rets;      // `: retlist`, or null
  Block* body;
};
struct FunctionExpr : cursiv::Extends<Expr> {
  FuncBody* body;
};

enum class BinOp : std::uint8_t {
  add,
  sub,
  mul,
  div,
  idiv,
  mod,
  pow,
  concat,
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  logical_and,
  logical_or,
  band,
  bor,
  bxor,
  shl,
  shr,
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

enum class UnOp : std::uint8_t { neg, lnot, len, bnot };
struct UnaryExpr : cursiv::Extends<Expr> {
  UnOp op;
  Expr* operand;
};

struct AsExpr : cursiv::Extends<Expr> {
  Expr* expr;
  Type* type;
};
// `NAME 'is' typ`: the left side is a bare name, never a general expression.
struct IsExpr : cursiv::Extends<Expr> {
  string_view name;
  Type* type;
};

struct PrimitiveType : cursiv::Extends<Type> {
  string_view kind;  // "string" | "boolean" | "nil" | "number"
};
struct NamedType : cursiv::Extends<Type> {
  string_view name;
  TypeArgs* typeArgs;  // null when absent
};
struct ArrayType : cursiv::Extends<Type> {
  Type* element;
};
struct MapType : cursiv::Extends<Type> {
  Type* key;
  Type* value;
};
struct FunctionSigType : cursiv::Extends<Type> {
  TypeArgs* typeParams;  // null when absent
  vector<Param*> params;
  RetList* rets;  // null when absent
};
// `typ`: a union of one or more base types. Always this node, even for a
// single base type -- a reader checks `alternatives.size()`.
struct UnionType : cursiv::Extends<Type> {
  vector<Type*> alternatives;
};

struct NamedNewType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  NewType* value;
};
struct AliasNewType : cursiv::Extends<NewType> {
  TypeArgs* typeParams = nullptr;
  Type* type = nullptr;
};
struct RecordNewType : cursiv::Extends<NewType> {
  TypeArgs* typeParams;  // null when absent
  Type* arrayPart;       // `{ typ }`, or null
  vector<NamedNewType*> nestedTypes;
  vector<Param*> fields;
};
struct EnumNewType : cursiv::Extends<NewType> {
  vector<string_view> values;
};
struct FuncNewType : cursiv::Extends<NewType> {
  FunctionSigType* signature;
};

struct AssignStat : cursiv::Extends<Stat> {
  vector<Expr*> targets;
  vector<Expr*> values;
};
struct CallStat : cursiv::Extends<Stat> {
  Expr* call;
};
struct LabelStat : cursiv::Extends<Stat> {
  string_view name;
};
struct BreakStat : cursiv::Extends<Stat> {};
struct GotoStat : cursiv::Extends<Stat> {
  string_view label;
};
struct DoStat : cursiv::Extends<Stat> {
  Block* body;
};
struct WhileStat : cursiv::Extends<Stat> {
  Expr* cond;
  Block* body;
};
struct RepeatStat : cursiv::Extends<Stat> {
  Block* body;
  Expr* cond;
};

struct IfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  Block* body;
};
struct IfStat : cursiv::Extends<Stat> {
  vector<IfClause*> clauses;  // the `if`, then each `elseif`
  Block* elseBody;            // null when there is no `else`
};

struct NumericForStat : cursiv::Extends<Stat> {
  string_view name;
  Expr* start;
  Expr* stop;
  Expr* step;  // null when omitted
  Block* body;
};
struct GenericForStat : cursiv::Extends<Stat> {
  vector<string_view> names;
  vector<Expr*> exprs;
  Block* body;
};

struct FuncStat : cursiv::Extends<Stat> {
  string_view name;          // the leading NAME
  vector<string_view> path;  // each further `.NAME`
  string_view method;        // the trailing `:NAME`, or empty
  FuncBody* body;
};
struct LocalFuncStat : cursiv::Extends<Stat> {
  string_view name;
  FuncBody* body;
};
struct GlobalFuncStat : cursiv::Extends<Stat> {
  string_view name;
  FuncBody* body;
};

// `NAME attrib?`: a `local`/`global` variable and its optional `<const>` /
// `<close>` marker.
struct AttribName : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view attrib;  // text between `<` and `>`, or empty
};
struct LocalAttrAssignStat : cursiv::Extends<Stat> {
  vector<AttribName*> vars;
  vector<Type*> types;   // `: typelist`, zero or more entries
  vector<Expr*> values;  // `= explist`, or empty
};
struct LocalNewTypeStat : cursiv::Extends<Stat> {
  string_view name;
  NewType* value;
};
struct GlobalAttrStat : cursiv::Extends<Stat> {
  vector<AttribName*> vars;
  vector<Type*> types;  // `: typelist`, required, one or more entries
};
struct GlobalAttrAssignStat : cursiv::Extends<Stat> {
  vector<AttribName*> vars;
  vector<Type*> types;   // `: typelist`, zero or more entries
  vector<Expr*> values;  // `= explist`
};
struct GlobalAssignStat : cursiv::Extends<Stat> {
  string_view name;
  NewType* value;
};

struct ReturnStat : cursiv::Extends<Stat> {
  vector<Expr*> values;
};

}  // namespace teal::ast
