#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "llvmir/core/ast.hpp"

namespace llvm_attribute_groups {

inline bool run(const auto& language) {
  namespace a = llvmir::ast;
  bool ok = true;
  auto check = [&](bool value, const char* reason, std::string_view source) {
    if (!value) {
      std::fprintf(stderr, "llvm attribute group: %s; source=%.*s\n", reason,
                   int(source.size()), source.data());
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
  constexpr std::string_view combined =
      "declare void @f() #0\n"
      "attributes #0 = { align = 8 alignstack = 16 \"target-cpu\"=\"x86-64\" nounwind }\n";
  {
    const auto result = language.parser->parse(combined);
    const auto* module = cursiv::ast_cast<a::Module>(result.value);
    const auto* function = module && module->entities.size() == 2
        ? cursiv::ast_cast<a::FuncDecl>(module->entities[0]) : nullptr;
    const auto* group = module && module->entities.size() == 2
        ? cursiv::ast_cast<a::AttrGroupDef>(module->entities[1]) : nullptr;
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && function && function->header &&
                   function->header->fields.size() == 1 &&
                   function->header->fields[0]->name == "#0" && group &&
                   group->id == "#0" && group->attrs.size() == 4;
    if (group && group->attrs.size() == 4) {
      const auto* align = group->attrs[0];
      const auto* stack = group->attrs[1];
      const auto* pair = group->attrs[2];
      const auto* bare = group->attrs[3];
      correct &= align && stack && pair && bare &&
                 align->name == "align" && align->intArg && *align->intArg == "8" &&
                 stack->name == "alignstack" && stack->intArg && *stack->intArg == "16" &&
                 pair->name == "\"target-cpu\"" && pair->stringArg &&
                 *pair->stringArg == "\"x86-64\"" && bare->name == "nounwind";
    }
    check(correct, "forward group reference and ordered attribute payloads retained", combined);
    check(owned(result), "complete unique AST/CST ownership", combined);
  }
  for (const std::string_view source : {
      "attributes #0 = { nounwind }\n",
      "attributes #0 = { align = 8 }\n",
      "attributes #0 = { alignstack = 16 }\n",
      "declare void @f() align 8\n"}) {
    const auto result = language.parser->parse(source);
    bool attr = false;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::Attr>(node))
        attr |= value->name == "nounwind" ||
                (value->name == "align" && value->intArg && *value->intArg == "8") ||
                (value->name == "alignstack" && value->intArg && *value->intArg == "16");
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
          attr && owned(result), "native nonempty group or inline alignment retains its AST", source);
  }
  unsigned negatives = 0;
  for (const std::string_view source : {
      "attributes #0 = {}\n",
      "attributes #0 = { #0 }\n",
      "attributes #0 = { #1 }\nattributes #1 = { nounwind }\n",
      "attributes #0 = { align 8 }\n",
      "attributes #0 = { align(8) }\n",
      "attributes #0 = { alignstack(16) }\n",
      "attributes #0 = { nounwind, noinline }\n",
      "attributes #0 = { align = }\n"}) {
    const auto result = language.parser->parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "group requires attributes, forbids nested group refs and uses equals-alignment", source);
    ++negatives;
  }
  std::printf("llvm attribute group: 5 positives, %u negatives, fields and complete ownership\n", negatives);
  return ok;
}

} // namespace llvm_attribute_groups
