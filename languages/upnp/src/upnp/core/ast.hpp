// The upnp language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace upnp::ast {

enum class BinOp : std::uint8_t {
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  contains,
  does_not_contain,
  derived_from,
};

enum class LogOp : std::uint8_t {
  and_,
  or_,
};

struct SearchExpr : cursiv::Extends<cursiv::AstNode> {};

struct BinaryExpr : cursiv::Extends<SearchExpr> {
  SearchExpr* left = nullptr;
  LogOp op = LogOp::and_;
  SearchExpr* right = nullptr;
};

// The property name plus its test, shared by the two shapes a relExp
// spells: a comparison against a quoted value, or an existence test.
struct RelExpr : cursiv::Extends<SearchExpr> {
  string_view property;
};

struct Comparison : cursiv::Extends<RelExpr> {
  BinOp op = BinOp::eq;
  string_view raw;
};

struct Exists : cursiv::Extends<RelExpr> {
  boolean value = false;
};

struct SearchCrit : cursiv::Extends<cursiv::AstNode> {
  SearchExpr* expr = nullptr;  // null when wildcard
  boolean wildcard = false;
};

}  // namespace upnp::ast
