// The logo language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace logo::ast {

struct Command : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};
struct Line : cursiv::Extends<cursiv::AstNode> {};

enum class Sign : std::uint8_t { plus, minus };
enum class Op : std::uint8_t { add, subtract, multiply, divide };
enum class CompareOp : std::uint8_t { less, greater, equal };

struct NumberLiteral : cursiv::Extends<Expression> {
  string_view value;
};

struct Deref : cursiv::Extends<Expression> {
  string_view name;
};

struct RandomCall : cursiv::Extends<Expression> {
  Expression* arg = nullptr;
};

// A run of leading +/- signs over an atom; the common sign-less case
// forwards the atom directly (LogoGrammar.hpp), so this node only exists
// when a sign was actually written.
struct SignExpression : cursiv::Extends<Expression> {
  vector<Sign> signs;
  Expression* operand = nullptr;
};

struct BinaryExpression : cursiv::Extends<Expression> {
  Expression* left = nullptr;
  Op op = Op::add;
  Expression* right = nullptr;
};

using Value = std::variant<std::string_view, Expression*>;

struct RepeatCmd : cursiv::Extends<Command> {
  Expression* count = nullptr;
  vector<Command*> body;
};

struct ForwardCmd : cursiv::Extends<Command> {
  Expression* distance = nullptr;
};

struct BackwardCmd : cursiv::Extends<Command> {
  Expression* distance = nullptr;
};

struct RightCmd : cursiv::Extends<Command> {
  Expression* angle = nullptr;
};

struct LeftCmd : cursiv::Extends<Command> {
  Expression* angle = nullptr;
};

struct ClearScreenCmd : cursiv::Extends<Command> {};
struct PenUpCmd : cursiv::Extends<Command> {};
struct PenDownCmd : cursiv::Extends<Command> {};
struct HideTurtleCmd : cursiv::Extends<Command> {};
struct ShowTurtleCmd : cursiv::Extends<Command> {};
struct HomeCmd : cursiv::Extends<Command> {};
struct LabelCmd : cursiv::Extends<Command> {};
struct StopCmd : cursiv::Extends<Command> {};

struct SetxyCmd : cursiv::Extends<Command> {
  Expression* x = nullptr;
  Expression* y = nullptr;
};

struct MakeCmd : cursiv::Extends<Command> {
  string_view name;
  Value value;
};

struct ProcedureInvocation : cursiv::Extends<Command> {
  string_view name;
  vector<Expression*> args;
};

struct IfCmd : cursiv::Extends<Command> {
  Expression* left = nullptr;
  CompareOp op = CompareOp::equal;
  Expression* right = nullptr;
  vector<Command*> body;
};

struct ForCmd : cursiv::Extends<Command> {
  string_view var;
  Expression* from = nullptr;
  Expression* to = nullptr;
  Expression* step = nullptr;
  vector<Command*> body;
};

struct CommandLine : cursiv::Extends<Line> {
  vector<Command*> commands;
  string_view comment;
};

struct CommentLine : cursiv::Extends<Line> {
  string_view text;
};

struct PrintLine : cursiv::Extends<Line> {
  Value value;
  string_view comment;
};

struct ProcedureDeclaration : cursiv::Extends<Line> {
  string_view name;
  vector<string_view> params;
  vector<Line*> body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace logo::ast
