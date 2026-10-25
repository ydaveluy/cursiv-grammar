// The edn language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace edn::ast {

// Every value that is a real arena node: a collection, or a leaf whose text
// a tool wants without decoding a variant. nil, a boolean and a string are
// scalar enough to sit inline in Val instead.
struct Value : cursiv::Extends<cursiv::AstNode> {};

using Val = std::variant<std::monostate, bool, std::string_view, Value *>;

// Raw text, N/M suffix included: an integer or a bigdecimal both overflow
// int64/double, so the digits are kept as written and never converted.
struct Number : cursiv::Extends<Value> {
  string_view text;
};

// The body after the backslash: a name (`newline`), a `u`-escape, or one byte.
struct Character : cursiv::Extends<Value> {
  string_view text;
};

struct Symbol : cursiv::Extends<Value> {
  string_view name;
};

// A tagged value keeps both its tag and its payload.
struct Discard;

struct Tag : cursiv::Extends<Value> {
  vector<Discard *> discards;
  string_view name;
  Val value;
};

struct Discard : cursiv::Extends<Value> {
  vector<Discard *> discards;
  Val value;
};

struct Keyword : cursiv::Extends<Value> {
  string_view name;
};

struct List : cursiv::Extends<Value> {
  vector<Val> items;
};

struct Vector : cursiv::Extends<Value> {
  vector<Val> items;
};

struct Set : cursiv::Extends<Value> {
  vector<Val> items;
};

// A map pair is a node of its own so a key and its value are one CST span.
struct Entry : cursiv::Extends<cursiv::AstNode> {
  vector<Discard *> discards;
  Val key;
  Val value;
};

struct Map : cursiv::Extends<Value> {
  optional<string_view> namespace_;
  vector<Discard *> discards;
  vector<Entry *> entries;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Val> values;
};

} // namespace edn::ast
