#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace ply::ast {
struct Header : cursiv::Extends<cursiv::AstNode> {};
struct Format : cursiv::Extends<Header> {
  string_view encoding;
  string_view version;
  string_view extra;
};
struct Element : cursiv::Extends<Header> {
  string_view name;
  string_view count;
  string_view extra;
};
struct Property : cursiv::Extends<Header> {
  bool list = false;
  string_view countType;
  string_view type;
  string_view name;
  string_view extra;
};
struct Note : cursiv::Extends<Header> {
  string_view kind;
  string_view text;
};
struct Unknown : cursiv::Extends<Header> {
  string_view keyword;
  string_view text;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  string_view magic;
  vector<Header *> header;
  string_view endHeader;
  string_view body;
};
} // namespace ply::ast
