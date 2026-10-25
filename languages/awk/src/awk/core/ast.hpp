// The awk language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace awk::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
  boolean international;
};

struct RegexLit : cursiv::Extends<Expr> {
  string_view text;
  boolean typed;
};

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

// name[e, e, ...] -- an array element; SUBSEP joins several indices.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  vector<Expr*> indices;
};

// $expr -- a field reference; index 0 is the whole record.
struct FieldExpr : cursiv::Extends<Expr> {
  Expr* index;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// ++x, --x
struct PrefixUpdateExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* target;
};

// x++, x--
struct PostfixUpdateExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
};

enum class BinOp : std::uint8_t {
  Pow, Mul, Div, Mod, Add, Sub,
  Lt, Le, Gt, Ge, Eq, Ne,
  Match, NoMatch,
  And, Or,
};

// Every binary level of the arithmetic/relational/match/logical ladder.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

// String concatenation by juxtaposition: no operator token to hold.
struct ConcatExpr : cursiv::Extends<Expr> {
  Expr* left;
  Expr* right;
};

// expr in name
struct InExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Expr* array;
};

// (e, e, ...) in name -- the multi-subscript membership test.
struct MultiInExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
  Expr* array;
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
  boolean indirect;
  boolean spaced;
};

// getline, getline var, getline < file, getline var < file, cmd | getline
// [var] -- one class, since the three forms differ only in which optional
// member was written.
struct GetlineExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* file;
  Expr* cmd;
  boolean coprocess;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> items;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  Expr* cond;
  Stmt* step;
  Stmt* body;
};

struct ForInStmt : cursiv::Extends<Stmt> {
  string_view var;
  Expr* array;
  Stmt* body;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};
struct NextStmt : cursiv::Extends<Stmt> {};
struct NextFileStmt : cursiv::Extends<Stmt> {};

struct ExitStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

// Delete an array or an array element.
struct DeleteStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean default_;
  vector<Stmt*> items;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<SwitchCase*> cases;
};

// ">" file, ">>" file, "|" cmd
struct OutputRedirect : cursiv::Extends<cursiv::AstNode> {
  string_view mode;
  Expr* target;
};

struct PrintStmt : cursiv::Extends<Stmt> {
  vector<Expr*> args;
  OutputRedirect* redirect;
};

struct PrintfStmt : cursiv::Extends<Stmt> {
  vector<Expr*> args;
  OutputRedirect* redirect;
};

struct Pattern : cursiv::Extends<cursiv::AstNode> {};
struct BeginPattern : cursiv::Extends<Pattern> {};
struct EndPattern : cursiv::Extends<Pattern> {};
struct BeginFilePattern : cursiv::Extends<Pattern> {};
struct EndFilePattern : cursiv::Extends<Pattern> {};

struct ExprPattern : cursiv::Extends<Pattern> {
  Expr* value;
};

struct RangePattern : cursiv::Extends<Pattern> {
  Expr* from;
  Expr* to;
};

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Directive : cursiv::Extends<Item> {
  string_view name;
  string_view value;
};

// A bare action (no pattern: runs on every record), a pattern with an
// action, or a bare pattern (no action: prints the matched record).
struct Rule : cursiv::Extends<Item> {
  Pattern* pattern;
  BlockStmt* action;
};

struct FunctionDef : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> params;
  BlockStmt* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace awk::ast
