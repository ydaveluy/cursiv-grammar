#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace quadratic_header_cases {
inline bool run(const mps::MpsCoreServices& services) {
  const std::string prefix =
      "NAME Q\nROWS\n N COST\n L LIMIT\nCOLUMNS\n X COST 1 LIMIT 2\nRHS\n R LIMIT 4\n";
  struct Case { const char* header; const char* kind; bool row; bool valid; };
  const Case cases[] = {
      {"QUADOBJ", "QUADOBJ", false, true},
      {"qmatrix", "qmatrix", false, true},
      {"QSECTION COST", "QSECTION", true, true},
      {"qsection COST", "qsection", true, true},
      {"QCMATRIX COST", "QCMATRIX", true, true},
      {"qcmatrix COST", "qcmatrix", true, true},
      {"QSECTION", "QSECTION", false, false},
      {"qsection", "qsection", false, false},
      {"QcSeCtIoN", "QcSeCtIoN", false, false},
      {"QCMATRIX", "QCMATRIX", false, false},
      {"qcmatrix", "qcmatrix", false, false},
      {"QcMaTrIx", "QcMaTrIx", false, false},
  };
  for (const auto& test : cases) {
    const std::string source = prefix + test.header + "\n X X 2\nENDATA\n";
    const auto v = cursiv::test::validationHelper(services, source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != test.valid) {
      std::fprintf(stderr, "mps quadratic header: '%s' expected=%d accepted=%d recovered=%d diagnostics=%zu\n%s",
                   test.header, test.valid, r.accepted, r.cst().recovered(),
                   v.diagnostics.size(), source.c_str());
      return false;
    }
    if (!clean) continue;
    const auto* root = r.value_as<mps::ast::Model>();
    const auto* q = root && root->quadratic.size() == 1 ? root->quadratic[0] : nullptr;
    if (!root || root->parent != v.document.get() || !q || q->kind != test.kind ||
        q->row.has_value() != test.row || (test.row && *q->row != "COST") ||
        q->cards.size() != 1 || q->cards[0]->name != "X" ||
        q->cards[0]->entries.size() != 1 || q->cards[0]->entries[0]->row != "X" ||
        q->cards[0]->entries[0]->value != "2") {
      std::fprintf(stderr, "mps quadratic header: '%s' AST kind/row/cards/entry value lost\n", test.header);
      return false;
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    bool owned = true;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained)
              std::fprintf(stderr, "mps quadratic header: '%s' parent=%d CST=%d\n", test.header, parent, contained);
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "mps quadratic header: '%s' ownership=%d seen=%zu nodes=%zu\n",
                   test.header, owned, seen.size(), r.nodes.size());
      return false;
    }
  }
  return true;
}
}  // namespace quadratic_header_cases
