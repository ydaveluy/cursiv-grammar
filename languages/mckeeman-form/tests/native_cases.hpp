#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <unordered_set>
inline int checkNativeCases(const auto& language) {
  {
    const std::string source = "root\n    'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "\nroot\n    'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = " \t\nroot\n    'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = " root\n    'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'a'\nnext\n    root\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 2) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    const auto* p1 = document->rules[1];
    if (p1->name != "next" || p1->alternatives.size() != 1) return 1;
    const auto* a1_0 = p1->alternatives[0];
    if (a1_0->items.size() != 1) return 1;
    const auto* x1_0_0 = std::get_if<mckeemanform::ast::RuleRef*>(&a1_0->items[0]);
    if (!x1_0_0) return 1;
    if ((*x1_0_0)->target.refText != "root") return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'a'\n\nnext\n    root\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 2) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    const auto* p1 = document->rules[1];
    if (p1->name != "next" || p1->alternatives.size() != 1) return 1;
    const auto* a1_0 = p1->alternatives[0];
    if (a1_0->items.size() != 1) return 1;
    const auto* x1_0_0 = std::get_if<mckeemanform::ast::RuleRef*>(&a1_0->items[0]);
    if (!x1_0_0) return 1;
    if ((*x1_0_0)->target.refText != "root") return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'a'\n\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'é'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "é" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    '中'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "中" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    '''\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "'" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    '0000' . '10FFFF' - 'a' - 'b' . 'd'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "0000" || bool(rx0_0_0->to) != true || rx0_0_0->excludes.size() != 2 || *rx0_0_0->to != "10FFFF") return 1;
    if (rx0_0_0->excludes[0]->from != "a" || bool(rx0_0_0->excludes[0]->to) != false || rx0_0_0->excludes[0]->excludes.size() != 0) return 1;
    if (rx0_0_0->excludes[1]->from != "b" || bool(rx0_0_0->excludes[1]->to) != true || rx0_0_0->excludes[1]->excludes.size() != 0 || *rx0_0_0->excludes[1]->to != "d") return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    \"a b é\" root\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 2) return 1;
    const auto* x0_0_0 = std::get_if<std::string_view>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    if (*x0_0_0 != "a b é") return 1;
    const auto* x0_0_1 = std::get_if<mckeemanform::ast::RuleRef*>(&a0_0->items[1]);
    if (!x0_0_1) return 1;
    if ((*x0_0_1)->target.refText != "root") return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    \"\"\n    'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 2) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<std::string_view>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    if (*x0_0_0 != "") return 1;
    const auto* a0_1 = p0->alternatives[1];
    if (a0_1->items.size() != 1) return 1;
    const auto* x0_1_0 = std::get_if<mckeemanform::ast::Range*>(&a0_1->items[0]);
    if (!x0_1_0) return 1;
    const auto* rx0_1_0 = *x0_1_0;
    if (rx0_1_0->from != "a" || bool(rx0_1_0->to) != false || rx0_1_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'a'\n    'b'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 2) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    const auto* a0_1 = p0->alternatives[1];
    if (a0_1->items.size() != 1) return 1;
    const auto* x0_1_0 = std::get_if<mckeemanform::ast::Range*>(&a0_1->items[0]);
    if (!x0_1_0) return 1;
    const auto* rx0_1_0 = *x0_1_0;
    if (rx0_1_0->from != "b" || bool(rx0_1_0->to) != false || rx0_1_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    '0041'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "0041" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'GGGG'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "GGGG" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    bool found = false;
    for (const auto& d : validation.diagnostics) found |= d.code && d.code->isString() && d.code->string() == "mckeemanform.codepoint";
    if (!found) return 1;
  }
  {
    const std::string source = "root\n    'D800'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "D800" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    bool found = false;
    for (const auto& d : validation.diagnostics) found |= d.code && d.code->isString() && d.code->string() == "mckeemanform.codepoint";
    if (!found) return 1;
  }
  {
    const std::string source = "root\n    'z' . 'a'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "z" || bool(rx0_0_0->to) != true || rx0_0_0->excludes.size() != 0 || *rx0_0_0->to != "a") return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    bool found = false;
    for (const auto& d : validation.diagnostics) found |= d.code && d.code->isString() && d.code->string() == "mckeemanform.range-order";
    if (!found) return 1;
  }
  {
    const std::string source = "root\n    'a' 'b'\n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 2) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    const auto* x0_0_1 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[1]);
    if (!x0_0_1) return 1;
    const auto* rx0_0_1 = *x0_0_1;
    if (rx0_0_1->from != "b" || bool(rx0_0_1->to) != false || rx0_0_1->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  {
    const std::string source = "root\n    'a' \n";
    const auto result = language.language.parser->parse(source);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty()) return 1;
    const auto* document = cursiv::ast_cast<mckeemanform::ast::Document>(result.value);
    if (!document || document->rules.size() != 1) return 1;
    const auto* p0 = document->rules[0];
    if (p0->name != "root" || p0->alternatives.size() != 1) return 1;
    const auto* a0_0 = p0->alternatives[0];
    if (a0_0->items.size() != 1) return 1;
    const auto* x0_0_0 = std::get_if<mckeemanform::ast::Range*>(&a0_0->items[0]);
    if (!x0_0_0) return 1;
    const auto* rx0_0_0 = *x0_0_0;
    if (rx0_0_0->from != "a" || bool(rx0_0_0->to) != false || rx0_0_0->excludes.size() != 0) return 1;
    std::unordered_set<const cursiv::AstNode*> seen;
    bool parents = result.value->parent == nullptr;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { parents = false; return; }
      language.language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        parents &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
        self(self, child);
      });
    };
    visit(visit, *result.value);
    if (!parents || seen.size() != result.nodes.size()) return 1;
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (!validation.diagnostics.empty()) return 1;
  }
  return 0;
}
