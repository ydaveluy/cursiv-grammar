// The snobol language: its AST.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace snobol::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

enum class Operator : std::uint8_t { add, subtract, multiply, divide, pow };

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct StringLiteral : cursiv::Extends<Expr> {
  string_view value;
};

struct IntegerLiteral : cursiv::Extends<Expr> {
  std::int64_t value;
};

// AMP marks an indirect reference (`&name`); subscript is empty for a plain
// name, else the array/table keys, themselves plain names, never expressions.
struct Subject : cursiv::Extends<Expr> {
  bool indirect = false;
  string_view name;
  vector<string_view> subscript;
};

enum class Builtin : std::uint8_t {
  ident,
  differ,
  eq,
  ne,
  ge,
  le,
  lt,
  integer,
  lgt,
  atan,
  chop,
  cos,
  exp,
  ln,
  remdr,
  sin,
  tan,
  date,
  dupl,
  reverse,
  replace,
  size,
  trim,
  array_,
  sort,
  table,
  break_,
};

struct FunctionCall : cursiv::Extends<Expr> {
  Builtin fn;
  vector<Expr*> args;
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// A `:` with nothing in it is a legal, empty transfer: mode and label both
// stay empty, toEnd stays false.
struct Transfer : cursiv::Extends<cursiv::AstNode> {
  string_view mode;
  bool toEnd = false;
  string_view label;
};

struct Line : cursiv::Extends<cursiv::AstNode> {};

struct Statement : cursiv::Extends<Line> {
  string_view label;
  Subject* subject = nullptr;
  Expr* pattern = nullptr;
  vector<Expr*> replacement;
  Transfer* transfer = nullptr;
};

struct GotoLine : cursiv::Extends<Line> {
  Transfer* transfer = nullptr;
};

struct CommentLine : cursiv::Extends<Line> {
  string_view text;
};

struct EndLine : cursiv::Extends<Line> {};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace snobol::ast
