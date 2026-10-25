#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "javadoc/core/CoreServices.hpp"
#include <algorithm>
#include <cstdio>
#include <unordered_set>
int main() {
  const auto lang = cursiv::create_language<javadoc::JavadocCoreServices,
                                            javadoc::JavadocSharedCoreServices,
                                            javadoc::install_core_module>();
  bool ok = true;
  auto check = [&](bool yes, const char *id, const char *what) {
    if (!yes) {
      std::fprintf(stderr, "javadoc %s: %s\n", id, what);
      ok = false;
    }
  };
  for (const auto &c : cases) {
    const std::string_view source = c.source;
    auto parsed = lang.language.parser->parse(source);
    check((parsed.accepted && !parsed.cst().recovered()) == c.clean, c.name,
          "measured syntax result");
    check(parsed.cst().text() == source, c.name, "all source bytes");
    auto *doc = cursiv::ast_cast<javadoc::ast::Documentation>(parsed.value);
    check(doc, c.name, "Documentation root");
    if (!doc)
      continue;
    if (c.expected)
      check(audit::view(*doc).toJsonString({.pretty = false}) == c.expected,
            c.name, "tag order, raw captures, scalar values and delimiters");
    std::unordered_set<const cursiv::AstNode *> seen;
    bool owned = true;
    auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        owned = false;
        return;
      }
      if (const auto *tag = cursiv::ast_cast<javadoc::ast::InlineTag>(&node))
        check(source.find(tag->content) != source.npos, c.name,
              "inline source retained");
      lang.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &node &&
                     parsed.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *doc);
    check(owned && seen.size() == parsed.nodes.size(), c.name,
          "complete ownership and parents");
    const auto validation =
        cursiv::test::validationHelper(lang.language, std::string(source));
    std::vector<std::string> codes;
    for (const auto &d : validation.diagnostics) {
      check(d.code && d.code->isString() && d.range.begin <= d.range.end &&
                d.range.end <= source.size(),
            c.name, "precise bounded diagnostic");
      if (d.code && d.code->isString())
        codes.push_back(d.code->string());
    }
    std::sort(codes.begin(), codes.end());
    std::string actual;
    for (const auto &code : codes) {
      if (!actual.empty())
        actual += ',';
      actual += code;
    }
    check(actual == c.errors, c.name, "runtime diagnostic codes/counts");
  }
  return ok ? 0 : 1;
}
