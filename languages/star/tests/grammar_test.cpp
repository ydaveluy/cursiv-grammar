// The smoke test of the language: build the container the way a workspace
// does, read the sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "star/core/CoreServices.hpp"
#include "star/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "star: %s\n", what);
  return 1;
}

int check(const char* path) {
  std::ifstream file(path);
  if (!file) return fail("a sample document is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const auto language = cursiv::create_language<
      star::StarCoreServices,
      star::StarSharedCoreServices,
      star::install_core_module>();

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) return fail("a sample does not parse");

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<star::ast::Doc>)
    return fail("the parse built no document");

  const auto& doc = *static_cast<const star::ast::Doc*>(root);
  if (doc.blocks.empty()) return fail("the document holds no data block");

  return 0;
}

}  // namespace

int main() {
  for (const char* const path :
       {CURSIV_EXAMPLE_DIR "/example1.star", CURSIV_EXAMPLE_DIR "/example3.star",
        CURSIV_EXAMPLE_DIR "/example4.star",
        CURSIV_TEST_DIR "/coverage.star"}) {
    if (const int rc = check(path); rc != 0) return rc;
  }
  std::puts("star: the samples parse");
  return 0;
}
