// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "audit_regressions.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "postalcode/core/CoreServices.hpp"
#include "postalcode/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "postalcode: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language =
      cursiv::create_language<postalcode::PostalcodeCoreServices,
                              postalcode::PostalcodeSharedCoreServices,
                              postalcode::install_core_module>();

  int total = 0;
  int failures = postalcode_regressions(language.language);
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".postalcode")
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
          root->type != &cursiv::ast_type_tag<postalcode::ast::PostalCode>)
        failures += fail(file, "the parse built no PostalCode");
    }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("postalcode: %d example(s) parse\n", total);
  return 0;
}
