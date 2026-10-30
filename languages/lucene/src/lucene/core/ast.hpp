#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <cstdint>
namespace lucene::ast {
enum class Modifier : std::uint8_t { none, require, prohibit, negate };
enum class Conjunction : std::uint8_t { none, logical_and, logical_or };
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct WordTerm : cursiv::Extends<Expr> {
  string_view raw;
};
struct QuotedTerm : cursiv::Extends<Expr> {
  string_view raw;
};
struct RegexTerm : cursiv::Extends<Expr> {
  string_view raw;
};
struct RangeBound : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
  bool quoted = false;
};
struct RangeTerm : cursiv::Extends<Expr> {
  bool leftInclusive = false;
  bool rightInclusive = false;
  RangeBound *left = nullptr;
  RangeBound *right = nullptr;
};
struct Suffix : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};
struct Term : cursiv::Extends<Expr> {
  Expr *primary = nullptr;
  vector<Suffix *> suffixes;
};
struct Query;
struct GroupingExpr : cursiv::Extends<Expr> {
  Query *query = nullptr;
  string_view minimumShouldMatch;
  vector<Suffix *> suffixes;
};
struct IntervalExpr : cursiv::Extends<Expr> {
  string_view function;
  vector<Expr*> arguments;
};
struct Clause : cursiv::Extends<cursiv::AstNode> {
  Conjunction conjunction = Conjunction::none;
  Modifier modifier = Modifier::none;
  string_view field;
  string_view fieldOperator;
  Expr *value = nullptr;
};
struct Query : cursiv::Extends<cursiv::AstNode> {
  vector<Clause *> clauses;
};
} // namespace lucene::ast
