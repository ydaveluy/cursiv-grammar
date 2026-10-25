// The velocity language: its AST. A document is a run of atoms -- literal
// text, references, directives -- and expressions inside directives are all
// nodes under one base, `Expr`, the way a heterogeneous chain of unrelated
// productions must be to feed one `infix<>` table.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace velocity::ast {

enum class Operator {
  mul, div, mod, add, sub,
  eq, ne, lt, le, gt, ge,
  logical_and, logical_or, logical_not, negate,
  range,
};

// A value or subexpression -- StringLit, IntLit, FloatLit, VariableRef,
// FormalRef, PropertyAccess, IndexAccess, CallAccess, IdExpr, BinaryExpr,
// UnaryExpr, ListLit, MapLit.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// `$foo`, `$!foo`: the bare base of a reference, no chain yet. Reachable
// both as a text atom and as an expression primary.
struct VariableRef : cursiv::Extends<Expr> {
  boolean quiet;
  string_view name;
};

// `${foo}`, `$!{foo}`: the braced base -- a text atom only, VTL never
// allows it inside an expression.
struct FormalRef : cursiv::Extends<Expr> {
  boolean quiet;
  string_view name;
};

// `<object>.name`: one step of a chain on a VariableRef, FormalRef, or
// another access -- the chain is a left fold, each step wrapping the one
// before it as `object`.
struct PropertyAccess : cursiv::Extends<Expr> {
  Expr* object = nullptr;
  string_view name;
};

struct IndexAccess : cursiv::Extends<Expr> {
  Expr* object = nullptr;
  Expr* index = nullptr;
};

struct CallAccess : cursiv::Extends<Expr> {
  Expr* object = nullptr;
  vector<Expr*> args;
};

// A bare word used as a value: an identifier, or one of VTL's soft
// operator words (`eq`, `true`, `null`, ...) read outside operator
// position.
struct IdExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  Operator op;
  Expr* right = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  Operator op;
  Expr* operand = nullptr;
};

struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key = nullptr;
  Expr* value = nullptr;
};

struct MapLit : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};

struct Directive : cursiv::Extends<cursiv::AstNode> {};

// A reference atom's static type is `Expr*` here, whatever it dynamically
// is: VariableRef/FormalRef for a bare base, PropertyAccess/IndexAccess/
// CallAccess when a chain follows it.
using Atom = std::variant<std::string_view, Expr*, Directive*>;

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Atom> atoms;
};

struct IfBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* cond = nullptr;
  Block* body = nullptr;
};

struct SetDirective : cursiv::Extends<Directive> {
  Expr* target = nullptr;
  Expr* value = nullptr;
};

struct IfDirective : cursiv::Extends<Directive> {
  Expr* cond = nullptr;
  Block* thenBlock = nullptr;
  vector<IfBranch*> elseifs;
  Block* elseBlock = nullptr;
};

struct ForeachDirective : cursiv::Extends<Directive> {
  VariableRef* var = nullptr;
  Expr* iterable = nullptr;
  Block* body = nullptr;
};

struct BreakDirective : cursiv::Extends<Directive> {};
struct StopDirective : cursiv::Extends<Directive> {};

struct MacroDirective : cursiv::Extends<Directive> {
  vector<Expr*> params;
  Block* body = nullptr;
};

struct ParseDirective : cursiv::Extends<Directive> {
  Expr* path = nullptr;
};

struct DefineDirective : cursiv::Extends<Directive> {
  Expr* target = nullptr;
  Block* body = nullptr;
};

struct IncludeDirective : cursiv::Extends<Directive> {
  vector<Expr*> paths;
};

struct EvaluateDirective : cursiv::Extends<Directive> {
  Expr* expr = nullptr;
};

struct MacroCallDirective : cursiv::Extends<Directive> {
  string_view name;
  vector<Expr*> args;
  Block* body = nullptr;
};

struct CustomDirective : cursiv::Extends<Directive> {
  string_view name;
  vector<Expr*> args;
};

}  // namespace velocity::ast
