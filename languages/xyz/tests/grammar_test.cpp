#include "native_regressions.hpp"
#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check each tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "xyz/core/CoreServices.hpp"
#include "xyz/core/ast.hpp"

namespace {

int fail(const std::string& name, const char* what) {
  std::fprintf(stderr, "xyz: %s: %s\n", name.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  static constexpr const char* kSamples[] = {"example1.xyz", "example2.xyz",
                                             "methane.xyz"};

  const auto language = cursiv::create_language<
      xyz::XyzCoreServices,
      xyz::XyzSharedCoreServices,
      xyz::install_core_module>();

  if (!native_regressions::run(language.language))
    return fail("native XYZ", "frame fields, coordinates or validation");

  for (const char* name : kSamples) {
    std::string path = std::string(CURSIV_EXAMPLE_DIR) + "/" + name;
    std::ifstream file(path);
    if (!file) {
      path = std::string(CURSIV_TEST_DIR) + "/" + name;
      file.open(path);
    }
    if (!file) return fail(name, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(name, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<xyz::ast::Document>)
      return fail(name, "the parse built no document");

    const auto& document = *static_cast<const xyz::ast::Document*>(root);
    if (document.frames.size() != 1 || document.frames[0]->atoms.empty()) return fail(name, "the document holds no atoms");
  }

  std::puts("xyz: the samples parse");
  return 0;
}
