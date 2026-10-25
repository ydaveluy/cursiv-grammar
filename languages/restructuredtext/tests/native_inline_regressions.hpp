#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "restructuredtext/generated/Restructuredtext/Module.hpp"

// Docutils 0.23 Parser.parse controls; expectations are embedded for CI.
inline int native_inline_regressions() {
  namespace ast = restructuredtext::ast;
  const auto language = cursiv::create_language<
      restructuredtext::RestructuredtextCoreServices,
      restructuredtext::RestructuredtextSharedCoreServices,
      restructuredtext::install_core_module>();
  const auto factory =
      restructuredtext::generated::Restructuredtext::create_parser();
  struct Case {
    std::string_view source;
    bool literal;
    std::string_view role;
    std::string_view text;
  };
  const Case cases[] = {
      {"``literal words``\n", true, "", "literal words"},
      {"``literal with ` inside``\n", true, "", "literal with ` inside"},
      {"``literal\\n\\*``\n", true, "", "literal\\n\\*"},
      {"`interpreted words`\n", false, "", "interpreted words"},
      {":emphasis:`role words`\n", false, "emphasis", "role words"},
      {":strong:`role words`\n", false, "strong", "role words"},
      {":literal:`role words`\n", false, "literal", "role words"},
      {":title-reference:`role words`\n", false, "title-reference",
       "role words"},
      {"`role words`:emphasis:\n", false, "emphasis", "role words"},
      {"`role words`:strong:\n", false, "strong", "role words"},
      {"`role words`:literal:\n", false, "literal", "role words"},
      {"`role words`:title-reference:\n", false, "title-reference",
       "role words"},
  };
  for (const auto &sample : cases) {
    // Both parse results borrow this source until every capture is checked.
    const std::string source(sample.source);
    const auto check = [&](const cursiv::ParseResult &r) {
      const auto *doc = r.value_as<ast::Document>();
      if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
          !doc || r.cst().text() != source || doc->blocks.size() != 1)
        return false;
      const auto *p = std::get_if<ast::Paragraph *>(&doc->blocks[0]);
      if (!p || !*p || (*p)->spans.size() != 1)
        return false;
      std::string_view text, role;
      if (sample.literal) {
        const auto *n = std::get_if<ast::Literal *>(&(*p)->spans[0]);
        if (!n || !*n)
          return false;
        text = (*n)->text;
      } else {
        const auto *n = std::get_if<ast::InterpretedText *>(&(*p)->spans[0]);
        if (!n || !*n)
          return false;
        text = (*n)->text;
        role = (*n)->role;
      }
      const auto captured = [&](std::string_view v) {
        if (v.empty())
          return true;
        const auto at = reinterpret_cast<std::uintptr_t>(v.data());
        const auto begin = reinterpret_cast<std::uintptr_t>(source.data());
        return at >= begin && at - begin <= source.size() &&
               v.size() <= source.size() - (at - begin);
      };
      if (text != sample.text || role != sample.role || !captured(text) ||
          !captured(role))
        return false;
      bool owned = !doc->parent;
      std::unordered_set<const cursiv::AstNode *> seen;
      const auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
        if (!seen.insert(&node).second || node.cstNodeId >= r.cst().size()) {
          owned = false;
          return;
        }
        const auto &span = r.cst()[node.cstNodeId];
        owned &= span.begin <= span.end && span.end <= source.size();
        language.language.astReflection->visitMembers(
            node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
              owned &= child.parent == &node &&
                       r.cst().contains(node.cstNodeId, child.cstNodeId);
              self(self, child);
            });
      };
      walk(walk, *doc);
      return owned && seen.size() == r.nodes.size();
    };
    const auto service = language.language.parser->parse(source);
    const auto generated = factory->parse(source);
    if (!check(service) || !check(generated)) {
      std::fprintf(stderr, "restructuredtext: native inline regression: %.*s",
                   int(sample.source.size()), sample.source.data());
      return 1;
    }
  }
  const std::string source = "`first` and `second`.\n";
  const auto r = language.language.parser->parse(source);
  const auto *doc = r.value_as<ast::Document>();
  if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() || !doc ||
      doc->blocks.size() != 1)
    return 1;
  const auto *p = std::get_if<ast::Paragraph *>(&doc->blocks[0]);
  if (!p || !*p || (*p)->spans.size() != 4)
    return 1;
  const auto *first = std::get_if<ast::InterpretedText *>(&(*p)->spans[0]);
  const auto *second = std::get_if<ast::InterpretedText *>(&(*p)->spans[2]);
  if (!first || !*first || !second || !*second || (*first)->text != "first" ||
      (*second)->text != "second")
    return 1;
  return 0;
}
