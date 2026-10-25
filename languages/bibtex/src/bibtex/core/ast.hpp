#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bibtex::ast {
struct ValuePart : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
struct BracedPart : cursiv::Extends<ValuePart> {};
struct QuotedPart : cursiv::Extends<ValuePart> {};
struct NumberPart : cursiv::Extends<ValuePart> {};
struct MacroPart : cursiv::Extends<ValuePart> {};
struct Value : cursiv::Extends<cursiv::AstNode> {
  vector<ValuePart*> parts;
};
struct Field : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  Value* value = nullptr;
};
struct Declaration : cursiv::Extends<cursiv::AstNode> {};
struct Entry : cursiv::Extends<Declaration> {
  string_view type;
  string_view citekey;
  string_view open, close;
  boolean hasKeySeparator = false;
  vector<Field*> fields;
};
struct StringDef : cursiv::Extends<Declaration> {
  string_view open, close;
  vector<Field*> fields;
};
struct Preamble : cursiv::Extends<Declaration> {
  string_view open, close;
  Value* value = nullptr;
};
struct Comment : cursiv::Extends<Declaration> {
  string_view raw;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration*> declarations;
};
}  // namespace bibtex::ast
