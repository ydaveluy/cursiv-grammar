// The JPA (JPQL) language: its AST. Every path-expression flavor (state
// field, single-valued association, collection-valued association)
// collapses to one PathExpr: telling them apart needs a symbol table, not
// a parse.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace jpa::ast {

enum class Operator : uint8_t {
  add,
  subtract,
  multiply,
  divide,
  lt,
  le,
  gt,
  ge,
  eq,
  ne,
  logical_and,
  logical_or,
};
enum class AggFunc : uint8_t { avg, max, min, sum, count };
enum class Func1 : uint8_t { length, abs, sqrt, size, lower, upper };
enum class TrimSpec : uint8_t { unspecified, leading, trailing, both };
enum class CurrentKind : uint8_t { date, time, timestamp };
enum class AllAnyKind : uint8_t { all, any, some };
enum class JoinKind : uint8_t { plain, left, left_outer, inner };

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct PathExpr : cursiv::Extends<Expr> {
  vector<string_view> segments;
};
struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NullLit : cursiv::Extends<Expr> {};
struct CharLit : cursiv::Extends<Expr> {
  string_view value;
};
struct InputParam : cursiv::Extends<Expr> {
  boolean positional;
  string_view text;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct AggregateExpr : cursiv::Extends<Expr> {
  AggFunc func;
  boolean distinct;
  Expr* arg;
};
struct Func1Expr : cursiv::Extends<Expr> {
  Func1 func;
  Expr* arg;
};
struct ModExpr : cursiv::Extends<Expr> {
  Expr* left;
  Expr* right;
};
struct ConcatExpr : cursiv::Extends<Expr> {
  Expr* left;
  Expr* right;
};
struct LocateExpr : cursiv::Extends<Expr> {
  Expr* pattern;
  Expr* text;
  Expr* start;
};
struct SubstringExpr : cursiv::Extends<Expr> {
  Expr* source;
  Expr* start;
  Expr* length;
};
struct TrimExpr : cursiv::Extends<Expr> {
  TrimSpec spec;
  Expr* trimChar;
  Expr* source;
};
struct CurrentValueExpr : cursiv::Extends<Expr> {
  CurrentKind kind;
};

struct ObjectExpr : cursiv::Extends<Expr> {
  string_view alias;
};
struct ConstructorExpr : cursiv::Extends<Expr> {
  PathExpr* className;
  vector<Expr*> args;
};

struct Subquery;
struct SubqueryExpr : cursiv::Extends<Expr> {
  Subquery* query;
};
struct AllAnyExpr : cursiv::Extends<Expr> {
  AllAnyKind kind;
  Subquery* query;
};
struct ExistsExpr : cursiv::Extends<Expr> {
  boolean negated;
  Subquery* query;
};

struct BetweenExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* operand;
  Expr* low;
  Expr* high;
};
struct LikeExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* operand;
  Expr* pattern;
  Expr* escape; // Single-character literal or character-valued parameter.
};
struct InExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* operand;
  vector<Expr*> items;
  Subquery* subquery;
};
struct IsNullExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* operand;
};
struct IsEmptyExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* operand;
};
struct MemberOfExpr : cursiv::Extends<Expr> {
  boolean negated;
  Expr* entity;
  PathExpr* collection;
};

struct RangeVarDecl : cursiv::Extends<cursiv::AstNode> {
  string_view schemaName;
  string_view alias;
};
struct JoinStep : cursiv::Extends<cursiv::AstNode> {
  boolean fetch;
  JoinKind kind;
  PathExpr* path;
  string_view alias;
};

// A from-clause item: an entity range (with joins), a bare aliased path
// (subqueries only) or an `IN (...)` collection membership.
struct FromItem : cursiv::Extends<cursiv::AstNode> {};
struct IdVarDecl : cursiv::Extends<FromItem> {
  RangeVarDecl* range;
  vector<JoinStep*> joins;
};
struct AliasedPathDecl : cursiv::Extends<FromItem> {
  PathExpr* path;
  string_view alias;
};
struct CollectionMemberDecl : cursiv::Extends<FromItem> {
  PathExpr* path;
  string_view alias;
};

struct OrderByItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  boolean descending;
};

struct Subquery : cursiv::Extends<cursiv::AstNode> {
  boolean distinct;
  Expr* column;
  vector<FromItem*> from;
  Expr* where;
  vector<Expr*> groupBy;
  Expr* having;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct SelectStatement : cursiv::Extends<Statement> {
  boolean distinct;
  vector<Expr*> columns;
  vector<FromItem*> from;
  Expr* where;
  vector<Expr*> groupBy;
  Expr* having;
  vector<OrderByItem*> orderBy;
};

struct UpdateItem : cursiv::Extends<cursiv::AstNode> {
  PathExpr* target;
  Expr* value;
};
struct UpdateStatement : cursiv::Extends<Statement> {
  string_view schemaName;
  string_view alias;
  vector<UpdateItem*> items;
  Expr* where;
};

struct DeleteStatement : cursiv::Extends<Statement> {
  string_view schemaName;
  string_view alias;
  Expr* where;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  Statement* statement;
};

}  // namespace jpa::ast
