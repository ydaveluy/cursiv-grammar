// The AQL (ArangoDB Query Language) AST: a query is a read (FOR/RETURN,
// possibly a graph traversal) or a write
// (INSERT/UPDATE/REPLACE/REMOVE/UPSERT).
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace aql::ast {

struct Query : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

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

// A bare name read as a value: the variable a FOR/LET bound, or a collection
// used without quoting. No scope is modelled, so it stays text.
struct IdRef : cursiv::Extends<Expr> {
  string_view name;
};

struct BindParamRef : cursiv::Extends<Expr> {
  string_view name;  // "@name", '@' kept
};

// `@@name`, valid where a FOR's source may be a collection rather than an
// expression.
struct CollectionRef : cursiv::Extends<Expr> {
  string_view name;
};

// NEW / OLD: available after a data-modification clause: no flag on `false`,
// the class itself is the fact.
struct NewOldRef : cursiv::Extends<Expr> {
  boolean isNew;
};

// `[ expr? ]`. A multi-item array is the same shape as any comma chain: see
// `BinaryExpr` with `op == comma`, reached through `value`.
struct ArrayLit : cursiv::Extends<Expr> {
  Expr* value;
};

struct Pair : cursiv::Extends<cursiv::AstNode> {
  string_view key;  // a bare name or a decoded string, joined to one text
  Expr* value;
};

// `{ pair, ... }` or the shorthand `{ id, ... }` (RETURN's `{ name }` reading
// a variable of the same name) -- mutually exclusive, never both populated.
struct ObjectLit : cursiv::Extends<Expr> {
  vector<Pair*> pairs;
  vector<string_view> shorthand;
};

// `( data_access_query )`: a read used where an expression is expected.
struct SubqueryExpr : cursiv::Extends<Expr> {
  Query* query;
};

// An empty call has no args; multiple arguments retain the comma expression.
struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  Expr* args = nullptr;
};

// `a[expr]`, `a[*]`.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
  boolean star;
};

// `a.b`. A full expression may follow `.` grammatically; restricted here to
// a name, which is what every real use (and every corpus document) writes.
struct DotExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

// `+x`, `-x`.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `!x`, `NOT x`, and a run of either: nested, one node per token.
struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

enum class BinOp : std::uint8_t {
  multiply,
  divide,
  modulo,
  add,
  subtract,
  eq,
  ne,
  gt,
  lt,
  ge,
  le,
  regexMatch,
  regexNotMatch,
  assign,
  comma,
  range,
  in,
  notIn,
  like,
  notLike,
  and_,
  or_,
};

// Every binary level of the operator ladder, and the comma chain, which is
// one more left-associative level.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

enum class Quantifier : std::uint8_t { all, none, any };
enum class CmpOp : std::uint8_t { in, eq, ne, gt, ge, lt, le };

// `expr ALL == expr`, `expr ANY IN expr`: an array comparison, kept apart
// from `BinaryExpr` since its operator is two words with independent
// meanings.
struct QuantifiedExpr : cursiv::Extends<Expr> {
  Expr* left;
  Quantifier quant;
  CmpOp cmp;
  Expr* right;
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* then_;
  Expr* else_;
};

// `(LET expr)+`: LET has no name token of its own -- a binding is
// ordinarily an assignment expression (`x = 1`), read generically.
struct LetList : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> bindings;
};

// `id_` or `@@name`.
struct Collection : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view bindParam;
};

struct Options : cursiv::Extends<cursiv::AstNode> {
  ObjectLit* value;
};

struct Search : cursiv::Extends<cursiv::AstNode> {
  Expr* condition;
  Options* options;
};

struct ForIn : cursiv::Extends<cursiv::AstNode> {
  LetList* letBefore;
  string_view var;
  Expr* source;  // an expression, or a CollectionRef (`@@name`)
  Search* search;
  LetList* letAfter;
};

struct Filter : cursiv::Extends<cursiv::AstNode> {
  LetList* letBefore;
  Expr* condition;
  LetList* letAfter;
};

struct SortTerm : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  boolean descending;  // ASC is the default and writes nothing
};

struct Sort : cursiv::Extends<cursiv::AstNode> {
  LetList* letBefore;
  vector<SortTerm*> terms;
  LetList* letAfter;
};

// `LIMIT count` or `LIMIT offset, count`: `second`, once present, is the
// count and `count` becomes the offset.
struct Limit : cursiv::Extends<cursiv::AstNode> {
  LetList* letBefore;
  float64 count;
  optional<float64> second;
  LetList* letAfter;
};

struct AggregateAssign : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

// The five shapes COLLECT admits: a group key, alone or with INTO, or with
// INTO...KEEP, or with WITH COUNT INTO, an AGGREGATE form with an optional
// leading key, or COLLECT WITH COUNT INTO alone. Unused fields stay at
// their default for whichever shape matched.
struct Collect : cursiv::Extends<cursiv::AstNode> {
  Expr* expr_;
  Expr* into;
  string_view intoVar;
  string_view keepVar;
  boolean withCount;
  AggregateAssign* aggregate;
  Options* options;
};

// `WINDOW (expr WITH)? object_literal aggregate_assign`.
struct Window : cursiv::Extends<cursiv::AstNode> {
  Expr* expr_;
  ObjectLit* spec;
  AggregateAssign* aggregate;
};

struct ForOp : cursiv::Extends<cursiv::AstNode> {
  ForIn* forIn;
  vector<std::variant<Collect*, Filter*, Sort*, Limit*, Window*>>
      operations;
};

struct ReturnExpr : cursiv::Extends<cursiv::AstNode> {
  LetList* letBefore;
  boolean distinct;
  Expr* value;
};

struct SimpleQuery : cursiv::Extends<Query> {
  vector<Collection*> with_;
  vector<ForOp*> forOps;
  ReturnExpr* return_;
};

// `FOR v[, e[, p]] IN [from..to] OUTBOUND|INBOUND|ANY start (GRAPH g |
// collections) [PRUNE p] filter* options? return?`.
struct TraversalQuery : cursiv::Extends<Query> {
  vector<Collection*> with_;
  string_view v;
  string_view e;
  string_view p;
  optional<float64> rangeFrom;
  optional<float64> rangeTo;
  string_view direction;  // OUTBOUND | INBOUND | ANY, as written
  Expr* target;
  Expr* graph;                     // GRAPH form
  vector<Collection*> collections;  // collection-list form
  Expr* prune;
  vector<Filter*> filters;
  Options* options;
  ReturnExpr* return_;
};

struct InsertOp : cursiv::Extends<cursiv::AstNode> {
  ObjectLit* doc;
  boolean isInto;  // IN vs INTO
  Collection* target;
  Options* options;
};

struct UpdateOp : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  ObjectLit* doc;
  Collection* target;
  Options* options;
};

struct ReplaceOp : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  ObjectLit* doc;
  Collection* target;
  Options* options;
};

struct RemoveOp : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Collection* target;
  Options* options;
};

struct UpsertOp : cursiv::Extends<cursiv::AstNode> {
  Expr* search;
  Expr* insert_;
  boolean isUpdate;  // UPDATE vs REPLACE
  Expr* modify;
  Collection* target;
  Options* options;
};

// Exactly one of insert_/update_/replace_/removes/upsert_ is populated: an
// ordered choice of the five write forms.
struct DataModificationQuery : cursiv::Extends<Query> {
  vector<Collection*> with_;
  vector<ForOp*> forOps;
  InsertOp* insert_;
  UpdateOp* update_;
  ReplaceOp* replace_;
  vector<RemoveOp*> removes;
  UpsertOp* upsert_;
  ReturnExpr* return_;
};

}  // namespace aql::ast
