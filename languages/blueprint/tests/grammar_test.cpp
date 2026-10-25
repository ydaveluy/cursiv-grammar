// The smoke test of the language: parse every example, fail on a recovered
// parse or a wrong root type.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "blueprint/core/CoreServices.hpp"
#include "blueprint/core/ast.hpp"

namespace {

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "blueprint: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  const auto language = cursiv::create_language<
      blueprint::BlueprintCoreServices, blueprint::BlueprintSharedCoreServices,
      blueprint::install_core_module>();

  for (int i = 1; i < argc; ++i) {
    const char* const path = argv[i];
    std::ifstream file(path);
    if (!file) return fail(path, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<blueprint::ast::Document>)
      return fail(path, "built no Document root");
  }

  std::puts("blueprint: every example parses");
  return 0;
}
