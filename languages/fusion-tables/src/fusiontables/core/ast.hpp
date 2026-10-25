// The fusiontables language: its AST -- Google Fusion Tables' SQL-shaped
// query language (ALTER/CREATE/DROP/DESCRIBE/SHOW TABLES, SELECT/INSERT/
// UPDATE/DELETE, and geospatial WHERE conditions).
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fusiontables::ast {

struct Coordinate : cursiv::Extends<cursiv::AstNode> {
  float64 lat;
  float64 lng;
};

struct Geometry : cursiv::Extends<cursiv::AstNode> {};

struct Circle : cursiv::Extends<Geometry> {
  Coordinate* center;
  float64 radius;
};

struct Rectangle : cursiv::Extends<Geometry> {
  Coordinate* corner1;
  Coordinate* corner2;
};

// `table` is empty when the column is written unqualified.
struct QualifiedColumnName : cursiv::Extends<cursiv::AstNode> {
  string_view table;
  string_view column;
};

enum class AggregateFunc : std::uint8_t { Sum, Count, Average, Maximum, Minimum };

struct AggregateExp : cursiv::Extends<cursiv::AstNode> {
  AggregateFunc func;
  QualifiedColumnName* column;
};

// One of the four shapes `*`, `table.*`, a column, or an aggregate call --
// mutually exclusive, told apart by which member is set.
struct ResultColumn : cursiv::Extends<cursiv::AstNode> {
  boolean star;             // '*'
  string_view tableStar;    // "<table>.*"; empty otherwise
  QualifiedColumnName* column;
  AggregateExp* aggregate;
};

// The value family: a number or a (quote-decoded) string.
struct Literal : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Literal> {
  float64 value;
};

struct StringLit : cursiv::Extends<Literal> {
  string_view value;
};

enum class CompareOp : std::uint8_t { Less, LessEqual, Greater, GreaterEqual, Equal };
enum class TextOp : std::uint8_t {
  Like,
  Matches,
  StartsWith,
  EndsWith,
  Contains,
  ContainsIgnoringCase,
  DoesNotContain,
  NotEqualTo,
};
enum class LogicalOp : std::uint8_t { And, Or };

// The WHERE-clause condition family, glued by AND/OR into a right-leaning
// chain (the grammar has no parenthesised grouping to nest instead).
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct CompareCond : cursiv::Extends<Expr> {
  QualifiedColumnName* column;
  CompareOp op;
  Literal* value;
};

struct TextCond : cursiv::Extends<Expr> {
  QualifiedColumnName* column;
  TextOp op;
  StringLit* value;
};

struct InCond : cursiv::Extends<Expr> {
  QualifiedColumnName* column;
  vector<StringLit*> values;
};

struct BetweenCond : cursiv::Extends<Expr> {
  QualifiedColumnName* column;
  Literal* low;
  Literal* high;
};

struct IntersectsCond : cursiv::Extends<Expr> {
  QualifiedColumnName* column;
  Geometry* shape;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  LogicalOp op;
  Expr* right;
};

struct TableItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view alias;  // empty when written without AS
};

struct JoinClause : cursiv::Extends<cursiv::AstNode> {
  TableItem* table;
  QualifiedColumnName* left;
  QualifiedColumnName* right;
};

// `distanceFrom` is set only for `ST_DISTANCE(column, coordinate)`; null
// means a plain column key.
struct OrderTerm : cursiv::Extends<cursiv::AstNode> {
  QualifiedColumnName* column;
  Coordinate* distanceFrom;
  boolean descending;
};

struct ColumnAssignment : cursiv::Extends<cursiv::AstNode> {
  string_view column;
  Literal* value;
};

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct SelectStmt : cursiv::Extends<Stmt> {
  vector<ResultColumn*> items;
  TableItem* from;
  vector<JoinClause*> joins;
  Expr* where;
  vector<QualifiedColumnName*> groupBy;
  vector<OrderTerm*> orderBy;
  optional<float64> offset;
  optional<float64> limit;
};

struct CreateViewStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<ResultColumn*> items;
  TableItem* from;
  Expr* where;
  vector<JoinClause*> joins;
};

struct CreateTableAsSelectStmt : cursiv::Extends<Stmt> {
  string_view name;
  string_view from;
};

struct AlterTableStmt : cursiv::Extends<Stmt> {
  string_view table;
  string_view newName;
};

struct DropTableStmt : cursiv::Extends<Stmt> {
  string_view table;
};

struct DescribeStmt : cursiv::Extends<Stmt> {
  string_view table;
};

struct ShowTablesStmt : cursiv::Extends<Stmt> {};

struct InsertStmt : cursiv::Extends<Stmt> {
  string_view table;
  vector<string_view> columns;
  vector<Literal*> values;
};

struct UpdateStmt : cursiv::Extends<Stmt> {
  string_view table;
  vector<ColumnAssignment*> assignments;
  string_view whereColumn;
  StringLit* whereValue;
};

struct DeleteStmt : cursiv::Extends<Stmt> {
  string_view table;
  string_view whereColumn;  // empty when the WHERE clause is absent
  Literal* whereValue;      // null when the WHERE clause is absent
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

}  // namespace fusiontables::ast
