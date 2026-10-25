
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "rfc1960/core/CoreServices.hpp"
#include "rfc1960/core/ast.hpp"
// The smoke test of the language: build the container the way a workspace
// does, parse every sample under example/, and check each tree.

#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

template <class Language>
bool checkFile(const Language& language, const char* path) {
  std::ifstream file(path);
  if (!file) {
    std::fprintf(stderr, "rfc1960: %s is not readable\n", path);
    return false;
  }
  const std::string text((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

  const cursiv::ParseResult result = language.language.parser->parse(text);
  if (result.cst().recovered()) {
    std::fprintf(stderr, "rfc1960: %s does not parse\n", path);
    return false;
  }

  const cursiv::AstNode* const root = result.value;
  if (root == nullptr ||
      !root->type->derivesFrom(&cursiv::ast_type_tag<rfc1960::ast::Filter>)) {
    std::fprintf(stderr, "rfc1960: %s built no filter\n", path);
    return false;
  }
  return true;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<rfc1960::Rfc1960CoreServices,
                              rfc1960::Rfc1960SharedCoreServices,
                              rfc1960::install_core_module>();
  {
    const auto r = language.language.parser->parse("(cn=)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("(cn=a-b)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  {
    const auto r = language.language.parser->parse("(cn=Jensen\\*)");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      rfc1960::Rfc1960CoreServices, rfc1960::Rfc1960SharedCoreServices,
      rfc1960::install_core_module>();

  bool ok = true;
  for (const char* const path : {
           CURSIV_EXAMPLE_DIR "/example1.rfc1960",
           CURSIV_EXAMPLE_DIR "/example2.rfc1960",
           CURSIV_EXAMPLE_DIR "/example3.rfc1960",
           CURSIV_EXAMPLE_DIR "/example4.rfc1960",
           CURSIV_TEST_DIR "/operators.rfc1960",
       })
    ok = checkFile(language, path) && ok;

  if (!ok) return 1;
  std::puts("rfc1960: every sample parses");
  return 0;
}
