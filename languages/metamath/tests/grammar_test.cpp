// The grammar end to end: every document of example/ parses clean -- no
// repair, and a Database at the root.

#include "modern_cases.hpp"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "metamath/core/CoreServices.hpp"
#include "metamath/core/ast.hpp"

namespace {

int fail(const char *file, const char *what) {
  std::fprintf(stderr, "metamath: %s: %s\n", file, what);
  return 1;
}

std::string read(const char *path) {
  std::ifstream in(path);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
  const auto language =
      cursiv::create_language<metamath::MetamathCoreServices,
                              metamath::MetamathSharedCoreServices,
                              metamath::install_core_module>();

  if (!metamath_audit::modern_cases(language))
    return 1;

  const char *const files[] = {
      CURSIV_EXAMPLE_DIR "/example1.mm",
      CURSIV_EXAMPLE_DIR "/example2.mm",
      CURSIV_TEST_DIR "/demo.mm",
  };

  for (const char *const path : files) {
    const std::string text = read(path);
    if (text.empty())
      return fail(path, "the document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path, "the document does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the document carries a diagnostic");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<metamath::ast::Database>)
      return fail(path, "the parse built no database");
  }

  std::puts("metamath: every example parses");
  return 0;
}
