#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "redis/core/CoreServices.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language = cursiv::create_language<redis::RedisCoreServices,
                                                redis::RedisSharedCoreServices,
                                                redis::install_core_module>();
  unsigned equal = 0, negative = 0;
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
    const auto *d = cursiv::ast_cast<redis::ast::Document>(r.value);
    const bool clean =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty();
    if (r.cst().text() != source || clean != c.clean)
      return fail("parse/source");
    if (!clean) {
      ++negative;
      continue;
    }
    if (!d)
      return fail("document");
    if (!c.native.empty()) {
      ++equal;
      if (view(*d, r.cst()).toJsonString({.pretty = false}) != c.native)
        return fail("all native argv bytes/order");
    }
    bool raw = true, owned = !d->parent;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *a = cursiv::ast_cast<redis::ast::Argument>(&n)) {
        auto lo = reinterpret_cast<std::uintptr_t>(source.data()),
             p = reinterpret_cast<std::uintptr_t>(a->raw.data());
        raw &= p >= lo && p - lo <= source.size() &&
               a->raw.size() <= source.size() - (p - lo);
      }
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
    const auto v = cursiv::test::validationHelper(language.language, source);
    std::vector<std::string> codes;
    for (const auto &x : v.diagnostics) {
      if (!x.code || !x.code->isString() || x.range.begin >= x.range.end ||
          x.range.end > source.size())
        return fail("runtime range/code");
      codes.push_back(x.code->string());
    }
    std::sort(codes.begin(), codes.end());
    std::string actual;
    for (auto &s : codes) {
      if (!actual.empty())
        actual += '|';
      actual += s;
    }
    if (actual != c.errors)
      return fail("runtime findings");
  }
  std::printf("Redis: %zu cases, %u native argv equal, %u negative parses\n",
              std::size(nativeCases), equal, negative);
  return equal == 97 && negative == 1 ? 0 : 1;
}
