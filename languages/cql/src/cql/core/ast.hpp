// The CQL (Z39.50 Contextual Query Language) AST: a boolean tree of search
// clauses, an optional sort spec, and the leading `>` prefix assignments.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cql::ast {

struct CqlQuery;

// `/name` or `/name=value` on a relation, a boolean group or a sort index.
struct Modifier : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view comparitor;  // empty when the modifier carries no value
  string_view value;
};

// `index comparitor` on a search clause, e.g. `title any`, `date >=`.
struct Relation : cursiv::Extends<cursiv::AstNode> {
  string_view comparitor;
  vector<Modifier*> modifiers;
};

// Base of every node a boolean tree combines.
struct Clause : cursiv::Extends<cursiv::AstNode> {};

// `index relation term`, or a bare `term` (index and relation then empty).
struct SearchClause : cursiv::Extends<Clause> {
  string_view index;
  Relation* relation;
  string_view term;
};

// `( cqlQuery )`.
struct GroupClause : cursiv::Extends<Clause> {
  CqlQuery* query;
};

// `left AND|OR|NOT|PROX right`, the operator's own modifiers kept with it.
struct BooleanGroup : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  vector<Modifier*> modifiers;
};

struct BooleanExpr : cursiv::Extends<Clause> {
  Clause* left;
  BooleanGroup* group;
  Clause* right;
};

// `> prefix = uri`, or the bare `> uri` (prefix then empty).
struct PrefixAssignment : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  string_view uri;
};

// One `index [modifierList]` of a `sortBy` clause.
struct SortIndex : cursiv::Extends<cursiv::AstNode> {
  string_view index;
  vector<Modifier*> modifiers;
};

// `prefixAssignment* scopedClause`, the shape parenthesised sub-queries take.
struct CqlQuery : cursiv::Extends<cursiv::AstNode> {
  vector<PrefixAssignment*> prefixes;
  Clause* clause;
};

// The document root: a CqlQuery plus the sort spec only the top level takes.
struct Cql : cursiv::Extends<cursiv::AstNode> {
  vector<PrefixAssignment*> prefixes;
  Clause* clause;
  vector<SortIndex*> sortSpec;
};

}  // namespace cql::ast
