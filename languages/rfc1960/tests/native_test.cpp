#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

#include "NativeCases.hpp"
#include "NativeView.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "rfc1960/core/CoreServices.hpp"

int main() {
  const auto language =
      cursiv::create_language<rfc1960::Rfc1960CoreServices,
                              rfc1960::Rfc1960SharedCoreServices,
                              rfc1960::install_core_module>();
  unsigned native = 0;
  for (const auto &c : nativeCases) {
    const auto fail = [&](const char *what) {
      std::fprintf(stderr, "%s: %s\n", std::string(c.name).c_str(), what);
      return 1;
    };
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native-cases/" +
                           std::string(c.file),
                       std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *domain = cursiv::ast_cast<rfc1960::ast::Filter>(result.value);
    if (!domain || !result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || result.cst().text() != source)
      return fail("parse/source");
    if (!c.native.empty()) {
      ++native;
      if (filter_view(domain).toJsonString({.pretty = false}) != c.native)
        return fail("native filter tree and value octets");
    }
    using namespace rfc1960::ast;
    if (const auto *simple = cursiv::ast_cast<Simple>(domain);
        simple && c.name != "leading-space") {
      const auto op = simple->op == CompareOp::equal              ? "="
                      : simple->op == CompareOp::approx           ? "~="
                      : simple->op == CompareOp::greater_or_equal ? ">="
                                                                  : "<=";
      if (source != "(" + std::string(simple->attr) + op +
                        std::string(simple->value) + ")")
        return fail("raw attribute/operator/value spelling");
    }
    if (const auto *ext = cursiv::ast_cast<Extensible>(domain)) {
      std::string spelling = "(" + (ext->attr ? std::string(*ext->attr) : "");
      for (auto qualifier : ext->qualifiers)
        spelling += ":" + std::string(qualifier);
      spelling += ":=" + std::string(ext->value) + ")";
      if (spelling != source)
        return fail("raw matching-rule qualifiers and value");
    }
    if (const auto *sub = cursiv::ast_cast<Substring>(domain)) {
      std::string spelling = "(" + std::string(sub->attr) + "=";
      if (sub->initial)
        spelling += *sub->initial;
      spelling += '*';
      for (auto part : sub->middle)
        spelling += std::string(part) + "*";
      if (sub->final_)
        spelling += *sub->final_;
      spelling += ')';
      if (spelling != source)
        return fail("raw substring components");
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
    bool owned = domain->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    auto walk = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        owned = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &node &&
                     result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *domain);
    if (!owned || seen.size() != result.nodes.size())
      return fail("ownership");
  }
  std::printf("rfc1960: %zu raw filters, %u native filter objects\n",
              std::size(nativeCases), native);
  return native == 34 ? 0 : 1;
}
