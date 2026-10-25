#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "sieve/core/CoreServices.hpp"
#include "sieve/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& path, const char* what) {
  std::fprintf(stderr, "sieve: %s: %s\n", path.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      sieve::SieveCoreServices,
      sieve::SieveSharedCoreServices,
      sieve::install_core_module>();

  std::vector<std::filesystem::path> documents;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".sieve")
        documents.push_back(entry.path());
  if (documents.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  for (const std::filesystem::path& path : documents) {
    std::ifstream file(path);
    if (!file) return fail(path, "the document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail(path, "the document does not parse");
    if (!result.diagnostics.empty())
      return fail(path, "the document parses but carries a diagnostic");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<sieve::ast::Script>)
      return fail(path, "the parse built no script");

    const auto& script = *static_cast<const sieve::ast::Script*>(root);
    if (script.commands.empty())
      return fail(path, "the script holds no command");
  }

  std::printf("sieve: %zu documents parse\n", documents.size());
  return 0;
}
