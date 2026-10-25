// The joss language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace joss::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};

enum class SelKind : std::uint8_t { step_, part_, form_ };
enum class AllKind : std::uint8_t { steps_, parts_, forms_, values_ };

// A selector names a stored step, part or form, or 'all' of a kind -- the
// two shapes a delete/to/do/type command can point at.
struct Selector : cursiv::Extends<cursiv::AstNode> {};

struct RefSelector : cursiv::Extends<Selector> {
  SelKind kind;
  string_view number;  // a STEPNUMBER ("n.n") or a PARTNUMBER ("n")
};

struct AllSelector : cursiv::Extends<Selector> {
  optional<AllKind> restriction;
};

struct MathExpr : cursiv::Extends<cursiv::AstNode> {};

struct GroupedMath : cursiv::Extends<MathExpr> {
  MathExpr* inner;  // '(' or '[' -- JOSS does not distinguish the two
};

struct NumberLit : cursiv::Extends<MathExpr> {
  float64 value;
};

struct Variable : cursiv::Extends<MathExpr> {
  string_view name;  // one letter
  MathExpr* index;   // ALPHA '(' mathExp ')' -- null when unsubscripted
};

enum class UnaryFn : std::uint8_t {
  sqrt_,
  log_,
  exp_,
  sin_,
  cos_,
  ip_,
  fp_,
  dp_,
  xp_,
  sgn_,
};

struct UnaryCall : cursiv::Extends<MathExpr> {
  UnaryFn fn;
  MathExpr* arg;
};

enum class NaryFn : std::uint8_t { max_, min_ };

struct NaryCall : cursiv::Extends<MathExpr> {
  NaryFn fn;
  MathExpr* first;
  vector<MathExpr*> rest;  // one or more, from argList
};

enum class MathOp : std::uint8_t { add, sub, mul, div };

struct BinMath : cursiv::Extends<MathExpr> {
  MathExpr* left;
  MathOp op;
  MathExpr* right;
};

struct Assignment : cursiv::Extends<cursiv::AstNode> {
  Variable* target;
  MathExpr* value;
};

struct BoolExpr : cursiv::Extends<cursiv::AstNode> {};

struct GroupedBool : cursiv::Extends<BoolExpr> {
  BoolExpr* inner;  // '(' or '[' -- merged, as for GroupedMath
};

// JOSS spells ">=" as '=>' (a typewriter-era substitute).
enum class BoolCompOp : std::uint8_t { eq, ne, le, ge, lt, gt };

struct Comparison : cursiv::Extends<BoolExpr> {
  MathExpr* left;
  BoolCompOp op;
  MathExpr* right;
};

enum class BoolOp : std::uint8_t { and_, or_ };

// Only a bare comparison, never a grouped one, may stand left of 'and'/'or'
// in JOSS -- kept as the field's type.
struct BoolChain : cursiv::Extends<BoolExpr> {
  Comparison* left;
  BoolOp op;
  BoolExpr* right;
};

struct RangeVal : cursiv::Extends<cursiv::AstNode> {
  MathExpr* start;
  MathExpr* step;   // null for a plain value
  RangeVal* rest;   // null unless step is set
};

struct Conditional : cursiv::Extends<cursiv::AstNode> {};

struct IfCond : cursiv::Extends<Conditional> {
  BoolExpr* cond;
};

struct ForCond : cursiv::Extends<Conditional> {
  Variable* var;
  vector<RangeVal*> values;
};

// Every direct/indirect command, whether or not a STEPNUMBER prefixes it.
struct Command : cursiv::Extends<cursiv::AstNode> {};

struct GoCmd : cursiv::Extends<Command> {};
struct CancelCmd : cursiv::Extends<Command> {};

struct DeleteCmdCmd : cursiv::Extends<Command> {
  Selector* target;
};

struct DoneCmd : cursiv::Extends<Command> {};
struct StopCmd : cursiv::Extends<Command> {};

struct DemandCmd : cursiv::Extends<Command> {
  Variable* var;
};

struct ToCmd : cursiv::Extends<Command> {
  Selector* target;
};

struct SetCmd : cursiv::Extends<Command> {
  Assignment* assignment;
};

struct DoCmd : cursiv::Extends<Command> {
  Selector* target;
  Conditional* cond;  // null when absent
};

struct TypeContent : cursiv::Extends<cursiv::AstNode> {};

struct TypeFormContent : cursiv::Extends<TypeContent> {
  vector<Variable*> vars;
  RefSelector* form;
};

// A heterogeneous list element: namespace scope keeps the AstNode vocabulary
// out of reach, so this is the plain std spelling (cursiv/core/reference/ast.md).
using TypeElemVal = std::variant<std::string_view, Variable*>;

struct TypeDataContent : cursiv::Extends<TypeContent> {
  vector<TypeElemVal> items;
};

struct TypeSelContent : cursiv::Extends<TypeContent> {
  Selector* sel;
};

enum class TypeSysKind : std::uint8_t { size_, time_, users_ };

struct TypeSysContent : cursiv::Extends<TypeContent> {
  TypeSysKind kind;
};

struct TypeCmd : cursiv::Extends<Command> {
  TypeContent* content;
};

struct DeleteValCmd : cursiv::Extends<Command> {
  vector<Variable*> vars;
  boolean allValues;
};

struct LineCmd : cursiv::Extends<Command> {};
struct PageCmd : cursiv::Extends<Command> {};

struct DirectStmt : cursiv::Extends<Statement> {
  Command* command;
};

struct IndirectStmt : cursiv::Extends<Statement> {
  string_view stepNumber;
  Command* command;
};

struct FormStmt : cursiv::Extends<Statement> {
  string_view partNumber;
};

struct AssignStmt : cursiv::Extends<Statement> {
  Assignment* assignment;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace joss::ast
