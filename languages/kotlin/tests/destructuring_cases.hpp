#pragma once

#include <cstdio>
#include <initializer_list>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "kotlin/core/ast.hpp"

namespace kotlin_destructuring {

inline bool run(const auto& language) {
  namespace a = kotlin::ast;
  using Kind = a::DestructuringKind;
  struct Entry {
    std::string_view name, type, valOrVar, source;
    unsigned annotations = 0;
  };
  bool ok = true;
  unsigned positives = 0;
  auto check = [&](bool value, const char* reason, std::string_view source) {
    if (!value) {
      std::fprintf(stderr, "kotlin destructuring: %s; source=%.*s\n",
                   reason, int(source.size()), source.data());
      ok = false;
    }
  };
  auto owned = [&](const cursiv::ParseResult& result) {
    if (!result.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *result.value);
    return valid && seen.size() == result.nodes.size();
  };
  auto verify = [&]<class Owner>(std::string_view source, Kind kind,
                                 std::initializer_list<Entry> entries,
                                 bool isVar = false,
                                 std::string_view loopPrefix = {}) {
    const auto result = language.parser->parse(source);
    const Owner* owner = nullptr;
    unsigned owners = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<Owner>(node);
          value && value->destructuring == kind) {
        owner = value;
        ++owners;
      }
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && owner && owners == 1 &&
                   owner->declarators.size() == entries.size();
    if constexpr (std::is_same_v<Owner, a::PropertyDecl>)
      correct &= owner && owner->isVar == isVar;
    if constexpr (std::is_same_v<Owner, a::ForExpr>)
      correct &= owner && owner->valOrVar == loopPrefix;
    if (owner && owner->declarators.size() == entries.size()) {
      unsigned i = 0;
      for (const auto& expected : entries) {
        const auto* entry = owner->declarators[i++];
        const auto* type = entry ? cursiv::ast_cast<a::UserType>(entry->type) : nullptr;
        correct &= entry && entry->parent == owner &&
                   entry->name == expected.name &&
                   entry->valOrVar == expected.valOrVar &&
                   entry->sourceName == expected.source &&
                   entry->modifiers.empty() &&
                   entry->annotations.size() == expected.annotations;
        if (expected.type.empty()) correct &= entry && !entry->type;
        else correct &= type && type->segments.size() == 1 &&
                        type->segments[0]->name == expected.type &&
                        type->parent == entry;
        if (entry)
          for (const auto* annotation : entry->annotations)
            correct &= annotation && annotation->name == "A" && annotation->parent == entry;
      }
    }
    check(correct, "form, ordered names, types, entry mutability and renaming", source);
    check(owned(result), "complete unique AST/CST ownership", source);
    ++positives;
  };
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ val (a: Int = aa, b,) = source }", Kind::named,
      {{"a", "Int", "", "aa"}, {"b", "", "", ""}});
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ var [a: Int, b,] = source }", Kind::positional,
      {{"a", "Int", "", ""}, {"b", "", "", ""}}, true);
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ (val a: Int = aa, var b,) = source }", Kind::named,
      {{"a", "Int", "val", "aa"}, {"b", "", "var", ""}});
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ [val a: Int, var b,] = source }", Kind::positional,
      {{"a", "Int", "val", ""}, {"b", "", "var", ""}});
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ val (@A a: Int = aa, b) = source }", Kind::named,
      {{"a", "Int", "", "aa", 1}, {"b", "", "", ""}});
  verify.template operator()<a::LambdaParam>(
      "fun f(){ val lambda = { (a: Int = aa, b,) -> a } }", Kind::named,
      {{"a", "Int", "", "aa"}, {"b", "", "", ""}});
  verify.template operator()<a::LambdaParam>(
      "fun f(){ val lambda = { [a: Int, b,] -> a } }", Kind::positional,
      {{"a", "Int", "", ""}, {"b", "", "", ""}});
  verify.template operator()<a::LambdaParam>(
      "fun f(){ val lambda = { (val a: Int = aa, val b,) -> a } }", Kind::named,
      {{"a", "Int", "val", "aa"}, {"b", "", "val", ""}});
  verify.template operator()<a::LambdaParam>(
      "fun f(){ val lambda = { [val a: Int, val b,] -> a } }", Kind::positional,
      {{"a", "Int", "val", ""}, {"b", "", "val", ""}});
  verify.template operator()<a::ForExpr>(
      "fun f(){ for ((a: Int = aa, b,) in items) {} }", Kind::named,
      {{"a", "Int", "", "aa"}, {"b", "", "", ""}});
  verify.template operator()<a::ForExpr>(
      "fun f(){ for ([a: Int, b,] in items) {} }", Kind::positional,
      {{"a", "Int", "", ""}, {"b", "", "", ""}});
  verify.template operator()<a::ForExpr>(
      "fun f(){ for ((val a: Int = aa, val b,) in items) {} }", Kind::named,
      {{"a", "Int", "val", "aa"}, {"b", "", "val", ""}});
  verify.template operator()<a::ForExpr>(
      "fun f(){ for ([val a: Int, val b,] in items) {} }", Kind::positional,
      {{"a", "Int", "val", ""}, {"b", "", "val", ""}});
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ val a: Int = 1 }", Kind::none, {{"a", "Int", "", ""}});
  verify.template operator()<a::ForExpr>(
      "fun f(){ for (val a: Int in items) {} }", Kind::none,
      {{"a", "Int", "", ""}}, false, "val");
  verify.template operator()<a::PropertyDecl>(
      "fun f(){ val (a,b) = source }", Kind::named,
      {{"a", "", "", ""}, {"b", "", "", ""}});
  unsigned negatives = 0;
  for (const std::string_view source : {
      "fun f(){ val [a = aa, b] = source }",
      "fun f(){ [val a = aa, val b] = source }",
      "fun f(){ (val a,b) = source }",
      "fun f(){ val (val a,b) = source }",
      "fun f(){ val [val a,b] = source }",
      "fun f(){ val lambda = { (var a,var b) -> a } }",
      "fun f(){ val lambda = { (val a,var b) -> a } }",
      "fun f(){ val lambda = { [val a,b] -> a } }",
      "fun f(){ for ((var a,var b) in items) {} }",
      "fun f(){ for ([val a,var b] in items) {} }",
      "fun f(){ for ([a = aa,b] in items) {} }",
      "fun f(){ val () = source }", "fun f(){ val [] = source }",
      "fun f(){ val (a = member.x,b) = source }",
      "fun f(){ val (a = 1,b) = source }"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "invalid rename, mixed mode, var parameter or empty list rejected", source);
    ++negatives;
  }
  std::printf("kotlin destructuring: %u positives, %u negatives, fields and complete ownership\n",
              positives, negatives);
  return ok;
}

} // namespace kotlin_destructuring
