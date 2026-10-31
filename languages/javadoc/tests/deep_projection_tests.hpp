#pragma once

#include <cstdio>
#include <memory_resource>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/syntax-tree/AstArena.hpp"
#include "javadoc/core/Content.hpp"

namespace javadoc_deep {
inline std::string source(unsigned depth) {
  std::string text;
  for (unsigned i = 0; i < depth; ++i) text += "{@summary pre {[";
  text += "{@literal end}";
  for (unsigned i = 0; i < depth; ++i) text += "]}}";
  return text;
}

template <class Language> bool run(const Language& language) {
  namespace ast = javadoc::ast;
  // Real parsed nesting checks source projection, complete reachability and
  // every owner; the larger direct AST then tests the public projection API
  // independently of either parser backend's own recursion limits.
  constexpr unsigned parsedDepth = 64;
  const auto tagSource = source(parsedDepth);
  const std::string parsedSource = "/** " + tagSource + " */";
  const auto parsed = language.language.parser->parse(parsedSource);
  const auto* doc = parsed.template value_as<ast::Documentation>();
  const auto* outer = doc && doc->description.size() == 3
      ? std::get_if<ast::InlineTag*>(&doc->description[1]) : nullptr;
  std::unordered_set<const cursiv::AstNode*> seen;
  std::vector<const cursiv::AstNode*> work;
  if (doc) work.push_back(doc);
  bool owned = true;
  unsigned tags = 0, braces = 0;
  while (!work.empty()) {
    const auto* node = work.back(); work.pop_back();
    if (!seen.insert(node).second) { owned = false; continue; }
    if (const auto* tag = cursiv::ast_cast<ast::InlineTag>(node)) {
      ++tags; owned &= tag->open == "{@" && tag->close == "}";
    }
    if (const auto* brace = cursiv::ast_cast<ast::InlineBrace>(node)) {
      ++braces; owned &= brace->open == "{" && brace->close == "}";
    }
    language.language.astReflection->visitMembers(*node,
        [&](std::string_view, std::uint32_t, const cursiv::AstNode& child) {
          owned &= child.parent == node &&
              parsed.cst().contains(node->cstNodeId, child.cstNodeId);
          work.push_back(&child);
        });
  }
  std::string projected;
  if (outer && *outer) javadoc::append_item_source(projected, ast::Item{*outer});
  if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
      projected != tagSource || !owned || seen.size() != parsed.nodes.size() ||
      tags != parsedDepth + 1 || braces != parsedDepth) {
    std::size_t difference = 0;
    while (difference < projected.size() && difference < tagSource.size() &&
           projected[difference] == tagSource[difference]) ++difference;
    std::fprintf(stderr, "javadoc deep64: accepted=%d recovered=%d diagnostics=%zu doc=%d "
        "description=%zu outer=%d projected=%zu expected=%zu first_difference=%zu "
        "parents=%d nodes=%zu/%zu tags=%u/%u braces=%u/%u\n",
        parsed.accepted, parsed.cst().recovered(), parsed.diagnostics.size(), doc != nullptr,
        doc ? std::size_t(doc->description.size()) : 0u, outer && *outer, projected.size(), tagSource.size(),
        difference, owned, seen.size(), parsed.nodes.size(), tags, parsedDepth + 1, braces, parsedDepth);
    return false;
  }

  constexpr unsigned deep = 16384;
  std::pmr::monotonic_buffer_resource pool;
  cursiv::AstArena arena(&pool);
  auto* leaf = arena.make<ast::InlineTag>();
  leaf->open = "{@";
  leaf->name = "literal";
  leaf->content = " end";
  leaf->close = "}";
  auto* top = leaf;
  for (unsigned i = 0; i < deep; ++i) {
    auto* brace = arena.make<ast::InlineBrace>();
    brace->open = "{";
    brace->close = "}";
    brace->parts.push_back(arena, ast::Item{std::string_view("[")});
    brace->parts.push_back(arena, ast::Item{top});
    brace->parts.push_back(arena, ast::Item{std::string_view("]")});
    top->parent = brace;
    auto* tag = arena.make<ast::InlineTag>();
    tag->open = "{@";
    tag->name = "summary";
    tag->close = "}";
    tag->structured = true;
    tag->parts.push_back(arena, ast::Item{std::string_view(" pre ")});
    tag->parts.push_back(arena, ast::Item{brace});
    brace->parent = tag;
    top = tag;
  }
  const auto expected = source(deep);
  constexpr std::string_view prefix = "{@summary";
  const auto body = expected.substr(prefix.size(), expected.size() - prefix.size() - 1);
  std::string whole = "prefix:";
  javadoc::append_item_source(whole, ast::Item{top});
  std::string content = "prefix:";
  javadoc::append_inline_content(content, *top);
  bool parents = top->parent == nullptr;
  const auto* current = top;
  for (unsigned i = 0; i < deep; ++i) {
    const auto* group = current->parts.size() == 2
        ? std::get_if<ast::InlineBrace*>(&current->parts[1]) : nullptr;
    const auto* next = group && *group && (*group)->parts.size() == 3
        ? std::get_if<ast::InlineTag*>(&(*group)->parts[1]) : nullptr;
    if (!group || !*group || !next || !*next || (*group)->parent != current ||
        (*next)->parent != *group) { parents = false; break; }
    current = *next;
  }
  if (!parents || current != leaf || whole != "prefix:" + expected ||
      content != "prefix:" + body || javadoc::raw_inline_content(*top) != body ||
      javadoc::inline_content(*top) != body.substr(1)) {
    std::fprintf(stderr, "javadoc deep:16384 owned tag/brace levels or projection changed\n");
    return false;
  }
  return true;
}
}  // namespace javadoc_deep
