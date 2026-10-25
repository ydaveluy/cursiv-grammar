#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every document under tests/ (the upstream corpus is empty),
// and check each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "tl/core/CoreServices.hpp"
#include "tl/core/ast.hpp"

namespace {

int fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "tl: %s: %s\n", path.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      tl::TlCoreServices,
      tl::TlSharedCoreServices,
      tl::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".tl")
      continue;
    ++count;
    const std::string path = entry.path().string();
    std::ifstream file(path, std::ios::binary);
    if (!file) {
      failures += fail(path, "the example is not readable");
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
        root->type != &cursiv::ast_type_tag<tl::ast::Formula>) {
      failures += fail(path, "built the wrong root type");
    }
  }

  if (count == 0) return fail(CURSIV_TEST_DIR, "no example found");
  if (failures != 0) return 1;
  std::printf("tl: %d examples parse\n", count);
  return 0;
}
