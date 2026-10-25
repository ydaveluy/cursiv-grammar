#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace icalendar::ast {

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view rawValue;
};

struct Property : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Param*> params;
  string_view value;
  bool hasValueDelimiter = false;
};

struct Component : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view closeName;
  vector<Param*> params;
  vector<Param*> closeParams;
  vector<Property*> properties;
  vector<Component*> subcomponents;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Component*> components;
  vector<Property*> properties;
};

}  // namespace icalendar::ast
