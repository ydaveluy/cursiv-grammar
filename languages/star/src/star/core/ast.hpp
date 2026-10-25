#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace star::ast {
struct Value : cursiv::Extends<cursiv::AstNode> {};
struct StringLit : cursiv::Extends<Value> {
  string_view raw;
};
struct Literal : cursiv::Extends<Value> {
  string_view text;
};
struct Stop : cursiv::Extends<Value> {};
struct List : cursiv::Extends<Value> {
  vector<Value *> values;
};
struct TableEntry : cursiv::Extends<cursiv::AstNode> {
  StringLit *key = nullptr;
  Value *value = nullptr;
};
struct Table : cursiv::Extends<Value> {
  vector<TableEntry *> entries;
};
struct LoopHeader : cursiv::Extends<cursiv::AstNode> {
  vector<variant<string_view, LoopHeader *>> items;
  bool closed = false;
};
struct Loop : cursiv::Extends<Value> {
  LoopHeader *header = nullptr;
  vector<Value *> values;
};
struct KeyVal : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Value *value = nullptr;
};
struct SaveFrame;
using Element = std::variant<KeyVal *, SaveFrame *, Loop *>;
struct Content : cursiv::Extends<cursiv::AstNode> {
  vector<Element> elements;
};
struct SaveFrame : cursiv::Extends<Content> {
  string_view name;
  bool closed = false;
};
struct DataBlock : cursiv::Extends<Content> {
  string_view name;
};
struct Global : cursiv::Extends<Content> {};
struct Doc : cursiv::Extends<cursiv::AstNode> {
  vector<variant<DataBlock *, Global *>> blocks;
};
} // namespace star::ast
