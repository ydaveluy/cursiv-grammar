#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "glsl/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"

namespace glsl_initializers {
inline bool run(const auto& language) {
  namespace a = glsl::ast;
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "glsl initializer: %s\n", message); ok = false; }
  };
  constexpr std::string_view source = R"GLSL(#version 460
float a[2][2] = {{1.0,2.0,},{3.0,4.0,},};
void main() {}
)GLSL";
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
  const auto result = language.parser->parse(source);
  const bool clean = result.accepted && !result.cst().recovered() && result.diagnostics.empty();
  check(clean && owned(result), "native nested trailing commas keep complete ownership");
  unsigned lists = 0, numbers = 0;
  bool nested = false;
  for (const auto* node : result.nodes) {
    if (const auto* list = cursiv::ast_cast<a::ListInit>(node)) {
      ++lists;
      check(list->items.size() == 2, "each nonempty aggregate keeps two initializers");
      nested |= list->items.size() == 2 && cursiv::ast_cast<a::ListInit>(list->items[0]) &&
                cursiv::ast_cast<a::ListInit>(list->items[1]);
    }
    if (const auto* number = cursiv::ast_cast<a::NumberLit>(node))
      if (number->text == "1.0" || number->text == "2.0" || number->text == "3.0" || number->text == "4.0")
        ++numbers;
  }
  check(lists == 3 && nested && numbers == 4, "nested aggregate AST retains all ordered values");
  constexpr std::string_view extension = R"GLSL(#version 460
#extension GL_EXT_null_initializer : require
struct S { float x; }; S s = {}; void main() {}
)GLSL";
  const auto empty = language.parser->parse(extension);
  bool emptyList = false;
  for (const auto* node : empty.nodes)
    if (const auto* list = cursiv::ast_cast<a::ListInit>(node)) emptyList |= list->items.empty();
  check(empty.accepted && !empty.cst().recovered() && empty.diagnostics.empty() && emptyList && owned(empty),
        "existing explicitly enabled empty initializer extension remains intact");
  for (const auto source : {
      "#version 460\nfloat a[1] = {,}; void main() {}\n",
      "#version 460\nfloat a[1][1] = {{,}}; void main() {}\n",
      "#version 460\nfloat a[2] = {1.0,,2.0}; void main() {}\n",
      "#version 460\n#extension GL_EXT_null_initializer : require\nstruct S { float x; }; S s={,}; void main() {}\n"}) {
    const auto invalid = language.parser->parse(source);
    const bool rejected = !invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty();
    if (!rejected) std::fprintf(stderr, "glsl initializer: unexpectedly clean: %s\n", source);
    check(rejected, "comma requires a preceding initializer, also under null-initializer extension");
  }
  return ok;
}
} // namespace glsl_initializers
