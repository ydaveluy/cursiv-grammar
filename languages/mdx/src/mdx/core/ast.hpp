// The mdx language: its AST. An OLAP query -- one SELECT, a formula list,
// an axis list, a cube, an optional slicer and cell properties.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace mdx::ast {

// A dotted name: member_name, set_name, cube_name, property_ all share this
// one shape -- an identifier, then zero or more dot-separated identifiers.
struct Name : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> parts;
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct KeyPart : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean quoted;
};

struct KeyIdentifier : cursiv::Extends<Expr> {
  vector<KeyPart*> parts;
};

struct KeyAccess : cursiv::Extends<Expr> {
  Expr* object;
  vector<KeyPart*> parts;
};

struct Identifier : cursiv::Extends<Expr> {
  string_view name;
  boolean quoted;
};

struct StringLiteral : cursiv::Extends<Expr> {
  string_view text;
};

struct NumberLiteral : cursiv::Extends<Expr> {
  float64 value;
};

// `( exp_list )`: a tuple of members/expressions.
struct Tuple : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// `{ exp_list? }`: a set of members/tuples.
struct SetExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct FunctionCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

// `object.name`: a property or a segment of a member's unique name.
struct MemberAccess : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
  boolean quoted;
};

// `object.name(args)`: a method call chained off a member, e.g.
// `Measures.CurrentMember.Properties("Format String")`.
struct MethodCall : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
  vector<Expr*> args;
};

enum class UnaryOp : std::uint8_t { plus, minus, logical_not };

struct UnaryExpr : cursiv::Extends<Expr> {
  UnaryOp op;
  Expr* operand;
};

enum class BinaryOp : std::uint8_t {
  range,
  logical_xor,
  logical_or,
  logical_and,
  eq,
  ne,
  lt,
  gt,
  le,
  ge,
  concat,
  add,
  subtract,
  divide,
  multiply,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinaryOp op;
  Expr* right;
};

struct WhenClause : cursiv::Extends<cursiv::AstNode> {
  Expr* when;
  Expr* then;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<WhenClause*> whens;
  Expr* elseValue;
};

struct SetFormula : cursiv::Extends<cursiv::AstNode> {
  Name* name;
  Expr* expr;
};

struct MemberProperty : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct MemberFormula : cursiv::Extends<cursiv::AstNode> {
  Name* name;
  Expr* expr;
  vector<MemberProperty*> properties;
};

// Order matters (formulas may be read in sequence by later ones), so the
// two shapes share one list instead of two.
using Formula = std::variant<MemberFormula*, SetFormula*>;

struct AxisSpec : cursiv::Extends<cursiv::AstNode> {
  boolean nonEmpty;
  Expr* expr;
  vector<Name*> dimProps;
  string_view axisName;
};

struct CellProperty : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct SelectStatement : cursiv::Extends<cursiv::AstNode> {
  vector<Formula> formulas;
  vector<AxisSpec*> axes;
  Name* cube;
  Expr* slicer;
  vector<CellProperty*> cellProps;
};

}  // namespace mdx::ast
