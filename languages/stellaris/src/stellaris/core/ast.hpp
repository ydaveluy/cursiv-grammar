#pragma once

#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace stellaris::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Value : cursiv::Extends<Item> {};

struct Scalar : cursiv::Extends<Value> {
  string_view text;
  boolean quoted;
};

struct Group;
struct KeyValue;

struct KeyValue : cursiv::Extends<Item> {
  Scalar* key;
  string_view op;
  Value* value;
};

struct Group : cursiv::Extends<Value> {
  Scalar* header;
  vector<Item*> items;
  boolean closed;
};

struct ParameterDefinition : cursiv::Extends<Item> {
  string_view name;
  boolean undefined;
  Value* value;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
  boolean bom;
};

}  // namespace stellaris::ast
