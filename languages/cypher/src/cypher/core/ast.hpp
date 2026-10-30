// The cypher language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cypher::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Literals and primaries.
struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// Hex, octal, decimal and float forms denote different numeric domains (an
// arbitrary-width integer vs a float64), so the text is kept raw rather than
// forced into one C++ type.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct MapPair : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Expr* value;
};

struct MapLit : cursiv::Extends<Expr> {
  vector<MapPair*> pairs;
};

struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// `$name` or `$0`: a query parameter, bound outside the document.
struct ParamRef : cursiv::Extends<Expr> {
  string_view name;
};

// A bare variable use: `n`, `count`, a backtick-escaped name.
struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct FuncCallExpr : cursiv::Extends<Expr> {
  string_view name;  // dotted, joined: `apoc.create.node`
  boolean distinct;
  vector<Expr*> args;
};

// `count(*)`.
struct CountAllExpr : cursiv::Extends<Expr> {};

struct CaseWhen : cursiv::Extends<cursiv::AstNode> {
  Expr* when;
  Expr* then;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr* subject;  // null for the searched form (bare CASE WHEN ...)
  vector<CaseWhen*> whens;
  Expr* elseValue;
};

// `symbol IN expression WHERE ...`, shared by a list comprehension and the
// ALL/ANY/NONE/SINGLE predicates.
struct FilterExpr : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  Expr* source;
  Expr* where_;
};

// `[x IN list WHERE cond | x.a]`.
struct ListComprehensionExpr : cursiv::Extends<Expr> {
  FilterExpr* filter;
  Expr* projection;  // null when the list itself is the result
};

// `ALL(x IN list WHERE cond)`, and its ANY/NONE/SINGLE siblings.
struct FilterWithExpr : cursiv::Extends<Expr> {
  string_view kind;  // the keyword as written
  FilterExpr* filter;
};

struct NodePattern : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  vector<string_view> labels;
  Expr* properties;  // a MapLit or a ParamRef
};

struct RangeLit : cursiv::Extends<cursiv::AstNode> {
  string_view min;
  string_view max;
  boolean hasRange;  // `..` was written -- distinguishes `*3` from `*3..`
};

struct RelationshipDetail : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  vector<string_view> types;
  RangeLit* range;
  Expr* properties;
};

struct RelationshipPattern : cursiv::Extends<cursiv::AstNode> {
  boolean pointsLeft;
  boolean pointsRight;
  RelationshipDetail* detail;
};

struct RelChainStep : cursiv::Extends<cursiv::AstNode> {
  RelationshipPattern* rel;
  NodePattern* node;
};

// `{1,3}`, `{1,}`, `{,3}`, `{,}`, `+`, `*` on a quantified path segment.
struct Quantifier : cursiv::Extends<cursiv::AstNode> {
  string_view min;
  string_view max;
  boolean hasComma;
  boolean plus;
  boolean star;
};

struct PatternElem : cursiv::Extends<cursiv::AstNode> {};

struct QppChainStep : cursiv::Extends<cursiv::AstNode> {
  PatternElem* inner;
  Quantifier* quantifier;
  NodePattern* node;
};

// `(a)-[r]->(b)`, and quantified path segments chained the same way.
struct SimplePatternElem : cursiv::Extends<PatternElem> {
  NodePattern* first;
  vector<std::variant<RelChainStep*, QppChainStep*>> chain;
};

// `((a)-->(b)){1,3}`: a parenthesized sub-pattern under its own quantifier.
struct GroupedPatternElem : cursiv::Extends<PatternElem> {
  PatternElem* inner;
  Quantifier* quantifier;
};

struct PatternPart : cursiv::Extends<cursiv::AstNode> {
  string_view variable;  // `p =` -- empty when unbound
  PatternElem* elem;
};

struct Pattern : cursiv::Extends<cursiv::AstNode> {
  vector<PatternPart*> parts;
};

// `(a)-->(b)` used as a value, not inside MATCH/CREATE/MERGE.
struct RelChainPatternExpr : cursiv::Extends<Expr> {
  NodePattern* first;
  vector<RelChainStep*> chain;
};

// `[p = (a)-->(b) WHERE cond | p]`.
struct PatternComprehensionExpr : cursiv::Extends<Expr> {
  string_view variable;  // `p =` -- empty when unbound
  RelChainPatternExpr* pattern;
  Expr* where_;
  Expr* projection;
};

struct RegularQuery;

// `EXISTS { MATCH ... }` or `EXISTS { <regular query> }`.
struct ExistsExpr : cursiv::Extends<Expr> {
  RegularQuery* query;   // the regular-query form
  Pattern* pattern;      // the bare-pattern form
  Expr* where_;          // only with the bare-pattern form
};

struct PropertyExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view property;
};

struct HasLabelsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<string_view> labels;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
};

struct SliceExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* from_;  // null when omitted
  Expr* to_;    // null when omitted
};

// Unary `+`/`-`, and the prefix `NOT` (chainable: `NOT NOT x`).
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

enum class Operator : std::uint8_t {
  power,
  multiply,
  divide,
  modulo,
  add,
  subtract,
  starts_with,
  ends_with,
  contains,
  regex_match,
  in_,
  equal,
  less_equal,
  greater_equal,
  less,
  greater,
  not_equal,
  logical_and,
  logical_xor,
  logical_or,
};

struct GroupExpr : cursiv::Extends<Expr> {
  Expr* value;
};
struct ComparisonChain : cursiv::Extends<Expr> {
  vector<Expr*> operands;
  vector<Operator> operators;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

// `x IS NULL` / `x IS NOT NULL`: no right operand, so not a BinaryExpr.
struct IsNullExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
};

// Reading and updating clauses.

struct MatchClause : cursiv::Extends<cursiv::AstNode> {
  boolean optional_;
  Pattern* pattern;
  Expr* where_;
};

struct UnwindClause : cursiv::Extends<cursiv::AstNode> {
  Expr* source;
  string_view alias;
};

// `(name AS)? bound` -- `name` is empty when the source keeps its own name.
struct YieldItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view alias;
};

struct QueryCallClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> args;
  vector<YieldItem*> yieldItems;
  Expr* where_;  // filters the yielded rows; only with yieldItems
};

struct CreateClause : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
};

struct SetProperty : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  Expr* value;
};

struct SetVariable : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  boolean add;  // `+=` merges rather than replaces
  Expr* value;
};

struct SetLabels : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  vector<string_view> labels;
};

struct SetClause : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<SetProperty*, SetVariable*, SetLabels*>> items;
};

struct MergeAction : cursiv::Extends<cursiv::AstNode> {
  boolean onCreate;  // false means ON MATCH
  SetClause* set;
};

struct MergeClause : cursiv::Extends<cursiv::AstNode> {
  PatternPart* part;
  vector<MergeAction*> actions;
};

struct DeleteClause : cursiv::Extends<cursiv::AstNode> {
  boolean detach;
  vector<Expr*> items;
};

struct RemoveLabels : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  vector<string_view> labels;
};

struct RemoveClause : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<RemoveLabels*, Expr*>> items;
};

struct OrderItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  boolean descending;
};

struct OrderClause : cursiv::Extends<cursiv::AstNode> {
  vector<OrderItem*> items;
};

struct ProjectionItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  string_view alias;
};

struct ProjectionBody : cursiv::Extends<cursiv::AstNode> {
  boolean distinct;
  boolean star;  // `RETURN *` / `WITH *`; explicit items may follow it
  vector<ProjectionItem*> items;
  OrderClause* orderBy;
  Expr* skip_;
  Expr* limit_;
};

struct WithClause : cursiv::Extends<cursiv::AstNode> {
  ProjectionBody* body;
  Expr* where_;
};

struct ReturnClause : cursiv::Extends<cursiv::AstNode> {
  ProjectionBody* body;
};

using Statement =
    std::variant<MatchClause*, UnwindClause*, QueryCallClause*, CreateClause*,
                 MergeClause*, DeleteClause*, SetClause*, RemoveClause*>;

// One WITH-bounded segment of a query: statements in source order, then the
// WITH that closes the segment (null on the final part, which may instead
// carry a trailing RETURN).
struct QueryPart : cursiv::Extends<cursiv::AstNode> {
  vector<Statement> statements;
  WithClause* with;
  ReturnClause* ret;
};

struct SingleQuery : cursiv::Extends<cursiv::AstNode> {
  vector<QueryPart*> parts;
};

struct UnionClause : cursiv::Extends<cursiv::AstNode> {
  boolean all;
  SingleQuery* query;
};

struct RegularQuery : cursiv::Extends<cursiv::AstNode> {
  SingleQuery* first;
  vector<UnionClause*> unions;
};

struct StandaloneCall : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> args;
  boolean yieldAll;  // `YIELD *`
  vector<YieldItem*> yieldItems;
  Expr* where_;  // filters the yielded rows; only with yieldItems
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  std::variant<RegularQuery*, StandaloneCall*> query;
};

}  // namespace cypher::ast
