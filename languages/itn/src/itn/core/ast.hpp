// The itn language: its AST. An itinerary is a list of waypoints, each a
// pair of coordinates, a description and a flag.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace itn::ast {

struct Line : cursiv::Extends<cursiv::AstNode> {
  int64 longitude;
  int64 latitude;
  string_view descr;
  string_view flag;
};

struct Itinerary : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  vector<Line*> lines;
};

}  // namespace itn::ast
