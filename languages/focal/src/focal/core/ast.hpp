// The focal language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace focal::ast {

struct LineNumber : cursiv::Extends<cursiv::AstNode> {
  int64 group;
  int64 line;
};

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
};

struct VariableRef : cursiv::Extends<Expression> {
  string_view name;
};

// `name(expr)`: FOCAL's array subscript, indistinguishable in shape from a
// builtin call -- BUILTIN takes lexical priority (see the grammar).
struct IndexedVariable : cursiv::Extends<Expression> {
  string_view name;
  Expression* index;
};

struct BuiltinCall : cursiv::Extends<Expression> {
  string_view name;
  Expression* arg;  // null only for the argument-free FRAN() form
};

// `(e)`, `[e]` or `<e>`: three bracket spellings, one grouping.
struct GroupedExpression : cursiv::Extends<Expression> {
  Expression* expression;
};

enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  power,
};

struct BinaryExpression : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};

struct Command : cursiv::Extends<cursiv::AstNode> {};

struct AskPair : cursiv::Extends<cursiv::AstNode> {
  string_view prompt;
  vector<string_view> variables;
};

struct AskCommand : cursiv::Extends<Command> {
  vector<AskPair*> pairs;
  Command* next;
};

struct DoCommand : cursiv::Extends<Command> {
  boolean all;             // explicit 'all'; no target/group also means all
  optional<int64> group;   // a bare group number
  LineNumber* target;      // a full line number
  Command* next;
};

struct ForCommand : cursiv::Extends<Command> {
  string_view variable;
  Expression* from;
  Expression* to;    // null: FOR A=B is the non-iterating SET form
  Expression* step;  // null: omitted increment, default one
  Command* body;
};

struct QuitCommand : cursiv::Extends<Command> {
  Command* next;
};

struct SetCommand : cursiv::Extends<Command> {
  string_view variable;
  Expression* index;  // null: a scalar assignment
  Expression* value;
  Command* next;  // null: no chained command
};

struct GotoCommand : cursiv::Extends<Command> {
  LineNumber* target;  // null: no line number was written
  Command* next;
};

struct IfCommand : cursiv::Extends<Command> {
  Expression* condition;
  LineNumber* negative;
  LineNumber* zero;
  LineNumber* positive;
  Command* next;  // null: no chained command
};

// The two repeated-punctuation forms of a TYPE item ('!'+ and '#'+), carried
// as a run length rather than as text.
struct BangRun {
  std::int64_t count;
};
struct HashRun {
  std::int64_t count;
};

// `%`, `%width`, `%width.decimals`: floating/default or fixed print format.
struct FormatSpec : cursiv::Extends<cursiv::AstNode> {
  boolean hasWidth;
  boolean hasDecimals;
  int64 width;
  int64 decimals;
};

struct DumpVariables : cursiv::Extends<cursiv::AstNode> {};

using TypeItem =
    std::variant<Expression*, BangRun, HashRun, std::string_view, FormatSpec*,
                 DumpVariables*>;

struct TypeCommand : cursiv::Extends<Command> {
  vector<TypeItem> items;
  Command* next;  // null: no chained command
};

struct ReturnCommand : cursiv::Extends<Command> {
  Command* next;
};

struct WriteCommand : cursiv::Extends<Command> {
  boolean all;
  optional<int64> group;  // a bare group number
  LineNumber* target;     // a full line number
  Command* next;
};

struct CommentCommand : cursiv::Extends<Command> {
  string_view text;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {
  LineNumber* number;
  Command* command;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace focal::ast
