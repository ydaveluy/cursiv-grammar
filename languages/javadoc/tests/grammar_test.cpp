// The smoke test of the language: every sample under example/ parses clean,
// with no diagnostic and no repair, and builds a Documentation root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "javadoc/core/CoreServices.hpp"
#include "javadoc/core/ast.hpp"
#include "native_followup_cases.hpp"
#include "nested_tests.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "javadoc: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      javadoc::JavadocCoreServices, javadoc::JavadocSharedCoreServices,
      javadoc::install_core_module>();

  if (!javadoc_native_followup::run(language)) return 1;
  if (!javadoc_nested::run(language)) return 1;

  int checked = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".javadoc") continue;
    std::ifstream file(entry.path());
    if (!file) return fail(entry.path().string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path().string() + ": the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(entry.path().string() + ": the sample carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<javadoc::ast::Documentation>)
      return fail(entry.path().string() + ": the parse built no Documentation");

    ++checked;
  }
  if (checked == 0) return fail("no .javadoc sample found");

  std::printf("javadoc: %d samples parse\n", checked);
  return 0;
}
