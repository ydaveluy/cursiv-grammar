// The lua language: its AST. A block is one class shared by the document,
// every loop/if/do body and every function body.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lua::ast {

struct Stat : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A block of statements; also the whole document (chunk = block).
struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Stat*> statements;
};

// The suffixed-expression family, one flattened chain: a bare NAME or a
// parenthesised expression, followed by any run of index/field/call
// suffixes. `Var` separates the three shapes real Lua allows as an
// assignment target or a `for`-loop name from the two that only a
// validator can rule out (a call, a bare parenthesised expression).
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
// The shared shape of `f(args)` and `o:m(args)`: one member both suffixes
// append into, so `args` can be parsed once and nested under either.
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
// Kept as text: Lua's four numeral forms (int, hex, float, hex-float)
// decode to two different runtime subtypes and hex ints can exceed a
// double's exact range, so the grammar does not collapse them into one.
struct NumberExpr : cursiv::Extends<Expr> {
  string_view text;
};
// Escapes are kept raw (unquoted, undecoded): a consumer that needs the
// runtime value decodes `\n`, `\ddd`, `\xXX`, `\u{...}` itself.
struct StringExpr : cursiv::Extends<Expr> {
  string_view value;
  boolean longForm;
};

struct TableField : cursiv::Extends<cursiv::AstNode> {
  Expr* key;         // `[exp]`, or null
  string_view name;  // `NAME =`, or empty
  Expr* value;
};
struct TableExpr : cursiv::Extends<Expr> {
  vector<TableField*> fields;
};

struct FuncBody : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> params;
  boolean vararg;
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

struct FunctionStat : cursiv::Extends<Stat> {
  string_view name;          // the leading NAME
  vector<string_view> path;  // each further `.NAME`
  string_view method;        // the trailing `:NAME`, or empty
  FuncBody* body;
};
struct LocalFunctionStat : cursiv::Extends<Stat> {
  string_view name;
  FuncBody* body;
};

struct LocalVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view attrib;  // `<const>` / `<close>` text, or empty
};
struct LocalStat : cursiv::Extends<Stat> {
  vector<LocalVar*> vars;
  vector<Expr*> values;
};

struct ReturnStat : cursiv::Extends<Stat> {
  vector<Expr*> values;
};

}  // namespace lua::ast
