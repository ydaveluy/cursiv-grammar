#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_projection.hpp"
#include <fstream>
#include <sstream>
#include <unordered_set>
namespace native_regressions {
inline int run(const auto &language) {
  int failures = 0;
  struct Case {
    const char *file;
    const char *fields;
  };
  const Case cases[] =
      {
          {"00.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":null}])WLN"},
          {"01.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["12"]}],"trailing":"","title":null}])WLN"},
          {"02.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["G","1","UU","1","G"]}],"trailing":"","title":null}])WLN"},
          {"03.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["L","6","T","J"]}],"trailing":"","title":null}])WLN"},
          {"04.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["L","6","J"]}],"trailing":"","title":null}])WLN"},
          {"05.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["T","6","N","J"]}],"trailing":"","title":null}])WLN"},
          {"07.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["O","V","1"]},{"separator":" ","symbols":["&","-","N","A","-"]}],"trailing":"","title":null}])WLN"},
          {"08.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["N","A"]},{"separator":" ","symbols":["G"]}],"trailing":"","title":null}])WLN"},
          {"09.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["F","E"]},{"separator":" ","symbols":["G","3"]}],"trailing":"","title":null}])WLN"},
          {"10.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["C","A"]},{"separator":" ","symbols":["C","-","O","3"]}],"trailing":"","title":null}])WLN"},
          {"11.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["P","1"]}],"trailing":"","title":null}])WLN"},
          {"12.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","Y","1","&","1"]}],"trailing":"","title":null}])WLN"},
          {"13.wln",
           R"WLN([{"prefix":"WLN: ","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":null}])WLN"},
          {"14.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":"acetone"}])WLN"},
          {"15.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["R"]}],"trailing":"","title":"benzène"}])WLN"},
          {"16.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1"]}],"trailing":"","title":null},{"prefix":"","groups":[{"separator":"","symbols":["2","O","2"]}],"trailing":"","title":null},{"prefix":"","groups":[{"separator":"","symbols":["R"]}],"trailing":"","title":null}])WLN"},
          {"23.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":""}])WLN"},
          {"24.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["L","-","12","-","T","J"]}],"trailing":"","title":null}])WLN"},
          {"25.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["L","I"]},{"separator":" ","symbols":["G"]}],"trailing":"","title":null}])WLN"},
          {"26.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["Z","N"]},{"separator":" ","symbols":["S","-","O","4"]}],"trailing":"","title":null}])WLN"},
          {"27.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["N","A","2"]},{"separator":" ","symbols":["S","-","O","4"]}],"trailing":"","title":null}])WLN"},
          {"29.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":"name\twithtab"}])WLN"},
          {"31.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["-","N","A","-"]}],"trailing":"","title":null}])WLN"},
          {"32.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["W","G","W"]}],"trailing":"","title":null}])WLN"},
          {"34.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["N","A"]},{"separator":" ","symbols":["N","-","O","3","*","2"]}],"trailing":"","title":null}])WLN"},
          {"36.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","1"]}],"trailing":"","title":null}])WLN"},
          {"37.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","V","R"]}],"trailing":"","title":null}])WLN"},
          {"38.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["2","O","2"]}],"trailing":"","title":null}])WLN"},
          {"39.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["2","H"]}],"trailing":"","title":null}])WLN"},
          {"40.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","Y"]}],"trailing":"","title":null}])WLN"},
          {"41.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","H"]}],"trailing":"","title":null}])WLN"},
          {"42.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["Q","1"]}],"trailing":"","title":null}])WLN"},
          {"43.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","X"]}],"trailing":"","title":null}])WLN"},
          {"44.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["Q","V","Y","Z","1","R"]}],"trailing":"","title":null}])WLN"},
          {"45.wln",
           R"WLN([{"prefix":"","groups":[{"separator":"","symbols":["1","R"]}],"trailing":"","title":null}])WLN"},
  };
  for (const auto &test : cases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<wln::ast::Document>(result.value);
    std::ostringstream actual;
    Projection(actual).records(root);
    bool parents = root && root->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto visit = [&](auto &&self, const cursiv::AstNode &node) -> void {
      if (!seen.insert(&node).second) {
        parents = false;
        return;
      }
      language.language.astReflection->visitMembers(
          node, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            parents &= child.parent == &node &&
                       result.cst().contains(node.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (root)
      visit(visit, *root);
    parents &= seen.size() == result.nodes.size();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || actual.str() != test.fields ||
        !parents ||
        !cursiv::test::validationHelper(language.language, source)
             .diagnostics.empty()) {
      std::fprintf(stderr, "wln: wrong source fields: %s\n", test.file);
      ++failures;
    }
  }
  for (const auto &test :
       {std::pair{"R  AR", "wln.separator-space"},
        std::pair{"R ", "wln.trailing-space"},
        std::pair{"0", "wln.chain-length"}, std::pair{"abc", "wln.symbol"}}) {
    const auto result = language.language.parser->parse(test.first);
    const auto validation =
        cursiv::test::validationHelper(language.language, test.first);
    bool found = false;
    for (const auto &d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == test.second;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !found)
      ++failures;
  }
  return failures;
}
} // namespace native_regressions
