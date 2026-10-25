#include <cstdio>
#include <fstream>
#include <iterator>
#include <unordered_set>

#include "AsPatternView.hpp"
#include "as_pattern_cases.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "haskell/core/CoreServices.hpp"
#include "haskell/generated/Haskell/Module.hpp"
int main() {
  const auto language =
      cursiv::create_language<haskell::HaskellCoreServices,
                              haskell::HaskellSharedCoreServices,
                              haskell::install_core_module>();
  const auto factory = haskell::generated::Haskell::create_parser();
  unsigned faithful = 0, gaps = 0;
  for (const auto& c : cases) {
    std::ifstream f(std::string(CURSIV_TEST_DIR) + "/as-patterns/" + c.file);
    const std::string source((std::istreambuf_iterator<char>(f)), {});
    const auto r = language.language.parser->parse(source);
    const auto fail = [&](const char* m) {
      std::fprintf(stderr, "%s: %s\n", c.file, m);
      return 1;
    };
    if (!r.value || !r.accepted || r.cst().recovered() ||
        !r.diagnostics.empty() || r.cst().text() != source)
      return fail("parse");
    J::Array patterns;
    std::vector<std::string_view> arguments;
    std::vector<std::string> ownedArgs;
    unsigned types = 0;
    bool owned = true;
    std::unordered_set<const cursiv::AstNode*> seen;
    auto walk = [&](auto&& self, const cursiv::AstNode& n) -> void {
      if (!seen.insert(&n).second) {
        owned = false;
        return;
      }
      if (auto* p = cursiv::ast_cast<haskell::ast::AsPat>(&n))
        patterns.push_back(expr(p));
      if (auto* p = cursiv::ast_cast<haskell::ast::TypeApp>(&n)) {
        ++types;
        ownedArgs.push_back(type_name(p->arg));
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
            owned &= child.parent == &n &&
                     r.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    walk(walk, *r.value);
    for (auto& s : ownedArgs) arguments.push_back(s);
    if (!owned || seen.size() != r.nodes.size()) return fail("ownership");
    if (c.native && !c.invisible) {
      ++faithful;
      if (patterns.size() != c.patterns || types != c.types ||
          J(patterns).toJsonString({.pretty = false}) != c.expected ||
          arguments != c.arguments)
        return fail("native AST");
    } else if (c.invisible)
      ++gaps;
    const auto second = factory->parse(source);
    if (!second.value || !second.accepted || second.cst().recovered() ||
        !second.diagnostics.empty() || second.cst().text() != source)
      return fail("factory");
  }
  std::printf(
      "Haskell: %zu cases; %u native AST comparisons; %u native "
      "invisible-pattern gaps remain\n",
      std::size(cases), faithful, gaps);
  return faithful == 15 && gaps == 2 ? 0 : 1;
}
