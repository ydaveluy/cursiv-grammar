// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "srt/core/CoreServices.hpp"
#include "srt/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "srt: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      srt::SrtCoreServices,
      srt::SrtSharedCoreServices,
      srt::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".srt")
        continue;
      const std::filesystem::path& file = entry.path();
      ++total;

      std::ifstream in(file);
      if (!in) {
        failures += fail(file, "the document is not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      // `recovered()` is set when the parser had to repair the input to keep
      // going: a clean document leaves it false.
      if (result.cst().recovered()) {
        failures += fail(file, "the document does not parse clean");
        continue;
      }

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<srt::ast::Document>)
        failures += fail(file, "the parse built no Document");
    }

  if (!native_regressions(language.language)) ++failures;
  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("srt: %d example(s) parse\n", total);
  return 0;
}
