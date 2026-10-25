// JSON5 values keep numbers as positioned text nodes, preserving arbitrary
// precision and malformed spellings for runtime diagnostics.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace json5::ast {

struct Value : cursiv::Extends<cursiv::AstNode> {};

struct Member;

struct Number : cursiv::Extends<Value> {
  string_view text;
};

// Bool, null and string values are inline; structured and numeric values
// use nodes so their exact source syntax remains available.
using Val = std::variant<std::monostate, bool, std::string_view, Value *>;

struct Object : cursiv::Extends<Value> {
  vector<Member *> members;
};

struct Array : cursiv::Extends<Value> {
  vector<Val> items;
};

struct Member : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Val value;
};

// The entry rule is exempt from the "matches at least one token" rule, so
// an empty document is legal and `value` stays unset rather than
// defaulting to Null.
struct Doc : cursiv::Extends<cursiv::AstNode> {
  optional<Val> value;
};

} // namespace json5::ast
