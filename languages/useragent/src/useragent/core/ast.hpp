// The useragent language: its AST -- a document of product/version tokens,
// each optionally followed by the parenthesized comment that follows it.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace useragent::ast {

struct Product : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> version;
  optional<string_view> comment;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Product*> products;
};

}  // namespace useragent::ast
