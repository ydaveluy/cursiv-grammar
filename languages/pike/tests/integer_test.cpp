#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "integer_cases.hpp"
#include "pike/core/CoreServices.hpp"
#include "pike/core/Integer.hpp"
#include "pike/generated/Pike/Module.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>
int main() {
  const auto language = cursiv::create_language<pike::PikeCoreServices,
                                                pike::PikeSharedCoreServices,
                                                pike::install_core_module>();
  const auto factory = pike::generated::Pike::create_parser();
  for (const auto &c : cases) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/numbers/" + c.file,
                    std::ios::binary);
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto r = language.language.parser->parse(source);
    bool owned = true;
    const pike::ast::NumberLit *literal = nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    if (!r.value || !r.accepted || r.cst().recovered() ||
        !r.diagnostics.empty() || r.cst().text() != source)
      return 1;
    auto walk = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto *p = cursiv::ast_cast<pike::ast::NumberLit>(&n)) {
        if (literal)
          owned = false;
        literal = p;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *r.value);
    if (!owned || seen.size() != r.nodes.size() || !literal ||
        literal->value != c.raw)
      return 1;
    const auto value = pike::integer_value(literal->value);
    if ((value ? *value : std::string{}) != c.decimal)
      return 1;
    const bool negative = source.find("= -") != source.npos;
    const auto *unary = cursiv::ast_cast<pike::ast::UnaryExpr>(literal->parent);
    if (negative && (!unary || unary->op != "-" || unary->operand != literal))
      return 1;
    const auto v = cursiv::test::validationHelper(language.language, source);
    if (std::string_view(c.code).empty()) {
      if (!v.diagnostics.empty())
        return 1;
    } else {
      if (v.diagnostics.size() != 1 || !v.diagnostics[0].code ||
          v.diagnostics[0].code->string() != c.code ||
          std::string_view(source).substr(v.diagnostics[0].range.begin,
                                          v.diagnostics[0].range.end -
                                              v.diagnostics[0].range.begin) !=
              c.raw)
        return 1;
    }
    const auto other = factory->parse(source);
    if (!other.accepted || other.cst().recovered() || !other.value ||
        other.cst().text() != source)
      return 1;
  }
  std::printf("Pike: %zu native integer controls\n", std::size(cases));
  return 0;
}
