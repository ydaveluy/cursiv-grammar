#include "audit_regressions.hpp"
// The smoke test of the language: parse every document under example/, the way
// a workspace does, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "upnp/core/CoreServices.hpp"
#include "upnp/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "upnp: %s\n", what.c_str());
  return 1;
}

// Parses every `.upnp` file of `dir`, failing on the first one that does not
// build a clean SearchCrit. Returns the number parsed, or -1 on failure.
int checkDir(const cursiv::CoreLanguage<upnp::UpnpCoreServices,
                                        upnp::UpnpSharedCoreServices>&
                 language,
             const char* dir) {
  int count = 0;
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".upnp")
      continue;
    ++count;

    std::ifstream file(entry.path());
    if (!file) {
      fail(entry.path().string() + ": not readable");
      return -1;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input, and a
    // value converter that refuses leaves it false -- `diagnostics` is the
    // one list both land on.
    if (result.cst().recovered()) {
      fail(entry.path().string() + ": did not parse clean");
      return -1;
    }
    if (!result.diagnostics.empty()) {
      fail(entry.path().string() + ": left diagnostics");
      return -1;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<upnp::ast::SearchCrit>) {
      fail(entry.path().string() + ": built no SearchCrit");
      return -1;
    }
  }
  return count;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      upnp::UpnpCoreServices, upnp::UpnpSharedCoreServices,
      upnp::install_core_module>();

  const int examples = checkDir(language, CURSIV_EXAMPLE_DIR);
  if (examples < 0) return 1;
  if (examples == 0) return fail("no example document found");

  // wildcard.upnp, existence.upnp, operators.upnp: this port's own, not
  // part of the upstream corpus.
  const int own = checkDir(language, CURSIV_TEST_DIR);
  if (own < 0) return 1;
  if (own == 0) return fail("no test document found");

  std::printf("upnp: %d example(s) + %d own document(s) parse clean\n",
              examples, own);
  return 0;
}
