// The smoke test of the language: parse every file of example/ and check each
// builds a clean Model.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "scotty/core/CoreServices.hpp"
#include "scotty/core/ast.hpp"

namespace {

int fail(const char* path, const char* what) {
  std::fprintf(stderr, "scotty: %s: %s\n", path, what);
  return 1;
}

int checkFile(const cursiv::CoreLanguage<scotty::ScottyCoreServices,
                                         scotty::ScottySharedCoreServices>&
                  language,
              const char* path) {
  std::ifstream file(path);
  if (!file) return fail(path, "the sample document is not readable");
  const std::string text((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) return fail(path, "the sample does not parse");

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr || root->type != &cursiv::ast_type_tag<scotty::ast::Model>)
    return fail(path, "the parse built no model");

  return 0;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      scotty::ScottyCoreServices, scotty::ScottySharedCoreServices,
      scotty::install_core_module>();

  int status = 0;
  for (const char* path : {CURSIV_EXAMPLE_1, CURSIV_EXAMPLE_2})
    status |= checkFile(language, path);
  if (status != 0) return status;

  std::puts("scotty: the samples parse");
  return 0;
}
