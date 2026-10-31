#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "plantuml/core/ast.hpp"

namespace plantuml_containers {
inline bool run(const auto& language) {
  namespace a = plantuml::ast;
  bool ok = true;
  auto check = [&](bool value, const char* reason, std::string_view name) {
    if (!value) {
      std::fprintf(stderr, "plantuml container: %s; case=%.*s\n", reason,
                   int(name.size()), name.data());
      ok = false;
    }
  };
  struct Case { const char* name; const char* source; bool clean;
    const char* kind; const char* id; const char* display; const char* stereotype;
    const char* color; unsigned containers, children; };
  const Case cases[] = {
      {"package", "@startuml\npackage P {\nclass A\n}\n@enduml\n", true, "package", "P", "", "", "", 1, 1},
      {"namespace", "@startuml\nnamespace P {\nclass A\n}\n@enduml\n", true, "namespace", "P", "", "", "", 1, 1},
      {"nested", "@startuml\npackage P {\npackage Q {\nclass A\n}\n}\n@enduml\n", true, "package", "P", "", "", "", 2, 1},
      {"mixed", "@startuml\npackage P {\nnamespace Q {\nclass A\n}\n}\n@enduml\n", true, "package", "P", "", "", "", 2, 1},
      {"quoted", "@startuml\npackage \"Display name\" {\nclass A\n}\n@enduml\n", true, "package", "Display name", "", "", "", 1, 1},
      {"alias", "@startuml\npackage \"Display name\" as P {\nclass A\n}\n@enduml\n", true, "package", "P", "Display name", "", "", 1, 1},
      {"reverse-alias", "@startuml\npackage P as \"Display name\" {\nclass A\n}\n@enduml\n", true, "package", "P", "Display name", "", "", 1, 1},
      {"namespace-alias", "@startuml\nnamespace \"Display name\" as P {\nclass A\n}\n@enduml\n", true, "namespace", "P", "Display name", "", "", 1, 1},
      {"stereotype-color", "@startuml\npackage P <<Frame>> #red {\nclass A\n}\n@enduml\n", true, "package", "P", "", "Frame", "#red", 1, 1},
      {"namespace-stereotype-color", "@startuml\nnamespace P <<Folder>> #blue {\nclass A\n}\n@enduml\n", true, "namespace", "P", "", "Folder", "#blue", 1, 1},
      {"hex-color", "@startuml\npackage com.example #DDDDDD {\nclass A\n}\n@enduml\n", true, "package", "com.example", "", "", "#DDDDDD", 1, 1},
      {"empty", "@startuml\npackage P {\n}\n@enduml\n", true, "package", "P", "", "", "", 1, 0},
      {"empty-inline", "@startuml\npackage P {}\n@enduml\n", true, "package", "P", "", "", "", 1, 0},
      {"empty-namespace-inline", "@startuml\nnamespace P <<Folder>> #blue {}\n@enduml\n", true, "namespace", "P", "", "Folder", "#blue", 1, 0},
      {"elements", "@startuml\npackage P {\nclass A {\nfield\n}\nenum E {\nONE\n}\nnote right of A: text\nA --> B\n}\nclass Outside\n@enduml\n", true, "package", "P", "", "", "", 1, 4},
      {"namespace-separator", "@startuml\nnamespace a::b {\nclass A\n}\n@enduml\n", true, "namespace", "a::b", "", "", "", 1, 1},
      {"missing-open", "@startuml\npackage P\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"orphan-close", "@startuml\npackage P {}\n}\n@enduml\n", true, "package", "P", "", "", "", 1, 0},
      {"missing-alias", "@startuml\npackage \"Display\" as {\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"inline-member", "@startuml\npackage P { class A }\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"header-newline", "@startuml\npackage\nP {\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"empty-title", "@startuml\npackage \"\" {\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"stereotype-unclosed", "@startuml\npackage P <<Frame {\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"color-before-stereotype", "@startuml\npackage P #red <<Frame>> {\nclass A\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"root-close-after-class", "@startuml\nclass A\n}\n@enduml\n", true, "", "", "", "", "", 0, 0},
      {"root-close-twice", "@startuml\npackage P {}\n}\n}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"root-close-before-class", "@startuml\npackage P {}\n}\nclass A\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"root-close-before-package", "@startuml\nclass A\n}\npackage P {}\n@enduml\n", false, "", "", "", "", "", 0, 0},
      {"implicit-close", "@startuml\npackage P {\nclass A\n@enduml\n", true, "package", "P", "", "", "", 1, 1},
      {"implicit-nested-close", "@startuml\npackage P {\npackage Q {\nclass A\n@enduml\n", true, "package", "P", "", "", "", 2, 1},
      {"implicit-outer-close", "@startuml\npackage P {\npackage Q {\nclass A\n}\n@enduml\n", true, "package", "P", "", "", "", 2, 1},
      {"opening-brace-next-line", "@startuml\npackage P\n{\nclass A\n}\n@enduml\n", true, "package", "P", "", "", "", 1, 1},
      {"closed-followed-by-class", "@startuml\npackage P {\nclass A\n}\nclass Outside\n@enduml\n", true, "package", "P", "", "", "", 1, 1},
      {"namespace-implicit-close", "@startuml\nnamespace P {\nclass A\n@enduml\n", true, "namespace", "P", "", "", "", 1, 1},
  };
  for (const auto& c : cases) {
    const auto result = language.parser->parse(c.source);
    const bool clean = result.accepted && !result.cst().recovered() &&
                       result.diagnostics.empty();
    check(clean == c.clean, "native named-container syntax boundary", c.name);
    if (!clean || !c.clean) continue;
    const auto* doc = cursiv::ast_cast<a::Document>(result.value);
    const auto* first = doc && !doc->elements.empty()
        ? cursiv::ast_cast<a::ContainerDecl>(doc->elements[0]) : nullptr;
    const std::string_view name(c.name);
    unsigned containers = 0, closed = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::ContainerDecl>(node)) {
        ++containers; closed += value->hasClosingBrace;
      }
    const bool rootClose = name == "orphan-close" || name == "root-close-after-class";
    const unsigned expectedClosed = name == "implicit-close" ||
        name == "implicit-nested-close" || name == "namespace-implicit-close"
        ? 0u : name == "implicit-outer-close" ? 1u : c.containers;
    check(doc && doc->hasRootClose == rootClose && containers == c.containers &&
          closed == expectedClosed, "explicit container and root closing flags", c.name);
    check((!c.containers && !first) || (first && first->kind == c.kind && first->name == c.id &&
          first->displayName == c.display && first->stereotype == c.stereotype &&
          first->color == c.color && first->elements.size() == c.children),
          "container kind, names, decoration and child list retained", c.name);
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = result.value && !result.value->parent;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (result.value) walk(walk, *result.value);
    check(owned && seen.size() == result.nodes.size(),
          "complete unique AST/CST ownership", c.name);
    if (name == "root-close-after-class") {
      const auto* cls = doc && doc->elements.size() == 1
          ? cursiv::ast_cast<a::ClassDecl>(doc->elements[0]) : nullptr;
      check(cls && cls->name == "A", "root close preserves the existing class", c.name);
    }
    if (!first) continue;
    if (name == "nested" || name == "mixed" || name == "implicit-nested-close" ||
        name == "implicit-outer-close") {
      const auto* child = first->elements.size() == 1
          ? cursiv::ast_cast<a::ContainerDecl>(first->elements[0]) : nullptr;
      const auto* cls = child && child->elements.size() == 1
          ? cursiv::ast_cast<a::ClassDecl>(child->elements[0]) : nullptr;
      check(child && child->name == "Q" &&
            child->kind == (name == "mixed" ? "namespace" : "package") &&
            cls && cls->name == "A", "recursive container hierarchy retained", c.name);
      if (name == "implicit-outer-close")
        check(!first->hasClosingBrace && child && child->hasClosingBrace,
              "explicit inner close and implicit outer close remain distinct", c.name);
    } else if (name == "closed-followed-by-class") {
      const auto* child = first->elements.size() == 1
          ? cursiv::ast_cast<a::ClassDecl>(first->elements[0]) : nullptr;
      const auto* outside = doc->elements.size() == 2
          ? cursiv::ast_cast<a::ClassDecl>(doc->elements[1]) : nullptr;
      check(child && child->name == "A" && outside && outside->name == "Outside",
            "explicit close restores the document scope", c.name);
    } else if (name == "elements") {
      const auto* cls = first->elements.size() == 4
          ? cursiv::ast_cast<a::ClassDecl>(first->elements[0]) : nullptr;
      const auto* en = first->elements.size() == 4
          ? cursiv::ast_cast<a::EnumDecl>(first->elements[1]) : nullptr;
      const auto* note = first->elements.size() == 4
          ? cursiv::ast_cast<a::NoteDecl>(first->elements[2]) : nullptr;
      const auto* link = first->elements.size() == 4
          ? cursiv::ast_cast<a::Association>(first->elements[3]) : nullptr;
      const auto* outside = doc->elements.size() == 2
          ? cursiv::ast_cast<a::ClassDecl>(doc->elements[1]) : nullptr;
      check(cls && cls->name == "A" && cls->body && en && en->name == "E" &&
            en->body && note && note->target == "A" && note->text == " text" &&
            link && link->left == "A" && link->right == "B" && link->op == "-->" &&
            outside && outside->name == "Outside",
            "structured children retain source order and parent scope", c.name);
    } else if (c.children == 1) {
      const auto* cls = cursiv::ast_cast<a::ClassDecl>(first->elements[0]);
      check(cls && cls->name == "A", "class child retained", c.name);
    }
  }
  std::puts("plantuml container: 24 valid, 10 invalid CLASS forms; recursive AST and CST ownership");
  return ok;
}
} // namespace plantuml_containers
