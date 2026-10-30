// The maml language: its AST, variant-shaped like json's -- a scalar lives
// inline in the member that holds it, only the shapes that structure the
// document (Object, Array, Member, Doc) are real AstNode arena nodes.
#pragma once

#include <string_view>
#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace maml::ast {

struct Value : cursiv::Extends<cursiv::AstNode> {};

struct Member;

using Val = std::variant<std::monostate, bool, std::int64_t, double, std::string_view, Value*>;

struct Object : cursiv::Extends<Value> {
  vector<Member*> members;
};

struct Array : cursiv::Extends<Value> {
  vector<Val> items;
};

// A digit-only, keyword-spelled or bare key all read as the same key text: a
// key is never a scalar value, so one string_view covers every spelling.
struct Member : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Val value;
};

struct Doc : cursiv::Extends<cursiv::AstNode> {
  Val value;
};

}  // namespace maml::ast
