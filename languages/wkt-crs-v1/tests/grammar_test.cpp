#include "native_regressions.hpp"
#include "audit_regressions.hpp"
// The smoke test of the language: parse every file in example/, and check
// each tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "wktcrsv1/core/CoreServices.hpp"
#include "wktcrsv1/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "wktcrsv1: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !native_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      wktcrsv1::WktCrsV1CoreServices, wktcrsv1::WktCrsV1SharedCoreServices,
      wktcrsv1::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() ||
        entry.path().extension() != ".wktcrsv1")
      continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path, "the parse left diagnostics");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<wktcrsv1::ast::PropsFile>) {
      fail(path, "the parse built no PropsFile");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "wktcrsv1: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("wktcrsv1: %d example(s) parse\n", total);
  return 0;
}
