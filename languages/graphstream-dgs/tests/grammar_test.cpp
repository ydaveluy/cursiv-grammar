// The smoke test of the language: every document in example/ parses clean --
// no repair, no diagnostic -- and builds a Document at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "graphstreamdgs/core/CoreServices.hpp"
#include "graphstreamdgs/core/ast.hpp"


#include "audit_regressions.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "graphstreamdgs: %s: %s\n", file.string().c_str(),
              what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      graphstreamdgs::GraphstreamDgsCoreServices,
      graphstreamdgs::GraphstreamDgsSharedCoreServices,
      graphstreamdgs::install_core_module>();

  dgs_regressions();
  int failures = 0;
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".dgs")
        continue;
      ++count;

      std::ifstream file(entry.path());
      if (!file) {
        failures += fail(entry.path(), "not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(entry.path(), "the document does not parse");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(entry.path(), "the parse left diagnostics");
        continue;
      }

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<graphstreamdgs::ast::Document>) {
        failures += fail(entry.path(), "the parse built no document");
        continue;
      }
    }
  }

  if (count == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("graphstreamdgs: %d example document(s) parse\n", count);
  return cursiv::test::report("graphstreamdgs_grammar_test");
}
