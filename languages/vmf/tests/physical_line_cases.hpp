#pragma once

#include <cstdio>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace physical_line_cases {
inline bool run(const vmf::VmfCoreServices& services) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
      {"single-value", "a 1", true},
      {"quoted-adjacent", "\"a\"\"1\"", true},
      {"line-separated-values", "a 1\nb 2\n", true},
      {"CRLF-values", "a 1\r\nb 2\r\n", true},
      {"CR-values", "a 1\rb 2\r", true},
      {"block-next-line", "world\n{\na 1\n}\n", true},
      {"comment-before-block", "world //name\n{a 1}\n", true},
      {"value-close-same-line", "world {a 1}", true},
      {"adjacent-blocks", "one{}two{}", true},
      {"value-after-block", "one{}two 2", true},
      {"comment-after-value", "a 1 //done\nb 2\n", true},
      {"comment-at-EOF", "a 1 //done", true},
      {"multiline-value", "a \"one\ntwo\"\nb 2\n", true},
      {"escaped-LF-key", "\"a\\\nb\" 1\n", true},
      {"nested-layout", "world {\none{}two{a 1}\nthree 3}\n", true},
      {"comment-looking-key-tail", "world//name\n{a 1}\n", true},
      {"comment-looking-value-tail", "a path//name\n", true},
      {"numeric-looking-value-tail", "a 1//name\nb 2\n", true},
      {"comment-before-block-LF", "world //name\n{\na 1\n}\n", true},
      {"quoted-leading-slash", "a \"/name\"\n", true},
      {"split-value-LF", "a\n1\n", false},
      {"split-value-CRLF", "a\r\n1\n", false},
      {"split-value-CR", "a\r1\n", false},
      {"split-value-comment", "a //name\n1\n", false},
      {"two-values-same-line", "a 1 b 2\n", false},
      {"quoted-two-values", "\"a\"\"1\"\"b\"\"2\"", false},
      {"nested-two-values", "world {a 1 b 2}", false},
      {"value-then-block-same-line", "a 1 b{}", false},
      {"after-multiline-value", "a \"one\ntwo\" b 2", false},
      {"value-opens-block", "a 1 {}", false},
      {"physical-newline-key", "\"bad\nkey\" 1\n", false},
      {"bare-leading-slash-value", "a /name\n", false},
      {"bare-leading-slash-key", "/name 1\n", false},
      {"star-comment-default", "a /*not-default*/\n", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(services, test.source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "vmf physical-line: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(),
                   v.diagnostics.size(), test.source);
      return false;
    }
    if (!clean) continue;
    const auto* root = r.value_as<vmf::ast::Document>();
    if (!root || root->parent != v.document.get()) {
      std::fprintf(stderr, "vmf physical-line: %s lost/root-owned Document source=%s\n",
                   test.id, test.source);
      return false;
    }
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) {
        std::fprintf(stderr, "vmf physical-line: %s duplicate node source=%s\n",
                     test.id, test.source);
        owned = false; return;
      }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained)
              std::fprintf(stderr, "vmf physical-line: %s parent=%d CST=%d owner=%p child=%p actual-parent=%p source=%s\n",
                           test.id, parent, contained, static_cast<const void*>(&node),
                           static_cast<const void*>(&child), static_cast<const void*>(child.parent), test.source);
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "vmf physical-line: %s ownership=%d seen=%zu nodes=%zu\n",
                   test.id, owned, seen.size(), r.nodes.size());
      return false;
    }
  }
  const auto r = services.parser->parse("one{}two{a 1}\nthree \"a\nb\"\n");
  const auto* d = r.value_as<vmf::ast::Document>();
  const bool okay = d && d->entries.size() == 3 && d->entries[0]->key == "one" &&
      !d->entries[0]->value && d->entries[0]->children.empty() &&
      d->entries[1]->children.size() == 1 && d->entries[1]->children[0]->key == "a" &&
      d->entries[1]->children[0]->value && *d->entries[1]->children[0]->value == "1" &&
      d->entries[2]->key == "three" && d->entries[2]->value &&
      *d->entries[2]->value == "a\nb";
  if (!okay)
    std::fprintf(stderr, "vmf physical-line: combined readback entries=%zu expected3 accepted=%d recovered=%d parse-diags=%zu\n",
                 d ? d->entries.size() : 0, r.accepted, r.cst().recovered(), r.diagnostics.size());
  return okay;
}
}  // namespace physical_line_cases
