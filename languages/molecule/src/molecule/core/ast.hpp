// The molecule language: its AST -- a Hill-formula tree of nested groups
// and ions around elements, each with an optional count.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace molecule::ast {

// The three shapes a structure's symbol may take.
struct Symbol : cursiv::Extends<cursiv::AstNode> {};

struct Element : cursiv::Extends<Symbol> {
  string_view name;
};

struct Structure;

struct Group : cursiv::Extends<Symbol> {
  vector<Structure*> structures;
};

struct Ion : cursiv::Extends<Symbol> {
  vector<Structure*> structures;
};

struct Structure : cursiv::Extends<cursiv::AstNode> {
  Symbol* symbol = nullptr;
  optional<uint32> count;
};

// The grammar's `count? structure`: a multiplier in front of one structure
// (the leading 2 of "2H2O"), distinct from Structure::count, which follows.
struct Term : cursiv::Extends<cursiv::AstNode> {
  optional<uint32> multiplier;
  Structure* structure = nullptr;
};

struct Part : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> terms;
};

struct Molecule : cursiv::Extends<cursiv::AstNode> {
  vector<Part*> parts;
};

}  // namespace molecule::ast
