// The json AST: variant-shaped. A scalar lives inline in the member that
// holds it; only the shapes that structure the document -- Object, Array,
// Member, Doc -- are real AstNode arena nodes.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace json::ast {

// A container node: Object or Array. Scalars are not nodes.
struct Value : cursiv::Extends<cursiv::AstNode> {};

struct Member;

// `bool` and `double` sit side by side though each converts to the other --
// the builder matches the exact result type first, falling back to
// constructibility only when nothing matches exactly.
using Val =
    // Namespace scope, so the AstNode vocabulary (`string_view`, `vector`,
    // `reference`) is out of reach here and the std spelling is the only one.
    std::variant<std::monostate, bool, double, std::string_view, Value*>;

struct Object : cursiv::Extends<Value> {
  vector<Member*> members;
};

struct Array : cursiv::Extends<Value> {
  vector<Val> items;
};

// A member is a node of its own -- it carries the name, and its CST view is
// what an editor asks for when hovering `"name": ...`.
struct Member : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Val value;
};

// The entry: a JSON text is one value, scalars included -- so the root node
// is the document, carrying the variant.
struct Doc : cursiv::Extends<cursiv::AstNode> {
  Val value;
};

}  // namespace json::ast
