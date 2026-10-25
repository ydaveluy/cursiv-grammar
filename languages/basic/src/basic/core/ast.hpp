// The basic language: its AST. Statements group by shape rather than by
// keyword -- most take a fixed small list of expression arguments and carry
// no structure a tool would read beyond "which one, with what arguments".
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace basic::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

enum class VarSuffix : std::uint8_t { none, dollar, percent };

struct Subscript : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> args;
};

// A name is one contiguous letters-then-digits token: hidden whitespace
// would otherwise admit a space inside it.
struct VarRef : cursiv::Extends<Expr> {
  string_view name;
  VarSuffix suffix;
  vector<Subscript*> subscripts;
};

struct NumberLit : cursiv::Extends<Expr> {
  boolean negative;
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

enum class BuiltinFunc : std::uint8_t {
  chr, sqr, len, str, asc, scrn, mid, pdl, peek, intf, spc, fre, pos, usr,
  left, val, right, sin, cos, tan, atn, rnd, sgn, exp, log, abs, tabf,
};

struct FuncCall : cursiv::Extends<Expr> {
  BuiltinFunc kind;
  vector<Expr*> args;
};

// `FN name(arg)`: the one user-defined function form, distinct from the
// builtins because it names a variable rather than a fixed keyword.
struct FnCall : cursiv::Extends<Expr> {
  string_view name;
  VarSuffix suffix;
  Expr* arg;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  boolean logicalNot;
  boolean negative;
  Expr* operand;
};

enum class Op : std::uint8_t {
  pow, mul, div_, add, sub, ge, le, ne, eq, gt, lt, and_, or_,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

// The ~20 statements that carry no operand beyond the keyword itself.
enum class SimpleKind : std::uint8_t {
  cls, load, save, trace, notrace, flash, inverse, gr, normal, shload,
  clear, run, stop, text, home, hgr, hgr2, end_, return_, restore,
};

struct SimpleStmt : cursiv::Extends<Stmt> {
  SimpleKind kind;
};

// The statements that are one keyword plus a fixed or comma-separated run of
// expressions, with no further structure: the count of `args` says which
// shape was written where a statement admits more than one.
enum class ArgKind : std::uint8_t {
  plot, poke, vtab, htab, himem, lomem, tab_, speed, rot, scale, color,
  hcolor, hlin, vlin, prnumber, innumber, ampersand, include_, get, call,
  wait, xdraw, draw, list_, pop_,
};

struct ArgStmt : cursiv::Extends<Stmt> {
  ArgKind kind;
  vector<Expr*> args;
};

// HPLOT / VPLOT: an optional first point, then any number of `TO x,y`
// points, flattened to a run of coordinates (pairs of consecutive entries).
struct PlotPath : cursiv::Extends<Stmt> {
  boolean vertical;
  vector<Expr*> coords;
};

struct Comment : cursiv::Extends<Stmt> {
  string_view text;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  boolean explicitLet;
  VarRef* target;
  vector<Expr*> values;
};

struct PrintItem : cursiv::Extends<cursiv::AstNode> {
  string_view separator; // empty for the first item, otherwise ',' or ';'
  Expr *value;           // null for an empty or trailing slot
};
struct PrintStmt : cursiv::Extends<Stmt> {
  vector<PrintItem *> items;
};

struct InputStmt : cursiv::Extends<Stmt> {
  string_view prompt;
  vector<VarRef*> vars;
};

struct ReadStmt : cursiv::Extends<Stmt> {
  vector<VarRef*> vars;
};

struct DimStmt : cursiv::Extends<Stmt> {
  vector<VarRef*> vars;
};

struct StoreStmt : cursiv::Extends<Stmt> {
  VarRef* var;
};

struct RecallStmt : cursiv::Extends<Stmt> {
  VarRef* var;
};

struct DataStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct DefStmt : cursiv::Extends<Stmt> {
  boolean explicitFn;
  string_view name;
  VarSuffix nameSuffix;
  string_view param;
  VarSuffix paramSuffix;
  Expr* body;
};

struct OnErrStmt : cursiv::Extends<Stmt> {
  int64 target;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  int64 target;
};

struct GosubStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct OnGotoStmt : cursiv::Extends<Stmt> {
  Expr* selector;
  vector<int64> targets;
};

struct OnGosubStmt : cursiv::Extends<Stmt> {
  Expr* selector;
  vector<int64> targets;
};

// A FOR's optional trailing `statement NEXT var?` already covers every
// document a bare `FOR ... TO ...` header alone would.
struct ForStmt : cursiv::Extends<Stmt> {
  VarRef* var;
  Expr* from;
  Expr* to;
  Expr* step;
  Stmt* body;
  VarRef* nextVar;
};

struct NextStmt : cursiv::Extends<Stmt> {
  vector<VarRef*> vars;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenStmt;
  optional<int64> thenLine;
};

struct StmtItem : cursiv::Extends<cursiv::AstNode> {
  boolean ampersandPrefix;
  Stmt* stmt;
};

struct Line : cursiv::Extends<cursiv::AstNode> {
  int64 number;
  vector<StmtItem*> items;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace basic::ast
