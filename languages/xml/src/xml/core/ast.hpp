// The xml language: its AST. A document is an optional declaration plus a
// root element; an element is a name, its attributes, and mixed content --
// nested elements and text.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace xml::ast {

struct Element;

// Namespace scope: the AstNode field vocabulary is only in reach inside a
// class deriving it, so this spells std::string_view rather than the alias.
using Content = std::variant<Element*, std::string_view>;

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct Element : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view closeName;  // empty: self-closing, no separate close tag
  vector<Attribute*> attributes;
  vector<Content> children;
};

struct Declaration : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> doctypes;
  Declaration* declaration;  // the leading <?xml ...?>; null when absent
  Element* root;
};

}  // namespace xml::ast
