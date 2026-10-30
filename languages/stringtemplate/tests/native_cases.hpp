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
  int errors = 0;
  struct Case {
    const char *file;
    const char *tree;
  };
  const Case cases[] = {
      {"00.st",
       R"ST([["tag",["call","f",[["",["name","x"]],["",["name","y"]]]],[]]])ST"},
      {"01.st", R"ST([["tag",["call","f",[["",["name","x"]]]],[]]])ST"},
      {"02.st",
       R"ST([["tag",["call","f",[["",["name","x"]],["",["name","y"]],["",["name","z"]]]],[]]])ST"},
      {"03.st",
       R"ST([["tag",["call","f",[["a",["name","x"]],["b",["name","y"]]]],[]]])ST"},
      {"04.st", R"ST([["tag",["call","f",[["...",null]]],[]]])ST"},
      {"05.st",
       R"ST([["tag",["call","f",[["a",["name","x"]],["...",null]]],[]]])ST"},
      {"06.st", R"ST([["tag",["list",[["name","x"],["name","y"]]],[]]])ST"},
      {"07.st", R"ST([["tag",["list",[]],[]]])ST"},
      {"08.st",
       R"ST([["tag",["mapping",[["name","x"],["name","y"]],[[["sub",["a","b"],[["tag",["name","a"],[]],["text","-"],["tag",["name","b"],[]]]]]]],[]]])ST"},
      {"09.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["sub",["a"],[["tag",["name","a"],[]]]]]]],[]]])ST"},
      {"10.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["sub",[],[["text","literal"]]]]]],[]]])ST"},
      {"11.st", R"ST([["tag",["sub",[],[["text","literal"]]],[]]])ST"},
      {"12.st", R"ST([["tag",["sub",[],[]],[]]])ST"},
      {"13.st",
       R"ST([["tag",["call","/a/b",[["",["name","x"]],["",["name","y"]]]],[]]])ST"},
      {"14.st",
       R"ST([["tag",["super","f",[["",["name","x"]],["",["name","y"]]]],[]]])ST"},
      {"15.st",
       R"ST([["tag",["dynamic",["name","name"],[["",["name","x"]],["",["name","y"]]]],[]]])ST"},
      {"16.st",
       R"ST([["tag",["name","x"],[["separator",["string",","]],["null",["string","n"]]]]])ST"},
      {"17.st", R"ST([["tag",["bool",true],[]]])ST"},
      {"18.st",
       R"ST([["if",[[["&&",["name","x"],["!",["name","y"]]],[["text","Y"]]]],true,[["text","N"]]]])ST"},
      {"19.st",
       R"ST([["tag",["index",["field",["name","x"],"a"],["name","y"]],[]]])ST"},
      {"20.st", R"ST([["text","hello } world"]])ST"},
      {"21.st", R"ST([["text","a<b"]])ST"},
      {"22.st", R"ST([["text","\n"]])ST"},
      {"23.st", R"ST([["text","A"]])ST"},
      {"24.st",
       R"ST([["tag",["list",[["name","x"],["null"],["name","y"],["null"]]],[]]])ST"},
      {"25.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["call","t",[]],["call","u",[]]]]],[]]])ST"},
      {"26.st",
       R"ST([["tag",["mapping",[["name","x"],["name","y"]],[[["call","t",[]]]]],[]]])ST"},
      {"27.st",
       R"ST([["tag",["call","f",[["",["mapping",[["name","x"]],[[["call","t",[]]]]]],["",["name","y"]]]],[]]])ST"},
      {"28.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["sub",["v"],[["tag",["name","v"],[]]]]]]],[]]])ST"},
      {"29.st", R"ST([["text","tail"]])ST"},
      {"32.st", R"ST([["tag",["name","title"],[]],["text","\n\n"]])ST"},
      {"34.st", R"ST([["tag",["name","0-a"],[]]])ST"},
      {"35.st", R"ST([["tag",["string","a\n\tu0041"],[]]])ST"},
      {"36.st", R"ST([["if",[[["name","x"],[["text","}"]]]],false,[]]])ST"},
      {"37.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["sub",["v"],[["text","a}b"]]]]]],[]]])ST"},
      {"38.st", R"ST([["tag",["list",[["null"],["null"]]],[]]])ST"},
      {"39.st", R"ST([["text","ab"]])ST"},
      {"40.st",
       R"ST([["tag",["mapping",[["name","x"]],[[["sub",["v"],[["tag",["name","v"],[]]]]]]],[]]])ST"},
      {"41.st",
       R"ST([["tag",["mapping",[["call","f",[["",["name","x"]],["",["name","y"]]]]],[[["call","t",[]]]]],[]]])ST"},
  };
  for (const auto &c : cases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + c.file);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ast::Template>(result.value);
    std::ostringstream text;
    if (root)
      Projection(text).elements(root->elements);
    bool parents = root && root->parent == nullptr;
    std::unordered_set<const cursiv::AstNode *> seen;
    const auto visit = [&](auto &&self, const cursiv::AstNode &n) -> void {
      if (!seen.insert(&n).second) {
        parents = false;
        return;
      }
      language.language.astReflection->visitMembers(
          n, [&](std::string_view, std::uint32_t, cursiv::AstNode &child) {
            parents &= child.parent == &n &&
                       result.cst().contains(n.cstNodeId, child.cstNodeId);
            self(self, child);
          });
    };
    if (root)
      visit(visit, *root);
    parents &= seen.size() == result.nodes.size();
    const auto validation = cursiv::test::validationHelper(language.language, source);
    const bool expectsArityError = std::string_view(c.file) == "10.st";
    bool validDiagnostics = validation.diagnostics.empty();
    if (expectsArityError) {
      validDiagnostics = false;
      for (const auto& diagnostic : validation.diagnostics)
        validDiagnostics |= diagnostic.code && *diagnostic.code == "stringtemplate.map-arity";
    }
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !parents || text.str() != c.tree ||
        !validDiagnostics) {
      std::fprintf(stderr, "stringtemplate: incorrect native fields %s\n",
                   c.file);
      ++errors;
    }
  }
  for (const auto &c :
       {std::pair{"<f(x,a=y)>", "stringtemplate.argument-kinds"},
        std::pair{"<f(...,a=x)>", "stringtemplate.argument-forwarding"},
        std::pair{"<x:name>", "stringtemplate.map-target"},
        std::pair{"<x,y:t(),u()>", "stringtemplate.zip-target"},
        std::pair{"<x:{literal}>", "stringtemplate.map-arity"},
        std::pair{"<x,y:{one|<one>}>", "stringtemplate.map-arity"},
        std::pair{"<x;unknown=\"x\">", "stringtemplate.option"},
        std::pair{"<x;separator>", "stringtemplate.option-value"}}) {
    const auto result = language.language.parser->parse(c.first);
    const auto validation =
        cursiv::test::validationHelper(language.language, c.first);
    bool found = false;
    for (const auto &d : validation.diagnostics)
      found |= d.code && d.code->isString() && d.code->string() == c.second;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !found) {
      std::fprintf(stderr, "stringtemplate: missing runtime diagnostic %s\n",
                   c.second);
      ++errors;
    }
  }
  return errors;
}
} // namespace native_regressions
