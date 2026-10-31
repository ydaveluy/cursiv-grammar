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
  xor_,
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

struct ConcatExpr : cursiv::Extends<Expr> {
  vector<Expr*> operands;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view callee;
  vector<Expr*> args;
};

// A name read as a value. Function/program-local binding is left to a
// semantic pass; this syntax port retains the name as text.
struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NoobLit : cursiv::Extends<Expr> {};

struct NumLit : cursiv::Extends<Expr> {
  // Keep precision and the native NUMBR/NUMBAR distinction. Their numeric
  // ranges and evaluation belong to the host implementation.
  string_view text;
  boolean numbar;
};

struct StrLit : cursiv::Extends<Expr> {
  string_view text;  // between the quotes, escapes raw
};

struct Comment : cursiv::Extends<Statement> {
  string_view text;
};

struct LoopStmt : cursiv::Extends<Statement> {
  string_view label;
  string_view operation;
  string_view iterator;
  boolean until;
  Expr* condition;
  vector<Statement*> body;
  string_view endLabel;
};

struct Declaration : cursiv::Extends<Statement> {
  string_view name;
  Expr* init;
};

struct PrintBlock : cursiv::Extends<Statement> {
  boolean suppressNewline;
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

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  string_view literal;  // one complete token, with quotes/escapes retained
  vector<Statement*> body;
};

// IT is the selector. Ordered cases and ordinary BreakStmt children retain
// fallthrough; an empty default still differs from an absent one.
struct SwitchStmt : cursiv::Extends<Statement> {
  vector<CaseClause*> cases;
  boolean hasDefault = false;
  vector<Statement*> defaultBody;
};

struct InputBlock : cursiv::Extends<Statement> {
  string_view name;
};

struct FuncDecl : cursiv::Extends<Statement> {
  string_view name;
  vector<string_view> params;
  vector<Statement*> body;
};

struct ReturnStmt : cursiv::Extends<Statement> {
  Expr* value;
};
struct BreakStmt : cursiv::Extends<Statement> {};
struct RetypeStmt : cursiv::Extends<Statement> {
  string_view name;
  string_view targetType;
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
