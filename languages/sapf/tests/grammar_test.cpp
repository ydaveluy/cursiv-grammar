#include "native_regressions.hpp"
#include "binding_cases.hpp"
// The grammar's smoke test: every document under example/ parses clean (no
// repair) and builds a File root.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "sapf/core/CoreServices.hpp"
#include "sapf/core/ast.hpp"

namespace {

constexpr const char* kExamples[] = {
    CURSIV_EXAMPLE_DIR "/example1.sapf",  CURSIV_EXAMPLE_DIR "/example6.sapf",
    CURSIV_EXAMPLE_DIR "/example9.sapf",  CURSIV_EXAMPLE_DIR "/example10.sapf",
    CURSIV_EXAMPLE_DIR "/example14.sapf", CURSIV_EXAMPLE_DIR "/example17.sapf",
    CURSIV_EXAMPLE_DIR "/example25.sapf", CURSIV_EXAMPLE_DIR "/example27.sapf",
    CURSIV_EXAMPLE_DIR "/example28.sapf", CURSIV_TEST_DIR "/extras.sapf",
};

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "sapf: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      sapf::SapfCoreServices, sapf::SapfSharedCoreServices,
      sapf::install_core_module>();

  if (!sapf_native_regressions(language)) return 1;
  if (!sapf_bindings::run(language.language)) return 1;

  for (const char* const path : kExamples) {
    std::ifstream file(path);
    if (!file) return fail(path, "the document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path, "the document does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<sapf::ast::File>)
      return fail(path, "the parse built no File");

    if (!result.diagnostics.empty())
      return fail(path, "the document carries diagnostics");
  }

  std::puts("sapf: every example parses");
  return 0;
}
