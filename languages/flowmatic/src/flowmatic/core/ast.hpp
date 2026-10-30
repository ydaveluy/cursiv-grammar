// The flowmatic language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace flowmatic::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};

// An INPUT/OUTPUT filename and file-designator token (e.g. FILE-A).
// The source spelling is kept rather than resolving the designator to A.
struct FileBinding : cursiv::Extends<cursiv::AstNode> {
  string_view filename;
  string_view fileletter;
};

struct HspStmt : cursiv::Extends<Statement> {
  string_view file;
};

struct OutputStmt : cursiv::Extends<Statement> {
  vector<FileBinding *> files;
};

struct InputStmt : cursiv::Extends<Statement> {
  vector<FileBinding *> files;
};

struct JumpToStmt : cursiv::Extends<Statement> {
  string_view target;
};

struct ReadItemStmt : cursiv::Extends<Statement> {
  string_view file;
};

struct WriteItemStmt : cursiv::Extends<Statement> {
  string_view file;
  bool punched = false;
};

struct TransferStmt : cursiv::Extends<Statement> {
  string_view from;
  string_view to;
};

enum class Cond : std::uint8_t { equal, greater, less, end_of_data };

struct IfStmt : cursiv::Extends<Statement> {
  Cond cond;
  string_view target;
};

struct OtherwiseStmt : cursiv::Extends<Statement> {
  string_view target;
};

struct RewindStmt : cursiv::Extends<Statement> {
  string_view file;
};

struct StopStmt : cursiv::Extends<Statement> {};

struct CompareStmt : cursiv::Extends<Statement> {
  string_view leftField;
  string_view leftFile;
  string_view rightField;
  string_view rightFile;
};

struct Value : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  string_view file;
  bool quoted = false;
};

struct MoveStmt : cursiv::Extends<Statement> {
  Value *source = nullptr;
  Value *destination = nullptr;
};

enum class ArithmeticOp : std::uint8_t { add, subtract, multiply, divide };

struct ArithmeticStmt : cursiv::Extends<Statement> {
  ArithmeticOp op;
  Value *left = nullptr;
  Value *right = nullptr;
  Value *result = nullptr;
};

struct PrintItemStmt : cursiv::Extends<Statement> {
  string_view file;
};

struct ExecuteStmt : cursiv::Extends<Statement> {
  string_view first;
  string_view last;
};

// SET patches the target of a prior operation: `which` names the operation
// being rewritten, `target` its new destination.
struct SetStmt : cursiv::Extends<Statement> {
  string_view which;
  string_view target;
};

struct TestStmt : cursiv::Extends<Statement> {
  string_view field;
  string_view file;
  Value *against = nullptr;
};

struct CloseOutStmt : cursiv::Extends<Statement> {
  vector<string_view> files;
};

struct Line : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  vector<Statement *> statements;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  boolean hasEnd;
  vector<Line *> lines;
};

} // namespace flowmatic::ast
