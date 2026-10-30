#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace geekcode::ast {

enum class Qualifier : std::uint8_t {
  not_rigid,
  wannabe,
  for_living,
  no_knowledge,
  refused,
};

struct Crossover : cursiv::Extends<cursiv::AstNode> {
  string_view run;
};
using Modifier = std::variant<std::string_view, Crossover*, Qualifier>;

struct ShapeSegment : cursiv::Extends<cursiv::AstNode> {
  vector<Modifier> modifiers;
};
struct Shape : cursiv::Extends<cursiv::AstNode> {
  vector<ShapeSegment*> segments;
};
struct Unix : cursiv::Extends<cursiv::AstNode> {
  string_view variants;
};
using Kind = std::variant<std::string_view, Shape*, Unix*>;

struct Tuple : cursiv::Extends<cursiv::AstNode> {
  boolean refused;
  Kind kind;
  string_view exactAge;  // decimal age after category a; empty for a rating
  vector<Modifier> modifiers;
};
struct File : cursiv::Extends<cursiv::AstNode> {
  string_view header;
  string_view version;
  vector<Tuple*> tuples;
};

}  // namespace geekcode::ast
