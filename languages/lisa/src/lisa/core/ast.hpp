// The LISA language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lisa::ast {

enum class TypeKind : std::uint8_t { Int, Dfa, Nfa, Regex, Bool, String };
enum class GeneratorKind : std::uint8_t { Random, Enumerate };

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct Declaration : cursiv::Extends<cursiv::AstNode> {
  TypeKind type;
  string_view name;
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  string_view op;
  Expression* right;
};

struct NotExpr : cursiv::Extends<Expression> {
  Expression* operand;
};

struct AssignmentPart : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  Expression* value;
};
// A source-order assignment chain; the first pair keeps the existing fields.
struct AssignExpr : cursiv::Extends<Expression> {
  reference<Declaration> target;
  string_view op;
  Expression* value;
  vector<AssignmentPart*> continuations;
};

struct GroupedExpr : cursiv::Extends<Expression> {
  Expression* expression;
};

struct IntegerLiteral : cursiv::Extends<Expression> {
  string_view value;
};

struct BoolLiteral : cursiv::Extends<Expression> {
  boolean value;
};

struct StringLiteral : cursiv::Extends<Expression> {
  string_view value;
};

struct TypeLiteral : cursiv::Extends<Expression> {
  TypeKind value;
};

struct NextExpr : cursiv::Extends<Expression> {};
struct HasNextExpr : cursiv::Extends<Expression> {};

struct VariableRef : cursiv::Extends<Expression> {
  reference<Declaration> target;
};

struct FunctionCall : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> args;
};

struct ExpressionStatement : cursiv::Extends<Statement> {
  Expression* expression;
};

struct IfStatement : cursiv::Extends<Statement> {
  Expression* condition;
  Statement* body;
};

struct WhileStatement : cursiv::Extends<Statement> {
  Expression* condition;
  Statement* body;
};

struct BreakStatement : cursiv::Extends<Statement> {};
struct ContinueStatement : cursiv::Extends<Statement> {};

struct GeneratingStatement : cursiv::Extends<Statement> {
  GeneratorKind generator;
  Statement* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration*> declarations;
  vector<Statement*> statements;
};

}  // namespace lisa::ast
