#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace mps::ast {
struct RowDecl : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
};
struct Entry : cursiv::Extends<cursiv::AstNode> {
  string_view row;
  string_view value;
};
struct ColumnItem : cursiv::Extends<cursiv::AstNode> {};
struct ColumnCard : cursiv::Extends<ColumnItem> {
  string_view name;
  vector<Entry*> entries;
};
struct Marker : cursiv::Extends<ColumnItem> {
  string_view name;
  string_view kind;
};
struct DataCard : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> fields;
};
struct BoundCard : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  vector<string_view> fields;
};
struct QuadraticSection : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  optional<string_view> row;
  vector<ColumnCard*> cards;
};
struct Model : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean free;
  vector<string_view> objectiveSenses;
  vector<RowDecl*> rows;
  vector<ColumnItem*> columns;
  vector<DataCard*> rhs;
  vector<DataCard*> ranges;
  vector<BoundCard*> bounds;
  vector<QuadraticSection*> quadratic;
};
}  // namespace mps::ast
