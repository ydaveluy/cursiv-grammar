#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, and check that every document under example/ parses clean into an
// Expression.

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
#include "rpn/core/CoreServices.hpp"
#include "rpn/core/ast.hpp"

namespace {

void fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "rpn: %s: %s\n", path.c_str(), what);
}

}  // namespace

int main() {
  std::vector<std::filesystem::path> examples;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".rpn")
        examples.push_back(entry.path());
  std::ranges::sort(examples);
  if (examples.empty()) {
    std::fprintf(stderr, "rpn: no example documents found\n");
    return 1;
  }

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      rpn::RpnCoreServices, rpn::RpnSharedCoreServices,
      rpn::install_core_module>();
  if (!audit_regressions::run(language.language)) return 1;

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
    if (!result.diagnostics.empty()) {
      fail(path, "parses with diagnostics");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<rpn::ast::Expression>) {
      fail(path, "the parse built no Expression");
      ++failures;
    }
  }
  if (failures != 0) return 1;

  std::printf("rpn: %zu examples parse\n", examples.size());
  return 0;
}
