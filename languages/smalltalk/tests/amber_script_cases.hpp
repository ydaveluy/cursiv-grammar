#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>
#include <utility>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace amber_script_cases {
inline bool run(const auto& language) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
      {"empty-sequence", "", true},
      {"empty-periods", "...", true},
      {"separator-characters", "\v\f ﻿  1", true},
      {"ascii-name", "|name1| name1", true},
      {"spaced-block-parameter", "[: item | item]", true},
      {"commented-block-parameter", "[:\"name\" item : other | item]", true},
      {"empty-block", "[]", true},
      {"empty-parameter-block", "[: item |]", true},
      {"empty-block-periods", "[... ]", true},
      {"sequence-periods", "...1...2...^3...", true},
      {"temps-periods", "| x | ... x := 1... x", true},
      {"dynamic-array-periods", "{1...2...}", true},
      {"dynamic-array-empty-periods", "{...}", true},
      {"dynamic-dictionary-periods", "#{'a' -> 1... 'b' -> 2...}", true},
      {"dynamic-dictionary-empty-periods", "#{...}", true},
      {"parenthesized-association", "#{('a' -> 1)}", true},
      {"symbol-final-name", "#at:put", true},
      {"symbol-repeated-colon", "#a::b", true},
      {"symbol-selector", "#at:put:", true},
      {"bare-array-selector", "#(at:put a::b)", true},
      {"cascade-send", "1 foo; bar; + 2; at: 3", true},
      {"underscore-name", "x_y", false},
      {"underscore-parameter", "[: x_y | x]", false},
      {"underscore-symbol", "#foo_bar", false},
      {"question-selector", "1 ? 2", false},
      {"missing-parameter-name", "[: | 1]", false},
      {"missing-parameter-pipe", "[: item]", false},
      {"dictionary-non-association", "#{1}", false},
      {"dictionary-outer-other-send", "#{'a' -> 1 + 2}", false},
      {"dictionary-one-wrong-item", "#{'a' -> 1. 2}", false},
      {"dictionary-keyword-send", "#{foo at: 1}", false},
      {"cascade-no-send", "1; foo", false},
      {"cascade-variable", "receiver; foo", false},
      {"array-return", "{1...^2}", false},
      {"after-return", "^1...2", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(language.language, test.source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "smalltalk Amber script: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(),
                   v.diagnostics.size(), test.source);
      return false;
    }
    if (!clean) continue;
    const auto* root = r.template value_as<smalltalk::ast::Sequence>();
    if (!root || root->parent != v.document.get()) {
      std::fprintf(stderr, "smalltalk Amber script: %s lost/root-owned Sequence source=%s\n",
                   test.id, test.source);
      return false;
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) {
        std::fprintf(stderr, "smalltalk Amber script: %s duplicate AST node source=%s\n",
                     test.id, test.source);
        owned = false; return;
      }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained)
              std::fprintf(stderr, "smalltalk Amber script: %s parent=%d CST=%d owner=%p child=%p actual-parent=%p source=%s\n",
                           test.id, parent, contained, static_cast<const void*>(&node),
                           static_cast<const void*>(&child), static_cast<const void*>(child.parent), test.source);
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "smalltalk Amber script: %s ownership=%d seen=%zu parser-nodes=%zu source=%s\n",
                   test.id, owned, seen.size(), r.nodes.size(), test.source);
      return false;
    }
  }
  for (const auto& test : {
           std::pair{"#{1}", "smalltalk.dictionary-association"},
           std::pair{"#{'a' -> 1 + 2}", "smalltalk.dictionary-association"},
           std::pair{"1; foo", "smalltalk.cascade-send"},
           std::pair{"receiver; foo", "smalltalk.cascade-send"}}) {
    const auto v = cursiv::test::validationHelper(language.language, test.first);
    const auto& r = v.document->parseResult;
    bool found = false;
    for (const auto& d : v.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.second;
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !found) {
      std::fprintf(stderr, "smalltalk Amber script: expected-code=%s found=%d accepted=%d recovered=%d parse-diags=%zu validation-diags=%zu source=%s\n",
                   test.second, found, r.accepted, r.cst().recovered(),
                   r.diagnostics.size(), v.diagnostics.size(), test.first);
      for (const auto& d : v.diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return false;
    }
  }
  const auto r = language.language.parser->parse(
      "[: item | ... item]. {1...2...}. #{'a' -> 1...}. #at:put");
  const auto* root = r.template value_as<smalltalk::ast::Sequence>();
  if (!root || root->statements.size() != 4) {
    std::fprintf(stderr, "smalltalk Amber script: combined readback statements=%zu expected4 accepted=%d recovered=%d\n",
                 root ? root->statements.size() : 0, r.accepted, r.cst().recovered());
    return false;
  }
  const auto* block = cursiv::ast_cast<smalltalk::ast::Block>(root->statements[0]);
  const auto* array = cursiv::ast_cast<smalltalk::ast::DynamicArray>(root->statements[1]);
  const auto* dict = cursiv::ast_cast<smalltalk::ast::DynamicDictionary>(root->statements[2]);
  const auto* symbol = cursiv::ast_cast<smalltalk::ast::SymbolLiteral>(root->statements[3]);
  const auto* body = block && block->body && block->body->statements.size() == 1
      ? cursiv::ast_cast<smalltalk::ast::VarRef>(block->body->statements[0]) : nullptr;
  const bool okay = block && block->params.size() == 1 && block->params[0] == "item" &&
      block->body->parent == block && body && body->name == "item" &&
      array && array->items.size() == 2 && dict && dict->items.size() == 1 &&
      symbol && symbol->name == "at:put";
  if (!okay)
    std::fprintf(stderr, "smalltalk Amber script: combined readback block=%d params=%zu body=%d body-parent=%d var='%s' array-items=%zu dictionary-items=%zu symbol='%s'\n",
                 block != nullptr, block ? block->params.size() : 0, body != nullptr,
                 block && block->body && block->body->parent == block,
                 body ? std::string(body->name).c_str() : "<null>",
                 array ? array->items.size() : 0, dict ? dict->items.size() : 0,
                 symbol ? std::string(symbol->name).c_str() : "<null>");
  return okay;
}
}  // namespace amber_script_cases
