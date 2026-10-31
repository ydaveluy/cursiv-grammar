#pragma once

#include <cstdio>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "mdx/core/ast.hpp"

namespace mdx_query_axes {
inline bool run(const auto& services) {
  struct Case { const char* id; const char* source; bool valid; };
  const Case cases[] = {
      {"having", "SELECT {} HAVING [Measures].[Value]>0 ON 0 FROM Sales", true},
      {"properties-having", "SELECT NON EMPTY {} DIMENSION PROPERTIES [Name] HAVING [Measures].[Value]>0 ON AXIS(1-1) FROM Sales", true},
      {"axis-expression", "SELECT {} ON AXIS(1-1) FROM Sales", true},
      {"bare-expression", "SELECT {} ON 1-1 FROM Sales", true},
      {"parenthesized-expression", "SELECT {} ON AXIS((0)) FROM Sales", true},
      {"unknown-lower-axis", "SELECT {} ON AXIS([AxisNumber]), {} ON ROWS FROM Sales", true},
      {"lowercase-keyword", "select {} having 1>0 on axis(0) from Sales", true},
      {"literal-api", "SELECT {} ON AXIS(0) FROM Sales", true},
      {"alias-api", "SELECT {} ON COLUMNS FROM Sales", true},
      {"ordinal-comment", "SELECT {} ON 0 /* position */ FROM Sales", true},
      {"bare-name-expression", "SELECT {} ON [AxisNumber] FROM Sales", true},
      {"missing-having-expression", "SELECT {} HAVING ON 0 FROM Sales", false},
      {"double-having", "SELECT {} HAVING 1 HAVING 2 ON 0 FROM Sales", false},
      {"having-after-on", "SELECT {} ON 0 HAVING 1 FROM Sales", false},
      {"missing-axis-expression", "SELECT {} ON AXIS() FROM Sales", false},
      {"unfinished-axis-expression", "SELECT {} ON AXIS(1-) FROM Sales", false},
      {"unfinished-bare-expression", "SELECT {} ON 1- FROM Sales", false},
      {"axis-comma-expression", "SELECT {} ON AXIS(0,1) FROM Sales", false},
      {"trailing-axis-comma", "SELECT {} ON 0, FROM Sales", false},
      {"duplicate-known", "SELECT {} ON COLUMNS, {} ON AXIS(0) FROM Sales", false},
      {"gap-known", "SELECT {} ON ROWS FROM Sales", false},
      {"range-known", "SELECT {} ON AXIS(128) FROM Sales", false},
      {"invalid-alias", "SELECT {} ON UNKNOWN FROM Sales", false},
  };
  for (const auto& test : cases) {
    const auto v = cursiv::test::validationHelper(services, test.source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "mdx query axes: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   test.id, test.valid, r.accepted, r.cst().recovered(), v.diagnostics.size(), test.source);
      for (const auto& d : v.diagnostics) std::fprintf(stderr, "  %s\n", d.message.c_str());
      return false;
    }
    if (!clean) continue;
    const auto* root = r.template value_as<mdx::ast::SelectStatement>();
    if (!root || root->parent != v.document.get() || root->axes.empty()) {
      std::fprintf(stderr, "mdx query axes: %s root/Document/axes lost\n", test.id);
      return false;
    }
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto describe = [&](const char* role, const cursiv::AstNode& node) {
      const auto type = services.astReflection->getTypeMetaData(node.type).name;
      std::fprintf(stderr, "mdx query axes: %s %s type=%.*s cst=%u",
                   test.id, role, int(type.size()), type.data(), unsigned(node.cstNodeId));
      if (node.cstNodeId < r.cst().size()) {
        const auto& cst = r.cst()[node.cstNodeId];
        std::fprintf(stderr, " bytes=%u..%u", cst.begin, cst.end);
      }
      std::fputc('\n', stderr);
    };
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; describe("duplicate", node); return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view member, std::uint32_t index, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = node.cstNodeId < r.cst().size() && child.cstNodeId < r.cst().size() &&
                r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained) {
              std::fprintf(stderr, "mdx query axes: %s member=%.*s index=%u parent=%d contained=%d\n",
                           test.id, int(member.size()), member.data(), index, parent, contained);
              describe("owner", node); describe("child", child);
            }
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "mdx query axes: %s ownership=%d seen=%zu arena=%zu\n",
                   test.id, owned, seen.size(), r.nodes.size());
      return false;
    }
    const auto* axis = root->axes[0];
    if (std::string_view(test.id) == "properties-having") {
      const auto* ordinal = cursiv::ast_cast<mdx::ast::BinaryExpr>(axis->axisExpression);
      const auto* having = cursiv::ast_cast<mdx::ast::BinaryExpr>(axis->having);
      const auto* left = ordinal ? cursiv::ast_cast<mdx::ast::NumberLiteral>(ordinal->left) : nullptr;
      const auto* right = ordinal ? cursiv::ast_cast<mdx::ast::NumberLiteral>(ordinal->right) : nullptr;
      if (!axis->nonEmpty || !axis->axisFunction || !axis->axisName.empty() ||
          axis->dimProps.size() != 1 || axis->dimProps[0]->parts.size() != 1 ||
          axis->dimProps[0]->parts[0] != "Name" || !ordinal ||
          ordinal->op != mdx::ast::BinaryOp::subtract || !left || left->value != 1 ||
          !right || right->value != 1 || !having || having->op != mdx::ast::BinaryOp::gt) {
        std::fprintf(stderr, "mdx query axes: %s HAVING/properties/ordinal-expression AST lost\n", test.id);
        return false;
      }
    }
    if (std::string_view(test.id) == "literal-api" || std::string_view(test.id) == "alias-api") {
      const bool wrapper = std::string_view(test.id) == "literal-api";
      if (axis->axisExpression || axis->having || axis->axisFunction != wrapper ||
          axis->axisName != (wrapper ? "0" : "COLUMNS")) {
        std::fprintf(stderr, "mdx query axes: %s legacy axisName/axisFunction API lost\n", test.id);
        return false;
      }
    }
  }
  return true;
}
}  // namespace mdx_query_axes
