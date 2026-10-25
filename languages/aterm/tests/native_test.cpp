#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "aterm/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language = cursiv::create_language<aterm::AtermCoreServices,
                                                aterm::AtermSharedCoreServices,
                                                aterm::install_core_module>();
  unsigned equal = 0, policies = 0, limitations = 0, negative = 0;
  for (const auto &c : nativeCases) {
    auto fail = [&](const char *w) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), w);
      return 1;
    };
    std::ifstream in(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                         std::string(c.file),
                     std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(in)), {});
    const auto r = language.language.parser->parse(source);
    const auto *d = cursiv::ast_cast<aterm::ast::Doc>(r.value);
    const bool clean =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (r.cst().text() != source || clean != c.clean || !d)
      return fail("parse/source");
    bool raw = true, owned = !d->parent;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto inside = [&](std::string_view v) {
      if (v.empty())
        return true;
      auto lo = reinterpret_cast<std::uintptr_t>(source.data()),
           p = reinterpret_cast<std::uintptr_t>(v.data());
      return p >= lo && p - lo <= source.size() &&
             v.size() <= source.size() - (p - lo);
    };
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *f = cursiv::ast_cast<aterm::ast::Function>(&n))
        raw &= inside(f->afun);
      if (auto *v = cursiv::ast_cast<aterm::ast::Number>(&n))
        raw &= inside(v->raw);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *d);
    if (!raw || !owned || seen.size() != r.nodes.size())
      return fail("ownership/raw fields");
    if (!clean) {
      if (!c.native.empty())
        ++limitations;
      else
        ++negative;
      if (r.diagnostics.empty())
        return fail("negative diagnosis");
      continue;
    }
    const auto actual = view(d->term, r.cst()).toJsonString({.pretty = false});
    if (!c.rawPolicy.empty()) {
      ++policies;
      const auto *n = cursiv::ast_cast<aterm::ast::Number>(d->term);
      if (!n || n->raw != c.rawPolicy || actual == c.native)
        return fail("explicit raw numeric policy versus native coercion");
    } else if (!c.native.empty()) {
      ++equal;
      if (actual != c.native)
        return fail("native complete term tree");
    }
    const auto v = cursiv::test::validationHelper(language.language, source);
    if (v.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty()) {
      const auto &x = v.diagnostics[0];
      if (x.code != c.error || x.range.begin >= x.range.end ||
          x.range.end > source.size())
        return fail("runtime code/range");
    }
  }
  std::printf("ATerm: %zu sources, %u native trees, %u raw numeric policies, "
              "%u native blob limitation, %u negatives\n",
              std::size(nativeCases), equal, policies, limitations, negative);
  return equal == 43 && policies == 5 && limitations == 1 && negative == 5 ? 0
                                                                           : 1;
}
