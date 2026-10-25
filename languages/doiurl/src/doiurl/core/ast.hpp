// The doiurl language: its AST. One node for the whole URI -- prefix,
// suffix segments, an optional query and an optional fragment.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace doiurl::ast {

struct DoiUri : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  vector<string_view> suffix;
  optional<string_view> query;
  optional<string_view> fragment;
};

}  // namespace doiurl::ast
