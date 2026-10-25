// The robotwars language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace robotwars::ast {

enum class Operation : std::uint8_t { add, subtract, multiply, divide };

// Condition and accumulator prefixes share these four symbols.
enum class Comparison : std::uint8_t { less, greater, equal, notEqual };

enum class Register : std::uint8_t {
  a, b, c, d, e, f, g, h, i, j, k, l, m,
  n, o, p, q, r, s, t, u, v, w, x, y, z,
  aim, shot, radar, damage, speedx, speedy, random, index, data,
};

// The value family. Empty by design: every leaf and the two combinators
// derive it, so a member typed `Expr*` fits any of them.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A number's optional sign: the lexer never fuses it with the digits (it
// is a separate token from the arithmetic operators), so the value is a
// magnitude plus the fact of the minus.
struct NumberLit : cursiv::Extends<Expr> {
  boolean negative;
  int64 value;
};

struct RegisterRef : cursiv::Extends<Expr> {
  Register reg;
};

// A leading operator with no left operand: `- RADAR`, `* 5`.
struct UnaryExpr : cursiv::Extends<Expr> {
  Operation op;
  Expr* operand;
};

// The flat, precedence-free chain -- an operand, then any run of
// `operator operand` -- folds into a left-leaning ladder of these.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operation op;
  Expr* right;
};

struct Condition : cursiv::Extends<cursiv::AstNode> {
  Expr* left;
  Comparison op;
  Expr* right;
};

// A line's content: a label, a comment or a statement. Empty by design --
// every alternative derives it, so `Program::items` holds one pointer type.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Label : cursiv::Extends<Item> {
  string_view name;
};

struct Comment : cursiv::Extends<Item> {
  string_view text;
};

struct Statement : cursiv::Extends<Item> {};

struct IfStatement : cursiv::Extends<Statement> {
  Condition* condition;
  Statement* body;
};

struct ToStatement : cursiv::Extends<Statement> {
  Expr* value;
  vector<Register> targets;
};

struct GosubStatement : cursiv::Extends<Statement> {
  reference<Label> target;
};

struct GotoStatement : cursiv::Extends<Statement> {
  reference<Label> target;
};

struct EndSubStatement : cursiv::Extends<Statement> {};

// A comparison-shaped prefix (`=`, `#`, `< expr`, `> expr`) that guards the
// nested statement it carries. `operand` stays null for the `=`/`#` forms,
// which read no expression.
struct AccumStatement : cursiv::Extends<Statement> {
  Comparison op;
  Expr* operand;
  Statement* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace robotwars::ast
