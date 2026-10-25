// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "jam/core/CoreServices.hpp"
#include "jam/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "jam: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      jam::JamCoreServices, jam::JamSharedCoreServices,
      jam::install_core_module>();

  int failures = 0;
  int parsed = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".jam")
      continue;

    std::ifstream file(entry.path(), std::ios::binary);
    if (!file) {
      failures += fail(entry.path(), "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<jam::ast::Jam>) {
      failures += fail(entry.path(), "the parse built no Jam root");
      continue;
    }
    ++parsed;
  }

  if (parsed == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");
  if (failures != 0) {
    std::fprintf(stderr, "jam: %d of %d example(s) failed\n", failures,
                 parsed + failures);
    return 1;
  }
  std::printf("jam: %d example(s) parse\n", parsed);
  return 0;
}
