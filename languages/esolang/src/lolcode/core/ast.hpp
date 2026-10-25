// The lolcode language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lolcode::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};

// Every expression is also a statement: LOLCODE allows a bare expression as
// a line of code (an implicit IT assignment in the real language, simplified
// here to a plain expression statement).
struct Expr : cursiv::Extends<Statement> {};

enum class Op : std::uint8_t {
  eq,
  ne,
  both,
  either,
  greater,
  less,
  add,
  sub,
  mul,
  div,
  mod,
};

struct BinOp : cursiv::Extends<Expr> {
  Op op;
  Expr* left;
  Expr* right;
};

struct CastExpr : cursiv::Extends<Expr> {
  Expr* value;
  string_view targetType;
};

struct AllOf : cursiv::Extends<Expr> {
  vector<Expr*> operands;
};

struct AnyOf : cursiv::Extends<Expr> {
  vector<Expr*> operands;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view callee;
  vector<Expr*> args;
};

// A name read as a value. LOLCODE resolves it dynamically (loop and
// declaration scoping do not match a static linker's rules), so it is text,
// not a reference<>.
struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NoobLit : cursiv::Extends<Expr> {};

struct NumLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StrLit : cursiv::Extends<Expr> {
  string_view text;  // between the quotes, escapes raw
};

struct Comment : cursiv::Extends<Statement> {
  string_view text;
};

struct LoopStmt : cursiv::Extends<Statement> {
  string_view label;
  Expr* condition;
  vector<Statement*> body;
  string_view endLabel;
};

struct Declaration : cursiv::Extends<Statement> {
  string_view name;
  Expr* init;
};

struct PrintBlock : cursiv::Extends<Statement> {
  vector<Expr*> items;
};

// The MEBBE/NO WAI chain: a right-recursive list of clauses, closed by an
// optional trailing NO WAI.
struct ElseClause : cursiv::Extends<cursiv::AstNode> {};

struct MebbeClause : cursiv::Extends<ElseClause> {
  Expr* cond;
  vector<Statement*> body;
  ElseClause* next;
};

struct NoWaiClause : cursiv::Extends<ElseClause> {
  vector<Statement*> body;
};

struct IfBlock : cursiv::Extends<Statement> {
  vector<Statement*> body;  // the YA RLY branch
  ElseClause* elseChain;
};

struct InputBlock : cursiv::Extends<Statement> {
  string_view name;
};

struct FuncDecl : cursiv::Extends<Statement> {
  string_view name;
  vector<string_view> params;
  vector<Statement*> body;
};

struct Assignment : cursiv::Extends<Statement> {
  string_view name;
  Expr* value;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  Expr* version;
  vector<Statement*> statements;
  boolean hasKthxbye;
};

}  // namespace lolcode::ast
