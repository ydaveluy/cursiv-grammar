// The elixir language: its AST. `Expr` is the family every expression
// belongs to; do/def/case/... forms are expressions too, matching Elixir's
// own value-oriented design.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace elixir::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A sequence of expressions: the file itself, and any `do ... end` body.
struct Block : cursiv::Extends<Expr> {
  vector<Expr*> body;
};

struct BlockSection : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Block* body;
};
struct DoBlock : cursiv::Extends<cursiv::AstNode> {
  Block* body;
  vector<BlockSection*> sections;
};
// Literals. Numeric text is kept raw (digit separators, arbitrary
// precision) rather than converted to a fixed-width value.
struct IntegerLit : cursiv::Extends<Expr> {
  string_view text;
};
struct HexLit : cursiv::Extends<Expr> {
  string_view text;
};
struct OctalLit : cursiv::Extends<Expr> {
  string_view text;
};
struct BinLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
// Literal pieces retain their source spelling.
struct StringLit : cursiv::Extends<Expr> {
  // n interpolations have n+1 text pieces, including empty edge pieces.
  vector<string_view> texts;
  vector<Expr *> holes;
};
struct MultiStringLit : cursiv::Extends<Expr> {
  // n interpolations have n+1 text pieces, including empty edge pieces.
  vector<string_view> texts;
  vector<Expr *> holes;
};
struct CharlistLit : cursiv::Extends<Expr> {
  // n interpolations have n+1 text pieces, including empty edge pieces.
  vector<string_view> texts;
  vector<Expr *> holes;
};
struct MultiCharlistLit : cursiv::Extends<Expr> {
  // n interpolations have n+1 text pieces, including empty edge pieces.
  vector<string_view> texts;
  vector<Expr *> holes;
};
struct QuotedAtomLit : cursiv::Extends<Expr> {
  vector<string_view> texts;
  vector<Expr*> holes;
};
struct CodepointLit : cursiv::Extends<Expr> {
  string_view text;
};
struct AtomLit : cursiv::Extends<Expr> {
  string_view text;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NilLit : cursiv::Extends<Expr> {};
struct AliasLit : cursiv::Extends<Expr> {
  string_view name;
};
struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};
struct OperatorLit : cursiv::Extends<Expr> {
  string_view text;
};
struct SigilLit : cursiv::Extends<Expr> {
  string_view name;
  string_view delimiter;
  string_view modifiers;
  vector<string_view> texts;
  vector<Expr*> holes;
};

// A keyword entry: `key: value`, everywhere a call's trailing options, a
// map's short entries or a construct's `key: value` clause is spelled.
struct OptionEntry : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  QuotedAtomLit* quotedKey;
  Expr* value;
};

struct NestedExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

struct AtExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<OptionEntry*> options;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct AmpExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* index;
};

struct DotExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* member;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  boolean noParens;
  boolean dotCall;  // `callee.(args)` rather than `callee(args)`
  vector<Expr*> args;
  vector<OptionEntry*> options;
};

enum class Operator : std::uint8_t {
  multiply,
  divide,
  add,
  subtract,
  concat,
  list_sub,
  concat3,
  list_sub3,
  range,
  diamond,
  in_,
  not_in,
  pipe_op,
  ltlt_tilde,
  tilde_gtgt,
  lt_tilde,
  tilde_gt,
  lt_tilde_gt,
  lt_pipe_gt,
  lt3,
  gt3,
  less,
  greater,
  less_eq,
  greater_eq,
  equal,
  not_equal,
  match_op,
  strict_equal,
  strict_not_equal,
  and_and,
  and_and_and,
  and_kw,
  or_or,
  or_or_or,
  or_kw,
  pattern_match,
  prepend,
  type_op,
  when_op,
  default_value,
  rarrow,
  larrow,
  power,
  range_step,
  xor_,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
  Expr* tail;  // the `| tail` of `[head | tail]`, or null
  vector<OptionEntry*> keywords;
};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};

struct MapExpr : cursiv::Extends<Expr> {
  Expr* structType;
  Expr* base;              // the `m` of `%{m | k: v}`, or null
  vector<MapEntry*> entries;
  vector<OptionEntry*> keywords;
};

struct BitstringExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ClauseExpr : cursiv::Extends<Expr> {
  vector<Expr*> params;
  Expr* guard;
  Block* body;
};
struct BlockCallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  DoBlock* block;
};

struct AnonFnClause : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> params;
  Expr* guard;
  Block* body;
};

struct AnonFn : cursiv::Extends<Expr> {
  vector<AnonFnClause*> clauses;
};

}  // namespace elixir::ast
