#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace dot::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct AttrPair : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> value;
};

struct AttrList : cursiv::Extends<cursiv::AstNode> {
  vector<AttrPair*> pairs;
};

struct NodeId : cursiv::Extends<cursiv::AstNode> {
  string_view id;
  optional<string_view> portName;
  optional<string_view> compass;
};

struct Subgraph : cursiv::Extends<Stmt> {
  optional<string_view> id;
  vector<Stmt*> statements;
};

struct NodeGroup : cursiv::Extends<cursiv::AstNode> {
  vector<NodeId*> nodes;
};

using Endpoint = std::variant<NodeGroup*, Subgraph*>;

enum class EdgeOp : std::uint8_t { directed, undirected };

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Endpoint> endpoints;
  vector<EdgeOp> ops;
  AttrList* attrs;
};

enum class AttrTarget : std::uint8_t { graphAttr, nodeAttr, edgeAttr };

struct AttrStmt : cursiv::Extends<Stmt> {
  AttrTarget target;
  optional<string_view> macroName;
  AttrList* attrs;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  string_view name;
  string_view value;
};

struct Graph : cursiv::Extends<cursiv::AstNode> {
  boolean strict_;
  boolean directed;
  optional<string_view> id;
  vector<Stmt*> statements;
};

}  // namespace dot::ast
