// The smoke test of the language: build the container the way a workspace
// does, read every sample, and check the tree.

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "doiurl/core/CoreServices.hpp"
#include "doiurl/core/ast.hpp"

namespace {

int fail(const char* sample, const char* what) {
  std::fprintf(stderr, "doiurl: %s: %s\n", sample, what);
  return 1;
}

constexpr const char* kSamples[] = {
    "example1.doiurl",       "example2.doiurl",  "example3.doiurl",
    "example4.doiurl",       "example6.doiurl",  "example7.doiurl",
    "example10.doiurl",      "example12.doiurl", "multi-segment.doiurl",
    "query-fragment.doiurl",
};

}  // namespace

int main() {
  const auto language =
      cursiv::create_language<doiurl::DoiurlCoreServices,
                              doiurl::DoiurlSharedCoreServices,
                              doiurl::install_core_module>();
  if (doiurl_regressions(language.language)) return 1;

  for (const char* sample : kSamples) {
    const bool authored = std::string(sample) == "multi-segment.doiurl" ||
                          std::string(sample) == "query-fragment.doiurl";
    const std::string path = (authored ? std::string(CURSIV_TEST_DIR)
                                       : std::string(CURSIV_EXAMPLE_DIR)) +
                             "/" + sample;
    std::ifstream file(path);
    if (!file) return fail(sample, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(sample, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<doiurl::ast::DoiUri>)
      return fail(sample, "the parse built no DoiUri");
  }

  std::puts("doiurl: every sample parses");
  return 0;
}
