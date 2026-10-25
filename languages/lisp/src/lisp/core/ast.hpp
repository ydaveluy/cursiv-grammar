#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lisp::ast {
struct SExpr : cursiv::Extends<cursiv::AstNode> {};
using Value = SExpr *;
struct Symbol : cursiv::Extends<SExpr> {
  string_view text;
};
struct Number : cursiv::Extends<SExpr> {
  string_view text;
};
struct String : cursiv::Extends<SExpr> {
  string_view text;
};
struct Character : cursiv::Extends<SExpr> {
  string_view text;
};
struct BitVector : cursiv::Extends<SExpr> {
  string_view text;
};
struct GraphRef : cursiv::Extends<SExpr> {
  string_view text;
};
struct List : cursiv::Extends<SExpr> {
  string_view opener;
  vector<Value> items;
  Value tail;
};
struct Prefix : cursiv::Extends<SExpr> {
  string_view marker;
  Value value;
};
struct Conditional : cursiv::Extends<SExpr> {
  string_view marker;
  Value feature;
  Value value;
};
struct Doc : cursiv::Extends<cursiv::AstNode> {
  vector<Value> expressions;
};
} // namespace lisp::ast
