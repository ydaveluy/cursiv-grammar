#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "pike/core/ast.hpp"

namespace pike_typedef {
inline bool run(const auto& language) {
  namespace a = pike::ast;
  bool ok = true;
  auto check = [&](bool value, const char* reason, std::string_view name) {
    if (!value) {
      std::fprintf(stderr, "pike typedef: %s; case=%.*s\n", reason,
                   int(name.size()), name.data());
      ok = false;
    }
  };
  struct Case { const char* name; const char* source; bool clean; };
  const Case cases[] = {
      {"int", "typedef int Count;", true},
      {"string", "typedef string Text;", true},
      {"array", "typedef array(int) Counts;", true},
      {"mapping", "typedef mapping(string:int) Lookup;", true},
      {"function", "typedef function(int:int) Callback;", true},
      {"union", "typedef int|string Scalar;", true},
      {"range", "typedef int(0..255) Byte;", true},
      {"public", "public typedef int Count;", true},
      {"modifiers", "private static typedef float Ratio;", true},
      {"class", "class C { typedef int Count; }", true},
      {"nested", "class Outer { class Inner { public typedef array(int) Counts; } }", true},
      {"multiset", "typedef multiset(string) Set;", true},
      {"object", "typedef object Handle;", true},
      {"sequence", "typedef int Count; typedef string Text;", true},
      {"missing-type", "typedef Count;", false},
      {"missing-name", "typedef int;", false},
      {"missing-semicolon", "typedef int Count", false},
      {"initializer", "typedef int Count=1;", false},
      {"comma-names", "typedef int Count, Other;", false},
      {"local", "void f(){ typedef int Count; }", false},
      {"reserved-name", "typedef int if;", false},
      {"missing-keyword", "int typedef Count;", false},
  };
  for (const auto& c : cases) {
    const auto result = language.parser->parse(c.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    check(clean == c.clean, "native typedef syntax boundary", c.name);
    if (!clean || !c.clean) continue;
    unsigned declarations = 0;
    const a::Typedef* def = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::Typedef>(node)) {
        ++declarations; if (!def) def = value;
      }
    const std::string_view name(c.name);
    check(def && def->type && declarations == (name == "sequence" ? 2u : 1u),
          "all typed typedef declarations retained", c.name);
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = result.value && !result.value->parent;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (result.value) walk(walk, *result.value);
    check(owned && seen.size() == result.nodes.size(),
          "complete unique AST/CST ownership", c.name);
    if (!def) continue;
    if (name == "public")
      check(def->modifiers.size() == 1 && def->modifiers[0] == a::Modifier::Public,
            "public modifier retained", c.name);
    else if (name == "modifiers")
      check(def->modifiers.size() == 2 && def->modifiers[0] == a::Modifier::Private &&
            def->modifiers[1] == a::Modifier::Static,
            "modifier order retained", c.name);
    else if (name == "nested")
      check(def->modifiers.size() == 1 && def->modifiers[0] == a::Modifier::Public,
            "nested declaration modifier retained", c.name);
    else check(def->modifiers.empty(), "absent modifier stays empty", c.name);
    if (name == "int" || name == "class" || name == "public" || name == "sequence") {
      const auto* type = cursiv::ast_cast<a::SimpleType>(def->type);
      check(def->name == "Count" && type && type->kind == a::SimpleTypeKind::Int,
            "int type and alias name retained", c.name);
    } else if (name == "string") {
      const auto* type = cursiv::ast_cast<a::SimpleType>(def->type);
      check(def->name == "Text" && type && type->kind == a::SimpleTypeKind::String,
            "string type and alias name retained", c.name);
    } else if (name == "modifiers") {
      const auto* type = cursiv::ast_cast<a::SimpleType>(def->type);
      check(def->name == "Ratio" && type && type->kind == a::SimpleTypeKind::Float,
            "float type and alias name retained", c.name);
    } else if (name == "array" || name == "nested") {
      const auto* type = cursiv::ast_cast<a::ArrayType>(def->type);
      const auto* element = type ? cursiv::ast_cast<a::SimpleType>(type->element) : nullptr;
      check(def->name == "Counts" && element && element->kind == a::SimpleTypeKind::Int,
            "array element type retained", c.name);
    } else if (name == "mapping") {
      const auto* type = cursiv::ast_cast<a::MappingType>(def->type);
      const auto* key = type ? cursiv::ast_cast<a::SimpleType>(type->key) : nullptr;
      const auto* value = type ? cursiv::ast_cast<a::SimpleType>(type->value) : nullptr;
      check(def->name == "Lookup" && key && key->kind == a::SimpleTypeKind::String &&
            value && value->kind == a::SimpleTypeKind::Int,
            "mapping key/value types retained", c.name);
    } else if (name == "function") {
      const auto* type = cursiv::ast_cast<a::FunctionType>(def->type);
      check(def->name == "Callback" && type && type->params.size() == 1 && type->returnType,
            "function signature retained", c.name);
    } else if (name == "union") {
      const auto* type = cursiv::ast_cast<a::UnionType>(def->type);
      const auto* left = type ? cursiv::ast_cast<a::SimpleType>(type->left) : nullptr;
      const auto* right = type ? cursiv::ast_cast<a::SimpleType>(type->right) : nullptr;
      check(def->name == "Scalar" && left && left->kind == a::SimpleTypeKind::Int &&
            right && right->kind == a::SimpleTypeKind::String,
            "union alternatives retained", c.name);
    } else if (name == "range") {
      const auto* type = cursiv::ast_cast<a::SimpleType>(def->type);
      check(def->name == "Byte" && type && type->hasRange && type->lower == "0" && type->upper == "255",
            "integer range bounds retained", c.name);
    } else if (name == "multiset") {
      const auto* type = cursiv::ast_cast<a::MultisetType>(def->type);
      const auto* element = type ? cursiv::ast_cast<a::SimpleType>(type->element) : nullptr;
      check(def->name == "Set" && element && element->kind == a::SimpleTypeKind::String,
            "multiset element type retained", c.name);
    } else if (name == "object")
      check(def->name == "Handle" && cursiv::ast_cast<a::ObjectType>(def->type),
            "object type retained", c.name);
    if (name == "class" || name == "nested") {
      const auto* body = cursiv::ast_cast<a::Program>(def->parent);
      const auto* cls = body ? cursiv::ast_cast<a::ClassDef>(body->parent) : nullptr;
      check(cls && cls->name == (name == "class" ? "C" : "Inner") &&
            body->definitions.size() == 1 && body->definitions[0] == def,
            "typedef belongs to its class program", c.name);
    }
  }
  std::puts("pike typedef: 14 valid, 8 invalid; typed values and AST/CST ownership");
  return ok;
}
} // namespace pike_typedef
