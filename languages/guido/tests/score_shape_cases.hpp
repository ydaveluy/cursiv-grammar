#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace score_shape_cases {
inline bool run(const auto& language) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
      {"voice", "[c]", true},
      {"empty-score", "{}", true},
      {"empty-voice", "[]", true},
      {"one-voice-score", "{[c]}", true},
      {"two-voices", "{[c], [d]}", true},
      {"voices-comments", "{[c] %voice\n, (*nested(*x*)*) [d]}", true},
      {"integer-variable", "$n=-12; [c]", true},
      {"real-variable", "$r=+.5; [c]", true},
      {"string-variable", "$s=\"text\"; []", true},
      {"tag-id", "[\\tag:007<dx=2hs> c]", true},
      {"bar-parameter", "[|<2> c]", true},
      {"bar-range", "[|(c)]", true},
      {"unit-prefix-name", "[\\tag<cmValue=2cm>]", true},
      {"unit-prefix-bare", "[\\tag<msValue,s1>]", true},
      {"unit-string", "[\\tag<\"cm\">]", true},
      {"missing-voice-comma", "{[c] [d]}", false},
      {"leading-voice-comma", "{,[c]}", false},
      {"trailing-voice-comma", "{[c],}", false},
      {"bare-voice-list", "[c],[d]", false},
      {"two-scores", "{[c]}{[d]}", false},
      {"two-single-voices", "[c][d]", false},
      {"bare-variable", "$x=bare; []", false},
      {"variable-variable", "$x=$y; []", false},
      {"unit-variable", "$x=2cm; []", false},
      {"named-variable", "$x=name=2; []", false},
      {"bar-id", "[|:1]", false},
      {"missing-value", "$x=; []", false},
      {"missing-score", "$x=1;", false},
      {"bare-m", "[\\tag<m>]", false},
      {"bare-cm", "[\\tag<cm>]", false},
      {"bare-mm", "[\\tag<mm>]", false},
      {"bare-in", "[\\tag<in>]", false},
      {"bare-pt", "[\\tag<pt>]", false},
      {"bare-pc", "[\\tag<pc>]", false},
      {"bare-hs", "[\\tag<hs>]", false},
      {"bare-rl", "[\\tag<rl>]", false},
      {"bare-ms", "[\\tag<ms>]", false},
      {"bare-s", "[\\tag<s>]", false},
      {"unit-name", "[\\tag<cm=2>]", false},
      {"milliseconds-name", "[\\tag<ms=2>]", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(language.language, test.source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "guido score-shape: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(),
                   v.diagnostics.size(), test.source);
      return false;
    }
    if (!clean) continue;
    const auto* root = r.template value_as<guido::ast::Prog>();
    if (!root || root->parent != v.document.get() ||
        root->segments.size() + root->sequences.size() != 1) {
      std::fprintf(stderr, "guido score-shape: %s root=%p parent=%p segments=%zu sequences=%zu\n",
                   test.id, static_cast<const void*>(root),
                   root ? static_cast<const void*>(root->parent) : nullptr,
                   root ? root->segments.size() : 0, root ? root->sequences.size() : 0);
      return false;
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto visit = [&](auto&& self, const cursiv::AstNode* node) -> bool {
      if (!node || !seen.insert(node).second) {
        std::fprintf(stderr, "guido score-shape: %s duplicate/null node=%p\n",
                     test.id, static_cast<const void*>(node));
        return false;
      }
      bool okay = true;
      language.language.astReflection->visitMembers(*node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            const bool parent = child.parent == node;
            const bool contained = r.cst().contains(node->cstNodeId, child.cstNodeId);
            if (!parent || !contained)
              std::fprintf(stderr, "guido score-shape: %s parent=%d CST=%d owner=%p child=%p actual-parent=%p\n",
                           test.id, parent, contained, static_cast<const void*>(node),
                           static_cast<const void*>(&child), static_cast<const void*>(child.parent));
            okay &= parent && contained && self(self, &child);
          });
      return okay;
    };
    if (!visit(visit, root) || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "guido score-shape: %s seen=%zu parser-nodes=%zu\n",
                   test.id, seen.size(), r.nodes.size());
      return false;
    }
  }
  const auto result = language.language.parser->parse(
      "$n=-12; $r=+.5; $s=\"text\"; {[c], [d]}");
  const auto* root = result.template value_as<guido::ast::Prog>();
  if (!root || root->variables.size() != 3 || root->segments.size() != 1 ||
      root->segments[0]->sequences.size() != 2 || !root->sequences.empty()) {
    std::fprintf(stderr, "guido score-shape: combined variables/voices structure lost\n");
    return false;
  }
  const auto* n = root->variables[0]->value;
  const auto* real = root->variables[1]->value;
  const auto* text = root->variables[2]->value;
  const bool okay = n && real && text && n->text == "-12" && !n->real && !n->quoted &&
      real->text == "+.5" && real->real && !real->quoted &&
      text->text == "text" && text->quoted && !text->real &&
      n->parent == root->variables[0] && real->parent == root->variables[1] &&
      text->parent == root->variables[2];
  if (!okay)
    std::fprintf(stderr, "guido score-shape: variable values n='%s' real='%s' text='%s' realflags=%d/%d/%d quotedflags=%d/%d/%d parents=%d/%d/%d\n",
                 n ? std::string(n->text).c_str() : "<null>",
                 real ? std::string(real->text).c_str() : "<null>",
                 text ? std::string(text->text).c_str() : "<null>",
                 n && n->real, real && real->real, text && text->real,
                 n && n->quoted, real && real->quoted, text && text->quoted,
                 n && n->parent == root->variables[0],
                 real && real->parent == root->variables[1],
                 text && text->parent == root->variables[2]);
  return okay;
}
}  // namespace score_shape_cases
