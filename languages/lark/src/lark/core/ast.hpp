#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lark::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Value : cursiv::Extends<cursiv::AstNode> {};
struct LiteralRange : cursiv::Extends<Value> {
  string_view lo;
  string_view hi;
};
struct NameRef : cursiv::Extends<Value> {
  string_view name;
};
struct Literal : cursiv::Extends<Value> {
  string_view text;
};
struct TemplateUsage : cursiv::Extends<Value> {
  string_view name;
  vector<Value*> args;
};
struct Expansions;
struct Atom : cursiv::Extends<cursiv::AstNode> {
  boolean maybe;
  variant<Expansions*, Value*> content;
};
struct Expr : cursiv::Extends<cursiv::AstNode> {
  Atom* atom;
  string_view op;
  optional<string_view> repeatMin;
  optional<string_view> repeatMax;
};
struct Expansion : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> items;
};
struct Alias : cursiv::Extends<cursiv::AstNode> {
  Expansion* expansion;
  string_view label;
};
struct Expansions : cursiv::Extends<cursiv::AstNode> {
  vector<Alias*> alternatives;
};
struct Declaration : cursiv::Extends<Item> {
  string_view name;
  string_view modifiers;
  vector<string_view> params;
  optional<string_view> priority;
  Expansions* body;
  boolean terminal;
};
struct IgnoreStmt : cursiv::Extends<Item> {
  Expansions* body;
};
struct ImportStmt : cursiv::Extends<Item> {
  boolean relative;
  vector<string_view> path;
  string_view alias;
  vector<string_view> names;
};
struct OverrideStmt : cursiv::Extends<Item> {
  Declaration* declaration;
  boolean extend;
};
struct DeclareStmt : cursiv::Extends<Item> {
  vector<string_view> names;
};
struct Start : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace lark::ast
