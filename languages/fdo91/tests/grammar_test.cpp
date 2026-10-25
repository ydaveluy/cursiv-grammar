// The smoke test of the language: build the container the way a workspace
// does, then read every document under example/ and check its tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "fdo91/core/CoreServices.hpp"
#include "fdo91/core/ast.hpp"

#include "native_cases.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "fdo91: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      fdo91::Fdo91CoreServices,
      fdo91::Fdo91SharedCoreServices,
      fdo91::install_core_module>();

  if (!native_regressions::run(language.language, *language.shared)) return 1;

  int count = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& path = entry.path();

    std::ifstream file(path);
    if (!file) return fail(path.string() + ": the document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<fdo91::ast::Doc>)
      return fail(path.string() + ": the parse built no Doc");

    ++count;
  }

  if (count == 0) return fail("no example document found");

  std::printf("fdo91: %d example document(s) parse\n", count);
  return 0;
}
