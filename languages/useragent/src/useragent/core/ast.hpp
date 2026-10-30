// The useragent language: its AST -- a document of product/version tokens,
// each followed by its ordered parenthesized comments.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace useragent::ast {

struct Product : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> version;
  vector<string_view> comments;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Product*> products;
};

}  // namespace useragent::ast
