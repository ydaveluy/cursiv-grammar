#pragma once
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "native_projection.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <vector>
namespace native_regressions {
inline int run(const auto &language) {
  int failures = 0;
  struct Case {
    const char *file;
    const char *fields;
    bool bom;
    std::vector<std::string_view> codes;
  };
  const Case cases[] = {
      {"00.ged",
       R"GED([["0","","HEAD","",false],["1","","CHAR","UTF-8",true],["0","I1","INDI","",false],["1","","NAME","John /Doe/",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"01.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"02.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"03.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"04.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"05.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       true,
       {}},
      {"06.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","  leading and trailing  ",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"07.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","",true],["1","","NOTE","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"08.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","@I1@",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"09.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","a@@b",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"10.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","single @ mark",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"11.ged",
       R"GED([["0","","HEAD","",false],["0","I 1","_CUSTOM","text",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"12.ged",
       R"GED([["0","","HEAD","",false],["0","-1","INDI","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"13.ged",
       R"GED([["0","","HEAD","",false],["1","","_MY-TAG","text",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"14.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"15.ged",
       R"GED([["0","","HEAD","",false],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"18.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","one",true],["2","","CONC","two",true],["2","","CONT","three",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"19.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","one",true],["1","","CONC","two",true],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.continuation-level"}},
      {"20.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","one",true],["2","","CONT","two",true],["2","","CONC","three",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"21.ged",
       R"GED([["3","","NOTE","top-level-three",true],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.level-jump"}},
      {"22.ged",
       R"GED([["2147483647","","NOTE","max",true],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.level-jump"}},
      {"23.ged",
       R"GED([["0","","HEAD","",false],["2147483647","","NOTE","huge-jump",true],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.level-jump"}},
      {"24.ged",
       R"GED([["123456789012345678901234567890","","NOTE","huge",true],["123456789012345678901234567891","","NOTE","next",true],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.level-jump"}},
      {"27.ged",
       R"GED([["0","","HEAD","",false],["0","I","INDI","",false],["0","I","INDI","",false],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.duplicate-xref", "gedcom.duplicate-xref"}},
      {"28.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","value\u0009inside",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"29.ged",
       R"GED([["0","","head","",false],["0","","trlr","",false]])GED",
       false,
       {}},
      {"31.ged",
       R"GED([["123","","4","",false],["0","","TRLR","",false]])GED",
       false,
       {"gedcom.level-jump"}},
      {"32.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","\u0009value",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"33.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","zeros",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"34.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","@#DGREGORIAN@ 1 JAN 1900",true],["0","","TRLR","",false]])GED",
       false,
       {}},
      {"35.ged",
       R"GED([["0","","HEAD","",false],["1","","NOTE","  ",true],["0","","TRLR","",false]])GED",
       false,
       {}},
  };
  for (const auto &test : cases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + test.file);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<gedcom::ast::Document>(result.value);
    std::ostringstream actual;
    Projection(actual).tree(root);
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
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    std::vector<std::string_view> diagnostics;
    for (const auto &d : validation.diagnostics)
      diagnostics.push_back(d.code && d.code->isString()
                                ? std::string_view(d.code->string())
                                : std::string_view());
    std::sort(diagnostics.begin(), diagnostics.end());
    auto expected = test.codes;
    std::sort(expected.begin(), expected.end());
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || actual.str() != test.fields ||
        !parents || !root || root->bom != test.bom || diagnostics != expected) {
      std::fprintf(stderr, "gedcom: wrong native fields: %s\n", test.file);
      ++failures;
    }
  }
  {
    const auto result =
        language.language.parser->parse("000 HEAD\n001 NOTE  @@  ");
    const auto *root = cursiv::ast_cast<gedcom::ast::Document>(result.value);
    if (!root || root->lines.size() != 2 || root->lines[0]->level != "000" ||
        root->lines[1]->level != "001" || root->lines[1]->value != " @@  " ||
        !root->lines[1]->hasValue)
      ++failures;
  }
  return failures;
}
} // namespace native_regressions
