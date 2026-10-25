// The grammar's end to end check: every document under example/ parses
// clean, with no recovery and no diagnostics, and builds a Module root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "abb/core/CoreServices.hpp"
#include "abb/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

#include "native_regressions.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "abb: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<abb::AbbCoreServices, abb::AbbSharedCoreServices,
                              abb::install_core_module>();

  // Regression: inspect the semantic payload, not only clean acceptance.
  {
    const auto result = language.language.parser->parse(
        "MODULE T\nVAR num x := +12;\nVAR num y := -'H1F';\nVAR num z := "
        "999999999999999999999999;\nVAR tooldata a := [-1,+2];\n"
        "PROC Q()\nCall -3;\nENDPROC\nENDMODULE");
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty())
      return fail("integer regression did not parse");
    const auto *module = result.value_as<abb::ast::Module>();
    if (!module || module->declarations.size() != 4 ||
        module->procedures.size() != 1)
      return fail("integer regression lost declarations or procedure");
    const std::string_view expected[] = {"+12", "-'H1F'",
                                         "999999999999999999999999"};
    for (std::size_t i = 0; i < 3; ++i) {
      const auto *decl = module->declarations[i];
      const auto *integer =
          decl->value ? std::get_if<abb::ast::Integer *>(&*decl->value)
                      : nullptr;
      if (decl->type != "num" || !integer || !*integer ||
          (*integer)->text != expected[i])
        return fail("integer declaration lost its exact value");
    }
    const auto &arrayValue = module->declarations[3]->value;
    const auto *array =
        arrayValue ? std::get_if<abb::ast::Array *>(&*arrayValue) : nullptr;
    if (!array || !*array || (*array)->items.size() != 2)
      return fail("integer array was lost");
    for (std::size_t i = 0; i < 2; ++i) {
      const auto *value = std::get_if<abb::ast::Integer *>(&(*array)->items[i]);
      if (!value || !*value || (*value)->text != (i == 0 ? "-1" : "+2"))
        return fail("integer array item lost its sign");
    }
    const auto &calls = module->procedures[0]->statements;
    const auto *call = calls.size() == 1
                           ? cursiv::ast_cast<abb::ast::Call>(calls[0])
                           : nullptr;
    if (!call || call->arguments.size() != 1)
      return fail("integer call argument was lost");
    const auto *argument =
        std::get_if<abb::ast::Integer *>(&call->arguments[0]);
    if (!argument || !*argument || (*argument)->text != "-3")
      return fail("integer call argument lost its value");
  }

  if (!abb_tests::nativeRegressions(language.language))
    return fail("native semantic regressions failed");

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const fs::directory_entry &entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".sys")
        continue;
      const fs::path path = entry.path();

      std::ifstream file(path, std::ios::binary);
      if (!file)
        return fail(path.filename().string() + " is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(path.filename().string() + " does not parse clean");
      if (!result.diagnostics.empty())
        return fail(path.filename().string() + " left diagnostics");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<abb::ast::Module>)
        return fail(path.filename().string() + " built no module");
      ++count;
    }
  if (count == 0)
    return fail("no example documents found");

  std::printf("abb: %d examples parse\n", count);
  return 0;
}
