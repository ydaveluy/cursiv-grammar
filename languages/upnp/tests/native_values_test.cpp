#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "upnp/core/CoreServices.hpp"
#include <cstdio>
#include <string_view>
#include <unordered_set>
int main() {
  const auto lang = cursiv::create_language<upnp::UpnpCoreServices,
                                            upnp::UpnpSharedCoreServices,
                                            upnp::install_core_module>();
  bool ok = true;
  auto check = [&](bool yes, const char *id, const char *what) {
    if (!yes) {
      std::fprintf(stderr, "upnp %s: %s\n", id, what);
      ok = false;
    }
  };
  for (const auto &c : cases) {
    std::string_view text = c.source;
    auto r = lang.language.parser->parse(text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(), c.name,
          "clean parse");
    check(r.cst().text() == text, c.name, "exact source");
    auto *d = cursiv::ast_cast<upnp::ast::SearchCrit>(r.value);
    check(d, c.name, "SearchCrit root");
    if (!d)
      continue;
    if (c.expected)
      check(audit::view(*d).toJsonString({.pretty = false}) == c.expected,
            c.name, "native values/operators/tree");
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *x = cursiv::ast_cast<upnp::ast::Comparison>(&n))
        check(x->raw.size() >= 2 && text.find(x->raw) != text.npos, c.name,
              "quoted source capture");
      lang.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    check(owned && seen.size() == r.nodes.size(), c.name, "complete ownership");
    auto v = cursiv::test::validationHelper(lang.language, std::string(text));
    const bool nul = std::string_view(c.name) == "escaped-zero";
    check(nul ? v.diagnostics.size() == 1 : v.diagnostics.empty(), c.name,
          "runtime count");
    if (nul && v.diagnostics.size() == 1) {
      const auto &x = v.diagnostics[0];
      check(x.code && x.code->isString() &&
                x.code->string() == "upnp.value-nul" &&
                x.range.begin < x.range.end && x.range.end <= text.size(),
            c.name, "NUL diagnostic");
    }
  }
  return ok ? 0 : 1;
}
