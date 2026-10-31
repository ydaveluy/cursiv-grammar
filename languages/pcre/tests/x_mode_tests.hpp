#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "pcre/core/ast.hpp"
#include "x_mode_cases.hpp"

namespace pcre_x_modes {
template <class Language> bool run(const Language& language) {
  namespace ast = pcre::ast;
  const auto& parser = *language.language.parser;
  bool ok = true;
  for (const auto& test : cases) {
    const auto parsed = parser.parse(test.source);
    const auto* root = parsed.template value_as<ast::Pcre>();
    std::string literals;
    unsigned options = 0, groups = 0, classes = 0, quantified = 0;
    bool parents = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode* node,
                          const cursiv::AstNode* parent) -> void {
      if (!node || !seen.insert(node).second) { parents = false; return; }
      parents &= node->parent == parent;
      const auto child = [&](const auto* value) { if (value) self(self, value, node); };
      const auto children = [&](const auto& values) {
        for (const auto* value : values) self(self, value, node);
      };
      if (const auto* value = cursiv::ast_cast<ast::Pcre>(node)) child(value->pattern);
      if (const auto* value = cursiv::ast_cast<ast::Alternation>(node)) children(value->branches);
      if (const auto* value = cursiv::ast_cast<ast::Sequence>(node)) children(value->items);
      if (const auto* value = cursiv::ast_cast<ast::Literal>(node)) literals += value->text;
      options += cursiv::ast_cast<ast::OptionSetting>(node) != nullptr;
      if (const auto* value = cursiv::ast_cast<ast::Group>(node)) {
        ++groups; child(value->body);
      }
      if (const auto* value = cursiv::ast_cast<ast::CharacterClass>(node)) {
        ++classes; children(value->atoms);
      }
      if (const auto* value = cursiv::ast_cast<ast::CharacterClassRange>(node)) {
        child(value->from); child(value->to);
      }
      if (const auto* value = cursiv::ast_cast<ast::Quantified>(node)) {
        ++quantified; child(value->operand); children(value->annotations);
      }
      if (const auto* value = cursiv::ast_cast<ast::ConditionalPattern>(node)) {
        child(value->conditionNode); child(value->thenBranch); child(value->elseBranch);
      }
    };
    if (root) walk(walk, root, nullptr);
    const bool valid = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
        parsed.cst().text() == test.source && root && root->pattern &&
        root->pattern->branches.size() == test.rootBranches &&
        literals == test.literals && options == test.options && groups == test.groups &&
        classes == test.classes && quantified == test.quantified && parents &&
        seen.size() == parsed.nodes.size();
    if (!valid) {
      ok = false;
      std::fprintf(stderr, "pcre modes [%.*s]: accepted=%d recovered=%d diagnostics=%zu "
          "literals=[%s] options=%u groups=%u classes=%u quantified=%u parents=%d nodes=%zu/%zu\n",
          int(test.name.size()), test.name.data(), parsed.accepted, parsed.cst().recovered(),
          parsed.diagnostics.size(), literals.c_str(), options, groups, classes, quantified,
          parents, seen.size(), parsed.nodes.size());
      for (const auto& diagnostic : parsed.diagnostics)
        std::fprintf(stderr, "pcre modes diagnostic: kind=%u element=%u bytes=%u..%u\n",
            unsigned(diagnostic.kind), unsigned(diagnostic.element),
            unsigned(diagnostic.begin), unsigned(diagnostic.end));
    }
  }
  for (const auto& test : invalid) {
    const auto result = parser.parse(test.source);
    const auto checked = cursiv::test::validationHelper(language.language, std::string(test.source));
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        checked.diagnostics.empty()) {
      std::fprintf(stderr, "pcre modes [%.*s]: native-invalid input remained valid\n",
                   int(test.name.size()), test.name.data());
      ok = false;
    }
  }
  // Scoped flag captures and standalone spelling must survive mode selection.
  {
    const auto result = parser.parse("(?x)(?xx-i:a b)(?-xx:c d)(?^xx:e f)");
    const auto* root = result.template value_as<ast::Pcre>();
    const auto* seq = root && root->pattern && root->pattern->branches.size() == 1
        ? root->pattern->branches[0] : nullptr;
    const auto* first = seq && seq->items.size() == 4
        ? cursiv::ast_cast<ast::Group>(seq->items[1]) : nullptr;
    const auto* second = seq && seq->items.size() == 4
        ? cursiv::ast_cast<ast::Group>(seq->items[2]) : nullptr;
    const auto* third = seq && seq->items.size() == 4
        ? cursiv::ast_cast<ast::Group>(seq->items[3]) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !first || first->kind != ast::GroupKind::non_capturing || first->resetFlags ||
        first->onFlags != "xx" || first->offFlags != "i" ||
        !second || second->onFlags != "" || second->offFlags != "xx" ||
        !third || !third->resetFlags || third->onFlags != "xx" || third->offFlags != "") {
      std::fprintf(stderr, "pcre modes: scoped raw flags/reset/type lost\n"); ok = false;
    }
  }
  {
    const auto result = parser.parse("(?x)(?xx)(?-x)(?^)(?)");
    const auto* root = result.template value_as<ast::Pcre>();
    const auto* seq = root && root->pattern && root->pattern->branches.size() == 1
        ? root->pattern->branches[0] : nullptr;
    constexpr std::string_view spellings[] = {"x", "xx", "-x", "^", ""};
    bool retained = result.accepted && !result.cst().recovered() && result.diagnostics.empty() &&
        seq && seq->items.size() == 5;
    if (seq && seq->items.size() == 5)
      for (unsigned i = 0; i < 5; ++i) {
        const auto* setting = cursiv::ast_cast<ast::OptionSetting>(seq->items[i]);
        retained &= setting && setting->text == spellings[i] && setting->parent == seq;
      }
    if (!retained) { std::fprintf(stderr, "pcre modes: standalone option spelling lost\n"); ok = false; }
  }
  if (ok) std::puts("pcre modes:42 source/AST/ownership forms,6 invalid neighbours and raw flag controls pass");
  return ok;
}
}  // namespace pcre_x_modes
