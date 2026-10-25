#include "audit_regressions.hpp"
#include "native_conformance.hpp"
// The smoke test of the language: build the container the way a workspace
// does, and check that every document under example/ parses clean into a
// Semver.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "semver/core/CoreServices.hpp"
#include "semver/core/ast.hpp"

namespace {

void fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "semver: %s: %s\n", path.c_str(), what);
}

}  // namespace

int main() {
  if (!audit_regressions::run() || !native_conformance::run()) return 1;
  std::vector<std::filesystem::path> examples;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".semver")
        examples.push_back(entry.path());
  std::ranges::sort(examples);
  if (examples.empty()) {
    std::fprintf(stderr, "semver: no example documents found\n");
    return 1;
  }

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      semver::SemverCoreServices, semver::SemverSharedCoreServices,
      semver::install_core_module>();

  int failures = 0;
  for (const std::filesystem::path& example : examples) {
    const std::string path = example.string();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      fail(path, "does not parse clean");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<semver::ast::Semver>) {
      fail(path, "the parse built no Semver");
      ++failures;
    }
  }
  if (failures != 0) return 1;

  std::printf("semver: %zu examples parse\n", examples.size());
  return 0;
}
