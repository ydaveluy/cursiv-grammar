#include "audit_regressions.hpp"
#include "native_cases.hpp"
#include "parameter_regressions.hpp"
// The grammar of this language: every file under example/ and tests/
// parses clean and builds a Document root.

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
#include "stellaris/core/CoreServices.hpp"
#include "stellaris/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "stellaris: %s\n", what.c_str());
  return 1;
}

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      stellaris::StellarisCoreServices, stellaris::StellarisSharedCoreServices,
      stellaris::install_core_module>();

  if (!native_regressions::run(language.language, *language.shared)) return 1;
  if (!parameter_regressions::run(language.language, *language.shared)) return 1;

  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() &&
          entry.path().extension() == ".stellaris")
        files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  if (files.empty()) return fail("no example files found");

  for (const std::filesystem::path& path : files) {
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + ": did not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<stellaris::ast::Document>)
      return fail(path.filename().string() + ": root is not a Document");
  }

  std::printf("stellaris: %zu examples parse clean\n", files.size());
  return 0;
}
