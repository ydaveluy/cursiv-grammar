// The mumps language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace mumps::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  intdivide,
  modulo,
  exponent,
  concat,
  gt,
  lt,
  ngt,
  nlt,
  eq,
};

struct BinaryExpression : cursiv::Extends<Expression> {
  Expression* left = nullptr;
  Operator op{};
  Expression* right = nullptr;
};

struct UnaryExpression : cursiv::Extends<Expression> {
  string_view op;
  Expression* operand;
};

struct NumberLiteral : cursiv::Extends<Expression> {
  double value = 0;
};

// `global` is the leading `^` (a routine's global array), `extrinsic` the
// leading `&` (an extrinsic special variable); either may repeat, so only
// their presence is kept.
struct Variable : cursiv::Extends<Expression> {
  bool global = false;
  bool extrinsic = false;
  string_view name;
  vector<Expression*> subscripts;
};

struct FunctionCall : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> args;
};

struct GroupedExpression : cursiv::Extends<Expression> {
  Expression* expression = nullptr;
};

// `!` in an argument list: a newline in WRITE's output.
struct BangArg : cursiv::Extends<Expression> {};

struct StringArg : cursiv::Extends<Expression> {
  string_view value;
};

struct Assignment : cursiv::Extends<cursiv::AstNode> {
  vector<Expression*> targets;
  Expression* value = nullptr;
};

struct Command : cursiv::Extends<cursiv::AstNode> {};

struct BreakCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
};

struct DoCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  string_view name;
  vector<Expression*> params;
};

struct ForParameter : cursiv::Extends<cursiv::AstNode> {
  Expression* start;
  Expression* step;
  Expression* end;
};
struct ForCommand : cursiv::Extends<Command> {
  Expression* variable;
  vector<ForParameter*> parameters;
  vector<Command*> commands;
  Expression* condition;
};

struct HaltCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
};

struct HangCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  Expression* duration = nullptr;
};

struct IfCommand : cursiv::Extends<Command> {
  Expression* condition = nullptr;
  Command* command = nullptr;
};

struct KillCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Expression*> args;
};

struct MergeCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Assignment*> pairs;
};

struct NewCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Expression*> args;
};

struct QuitCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  Expression* value = nullptr;
};

struct ReadCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Expression*> args;
};

struct SetCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Assignment*> assigns;
};

struct ViewCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  string_view name;
};

struct WriteCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  vector<Expression*> args;
};

struct XecuteCommand : cursiv::Extends<Command> {
  Expression* postcondition = nullptr;
  string_view code;
};

// CLOSE, ELSE, GOTO, JOB, LOCK, OPEN, TCOMMIT, TRESTART, TROLLBACK, TSTART,
// USE: bare keywords, with no argument of their own.
struct OtherCommand : cursiv::Extends<Command> {
  string_view keyword;
  Expression* postcondition = nullptr;
  string_view argumentText;
};

struct Subproc : cursiv::Extends<Command> {
  string_view name;
  vector<Expression*> params;
  vector<Command*> commands;
};

struct Line : cursiv::Extends<cursiv::AstNode> {};

struct CodeLine : cursiv::Extends<Line> {
  string_view label;
  vector<Command*> commands;
};

struct RoutineDecl : cursiv::Extends<Line> {
  bool percent = false;
  string_view name;
  vector<Expression*> params;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace mumps::ast
