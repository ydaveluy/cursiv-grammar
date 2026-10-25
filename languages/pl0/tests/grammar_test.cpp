// The smoke test of the language: build the container the way a workspace
// does, read every document of example/, and check each tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "pl0/core/CoreServices.hpp"
#include "pl0/core/ast.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "pl0: %s: %s\n", file, what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      pl0::Pl0CoreServices, pl0::Pl0SharedCoreServices,
      pl0::install_core_module>();

  static constexpr const char* kExamples[] = {
      CURSIV_EXAMPLE_DIR "/example/example1.pl0",
      CURSIV_EXAMPLE_DIR "/example/example2.pl0",
      CURSIV_EXAMPLE_DIR "/example/example3.pl0",
      CURSIV_TEST_DIR "/constructs.pl0",
  };

  for (const char* path : kExamples) {
    const std::string text = read_file(path);
    if (text.empty()) return fail(path, "the sample document is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "the sample does not parse");
    if (!result.diagnostics.empty()) return fail(path, "the sample carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<pl0::ast::Program>)
      return fail(path, "the parse built no program");
  }

  std::puts("pl0: every sample parses");
  return 0;
}
