// The smoke test of the language: every file under example/ parses clean (no
// recovery) and builds a Document at its root.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "creole/core/CoreServices.hpp"
#include "creole/core/ast.hpp"

namespace {

constexpr const char* kExamples[] = {
    "bold.creole",       "italics.creole",   "linebreaks.creole",
    "links.creole",      "recursive.creole", "titles.creole",
    "complete.creole",   "lists-and-tables.creole",
};

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "creole: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      creole::CreoleCoreServices,
      creole::CreoleSharedCoreServices,
      creole::install_core_module>();

  for (const char* const name : kExamples) {
    const bool authored = std::string(name) == "lists-and-tables.creole";
    const std::string path = authored
        ? std::string(CURSIV_TEST_DIR) + "/" + name
        : std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    std::ifstream file(path);
    if (!file) return fail(name, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(name, "the sample does not parse");
    if (!result.diagnostics.empty())
      return fail(name, "the sample carries a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<creole::ast::Document>)
      return fail(name, "the parse built no document");
  }

  std::puts("creole: every sample parses");
  return 0;
}
