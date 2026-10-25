// The fol language: its AST -- first-order logic formulas over terms
// (variables, individual constants, function applications).
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fol::ast {

struct Term : cursiv::Extends<cursiv::AstNode> {};

struct Variable : cursiv::Extends<Term> {
  string_view name;
};

struct IndConstant : cursiv::Extends<Term> {
  string_view name;
};

struct FuncCall : cursiv::Extends<Term> {
  string_view func;
  vector<Term*> args;
};

// The connective, not the text that denotes it: each operator token hands
// its constant over, so no reader downstream compares spellings.
enum class BinOp : std::uint8_t {
  conj,
  disj,
  impl,
  bicond,
};

struct Formula : cursiv::Extends<cursiv::AstNode> {};

struct BinaryFormula : cursiv::Extends<Formula> {
  Formula* left;
  BinOp op;
  Formula* right;
};

struct Negation : cursiv::Extends<Formula> {
  Formula* formula;
};

struct ForallFormula : cursiv::Extends<Formula> {
  Variable* variable;
  Formula* formula;
};

struct ExistsFormula : cursiv::Extends<Formula> {
  Variable* variable;
  Formula* formula;
};

struct PredicateAtom : cursiv::Extends<Formula> {
  string_view pred;
  vector<Term*> args;
};

struct EqualityAtom : cursiv::Extends<Formula> {
  Term* left;
  Term* right;
};

struct Condition : cursiv::Extends<cursiv::AstNode> {
  vector<Formula*> formulas;
};

}  // namespace fol::ast
