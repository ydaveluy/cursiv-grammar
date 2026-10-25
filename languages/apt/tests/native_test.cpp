#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <unordered_set>
#include "NativeCases.hpp"
#include "apt/core/CoreServices.hpp"
#include "apt/core/Word.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

int main() {
  const auto language = cursiv::create_language<apt::AptCoreServices,
      apt::AptSharedCoreServices, apt::install_core_module>();
  for (const auto& c : nativeCases) {
    const auto fail = [&](const char* what) {
      std::fprintf(stderr, "%.*s: %s\n", static_cast<int>(c.file.size()), c.file.data(), what);
      return 1;
    };
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/native-cases/" + std::string(c.file));
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto result = language.language.parser->parse(source);
    const auto* doc = result.value_as<apt::ast::Document>();
    if (!doc || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source || doc->records.size() != 1)
      return fail("parse/source/record count");
    const auto* record = doc->records[0];
    if (record->uri != c.uri || record->distribution != c.suite || record->commented ||
        (record->type == apt::ast::Type::Deb ? "deb" : "deb-src") != c.type ||
        record->components.size() != c.components.size() || record->options.size() != c.options.size())
      return fail("raw fields");
    for (std::size_t i = 0; i < c.components.size(); ++i)
      if (record->components[i] != c.components[i]) return fail("raw component");
    for (std::size_t i = 0; i < c.options.size(); ++i)
      if (record->options[i]->name != c.options[i].first ||
          record->options[i]->value != c.options[i].second) return fail("raw option/order");
    if (c.native) {
      if (apt::word_value(record->distribution) != c.nativeSuite)
        return fail("native suite");
      for (std::size_t i = 0; i < c.nativeComponents.size(); ++i)
        if (apt::word_value(record->components[i]) != c.nativeComponents[i])
          return fail("native component");
      std::map<std::string, std::string> options, expected;
      for (const auto* o : record->options) options[apt::word_value(o->name)] = apt::word_value(o->value);
      for (const auto& [key, value] : c.nativeOptions) expected[std::string(key)] = value;
      if (options != expected) return fail("native options");
    }
    bool owned = doc->parent == nullptr;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& n) -> void {
      if (!seen.insert(&n).second) { owned = false; return; }
      language.language.astReflection->visitMembers(n,
          [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &n && result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *doc);
    if (!owned || seen.size() != result.nodes.size()) return fail("ownership");
    const auto validation = cursiv::test::validationHelper(language.language, source);
    if (validation.diagnostics.size() != (c.error.empty() ? 0u : 1u))
      return fail("runtime count");
    if (!c.error.empty() && (validation.diagnostics[0].code != c.error ||
                            validation.diagnostics[0].range.begin != c.errorBegin))
      return fail("runtime code/range");
  }
  std::puts("apt: 19 source AST cases, 16 native value comparisons, 3 runtime diagnostics");
}
