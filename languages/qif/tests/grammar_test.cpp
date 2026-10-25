// The smoke test of the language: parse every document in `example/`, and
// fail on a recovered parse, a wrong root type, or a non-empty diagnostics
// list -- a converter that quietly refuses would otherwise pass.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "qif/core/CoreServices.hpp"
#include "qif/core/ast.hpp"

namespace {

int fail(const std::string &what) {
  std::fprintf(stderr, "qif: %s\n", what.c_str());
  return 1;
}

} // namespace

int main() {
  const auto language =
      cursiv::create_language<qif::QifCoreServices, qif::QifSharedCoreServices,
                              qif::install_core_module>();

  qif_regressions(language.language);

  int count = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
      if (entry.path().extension() != ".qif")
        continue;
      ++count;

      std::ifstream file(entry.path());
      if (!file)
        return fail(entry.path().string() + ": not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(entry.path().string() + ": did not parse clean");
      if (!result.diagnostics.empty())
        return fail(entry.path().string() + ": left diagnostics");

      const cursiv::AstNode *const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<qif::ast::Document>)
        return fail(entry.path().string() + ": built no document");
    }
  if (count == 0)
    return fail("no .qif example found");

  std::printf("qif: %d example(s) parse\n", count);
  return cursiv::test::report("qif_grammar_test");
}
