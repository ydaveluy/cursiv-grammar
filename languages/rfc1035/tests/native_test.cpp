#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "rfc1035/core/CoreServices.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

int main() {
  const auto language = cursiv::create_language<rfc1035::Rfc1035CoreServices,
      rfc1035::Rfc1035SharedCoreServices, rfc1035::install_core_module>();
  for (const bool absolute : {false, true}) {
    const std::string prefix = std::string(63, 'a') + "." + std::string(63, 'b') +
        "." + std::string(63, 'c') + ".";
    for (const std::size_t finalSize : {61u, 62u}) {
      const std::string source = prefix + std::string(finalSize, 'd') +
          (absolute ? "." : "");
      const auto result = language.language.parser->parse(source);
      const auto validation = cursiv::test::validationHelper(language.language, source);
      if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
          validation.diagnostics.size() != (finalSize == 62 ? 1u : 0u)) return 1;
      if (finalSize == 62 && validation.diagnostics[0].code != "rfc1035.name-length") return 1;
    }
  }
  unsigned native = 0;
  for (const auto& c : nativeCases) {
    const auto fail = [&](const char* what) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), what);
      return 1;
    };
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native-cases/" + std::string(c.file), std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto* domain = cursiv::ast_cast<rfc1035::ast::Domain>(result.value);
    if (!domain || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    std::string spelling;
    for (std::size_t i = 0; i < domain->labels.size(); ++i) {
      if (i) spelling += '.';
      spelling += domain->labels[i];
    }
    if (spelling != source) return fail("raw label spellings and separators");
    if (!c.native.empty()) {
      ++native;
      if (native_view(*domain).toJsonString({.pretty = false}) != c.native)
        return fail("native labels, octets and absolute/root identity");
    }
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() && (validation.diagnostics[0].code != c.error ||
        validation.diagnostics[0].range.begin >= validation.diagnostics[0].range.end ||
        validation.diagnostics[0].range.end > source.size()))
      return fail("runtime code/range");
    bool owned = domain->parent == nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { owned = false; return; }
      language.language.astReflection->visitMembers(node,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *domain);
    if (!owned || seen.size() != result.nodes.size()) return fail("ownership");
  }
  std::printf("rfc1035: %zu raw names, %u native label objects\n", std::size(nativeCases), native);
  return native == 20 ? 0 : 1;
}
