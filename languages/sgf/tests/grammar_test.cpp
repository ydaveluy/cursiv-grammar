#include "audit_regressions.hpp"
#include "code_audit.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "sgf/core/CoreServices.hpp"
#include "sgf/core/ast.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "sgf: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !code_audit::run()) return 1;
  static constexpr const char* kFiles[] = {
      CURSIV_EXAMPLE_DIR "/8718-KuroNeko-creedfoxx-chensicha.sgf",
      CURSIV_TEST_DIR "/properties.sgf",
  };

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language =
      cursiv::create_language<sgf::SgfCoreServices, sgf::SgfSharedCoreServices,
                              sgf::install_core_module>();

  for (const char* path : kFiles) {
    std::ifstream file(path);
    if (!file) return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path, "the sample does not parse");
    // A value converter that refuses leaves the root type right and
    // `recovered()` false, so it only shows up in the diagnostics list.
    if (!result.diagnostics.empty())
      return fail(path, "the sample carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<sgf::ast::Collection>)
      return fail(path, "the parse built no collection");

    const auto& collection = *static_cast<const sgf::ast::Collection*>(root);
    if (collection.trees.empty())
      return fail(path, "the collection holds no game tree");
  }

  std::puts("sgf: the samples parse");
  return 0;
}
