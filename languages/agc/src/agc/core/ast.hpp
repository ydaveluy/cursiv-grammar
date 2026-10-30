// The agc language: its AST. A document is a sequence of lines, each one of
// five shapes an AGC assembly source line can take.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace agc::ast {

// A name, plus the chain of parenthesised indices applied to it --
// `variable: ... | variable LPAREN LABEL RPAREN` flattened from its
// recursion into one list. A numeric-looking operand also reads back here,
// in `name`: LABEL's alphabet is a superset of a plain number's, and under
// longest-match-then-declaration-order a digit run always lexes as LABEL,
// never as its own numeric token, so no separate numeric node kind exists.
struct VariableRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // empty when the variable is written as bare index groups
  vector<string_view> indices;
};

struct MulTerm : cursiv::Extends<cursiv::AstNode> {
  string_view op;  // "*", "/" or empty for the first term
  VariableRef* operand;
};

struct MulExpr : cursiv::Extends<cursiv::AstNode> {
  vector<MulTerm*> terms;
};

struct AddTerm : cursiv::Extends<cursiv::AstNode> {
  string_view op;  // "+", "-" or empty for the first term
  MulExpr* operand;
};

struct Expression : cursiv::Extends<cursiv::AstNode> {
  vector<AddTerm*> terms;
};

struct CommentLine : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct SourceInclude : cursiv::Extends<cursiv::AstNode> {
  string_view path;
};

struct BlankLine : cursiv::Extends<cursiv::AstNode> {
  string_view label;
};

// One physical operand row of an instruction: `argument: (ws expression)*
// (ws comment)?`. An instruction spans one or more of these.
struct ArgumentLine : cursiv::Extends<cursiv::AstNode> {
  vector<Expression*> expressions;
  string_view comment;
};

struct InstructionLine : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  string_view opcode;
  string_view opcode2;      // the "2-opcodes on a line" format; empty otherwise
  CommentLine* midComment;  // an optional comment row between the opcode
                            // row and the first argument row
  vector<ArgumentLine*> arguments;
};

struct EraseLine : cursiv::Extends<cursiv::AstNode> {
  VariableRef* variable;  // null: erase with no variable is legal
  vector<Expression*> expressions;
  string_view comment;
};

struct AssignmentLine : cursiv::Extends<cursiv::AstNode> {
  VariableRef* variable;
  vector<Expression*> expressions;
  string_view comment;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<CommentLine*, SourceInclude*, BlankLine*, InstructionLine*, EraseLine*,
                       AssignmentLine*>>
      lines;
};

}  // namespace agc::ast
