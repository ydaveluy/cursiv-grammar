#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace blueprint::ast {

struct Variable;
struct IntegerLit;
struct StringLit;
struct Select;
struct ListValue;
struct MapValue;

using Val = std::variant<bool, IntegerLit *, StringLit *, Variable *, Select *,
                         ListValue *, MapValue *>;

// A chain of values joined by `+` is a flat list of terms, not a
// right-recursive tree.
struct Expr : cursiv::Extends<cursiv::AstNode> {
  vector<Val> terms;
};

struct IntegerLit : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};

struct StringLit : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};

struct Variable : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct ListValue : cursiv::Extends<cursiv::AstNode> {
  vector<Expr *> items;
  bool trailingComma = false;
};

// One entry of a propertyList -- shared by a module's body and a mapValue.
struct Property : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value = nullptr;
  bool equals = false;
};

struct MapValue : cursiv::Extends<cursiv::AstNode> {
  vector<Property *> properties;
  bool trailingComma = false;
};

struct Condition : cursiv::Extends<cursiv::AstNode> {
  string_view function;
  vector<StringLit *> args;
  bool trailingComma = false;
};

// `any [@ binding]`; binding is empty when written without one.
struct AnyPattern : cursiv::Extends<cursiv::AstNode> {
  string_view binding;
};

struct DefaultPattern : cursiv::Extends<cursiv::AstNode> {};

using Pattern = std::variant<AnyPattern *, DefaultPattern *, bool, IntegerLit *,
                             StringLit *>;

// `value` is null on the `unset` branch: absence already fits the pointer.
struct SelectCase : cursiv::Extends<cursiv::AstNode> {
  vector<Pattern> patterns;
  bool tuple = false;
  bool comma = false;
  Expr *value = nullptr;
};

struct Select : cursiv::Extends<cursiv::AstNode> {
  vector<Condition *> conditions;
  bool tuple = false;
  vector<SelectCase *> cases;
};

struct Assignment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool append = false; // `+=` instead of `=`
  Expr *value = nullptr;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  vector<Property *> properties;
  bool parenthesized = false;
  bool trailingComma = false;
};

using Definition = std::variant<Assignment *, Module *>;

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Definition> definitions;
  bool bom = false;
};

} // namespace blueprint::ast
