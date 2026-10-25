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
#include "telephone/core/CoreServices.hpp"
#include "telephone/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "telephone: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      telephone::TelephoneCoreServices, telephone::TelephoneSharedCoreServices,
      telephone::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
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
        root->type != &cursiv::ast_type_tag<telephone::ast::Number>)
      failures += fail(file, "the parse built no Number");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("telephone: %d example(s) parse\n", total);
  return 0;
}
