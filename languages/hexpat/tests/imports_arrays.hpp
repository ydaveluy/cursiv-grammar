#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "hexpat/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace hexpat_imports_arrays {
inline bool run(const auto& language) {
  namespace a = hexpat::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "hexpat imports/arrays: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"P(import std.mem;
import "types/base.hexpat" as local::types;
import * from std.core as core;
namespace auto file::scope {
  u8 values[3] = {1, 2+3, (true ? 4 : 5)};
  u8 empty[] = {};
  union U { u8 unionValues[2] = {8,9}; u8 unionEmpty[] = {}; u8 unionUninit[]; u8 scalar; };
}
namespace normal {
  fn f() {
    u8 local[2] = {6, 7};
    u8 none[] = {};
  };
}
)P";
  const auto result = language.parser->parse(source);
  const auto* root = cursiv::ast_cast<a::Document>(result.value);
  const bool clean = result.accepted && !result.cst().recovered() &&
                     result.diagnostics.empty() && root && root->items.size() == 5;
  check(clean, "native import/namespace and declaration-only brace arrays parse cleanly");
  if (!clean) {
    std::fprintf(stderr, "hexpat imports/arrays: accepted=%u recovered=%u items=%zu\n",
        unsigned(result.accepted), unsigned(result.cst().recovered()), root ? root->items.size() : 0);
    for (const auto& d : result.diagnostics)
      std::fprintf(stderr, "hexpat imports/arrays: kind=%u bytes=%u..%u element=%u\n",
          unsigned(d.kind), d.begin, d.end, unsigned(d.element));
    return false;
  }
  const auto* dotted = cursiv::ast_cast<a::Import>(root->items[0]);
  const auto* quoted = cursiv::ast_cast<a::Import>(root->items[1]);
  const auto* all = cursiv::ast_cast<a::Import>(root->items[2]);
  check(dotted && dotted->parts.size() == 2 && dotted->parts[0] == "std" &&
        dotted->parts[1] == "mem" && !dotted->source && !dotted->all && dotted->alias.empty() &&
        quoted && quoted->parts.empty() && quoted->source &&
        quoted->source->text == "\"types/base.hexpat\"" && quoted->alias == "local::types" &&
        !quoted->all && all && all->all && all->parts.size() == 2 &&
        all->parts[0] == "std" && all->parts[1] == "core" && !all->source && all->alias == "core",
        "quoted/dotted path, wildcard and qualified alias are distinct typed captures");
  const auto* automatic = cursiv::ast_cast<a::Namespace>(root->items[3]);
  const auto* ordinary = cursiv::ast_cast<a::Namespace>(root->items[4]);
  check(automatic && automatic->auto_ && automatic->name == "file::scope" &&
        automatic->items.size() == 3 && ordinary && !ordinary->auto_ && ordinary->name == "normal",
        "namespace auto marker and qualified name survive independently");
  bool memberArray = false, memberEmpty = false, localArray = false, localEmpty = false;
  bool unionArray = false, unionEmpty = false, unionUninit = false, unionScalar = false;
  for (const auto* node : result.nodes) {
    if (const auto* declaration = cursiv::ast_cast<a::StructMemberDeclarator>(node)) {
      const auto* init = cursiv::ast_cast<a::ArrayInitializer>(declaration->init);
      if (declaration->name == "values") {
        const auto* size = cursiv::ast_cast<a::IntLit>(declaration->size);
        const auto* first = init && init->values.size() == 3 ? cursiv::ast_cast<a::IntLit>(init->values[0]) : nullptr;
        const auto* sum = init && init->values.size() == 3 ? cursiv::ast_cast<a::BinaryExpr>(init->values[1]) : nullptr;
        const auto* condition = init && init->values.size() == 3 ? cursiv::ast_cast<a::CondExpr>(init->values[2]) : nullptr;
        memberArray = declaration->hasBrackets && !declaration->isPointer && size && size->text == "3" &&
            first && first->text == "1" && sum && sum->op == "+" && condition && condition->thenValue &&
            condition->elseValue && init->parent == declaration;
      }
      if (declaration->name == "empty")
        memberEmpty = declaration->hasBrackets && !declaration->size && init && init->values.empty();
    }
    if (const auto* declaration = cursiv::ast_cast<a::VarDeclarator>(node)) {
      const auto* init = cursiv::ast_cast<a::ArrayInitializer>(declaration->init);
      if (declaration->name == "local") {
        const auto* first = init && init->values.size() == 2 ? cursiv::ast_cast<a::IntLit>(init->values[0]) : nullptr;
        const auto* second = init && init->values.size() == 2 ? cursiv::ast_cast<a::IntLit>(init->values[1]) : nullptr;
        localArray = declaration->isArray && first && first->text == "6" && second && second->text == "7";
      }
      if (declaration->name == "none")
        localEmpty = declaration->isArray && !declaration->size && init && init->values.empty();
    }
    if (const auto* declaration = cursiv::ast_cast<a::UnionMember>(node)) {
      const auto* init = cursiv::ast_cast<a::ArrayInitializer>(declaration->init);
      if (declaration->name == "unionValues") {
        const auto* first = init && init->values.size() == 2 ? cursiv::ast_cast<a::IntLit>(init->values[0]) : nullptr;
        const auto* second = init && init->values.size() == 2 ? cursiv::ast_cast<a::IntLit>(init->values[1]) : nullptr;
        unionArray = declaration->hasBrackets && first && first->text == "8" && second && second->text == "9";
      }
      if (declaration->name == "unionEmpty")
        unionEmpty = declaration->hasBrackets && !declaration->size && init && init->values.empty();
      if (declaration->name == "unionUninit")
        unionUninit = declaration->hasBrackets && !declaration->size && !declaration->init;
      if (declaration->name == "scalar")
        unionScalar = !declaration->hasBrackets && !declaration->size && !declaration->init;
    }
  }
  check(memberArray && memberEmpty && localArray && localEmpty && unionArray && unionEmpty && unionUninit && unionScalar,
        "struct/union/function array initializers keep empty/nonempty values, size, order and node kinds");
  auto owned = [&](const cursiv::ParseResult& parsed) {
    if (!parsed.value) return false;
    bool valid = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { valid = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            valid &= child.parent == &node && parsed.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *parsed.value);
    return valid && seen.size() == parsed.nodes.size();
  };
  check(owned(result), "all path/namespace/array children have unique parents and CST ownership");
  for (const auto source : {
      "import \"types/base.hexpat\";", "import std.mem as local;",
      "import * from \"types/base.hexpat\" as Types;", "namespace auto Root {}",
      "namespace a::b {}", "u8 a[] = {};", "u8 a[2] = {1,2};",
      "struct S { u8 a[1] = {1}; };", "fn f() { u8 a[1] = {1}; };"}) {
    const auto r = language.parser->parse(source);
    const bool clean = r.accepted && !r.cst().recovered() && r.diagnostics.empty() && owned(r);
    if (!clean) std::fprintf(stderr, "hexpat imports/arrays: positive failed: %s\n", source);
    check(clean, "native import/namespace/array control rejected");
  }
  for (const auto source : {
      "import * \"types/base.hexpat\" as Types;", "import * from std.mem;",
      "import * from std.mem as a::b;", "import std..mem;", "import \"x\" from std.mem;",
      "namespace auto {}", "namespace auto n:: {}", "u8 a = {1,2};",
      "u8 a[2] = {1,};", "u8 a[2] = {,1};", "u8 a[2] = {1 2};",
      "u8 a[1] = {{1}};", "u8 a[1] = {x=1};", "u8 a[1] = {1} @ 0;",
      "u8 *p[1]:u32 = {1};", "fn f() { u8 a = {1}; };"}) {
    const auto r = language.parser->parse(source);
    const bool rejected = !r.accepted || r.cst().recovered() || !r.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "hexpat imports/arrays: unexpectedly clean: %s\n", source);
    check(rejected, "native import/array context or shape must not be bypassed");
  }
  return ok;
}
} // namespace hexpat_imports_arrays
