#pragma once

#include <cstdio>
#include <iterator>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/syntax-tree/AstReflection.hpp"
#include "pike/ast.hpp"

namespace pike_enum {
namespace a = pike::ast;
template <class T> const T* as(const cursiv::AstNode* node) {
  return cursiv::ast_cast<T>(node);
}

inline bool run(const auto& language) {
  bool ok = true;
  const auto check = [&](bool pass, const char* reason, std::string_view name) {
    if (!pass) {
      std::fprintf(stderr, "pike enum: %s; case=%.*s\n", reason,
                   int(name.size()), name.data());
      ok = false;
    }
  };
  const auto wholeTree = [&](const auto& result, std::string_view name) {
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = result.value && !result.value->parent;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool contained = node.cstNodeId < result.cst().size() &&
                child.cstNodeId < result.cst().size() &&
                result.cst().contains(node.cstNodeId, child.cstNodeId);
            if (child.parent != &node || !contained)
              std::fprintf(stderr, "pike enum %.*s: member=%.*s index=%u parent=%d CST=%d\n",
                           int(name.size()), name.data(), int(member.size()), member.data(),
                           unsigned(index), child.parent == &node, contained);
            owned &= child.parent == &node && contained;
            self(self, child);
          });
    };
    if (result.value) walk(walk, *result.value);
    check(owned && seen.size() == result.nodes().size(),
          "complete unique AST ownership and CST containment", name);
  };
  struct Case { const char* name; const char* source; bool clean; };
  const Case cases[] = {
      {"named", "enum E { A, B }", true},
      {"anonymous-semicolons", "enum { A, B };;", true},
      {"qualified-type", "class C { public enum E { A, B } } C.E value;", true},
      {"empty", "enum Empty {}", true},
      {"holes", "enum E { , A, , B, }", true},
      {"only-holes", "enum E {,,,}", true},
      {"arithmetic", "enum E { A=1, B=A+2, C }", true},
      {"signed-shift", "enum E { A=-2, B, C=1<<4 }", true},
      {"string-values", "enum E { A=\"a\", B=\"b\" }", true},
      {"float-values", "enum E { A=1.5, B=2.5 }", true},
      {"constant-expression", "constant E = enum { A=1, B=2 };", true},
      {"parenthesized-expression", "constant E = (enum { A, B });", true},
      {"local-constant", "void f(){ constant E = enum { A=1, B=2 }; }", true},
      {"local-expression", "void f(){ enum E { A, B }; }", true},
      {"nested-class", "class Outer { class Inner { enum E { A, B } } }", true},
      {"named-variable-type", "enum E { A, B } E value;", true},
      {"named-array-type", "enum E { A, B } array(E) values;", true},
      {"named-typedef-type", "enum E { A, B } typedef E Alias;", true},
      {"named-function-type", "enum E { A, B } E identity(E value){ return value; }", true},
      {"cast-observation", "enum E { A=1, B=2 } int f(){ return (E)A; }", false},
      {"missing-open", "enum E A, B }", false},
      {"missing-close", "enum E { A, B", false},
      {"missing-value", "enum E { A= }", false},
      {"reserved-member", "enum E { if }", false},
      {"missing-comma", "enum E { A B }", false},
  };
  for (const auto& test : cases) {
    const auto result = language.parser->parse(test.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty() && result.value;
    const std::string_view name(test.name);
    check(clean == test.clean, "native enum syntax verdict", name);
    if (clean != test.clean) {
      std::fprintf(stderr, "  accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   result.accepted, result.cst().recovered(), result.diagnostics.size(), test.source);
      for (const auto& diagnostic : result.diagnostics)
        std::fprintf(stderr, "  kind=%u bytes=%u..%u %s\n", unsigned(diagnostic.kind),
                     diagnostic.begin, diagnostic.end, diagnostic.message.c_str());
    }
    if (!clean || !test.clean) continue;
    check(result.cst().text() == test.source, "whole source text retained", name);
    wholeTree(result, name);
    std::vector<const a::EnumExpr*> enums;
    std::vector<const a::EnumDefinition*> definitions;
    std::vector<const a::NamedType*> namedTypes;
    for (const auto* node : result.nodes()) {
      if (const auto* value = as<a::EnumExpr>(node)) enums.push_back(value);
      if (const auto* value = as<a::EnumDefinition>(node)) definitions.push_back(value);
      if (const auto* value = as<a::NamedType>(node)) namedTypes.push_back(value);
    }
    check(enums.size() == 1, "one shared type-valued enum payload", name);
    if (enums.size() != 1) continue;
    const auto* value = enums[0];
    const bool expression = name == "constant-expression" || name == "parenthesized-expression" ||
                            name == "local-constant" || name == "local-expression";
    check(definitions.size() == (expression ? 0u : 1u),
          "declarations wrap the payload, expression contexts keep it as an Expr", name);
    if (!definitions.empty()) {
      const auto* def = definitions[0];
      check(def->value == value && value->parent == def,
            "definition owns its reusable enum expression", name);
      if (name == "qualified-type")
        check(def->modifiers.size() == 1 && def->modifiers[0] == a::Modifier::Public,
              "declaration modifier retained on Definition", name);
      else check(def->modifiers.empty(), "absent modifiers remain empty", name);
    } else check(!as<a::EnumDefinition>(value->parent), "no declaration wrapper in expression", name);
    const bool anonymous = name == "anonymous-semicolons" || name == "constant-expression" ||
                           name == "parenthesized-expression" || name == "local-constant";
    check(value->name == (anonymous ? "" : name == "empty" ? "Empty" : "E"),
          "explicit or anonymous enum name retained", name);
    const std::size_t count = name == "empty" || name == "only-holes" ? 0u :
        name == "arithmetic" || name == "signed-shift" ? 3u : 2u;
    check(value->entries.size() == count, "empty comma slots create no phantom members", name);
    if (value->entries.size() == count) {
      for (std::size_t i = 0; i < count; ++i)
        check(value->entries[i] && value->entries[i]->name == (i == 0 ? "A" : i == 1 ? "B" : "C") &&
              value->entries[i]->parent == value, "member names, source order and owners", name);
      const auto number = [&](const a::Expr* expr, std::string_view spelling) {
        const auto* n = as<a::NumberLit>(expr);
        return n && n->value == spelling;
      };
      if (name == "arithmetic") {
        const auto* sum = as<a::BinaryExpr>(value->entries[1]->value);
        const auto* first = sum ? as<a::ConstantIdentifier>(sum->left) : nullptr;
        check(number(value->entries[0]->value, "1") && sum && sum->op == a::BinOp::Add &&
              first && first->parts.size() == 1 && first->parts[0] == "A" &&
              number(sum->right, "2") && !value->entries[2]->value,
              "explicit constant expressions retained without synthesizing implicit values", name);
      } else if (name == "signed-shift") {
        const auto* minus = as<a::UnaryExpr>(value->entries[0]->value);
        const auto* shift = as<a::BinaryExpr>(value->entries[2]->value);
        check(minus && minus->op == "-" && number(minus->operand, "2") &&
              !value->entries[1]->value && shift && shift->op == a::BinOp::ShiftLeft &&
              number(shift->left, "1") && number(shift->right, "4"),
              "signed, implicit and shifted members keep their expression trees", name);
      } else if (name == "string-values") {
        const auto* first = as<a::StringLit>(value->entries[0]->value);
        const auto* second = as<a::StringLit>(value->entries[1]->value);
        check(first && second && first->pieces.size() == 1 && second->pieces.size() == 1 &&
              first->pieces[0] == "\"a\"" && second->pieces[0] == "\"b\"",
              "non-integer explicit constant expressions are not narrowed to numbers", name);
      } else if (name == "float-values") {
        const auto* first = as<a::FloatLit>(value->entries[0]->value);
        const auto* second = as<a::FloatLit>(value->entries[1]->value);
        check(first && second && first->value == 1.5 && second->value == 2.5,
              "float constant expressions retained", name);
      } else if (name == "constant-expression" || name == "local-constant") {
        check(number(value->entries[0]->value, "1") && number(value->entries[1]->value, "2"),
              "constant-expression enum entries own their explicit values", name);
      } else {
        for (const auto* entry : value->entries)
          check(!entry->value, "implicit members have no invented initializer", name);
      }
    }
    const std::size_t expectedNames = name == "qualified-type" || name == "named-variable-type" ||
        name == "named-array-type" || name == "named-typedef-type" ? 1u :
        name == "named-function-type" ? 2u : 0u;
    check(namedTypes.size() == expectedNames, "named enum type positions retained", name);
    for (const auto* type : namedTypes) {
      const auto* id = type->name;
      check(id && id->parent == type && !type->pointer,
            "named type owns its existing ConstantIdentifier node", name);
      if (!id) continue;
      if (name == "qualified-type")
        check(id->parts.size() == 2 && id->parts[0] == "C" && id->parts[1] == "E",
              "qualified enum type path retained", name);
      else check(id->parts.size() == 1 && id->parts[0] == "E", "enum type name retained", name);
    }
    if (name == "local-expression") {
      bool found = false;
      for (const auto* node : result.nodes()) {
        const auto* statement = as<a::ExprStatement>(node);
        const auto* comma = statement ? as<a::CommaExpr>(statement->expr) : nullptr;
        found |= comma && comma->items.size() == 1 && comma->items[0] == value;
      }
      check(found, "local enum uses the native expression-statement route", name);
    }
  }
  // Primary grammar cast/type distinction: an ordinary parenthesized name
  // must not turn into a named-type cast when identifier_type is introduced.
  const auto parentheses = language.parser->parse("int f(int x){return (x)+1;}");
  const bool clean = parentheses.accepted && !parentheses.cst().recovered() &&
                     parentheses.diagnostics.empty() && parentheses.value;
  check(clean, "parenthesized expression control parses", "parentheses-control");
  if (clean) {
    const a::BinaryExpr* sum = nullptr;
    unsigned casts = 0;
    for (const auto* node : parentheses.nodes()) {
      if (const auto* n = as<a::BinaryExpr>(node)) sum = n;
      casts += as<a::CastExpr>(node) != nullptr;
    }
    check(sum && sum->op == a::BinOp::Add && as<a::ParenExpr>(sum->left) && casts == 0,
          "parenthesized name remains the left operand of addition", "parentheses-control");
    wholeTree(parentheses, "parentheses-control");
  }
  std::printf("pike enum: %zu native controls and one primary AST control\n", std::size(cases));
  return ok;
}
}  // namespace pike_enum
