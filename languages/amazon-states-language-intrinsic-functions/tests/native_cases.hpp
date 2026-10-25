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
    bool arity;
  };
  const Case cases[] =
      {
          {"00.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"01.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayContains","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"02.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayGetItem","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"03.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayLength","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"04.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayPartition","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"05.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayRange","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"},{"kind":"path","value":"$.FunctionInput.trd"}]})ASL",
           false},
          {"06.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayUnique","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"07.asl-intrinsic",
           R"ASL({"kind":"call","function":"Base64Decode","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"08.asl-intrinsic",
           R"ASL({"kind":"call","function":"Base64Encode","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"09.asl-intrinsic",
           R"ASL({"kind":"call","function":"Format","args":[{"kind":"string","value":"Formatting arguments fst={} and snd={}."},{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"10.asl-intrinsic",
           R"ASL({"kind":"call","function":"Hash","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"11.asl-intrinsic",
           R"ASL({"kind":"call","function":"JsonMerge","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"},{"kind":"bool","value":"false"}]})ASL",
           false},
          {"12.asl-intrinsic",
           R"ASL({"kind":"call","function":"JsonToString","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"13.asl-intrinsic",
           R"ASL({"kind":"call","function":"MathAdd","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"}]})ASL",
           false},
          {"14.asl-intrinsic",
           R"ASL({"kind":"call","function":"MathRandom","args":[{"kind":"path","value":"$.FunctionInput.fst"},{"kind":"path","value":"$.FunctionInput.snd"},{"kind":"path","value":"$.FunctionInput.trd"}]})ASL",
           false},
          {"15.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayGetItem","args":[{"kind":"call","function":"StringSplit","args":[{"kind":"context","value":"$$.StateMachine.Name"},{"kind":"string","value":"-"}]},{"kind":"call","function":"MathAdd","args":[{"kind":"call","function":"ArrayLength","args":[{"kind":"call","function":"StringSplit","args":[{"kind":"context","value":"$$.StateMachine.Name"},{"kind":"string","value":"-"}]}]},{"kind":"int","value":"-1"}]}]})ASL",
           false},
          {"16.asl-intrinsic",
           R"ASL({"kind":"call","function":"StringSplit","args":[{"kind":"string","value":"Hello\\nWorld"},{"kind":"string","value":"\\n"}]})ASL",
           false},
          {"17.asl-intrinsic",
           R"ASL({"kind":"call","function":"StringToJson","args":[{"kind":"path","value":"$.FunctionInput"}]})ASL",
           false},
          {"18.asl-intrinsic",
           R"ASL({"kind":"call","function":"UUID","args":[]})ASL", false},
          {"19.asl-intrinsic",
           R"ASL({"kind":"call","function":"Base64Decode","args":[{"kind":"call","function":"Base64Encode","args":[{"kind":"string","value":"\\u0041"}]}]})ASL",
           false},
          {"20.asl-intrinsic",
           R"ASL({"kind":"call","function":"ArrayContains","args":[{"kind":"path","value":"$.Grid[[0]]"},{"kind":"call","function":"ArrayUnique","args":[{"kind":"path","value":"$.Items[0].tags"}]}]})ASL",
           false},
          {"21.asl-intrinsic",
           R"ASL({"kind":"call","function":"MathAdd","args":[{"kind":"call","function":"MathRandom","args":[{"kind":"int","value":"1"},{"kind":"int","value":"100"},{"kind":"int","value":"5"}]},{"kind":"float","value":"3.14e2"}]})ASL",
           false},
          {"22.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"variable","value":"$input"}]})ASL",
           false},
          {"23.asl-intrinsic",
           R"ASL({"kind":"call","function":"Format","args":[{"kind":"string","value":"{}"},{"kind":"variable","value":"$data.name"}]})ASL",
           false},
          {"24.asl-intrinsic", R"ASL({"kind":"call","function":"Array","args":[{"kind":"variable","value":"$items[0]"},{"kind":"context","value":"$$.Execution.Id"},{"kind":"path","value":"$.x"}]})ASL",
           false},
          {"25.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"int","value":"9223372036854775808"}]})ASL",
           false},
          {"26.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"int","value":"-9223372036854775809"}]})ASL",
           false},
          {"27.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"float","value":"1e400"},{"kind":"float","value":"1e-400"}]})ASL",
           false},
          {"28.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"float","value":"1.00000000000000000001"}]})ASL",
           false},
          {"29.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"string","value":"a\\n"},{"kind":"string","value":"a\\u0041"},{"kind":"string","value":"\\{x\\}"}]})ASL",
           false},
          {"31.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"path","value":"$.items[?(@.x == 1)]"}]})ASL",
           false},
          {"32.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"bool","value":"true"},{"kind":"bool","value":"false"},{"kind":"int","value":"-0"},{"kind":"float","value":"0.0"},{"kind":"float","value":"1E+2"}]})ASL",
           false},
          {"35.asl-intrinsic",
           R"ASL({"kind":"call","function":"Format","args":[]})ASL", true},
          {"36.asl-intrinsic",
           R"ASL({"kind":"call","function":"UUID","args":[{"kind":"int","value":"1"}]})ASL",
           true},
          {"38.asl-intrinsic",
           R"ASL({"kind":"call","function":"Array","args":[{"kind":"variable","value":"$var.deep[0].name"}]})ASL",
           false},
  };
  for (const auto &c : cases) {
    std::ifstream file(std::string(CURSIV_TEST_DIR) + "/native/" + c.file);
    const std::string source((std::istreambuf_iterator<char>(file)), {});
    const auto result = language.language.parser->parse(source);
    const auto *root = cursiv::ast_cast<ns::ast::Arg>(result.value);
    std::ostringstream text;
    Projection(text).arg(root);
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
    const auto validation =
        cursiv::test::validationHelper(language.language, source);
    bool arity = false;
    for (const auto &d : validation.diagnostics)
      arity |= d.code && d.code->isString() &&
               d.code->string() == "asl-intrinsic.arity";
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || text.str() != c.tree || !parents ||
        arity != c.arity) {
      std::fprintf(stderr, "intrinsic: incorrect source fields: %s\n", c.file);
      ++errors;
    }
  }
  return errors;
}
} // namespace native_regressions
