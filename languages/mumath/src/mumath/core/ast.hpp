// The mumath language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace mumath::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};

// One shape for every binary operator (relational, adding, multiplying): the
// token's own text is the operand, so no enum stands between the grammar and
// the tree.
struct BinaryExpression : cursiv::Extends<Expression> {
  Expression* left;
  string_view op;
  Expression* right;
};

struct NegateExpression : cursiv::Extends<Expression> {
  Expression* operand;
};

struct PositiveExpression : cursiv::Extends<Expression> {
  Expression* operand;
};

struct NotExpression : cursiv::Extends<Expression> {
  Expression* operand;
};

struct NameExpression : cursiv::Extends<Expression> {
  string_view name;
};

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
};

struct StringLiteral : cursiv::Extends<Expression> {
  string_view text;
};

// A quoted name or string denotes itself rather than being looked up or
// re-lexed: the leading QUOTE is the only difference from the bare forms.
struct QuotedName : cursiv::Extends<Expression> {
  string_view name;
};

struct QuotedString : cursiv::Extends<Expression> {
  string_view text;
};

struct GroupedExpression : cursiv::Extends<Expression> {
  Expression* expression;
};

struct Assignment : cursiv::Extends<Statement> {
  vector<string_view> targets;
  Expression* expr;
};

// A call argument is a value or a binding of one -- `F(x:1)` passes 1 under
// the name x.
using Argument = std::variant<Expression*, Assignment*>;

struct CallExpression : cursiv::Extends<Expression> {
  string_view func;
  vector<Argument> args;
};

struct ExpressionStatement : cursiv::Extends<Statement> {
  Expression* expr;
};

struct Block : cursiv::Extends<Statement> {
  vector<Statement*> body;
};

struct Loop : cursiv::Extends<Statement> {
  vector<Statement*> body;
};

struct When : cursiv::Extends<Statement> {
  Expression* condition;
  vector<Statement*> body;
};

struct FunctionDefinition : cursiv::Extends<Statement> {
  string_view name;
  vector<string_view> params;
  vector<Statement*> body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace mumath::ast
