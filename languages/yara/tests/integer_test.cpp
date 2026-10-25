#include "IntegerView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "integer_cases.hpp"
#include "yara/core/CoreServices.hpp"
#include "yara/generated/Yara/Module.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language = cursiv::create_language<yara::YaraCoreServices,
                                                yara::YaraSharedCoreServices,
                                                yara::install_core_module>();
  const auto factory = yara::generated::Yara::create_parser();
  for (const auto &c : cases) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/numbers/" + c.file,
                    std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto fail = [&](const char *m) {
      std::fprintf(stderr, "%s: %s\n", c.file, m);
      return 1;
    };
    const auto r = language.language.parser->parse(source);
    if (!r.value || !r.accepted || r.cst().recovered() ||
        !r.diagnostics.empty() || r.cst().text() != source)
      return fail("parse");
    J::Array numbers;
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      const auto first = numbers.size();
      append_numbers(n, numbers);
      for (auto i = first; i < numbers.size(); ++i) {
        const auto &raw = numbers[i].object().at("raw").string();
        if (source.find(raw) == source.npos)
          owned = false;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *r.value);
    if (!owned || seen.size() != r.nodes.size() ||
        numbers.size() != c.raw.size())
      return fail("ownership");
    for (std::size_t i = 0; i < numbers.size(); ++i)
      if (numbers[i].object().at("raw").string() != c.raw[i] ||
          numbers[i].object().at("value").string() != c.value[i])
        return fail("native numeric value or raw source");
    const auto v = cursiv::test::validationHelper(language.language, source);
    bool found = c.code.empty() && v.diagnostics.empty();
    for (const auto &d : v.diagnostics)
      if (d.code && d.code->isString() && d.code->string() == c.code) {
        found = true;
        const auto text = std::string_view(source).substr(
            d.range.begin, d.range.end - d.range.begin);
        bool precise = false;
        for (auto raw : c.raw)
          precise |= text == raw;
        if (!precise)
          return fail("diagnostic range");
      }
    if (!found)
      return fail("runtime");
    const auto second = factory->parse(source);
    if (!second.accepted || second.cst().recovered() || !second.value ||
        !second.diagnostics.empty() || second.cst().text() != source)
      return fail("factory");
  }
  std::printf("YARA: %zu integer controls\n", std::size(cases));
  return 0;
}
