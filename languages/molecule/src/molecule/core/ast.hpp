// The molecule language: its AST -- a formula tree of nested groups
// and coordination entities around elements, each with an optional count.
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

// A leading coefficient applies to the entire dot-separated component.
struct Part : cursiv::Extends<cursiv::AstNode> {
  optional<uint32> multiplier;
  vector<Structure*> structures;
};

struct Molecule : cursiv::Extends<cursiv::AstNode> {
  vector<Part*> parts;
};

}  // namespace molecule::ast
