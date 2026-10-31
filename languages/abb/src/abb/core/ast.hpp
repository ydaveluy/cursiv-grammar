#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <string_view>

namespace abb::ast {
enum class InitKind { Const, Pers, Var };
struct Argument : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<Argument> {};
using Val = Expr*;
struct Integer : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLiteral : cursiv::Extends<Expr> {
  string_view text;
};
struct BoolLiteral : cursiv::Extends<Expr> {
  boolean value = false;
};
struct StringLiteral : cursiv::Extends<Expr> {
  string_view value;
};
// Retained imported character syntax; RAPID's native text literal is string.
struct CharacterLiteral : cursiv::Extends<Expr> {
  string_view value;
};
struct Reference : cursiv::Extends<Expr> {
  string_view name;
};
struct Array : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct Index : cursiv::Extends<Expr> {
  Expr* target = nullptr;
  vector<Expr*> indexes;
};
struct Component : cursiv::Extends<Expr> {
  Expr* target = nullptr;
  string_view name;
};
struct Unary : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};
struct Binary : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  string_view op;
  Expr* right = nullptr;
};
struct FunctionCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Argument*> arguments;
};
struct Declaration : cursiv::Extends<cursiv::AstNode> {
  boolean local = false;
  boolean task = false;
  InitKind kind{};
  string_view type;
  string_view name;
  Array *dimensions = nullptr;
  Expr* value = nullptr;
};
struct BindingArgument : cursiv::Extends<Argument> {
  string_view name;
  boolean isOptional = false;
  boolean isConditional = false;
  Expr* value = nullptr;
};
struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Call : cursiv::Extends<Statement> {
  string_view name;
  vector<Argument*> arguments;
};
struct Assignment : cursiv::Extends<Statement> {
  Expr* target = nullptr;
  Expr* value = nullptr;
};
struct Return : cursiv::Extends<Statement> {
  Expr* value = nullptr;
};
struct Branch : cursiv::Extends<cursiv::AstNode> {
  Expr* condition = nullptr;
  vector<Statement *> statements;
};
struct If : cursiv::Extends<Statement> {
  vector<Branch *> branches;
  boolean hasElse = false;
  vector<Statement *> elseStatements;
};
struct While : cursiv::Extends<Statement> {
  Expr* condition = nullptr;
  vector<Statement *> statements;
};
struct For : cursiv::Extends<Statement> {
  string_view variable;
  Expr* from = nullptr;
  Expr* to = nullptr;
  Expr* step = nullptr;
  vector<Statement*> statements;
};
struct TestCase : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> values;
  vector<Statement*> statements;
};
struct Test : cursiv::Extends<Statement> {
  Expr* value = nullptr;
  vector<TestCase*> cases;
  boolean hasDefault = false;
  vector<Statement*> defaultStatements;
};
struct Label : cursiv::Extends<Statement> { string_view name; };
struct Goto : cursiv::Extends<Statement> { string_view name; };
struct Break : cursiv::Extends<Statement> {};
struct Continue : cursiv::Extends<Statement> {};
struct Exit : cursiv::Extends<Statement> {};
struct Retry : cursiv::Extends<Statement> {};
struct TryNext : cursiv::Extends<Statement> {};
struct Raise : cursiv::Extends<Statement> { Expr* error = nullptr; };
struct Connect : cursiv::Extends<Statement> {
  Expr* target = nullptr;
  string_view trap;
};
struct Handler : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};
struct BackwardHandler : cursiv::Extends<Handler> {};
struct ErrorHandler : cursiv::Extends<Handler> {
  vector<Expr*> numbers;
};
struct UndoHandler : cursiv::Extends<Handler> {};
struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
  string_view mode;
  boolean isOptional = false;
  Array *dimensions = nullptr;
};
struct Routine : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean local = false;
  boolean parenthesized = false;
  vector<Parameter *> parameters;
  vector<Declaration *> locals;
  vector<Statement *> statements;
  BackwardHandler* backward = nullptr;
  ErrorHandler* error = nullptr;
  UndoHandler* undo = nullptr;
};
struct Procedure : cursiv::Extends<Routine> {};
struct Trap : cursiv::Extends<Routine> {};
struct Function : cursiv::Extends<Routine> {
  string_view returnType;
};
struct RecordField : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
};
struct Record : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<RecordField *> fields;
};
struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean parenthesized = false;
  vector<string_view> options;
  vector<Record *> records;
  vector<Declaration *> declarations;
  vector<Procedure *> procedures;
  vector<Function *> functions;
  vector<Trap*> traps;
};
} // namespace abb::ast
