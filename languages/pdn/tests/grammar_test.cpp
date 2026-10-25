
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pdn/core/CoreServices.hpp"
#include "pdn/core/ast.hpp"
// The smoke test of the language: every file in example/ parses clean and
// builds a Game.

#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "cursiv/core/syntax-tree/AstNode.hpp"

#include "audit_regressions.hpp"

namespace {

int fail(const std::string &path, const char *what) {
  std::fprintf(stderr, "pdn: %s: %s\n", path.c_str(), what);
  return 1;
}

std::vector<std::string> split(const std::string &s, char sep) {
  std::vector<std::string> parts;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, sep))
    if (!item.empty())
      parts.push_back(item);
  return parts;
}

} // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<pdn::PdnCoreServices, pdn::PdnSharedCoreServices,
                              pdn::install_core_module>();
  {
    const auto r = language.language.parser->parse("1. 32x23x14 1-0");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto *doc = cursiv::ast_cast<pdn::ast::Document>(r.value);
    const auto *g = doc && !doc->games.empty() ? doc->games[0] : nullptr;
    const auto *ply = g && !g->items.empty()
                          ? cursiv::ast_cast<pdn::ast::Ply>(g->items[0])
                          : nullptr;
    if (!ply || ply->notation != "32x23x14")
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language =
      cursiv::create_language<pdn::PdnCoreServices, pdn::PdnSharedCoreServices,
                              pdn::install_core_module>();

  pdn_regressions(language.language);

  std::vector<std::string> paths = split(CURSIV_SAMPLES, ',');
  const std::vector<std::string> testPaths = split(CURSIV_TEST_SAMPLES, ',');
  paths.insert(paths.end(), testPaths.begin(), testPaths.end());

  for (const std::string &path : paths) {
    std::ifstream file(path);
    if (!file)
      return fail(path, "the sample document is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path, "the sample does not parse");

    const cursiv::AstNode *const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pdn::ast::Document>)
      return fail(path, "the parse built no Document");

    cursiv::Document document(
        cursiv::TextDocument::create("memory:pdn-example", "pdn", 0, text));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(text);
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document,
                                                          diagnostics);
    if (path.ends_with("/example1.pdn")) {
      CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
            *diagnostics[0].code == "pdn.results");
    } else {
      CHECK(diagnostics.empty());
    }

    std::printf("pdn: %s parses\n", path.c_str());
  }

  return cursiv::test::report("pdn_grammar_test");
}
