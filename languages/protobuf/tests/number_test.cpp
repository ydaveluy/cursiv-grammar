#include "NumberView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "number_cases.hpp"
#include "protobuf/core/CoreServices.hpp"
#include "protobuf/generated/Protobuf/Module.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language =
      cursiv::create_language<protobuf::ProtobufCoreServices,
                              protobuf::ProtobufSharedCoreServices,
                              protobuf::install_core_module>();
  const auto factory = protobuf::generated::Protobuf::create_parser();
  unsigned natives = 0;
  for (const auto &c : cases) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/numbers/" + c.file,
                    std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto r = language.language.parser->parse(source);
    const auto fail = [&](const char *m) {
      std::fprintf(stderr, "%s: %s\n", c.file, m);
      return 1;
    };
    if (!r.value || !r.accepted || r.cst().recovered() ||
        !r.diagnostics.empty() || r.cst().text() != source)
      return fail("parse");
    J::Array numbers;
    std::vector<std::string_view> raw;
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto capture = [&](std::string_view s) {
      if (s.data() < source.data() ||
          s.data() + s.size() > source.data() + source.size() || s.empty())
        owned = false;
      raw.push_back(s);
    };
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      append_numbers(n, numbers);
      if (auto *p = cursiv::ast_cast<protobuf::ast::Field>(&n))
        capture(p->number);
      if (auto *p = cursiv::ast_cast<protobuf::ast::OneofField>(&n))
        capture(p->number);
      if (auto *p = cursiv::ast_cast<protobuf::ast::MapField>(&n))
        capture(p->number);
      if (auto *p = cursiv::ast_cast<protobuf::ast::EnumField>(&n))
        capture(p->value);
      if (auto *p = cursiv::ast_cast<protobuf::ast::ReservedRange>(&n)) {
        capture(p->from);
        if (p->to)
          capture(*p->to);
      }
      if (auto *p = cursiv::ast_cast<protobuf::ast::NumberLiteral>(&n))
        capture(p->text);
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *r.value);
    if (!owned || seen.size() != r.nodes.size())
      return fail("ownership");
    std::sort(raw.begin(), raw.end(),
              [](auto a, auto b) { return a.data() < b.data(); });
    if (raw != c.raw)
      return fail("raw numbers");
    for (auto &n : numbers)
      n.object().erase("raw");
    if (!c.native.empty()) {
      ++natives;
      if (J(numbers).toJsonString({.pretty = false}) != c.native)
        return fail("native values");
    }
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    bool found = c.code.empty() && validation.diagnostics.empty();
    for (auto &d : validation.diagnostics)
      if (d.code && d.code->isString() && d.code->string() == c.code) {
        found = true;
        auto text = std::string_view(source).substr(
            d.range.begin, d.range.end - d.range.begin);
        if (std::find(raw.begin(), raw.end(), text) == raw.end())
          return fail("diagnostic span");
      }
    if (!found)
      return fail("runtime");
    const auto second = factory->parse(source);
    if (!second.accepted || second.cst().recovered() || !second.value ||
        !second.diagnostics.empty() || second.cst().text() != source)
      return fail("factory");
  }
  std::printf("Protobuf: %zu numeric controls, %u complete native positives\n",
              std::size(cases), natives);
  return natives == 28 ? 0 : 1;
}
