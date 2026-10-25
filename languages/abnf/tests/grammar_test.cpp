// The smoke test of the language: build the container the way a workspace
// does, read every document under example/, and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "abnf/core/CoreServices.hpp"
#include "abnf/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "abnf: %s: %s\n", path.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.path().extension() == ".abnf") files.push_back(entry.path());
  if (files.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  const auto language = cursiv::create_language<
      abnf::AbnfCoreServices,
      abnf::AbnfSharedCoreServices,
      abnf::install_core_module>();

  if (!native_regressions(language.language)) return 1;

  int checked = 0;
  for (const std::filesystem::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail(path.string(), "does not parse");
    if (!result.diagnostics.empty())
      return fail(path.string(), "parses with diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<abnf::ast::Rulelist>)
      return fail(path.string(), "built no rulelist");

    const auto& rulelist = *static_cast<const abnf::ast::Rulelist*>(root);
    if (rulelist.rules.empty())
      return fail(path.string(), "the rulelist holds no rule");
    ++checked;
  }

  std::printf("abnf: %d sample(s) parse\n", checked);
  return 0;
}
