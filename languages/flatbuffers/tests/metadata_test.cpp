#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/services/JsonValue.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "flatbuffers/core/CoreServices.hpp"
#include "flatbuffers/generated/Flatbuffers/Module.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_set>
using J = cursiv::JsonValue;
#include "metadata_cases.hpp"
#include <charconv>
int main() {
  auto language =
      cursiv::create_language<flatbuffers::FlatbuffersCoreServices,
                              flatbuffers::FlatbuffersSharedCoreServices,
                              flatbuffers::install_core_module>();
  for (const auto &c : cases) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/metadata/" + c.file);
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto r = language.language.parser->parse(source);
    if (!r.value || !r.accepted || r.cst().recovered() ||
        !r.diagnostics.empty() || r.cst().text() != source)
      return 1;
    bool owned = true;
    std::unordered_set<const cursiv::AstNode *> seen;
    J::Array es;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *e = cursiv::ast_cast<flatbuffers::ast::EnumDecl>(&n)) {
        J::Array vs;
        std::int64_t number = 0;
        for (auto *v : e->values) {
          if (v->value) {
            auto text = *v->value;
            auto [end, error] =
                std::from_chars(text.data(), text.data() + text.size(), number);
            if (error != std::errc{} || end != text.data() + text.size())
              owned = false;
          }
          J::Array attrs;
          for (auto *a : v->metadata)
            attrs.push_back(
                J::Object{{"name", a->name},
                          {"value", a->value ? J(*a->value) : J(nullptr)}});
          vs.push_back(J::Object{{"name", v->name},
                                 {"value", std::to_string(number++)},
                                 {"attrs", attrs}});
        }
        es.push_back(J::Object{{"name", e->name}, {"values", vs}});
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
        J(es).toJsonString({.pretty = false}) != c.data) {
      std::fprintf(stderr, "FlatBuffers metadata: %s\n", c.file);
      return 1;
    }
  }
  return 0;
}
