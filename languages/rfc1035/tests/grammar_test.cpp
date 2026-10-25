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
#include "rfc1035/core/CoreServices.hpp"
#include "rfc1035/core/ast.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "rfc1035: %s: %s\n", path.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      rfc1035::Rfc1035CoreServices, rfc1035::Rfc1035SharedCoreServices,
      rfc1035::install_core_module>();

  int examples = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& dirent : std::filesystem::directory_iterator(dir)) {
    if (!dirent.is_regular_file() ||
        dirent.path().extension() != ".rfc1035")
      continue;
    const std::string path = dirent.path().string();
    ++examples;

    std::ifstream file(path);
    if (!file) {
      failures += fail(path, "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(path, "did not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<rfc1035::ast::Domain>)
      failures += fail(path, "did not build a Domain");
  }

  if (examples == 0) return fail(CURSIV_EXAMPLE_DIR, "no examples found");
  if (failures != 0) return 1;

  std::printf("rfc1035: %d examples parse\n", examples);
  return 0;
}
