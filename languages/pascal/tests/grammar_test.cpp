// The smoke test of the language: build the container the way a workspace
// does, read every example, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "native_regressions.hpp"
#include "pascal/core/CoreServices.hpp"
#include "pascal/core/ast.hpp"

namespace {

constexpr const char* kExamples[] = {
    "helloworld.pas",  "passfail.pas", "if.pas",     "case.pas",
    "fact.pas",        "array.pas",    "array2.pas", "set.pas",
    "linkedlist2.pas", "schedule.pas",
};

// This port's own documents, exercising variant record fields, the
// `(.`/`.)` alternate brackets and empty statements, which no upstream
// example does.
constexpr const char* kOwnDocs[] = {
    "variants.pas",
    "altsyntax.pas",
    "emptystmt.pas",
};

int fail(const char* file, const char* what) {
  std::fprintf(stderr, "pascal: %s: %s\n", file, what);
  return 1;
}

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<pascal::PascalCoreServices,
                              pascal::PascalSharedCoreServices,
                              pascal::install_core_module>();

  const auto parses = [&](const char* dir, const char* name) {
    const std::string path = std::string(dir) + "/" + name;
    std::ifstream file(path);
    if (!file) return fail(name, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(name, "the sample does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pascal::ast::Program>)
      return fail(name, "the parse built no Program");
    return 0;
  };

  for (const char* const name : kExamples) {
    const int status = parses(CURSIV_EXAMPLE_DIR, name);
    if (status != 0) return status;
  }
  for (const char* const name : kOwnDocs) {
    const int status = parses(CURSIV_TEST_DIR, name);
    if (status != 0) return status;
  }

  if (!native_regressions::run(language.language)) return 1;
  std::puts("pascal: every sample parses");
  return 0;
}
