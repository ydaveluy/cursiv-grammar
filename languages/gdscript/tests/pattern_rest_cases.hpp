#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "gdscript/core/ast.hpp"

namespace gdscript_pattern_rest {

inline bool run(const auto& language) {
  namespace a = gdscript::ast;
  bool ok = true;
  auto source = [](std::string_view pattern) {
    return "func f(value):\n    match value:\n        " +
           std::string(pattern) + ":\n            pass\n";
  };
  auto check = [&](bool value, const char* reason, std::string_view pattern) {
    if (!value) {
      std::fprintf(stderr, "gdscript pattern: %s; pattern=%.*s\n",
                   reason, int(pattern.size()), pattern.data());
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
  auto verify = [&]<class Pattern>(std::string_view pattern, unsigned count,
                                   bool rest, bool binding = true) {
    const auto text = source(pattern);
    const auto result = language.parser->parse(text);
    const a::MatchCase* branch = nullptr;
    for (const auto* node : result.nodes)
      if (const auto* matchCase = cursiv::ast_cast<a::MatchCase>(node)) branch = matchCase;
    const auto* value = branch && branch->patterns.size() == 1
        ? cursiv::ast_cast<Pattern>(branch->patterns[0]) : nullptr;
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && value &&
                   value->items.size() == count && value->openEnded == rest;
    if (value && count && value->items.size() == count) {
      const a::BindingPattern* entry = nullptr;
      if constexpr (std::is_same_v<Pattern, a::ArrayPattern>)
        entry = cursiv::ast_cast<a::BindingPattern>(value->items[0]);
      else {
        const auto* item = value->items[0];
        const auto* key = item ? cursiv::ast_cast<a::StringLit>(item->key) : nullptr;
        correct &= key && key->text == "\"key\"";
        entry = item ? cursiv::ast_cast<a::BindingPattern>(item->value) : nullptr;
        if (!binding) correct &= item && !item->value;
      }
      if (binding) correct &= entry && entry->name == "head";
    }
    check(correct, "pattern entries, rest flag and binding/key values", pattern);
    check(owned(result), "complete unique AST/CST ownership", pattern);
  };
  verify.template operator()<a::ArrayPattern>("[]", 0, false);
  verify.template operator()<a::ArrayPattern>("[..]", 0, true);
  verify.template operator()<a::ArrayPattern>("[. .]", 0, true);
  verify.template operator()<a::ArrayPattern>("[..,]", 0, true);
  verify.template operator()<a::ArrayPattern>("[var head]", 1, false);
  verify.template operator()<a::ArrayPattern>("[var head,]", 1, false);
  verify.template operator()<a::ArrayPattern>("[var head, ..]", 1, true);
  verify.template operator()<a::ArrayPattern>("[var head, ..,]", 1, true);
  verify.template operator()<a::DictPattern>("{}", 0, false);
  verify.template operator()<a::DictPattern>("{..}", 0, true);
  verify.template operator()<a::DictPattern>("{. .}", 0, true);
  verify.template operator()<a::DictPattern>("{..,}", 0, true);
  verify.template operator()<a::DictPattern>("{\"key\": var head,}", 1, false);
  verify.template operator()<a::DictPattern>("{\"key\": var head, ..}", 1, true);
  verify.template operator()<a::DictPattern>("{\"key\": var head, ..,}", 1, true);
  verify.template operator()<a::DictPattern>("{\"key\", ..}", 1, true, false);
  unsigned negatives = 0;
  for (const std::string_view pattern : {
      "[var head ..]", "[_ ..]", "[var head . .]", "[a, b..]",
      "{\"key\": var head ..}", "{\"key\" ..}", "{\"key\": value ..}",
      "[.., var head]", "{.., \"key\": var head}",
      "[.., ..]", "{.., ..}", "[,]", "{,}", "[.]", "{.}"}) {
    const auto text = source(pattern);
    const auto result = language.parser->parse(text);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "comma required before final rest and rest cannot repeat or precede an item", pattern);
    ++negatives;
  }
  std::printf("gdscript pattern: 16 positives, %u negatives, raw values and ownership\n", negatives);
  return ok;
}

} // namespace gdscript_pattern_rest
