// The plucid language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace plucid::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct IntegerLit : cursiv::Extends<Expression> {
  int64 value;
};

struct RealLit : cursiv::Extends<Expression> {
  float64 value;
};

struct WordLit : cursiv::Extends<Expression> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

struct NilLit : cursiv::Extends<Expression> {};

struct ListLit;

// A list element is heterogeneous: a number, a string, a nested nil or a
// nested list all fit the one polymorphic `Expression*` arm; a bare word
// (unquoted, unlike WordLit) is the one non-node shape. Namespace scope, so
// the AstNode vocabulary is out of reach and the std spelling is the only
// one (see examples/json/core/ast.hpp).
using ListElement = std::variant<Expression*, std::string_view>;

struct ListLit : cursiv::Extends<Expression> {
  vector<ListElement> elements;
};

struct Identifier : cursiv::Extends<Expression> {
  string_view name;
};

struct ErrorLit : cursiv::Extends<Expression> {};

struct EodLit : cursiv::Extends<Expression> {};

enum class PrefixOp : std::uint8_t {
  sin,
  cos,
  tan,
  sqrt,
  abs,
  log10,
  log,
  isnumber,
  isword,
  lognot,
  mkstring,
  isstring,
  mkword,
  hd,
  tl,
  isatom,
  isnull,
  islist,
  first,
  next,
  iseod,
  iserror,
};

struct PrefixExpr : cursiv::Extends<Expression> {
  PrefixOp op;
  Expression* operand;
};

enum class InfixOp : std::uint8_t {
  add,
  subtract,
  power,
  multiply,
  int_div,
  mod,
  divide,
  eq,
  ne,
  le,
  lt,
  gt,
  ge,
  logical_and,
  logical_or,
  concat,
  list_append,
  list_cons,
  fby,
  whenever,
  wvr,
  upon,
  asa,
  attime,
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  InfixOp op;
  Expression* right;
};

// filter/substr take three operands the grammar leaves unnamed.
struct FilterExpr : cursiv::Extends<Expression> {
  vector<Expression*> args;
};

struct SubstrExpr : cursiv::Extends<Expression> {
  vector<Expression*> args;
};

struct LengthExpr : cursiv::Extends<Expression> {
  Expression* operand;
};

struct ArgExpr : cursiv::Extends<Expression> {
  Expression* operand;
};

struct ListExpr : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

struct ElseTail : cursiv::Extends<cursiv::AstNode> {
  Expression* value;
};

struct ElseifTail : cursiv::Extends<cursiv::AstNode> {
  Expression* cond;
  Expression* then_;
};

struct IfExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* then_;
  variant<ElseTail*, ElseifTail*> tail;
};

struct CondCase : cursiv::Extends<cursiv::AstNode> {
  Expression* test;
  Expression* result;
};

struct CaseExpr : cursiv::Extends<Expression> {
  Expression* subject;
  vector<CondCase*> cases;
  Expression* default_;
};

struct CondExpr : cursiv::Extends<Expression> {
  vector<CondCase*> cases;
  Expression* default_;
};

struct GroupExpr : cursiv::Extends<Expression> {
  Expression* inner;
};

// No reference<>: a call name is never resolved against a where-clause
// definition (would need scoping across nested/recursive where bodies).
struct FunctionCall : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> args;
};

struct CurrentDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expression* value;
};

// Covers both `simple_definition` and `function_definition`: empty params
// is the simple form.
struct Definition : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> params;
  Expression* value;
};

struct WhereExpr : cursiv::Extends<Expression> {
  Expression* subject;
  vector<CurrentDecl*> decls;
  vector<Definition*> definitions;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  Expression* expression;
};

}  // namespace plucid::ast
