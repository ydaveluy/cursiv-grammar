// The datalog language: its AST -- facts, rules, retractions and queries
// over predicates applied to variables and constants.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace datalog::ast {

struct Term : cursiv::Extends<cursiv::AstNode> {};

struct Variable : cursiv::Extends<Term> {
  string_view name;
};

struct AtomConstant : cursiv::Extends<Term> {
  string_view name;
};

struct StringConstant : cursiv::Extends<Term> {
  string_view value;
};

struct IntConstant : cursiv::Extends<Term> {
  string_view value;
};

struct BoolConstant : cursiv::Extends<Term> {
  boolean value;
};

struct Literal : cursiv::Extends<cursiv::AstNode> {};

// `edge(a, b)`, `edge()`, or `edge` bare -- the three shapes of a call
// collapse to one node, arity told by `args.size()`.
struct PredicateAtom : cursiv::Extends<Literal> {
  string_view predicate;
  boolean quoted;
  vector<Term*> args;
};

struct EqualityLiteral : cursiv::Extends<Literal> {
  Term* left;
  boolean negated;  // `!=` instead of `=`
  Term* right;
};

// `X :- add(A, B)`: a variable bound to what an external predicate answers.
struct ExternalLiteral : cursiv::Extends<Literal> {
  Variable* variable;
  string_view external;
  vector<Term*> args;
};

struct Clause : cursiv::Extends<cursiv::AstNode> {
  Literal* head;
  vector<Literal*> body;  // empty for a fact, one entry per `:-` conjunct
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct Assertion : cursiv::Extends<Statement> {
  Clause* clause;
};

struct Retraction : cursiv::Extends<Statement> {
  Clause* clause;
};

struct Query : cursiv::Extends<Statement> {
  Literal* literal;
};

struct Requirement : cursiv::Extends<Statement> {
  string_view name;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace datalog::ast
