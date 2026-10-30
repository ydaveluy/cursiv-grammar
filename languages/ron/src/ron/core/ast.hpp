// The ron language: its AST. A value carries an optional leading type name
// (classname) and a payload; CHAR and a bare identifier get their own node so
// they don't collide with STRING's string_view inside the payload variant.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ron::ast {

struct Number;
struct Range;
struct Bytes;
struct Tuple;
struct ListV;
struct StructLit;
struct MapLit;
struct CharLit;
struct Ident;

using Val = std::variant<Number *, Range *, Bytes *, bool, std::string_view, CharLit *,
                         Ident *, Tuple *, ListV *, StructLit *, MapLit *>;

struct Value : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> classname;
  Val payload;
};

struct Number : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Range : cursiv::Extends<cursiv::AstNode> {
  Number *start = nullptr;
  Number *end = nullptr;
  bool inclusive = false;
};

struct Bytes : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct CharLit : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Ident : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct Tuple : cursiv::Extends<cursiv::AstNode> {
  vector<Value *> items;
};

struct ListV : cursiv::Extends<cursiv::AstNode> {
  vector<Value *> items;
};

struct StructItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Value *value;
};

struct StructLit : cursiv::Extends<cursiv::AstNode> {
  vector<StructItem *> items;
};

struct MapItem : cursiv::Extends<cursiv::AstNode> {
  Value *key;
  Value *value;
};

struct MapLit : cursiv::Extends<cursiv::AstNode> {
  vector<MapItem *> items;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> attributes;
  vector<Value *> values;
};

} // namespace ron::ast
