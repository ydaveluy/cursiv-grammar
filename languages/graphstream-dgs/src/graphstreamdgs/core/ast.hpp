// The graphstreamdgs language: its AST. A DGS document is a header
// (magic and an ignored header line) followed by a stream of graph
// events; scalar attribute values are a variant, arrays/maps are nodes.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

// Namespace scope, so the AstNode vocabulary aliases are out of reach here:
// these two use the std spelling, like the struct members that hold them.
namespace graphstreamdgs::ast {

using Ident = std::string_view;

struct Array;
struct Map;

using Value = std::variant<std::string_view, std::int64_t, double,
                           std::uint32_t, bool, Array*, Map*>;

enum class AttrOp : std::uint8_t { Set, Add, Remove };
enum class Direction : std::uint8_t { Forward, Backward };

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  AttrOp op = AttrOp::Set;
  Ident name;
  vector<Value> values;
};

struct Mapping : cursiv::Extends<cursiv::AstNode> {
  Ident key;
  Value value = true;
};

struct Array : cursiv::Extends<cursiv::AstNode> {
  vector<Value> items;
};

struct Map : cursiv::Extends<cursiv::AstNode> {
  vector<Mapping*> entries;
};

// The base of every DGS event -- `an`, `cn`, `dn`, `ae`, `ce`, `de`, `cg`,
// `st`, `cl` -- so `Document::events` holds the stream in source order.
struct Event : cursiv::Extends<cursiv::AstNode> {};

struct AddNode : cursiv::Extends<Event> {
  Ident id;
  vector<Attribute*> attributes;
};

struct ChangeNode : cursiv::Extends<Event> {
  Ident id;
  vector<Attribute*> attributes;
};

struct DelNode : cursiv::Extends<Event> {
  Ident id;
};

struct AddEdge : cursiv::Extends<Event> {
  Ident id;
  Ident source;
  optional<Direction> direction;
  Ident target;
  vector<Attribute*> attributes;
};

struct ChangeEdge : cursiv::Extends<Event> {
  Ident id;
  vector<Attribute*> attributes;
};

struct DelEdge : cursiv::Extends<Event> {
  Ident id;
};

struct ChangeGraph : cursiv::Extends<Event> {
  vector<Attribute*> attributes;
};

struct Step : cursiv::Extends<Event> {
  float64 time;
};

struct Clear : cursiv::Extends<Event> {};

struct Document : cursiv::Extends<cursiv::AstNode> {
  string_view magic;
  string_view header;
  vector<Event*> events;
};

}  // namespace graphstreamdgs::ast
