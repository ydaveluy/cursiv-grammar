#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "mps/core/CoreServices.hpp"
#include "mps/core/ast.hpp"

namespace {

int fail(const char* sample, const char* what) {
  std::fprintf(stderr, "mps: %s: %s\n", sample, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      mps::MpsCoreServices,
      mps::MpsSharedCoreServices,
      mps::install_core_module>();
  if (!audit_regressions::run(language.language, *language.shared)) return 1;

  const char* const samples[] = {
      CURSIV_EXAMPLE_DIR "/example1.mps",
      CURSIV_EXAMPLE_DIR "/sample1.mps",
      CURSIV_EXAMPLE_DIR "/sample2.mps",
      CURSIV_TEST_DIR "/integer.mps",
  };

  for (const char* const sample : samples) {
    std::ifstream file(sample);
    if (!file) return fail(sample, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(sample, "does not parse clean");
    if (!result.diagnostics.empty())
      return fail(sample, "carries diagnostics");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mps::ast::Model>)
      return fail(sample, "the parse built no Model");

    const auto& model = *static_cast<const mps::ast::Model*>(root);
    if (model.rows.empty() || model.columns.empty() || model.rhs.empty())
      return fail(sample, "the model is missing a mandatory section");
  }

  std::puts("mps: every sample parses");
  return 0;
}
