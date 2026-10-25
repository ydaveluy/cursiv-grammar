#pragma once
// Boundary cases frozen from python-semver 3.1.0 (2026-10-25).
// The complete 199-case oracle corpus and executable receipts live in audit/.
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include <unordered_set>

namespace native_conformance {
inline bool run() {
  auto language = cursiv::create_language<semver::SemverCoreServices,
                                         semver::SemverSharedCoreServices,
                                         semver::install_core_module>();
  auto factory = semver::generated::Semver::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) std::fprintf(stderr, "native SemVer: %s\n", message);
    ok &= value;
  };
  for (std::string_view text : {"0.0.0", "123.456.789", "1.2.3-alpha", "1.2.3--",
       "1.2.3-0", "1.2.3-1.0.10", "1.2.3-01a", "1.2.3-x.000a", "1.2.3+a.001",
       "1.2.3+---", "1.2.3-alpha.0+build.001", "18446744073709551616.0.1",
       "9007199254740993.0.1"}) {
    auto r = language.language.parser->parse(text);
    auto f = factory->parse(text);
    const auto *v = r.value_as<semver::ast::Semver>();
    const auto *w = f.value_as<semver::ast::Semver>();
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty() && v && v->core,
          "native positive parses cleanly");
    if (!v || !v->core) continue;
    auto spelling = [](const semver::ast::Semver &n) {
      std::string s(n.core->major);
      s += "."; s += n.core->minor; s += "."; s += n.core->patch;
      for (auto [prefix, seg] : {std::pair{'-', n.preRelease}, std::pair{'+', n.build}})
        if (seg) {
          s += prefix;
          for (std::size_t i = 0; i < seg->identifiers.size(); ++i) {
            if (i) s += '.';
            s += seg->identifiers[i];
          }
        }
      return s;
    };
    check(spelling(*v) == text && r.cst().text() == text, "raw AST roundtrip");
    check(w && w->core && f.accepted && !f.cst().recovered() && spelling(*w) == text,
          "generated factory agrees");
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      check(seen.insert(&n).second, "unique AST ownership");
      language.language.astReflection->visitMembers(n,
        [&](std::string_view, std::uint32_t, cursiv::AstNode &c) {
          check(c.parent == &n && r.cst().contains(n.cstNodeId, c.cstNodeId), "parent/CST containment");
          self(self, c);
        });
    };
    walk(walk, *v);
    check(seen.size() == r.nodes.size(), "all AST nodes reachable");
    check(cursiv::test::validationHelper(language.language, std::string(text)).diagnostics.empty(),
          "native positive validates");
  }
  for (std::string_view text : {"", "1", "1.2", "v1.2.3", " 1.2.3", "1.2.3\n",
       "1.2.3-", "1.2.3+", "1.2.3-a..b", "1.2.3-a_b", "1.2.3-å", "1.2.3-١"}) {
    const auto r = language.language.parser->parse(text);
    check(!r.accepted || r.cst().recovered() || !r.diagnostics.empty(), "native syntax rejection");
  }
  const std::string_view invalid = "00.01.002-alpha.00.01";
  const auto validation = cursiv::test::validationHelper(language.language, std::string(invalid));
  const std::pair<std::size_t, std::size_t> spans[] = {{0,2}, {3,5}, {6,9}, {16,18}, {19,21}};
  check(validation.diagnostics.size() == 5, "every invalid numeric identifier diagnosed");
  for (std::size_t i = 0; i < validation.diagnostics.size() && i < 5; ++i) {
    const auto &d = validation.diagnostics[i];
    check(d.severity == cursiv::DiagnosticSeverity::Error && d.range.begin == spans[i].first &&
          d.range.end == spans[i].second && d.code && d.code->isString() &&
          d.code->string() == (i < 3 ? "semver.core-leading-zero" : "semver.prerelease-leading-zero"),
          "diagnostic code and exact identifier span");
  }
  return ok;
}
} // namespace native_conformance
