// The smoke test of the language: every document under example/ parses clean
// and builds a Document root.

#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "mckeemanform/core/CoreServices.hpp"
#include "mckeemanform/core/ast.hpp"

#include "native_cases.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "mckeemanform: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<mckeemanform::MckeemanFormCoreServices,
                              mckeemanform::MckeemanFormSharedCoreServices,
                              mckeemanform::install_core_module>();

  static constexpr std::array<const char*, 3> kExamples = {
      CURSIV_EXAMPLE_DIR "/json.mckeemanform",
      CURSIV_EXAMPLE_DIR "/mckeeman.mckeemanform",
      CURSIV_TEST_DIR "/charclass.mckeemanform",
  };

  for (const char* const path : kExamples) {
    std::ifstream file(path);
    if (!file) return fail(path, "the document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<mckeemanform::ast::Document>)
      return fail(path, "built no document");

    const auto& document =
        *static_cast<const mckeemanform::ast::Document*>(root);
    if (document.rules.empty()) return fail(path, "holds no rules");
  }

  if (checkNativeCases(language)) return fail("native cases", "AST or validation mismatch");

  std::puts("mckeemanform: every example parses");
  return 0;
}
