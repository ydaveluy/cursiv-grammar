#pragma once

#include <algorithm>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "javadoc/core/Content.hpp"
#include "deep_projection_tests.hpp"
#include "nested_cases.hpp"

namespace javadoc_nested {
inline std::string joined(const std::vector<std::string>& values) {
  std::string out;
  for (const auto& value : values) {
    if (!out.empty()) out += ',';
    out += value;
  }
  return out;
}

template <class Language> bool run(const Language& language) {
  namespace ast = javadoc::ast;
  if (!javadoc_deep::run(language)) return false;
  bool ok = true;
  for (const auto& test : cases) {
    const auto parsed = language.language.parser->parse(test.source);
    const auto* doc = parsed.template value_as<ast::Documentation>();
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<std::string> names, edges;
    unsigned braces = 0;
    bool owned = true, raw = true;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      if (const auto* tag = cursiv::ast_cast<ast::InlineTag>(&node)) {
        names.emplace_back(tag->name);
        const auto text = javadoc::raw_inline_content(*tag);
        raw &= parsed.cst().text(tag->cstNodeId) ==
            "{@" + std::string(tag->name) + text + "}";
        raw &= tag->open == "{@" && tag->close == "}";
        const bool structured = tag->name == "summary" || tag->name == "return" ||
            tag->name == "link" || tag->name == "linkplain" || tag->name == "index";
        raw &= tag->structured == structured;
        raw &= structured ? tag->content.empty() : tag->parts.empty();
        for (const auto* parent = tag->parent; parent; parent = parent->parent) {
          if (const auto* outer = cursiv::ast_cast<ast::InlineTag>(parent)) {
            edges.emplace_back(std::string(outer->name) + ">" + std::string(tag->name));
            break;
          }
          if (const auto* outer = cursiv::ast_cast<ast::BlockTag>(parent)) {
            edges.emplace_back(std::string(outer->name) + ">" + std::string(tag->name));
            break;
          }
        }
      }
      if (const auto* brace = cursiv::ast_cast<ast::InlineBrace>(&node)) {
        ++braces;
        raw &= brace->open == "{" && brace->close == "}";
        std::string text = "{";
        for (const auto& part : brace->parts) javadoc::append_item_source(text, part);
        text += '}';
        raw &= parsed.cst().text(brace->cstNodeId) == text;
      }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, const cursiv::AstNode& child) {
            owned &= child.parent == &node &&
                parsed.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (doc) walk(walk, *doc);
    std::sort(edges.begin(), edges.end());
    const bool valid = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
        parsed.cst().text() == test.source && doc && raw && owned &&
        seen.size() == parsed.nodes.size() && joined(names) == test.names &&
        joined(edges) == test.edges && braces == test.braces;
    if (!valid) {
      std::fprintf(stderr, "javadoc nested [%.*s]: accepted=%d recovered=%d diagnostics=%zu "
          "names=[%s] edges=[%s] braces=%u raw=%d parents=%d nodes=%zu/%zu\n",
          int(test.name.size()), test.name.data(), parsed.accepted, parsed.cst().recovered(),
          parsed.diagnostics.size(), joined(names).c_str(), joined(edges).c_str(), braces,
          raw, owned, seen.size(), parsed.nodes.size());
      ok = false;
    }
  }
  for (const auto source : invalid) {
    const auto parsed = language.language.parser->parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) {
      std::fprintf(stderr, "javadoc nested: invalid structured header/escape accepted\n");
      ok = false;
    }
  }
  return ok;
}
}  // namespace javadoc_nested
