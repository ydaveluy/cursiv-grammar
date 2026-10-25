// The quakemap language: its AST. An entity is a bag of key/value strings
// plus the brushes carving its geometry; a brush is one or more planes, each
// a leading list of 3D points, a texture name, then alignment numbers.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace quakemap::ast {

struct KeyValue : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view value;
};

struct Coord : cursiv::Extends<cursiv::AstNode> {
  vector<double> values;
};

struct BrushLine : cursiv::Extends<cursiv::AstNode> {
  vector<Coord*> coords;
  string_view texture;
  vector<double> values;
};

struct Brush : cursiv::Extends<cursiv::AstNode> {
  vector<BrushLine*> lines;
};

struct Entity : cursiv::Extends<cursiv::AstNode> {
  vector<KeyValue*> keyvalues;
  vector<Brush*> brushes;
};

struct Map : cursiv::Extends<cursiv::AstNode> {
  vector<Entity*> entities;
};

}  // namespace quakemap::ast
