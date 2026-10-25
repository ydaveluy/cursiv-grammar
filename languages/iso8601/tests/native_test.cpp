#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "iso8601/core/CoreServices.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
int main() {
  const auto language =
      cursiv::create_language<iso8601::Iso8601CoreServices,
                              iso8601::Iso8601SharedCoreServices,
                              iso8601::install_core_module>();
  unsigned equal = 0, policies = 0;
  for (const auto &c : nativeCases) {
    auto fail = [&](const char *message) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), message);
      return 1;
    };
    std::ifstream in(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                         std::string(c.file),
                     std::ios::binary);
    std::string source((std::istreambuf_iterator<char>(in)), {});
    const auto result = language.language.parser->parse(source);
    const auto *doc = cursiv::ast_cast<ia::Document>(result.value);
    if (!doc || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    const auto view =
        normalize_separator(iso_view(*doc)).toJsonString({.pretty = false});
    if (!c.native.empty()) {
      if (!c.policy.empty()) {
        ++policies;
        if (view == c.native || view != c.policy)
          return fail("explicit date/time ambiguity policy");
      } else {
        ++equal;
        if (view != c.native)
          return fail("native raw components");
      }
    }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() &&
        (validation.diagnostics[0].code != c.error ||
         validation.diagnostics[0].range.begin >=
             validation.diagnostics[0].range.end ||
         validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    bool owned = doc->parent == nullptr, raw = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto inside = [&](std::string_view v) {
      auto lo = reinterpret_cast<std::uintptr_t>(source.data()),
           p = reinterpret_cast<std::uintptr_t>(v.data());
      return p >= lo && p - lo <= source.size() &&
             v.size() <= source.size() - (p - lo);
    };
    const auto fields = [&](const auto &values) {
      for (const auto &v : values)
        if (v)
          raw &= inside(*v);
    };
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (const auto *d = cursiv::ast_cast<ia::DateValue>(&n))
        fields(std::array{d->century, d->year, d->month, d->day, d->week,
                          d->weekDay, d->ordinalDay});
      if (const auto *t = cursiv::ast_cast<ia::TimeValue>(&n)) {
        raw &= inside(t->hour);
        fields(std::array{t->minute, t->second});
      }
      if (const auto *z = cursiv::ast_cast<ia::TimeZoneValue>(&n))
        fields(std::array{z->hour, z->minute});
      if (const auto *d = cursiv::ast_cast<ia::DurationParts>(&n))
        fields(std::array{d->years, d->months, d->weeks, d->days, d->hours,
                          d->minutes, d->seconds});
      if (const auto *r = cursiv::ast_cast<ia::Recurring>(&n); r && r->count)
        raw &= inside(*r->count);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *doc);
    if (!owned || seen.size() != result.nodes.size() || !raw)
      return fail("ownership/raw source-backed components");
    if (c.name == "expanded-year") {
      const auto *d = std::get_if<ia::DateValue *>(&doc->value);
      if (!d || !*d || (*d)->year != "+123456789012345678901234")
        return fail("expanded year exact raw value");
    }
  }
  std::printf(
      "ISO8601: %zu files, %u native trees, %u explicit ambiguity policies\n",
      std::size(nativeCases), equal, policies);
  return equal == 53 && policies == 2 ? 0 : 1;
}
