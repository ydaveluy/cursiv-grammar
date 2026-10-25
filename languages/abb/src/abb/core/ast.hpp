#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <string_view>
#include <variant>

namespace abb::ast {
enum class InitKind { Const, Pers, Var };
struct Array;
struct Integer : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
struct FloatLiteral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
struct Reference : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
using Val = std::variant<bool, std::string_view, Integer *, FloatLiteral *,
                         Array *, Reference *>;
struct Array : cursiv::Extends<cursiv::AstNode> {
  vector<Val> items;
};
struct Declaration : cursiv::Extends<cursiv::AstNode> {
  boolean local = false;
  boolean task = false;
  InitKind kind{};
  string_view type;
  string_view name;
  Array *dimensions = nullptr;
  optional<Val> value;
};
struct BindingArgument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isOptional = false;
  optional<Val> value;
};
using Argument = std::variant<bool, std::string_view, Integer *, FloatLiteral *,
                              Array *, Reference *, BindingArgument *>;
struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Call : cursiv::Extends<Statement> {
  string_view name;
  vector<Argument> arguments;
};
struct Assignment : cursiv::Extends<Statement> {
  Reference *target = nullptr;
  Val value;
};
struct Return : cursiv::Extends<Statement> {
  optional<Val> value;
};
struct Branch : cursiv::Extends<cursiv::AstNode> {
  Val condition;
  vector<Statement *> statements;
};
struct If : cursiv::Extends<Statement> {
  vector<Branch *> branches;
  boolean hasElse = false;
  vector<Statement *> elseStatements;
};
struct While : cursiv::Extends<Statement> {
  Val condition;
  vector<Statement *> statements;
};
struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
  string_view mode;
  boolean isOptional = false;
  Array *dimensions = nullptr;
};
struct Routine : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean parenthesized = false;
  vector<Parameter *> parameters;
  vector<Declaration *> locals;
  vector<Statement *> statements;
};
struct Procedure : cursiv::Extends<Routine> {};
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
};
} // namespace abb::ast
