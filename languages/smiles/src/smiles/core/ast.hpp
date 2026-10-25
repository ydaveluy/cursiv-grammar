// The smiles language: its AST. A chain of bonded atoms, each atom carrying
// its own bracket attributes when it has any.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace smiles::ast {

enum class Bond { Single, Double, Triple, Quad, Aromatic, Up, Down, Dot, DativeRight, DativeLeft, Any };

struct BranchedAtom;

// One class for both atom shapes: an organic-subset atom (`C`, `c`, `*`)
// leaves every optional member unset, a bracket atom sets whichever of its
// own parts were written.
struct Atom : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  boolean bracket;
  optional<string_view> isotope;
  optional<string_view> chiral;
  optional<string_view> hcount;
  optional<string_view> charge;
  optional<string_view> atomClass;
};

struct AtomSuffix : cursiv::Extends<cursiv::AstNode> {};

struct RingBond : cursiv::Extends<AtomSuffix> {
  optional<Bond> bond;
  string_view number;
};

struct Chain;

struct Branch : cursiv::Extends<AtomSuffix> {
  optional<Bond> bond;
  Chain* chain = nullptr;
};

struct BranchedAtom : cursiv::Extends<cursiv::AstNode> {
  Atom* atom = nullptr;
  vector<AtomSuffix*> suffixes;
};

// One link past the first atom of a `Chain`: the bond (or dot) that joins it
// to what came before, and the atom it introduces.
struct ChainLink : cursiv::Extends<cursiv::AstNode> {
  optional<Bond> bond;
  BranchedAtom* atom = nullptr;
};

struct Chain : cursiv::Extends<cursiv::AstNode> {
  BranchedAtom* first = nullptr;
  vector<ChainLink*> rest;
};

struct Smiles : cursiv::Extends<cursiv::AstNode> {
  Chain* chain = nullptr;
  boolean hasTerminator;
};

}  // namespace smiles::ast
