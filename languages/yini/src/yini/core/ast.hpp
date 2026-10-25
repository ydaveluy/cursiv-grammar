#pragma once

#include <variant>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace yini::ast {

struct Value : cursiv::Extends<cursiv::AstNode> {};
using Val = std::variant<std::monostate, bool, Value*>;
struct Number : cursiv::Extends<Value> { string_view literal; };
struct String : cursiv::Extends<Value> { string_view literal; };
struct Concat : cursiv::Extends<Value> { Val first; vector<Val> rest; };
struct List : cursiv::Extends<Value> { vector<Val> items; bool trailingComma; };
struct ObjectMember : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view separator;
  Val value;
};
struct Object : cursiv::Extends<Value> { vector<ObjectMember*> members; bool trailingComma; };

struct Entry : cursiv::Extends<cursiv::AstNode> {};
struct Section : cursiv::Extends<Entry> { string_view markerText; string_view name; };
struct Member : cursiv::Extends<Entry> {
  string_view key;
  bool colon;
  bool hasValue;
  Val value;
};
struct Directive : cursiv::Extends<Entry> { string_view tag; vector<string_view> arguments; };
struct Comment : cursiv::Extends<Entry> { string_view literal; };
struct Shebang : cursiv::Extends<Entry> { string_view literal; };
struct End : cursiv::Extends<Entry> { string_view literal; };
struct Document : cursiv::Extends<cursiv::AstNode> { bool bom; vector<Entry*> entries; };

}  // namespace yini::ast
