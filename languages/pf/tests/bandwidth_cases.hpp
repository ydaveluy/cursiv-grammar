#pragma once

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"

namespace bandwidth_cases {
inline bool run(const pf::PfCoreServices& services) {
  const auto check = [&](const char* id, const std::string& source, bool valid,
                         double value, std::string_view unit,
                         bool burst = false, double burstValue = 0,
                         std::string_view burstUnit = {}) -> bool {
    const auto v = cursiv::test::validationHelper(services, source);
    const auto& r = v.document->parseResult;
    const bool clean = r.accepted && !r.cst().recovered() && v.diagnostics.empty();
    if (clean != valid) {
      std::fprintf(stderr, "pf bandwidth: %s expected=%d accepted=%d recovered=%d diagnostics=%zu source=%s\n",
                   id, valid, r.accepted, r.cst().recovered(), v.diagnostics.size(), source.c_str());
      return false;
    }
    if (!clean) return true;
    const auto* root = r.value_as<pf::ast::File>();
    const auto* entry = root && root->lines.size() == 1
        ? std::get_if<pf::ast::QueueRule*>(&root->lines[0]) : nullptr;
    const auto* bw = entry && *entry && (*entry)->opts.size() == 1
        ? (*entry)->opts[0]->bw : nullptr;
    if (!root || root->parent != v.document.get() || !bw || bw->value != value ||
        bw->unit != unit || bw->burstValue.has_value() != burst ||
        (burst && (*bw->burstValue != burstValue || bw->burstUnit != burstUnit ||
                   !bw->burstMs || *bw->burstMs != 10))) {
      std::fprintf(stderr, "pf bandwidth: %s AST value/unit/burst lost bw=%d value=%g expected=%g unit='%s' expected='%s' burst=%d source=%s\n",
                   id, bw != nullptr, bw ? bw->value : 0, value,
                   bw ? std::string(bw->unit).c_str() : "<null>",
                   std::string(unit).c_str(), bw && bw->burstValue.has_value(), source.c_str());
      return false;
    }
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      services.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            const bool parent = child.parent == &node;
            const bool contained = r.cst().contains(node.cstNodeId, child.cstNodeId);
            if (!parent || !contained)
              std::fprintf(stderr, "pf bandwidth: %s parent=%d CST=%d source=%s\n", id, parent, contained, source.c_str());
            owned &= parent && contained;
            self(self, child);
          });
    };
    walk(walk, *root);
    if (!owned || seen.size() != r.nodes.size()) {
      std::fprintf(stderr, "pf bandwidth: %s ownership=%d seen=%zu nodes=%zu source=%s\n",
                   id, owned, seen.size(), r.nodes.size(), source.c_str());
      return false;
    }
    return true;
  };
  for (const auto unit : {"b", "K", "KB", "KBit", "Kbit", "Kb", "M", "MB",
                          "MBit", "Mbit", "Mb", "G", "GB", "GBit", "Gbit", "Gb"})
    if (!check(unit, std::string("queue root on em0 bandwidth 1") + unit + "\n", true, 1, unit)) return false;
  struct Case { const char* spec; double value; const char* unit; bool burst; double burstValue; const char* burstUnit; };
  const Case cases[] = {
      {"1000", 1000, "", false, 0, ""},
      {"1.5Mbit", 1.5, "Mbit", false, 0, ""},
      {"1e3K", 1000, "K", false, 0, ""},
      {"1.M", 1, "M", false, 0, ""},
      {"1M burst 2K for 10ms", 1, "M", true, 2, "K"},
      {"1M burst 2 for 10ms", 1, "M", true, 2, ""},
      {"1M burst 2.5Kbit for 10ms", 1, "M", true, 2.5, "Kbit"},
  };
  for (const auto& c : cases)
    if (!check(c.spec, std::string("queue root on em0 bandwidth ") + c.spec + "\n",
               true, c.value, c.unit, c.burst, c.burstValue, c.burstUnit)) return false;
  for (const auto spec : {"1 B", "1B", "1k", "1MBitX", "1 Mbit", "1.5", "1e3",
                          "1MK", "1M bit", "1M burst 2K for 10 ms",
                          "1M burst 2K for 10MS", "1M burst 2.5 for 10ms",
                          "1M burst 2 K for 10ms"})
    if (!check(spec, std::string("queue root on em0 bandwidth ") + spec + "\n", false, 0, "")) return false;
  const char* path = CURSIV_TEST_DIR "/native-invalid/tables_queues_anchors-spaced-ms.pf";
  std::ifstream file(path);
  if (!file) { std::fprintf(stderr, "pf bandwidth: cannot read %s\n", path); return false; }
  const std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  return check("original-spaced-ms-fixture", source, false, 0, "");
}
}  // namespace bandwidth_cases
