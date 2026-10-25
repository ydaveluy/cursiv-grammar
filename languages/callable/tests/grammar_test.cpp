// The smoke test of the language: parse every file under example/, the way a
// workspace would, and check each tree.

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
#include "callable/core/CoreServices.hpp"
#include "callable/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "callable: %s\n", what.c_str());
  return 1;
}

std::string read(const std::filesystem::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      callable::CallableCoreServices, callable::CallableSharedCoreServices,
      callable::install_core_module>();

  std::vector<std::filesystem::path> examples;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".callable")
        examples.push_back(entry.path());
  std::sort(examples.begin(), examples.end());
  if (examples.empty()) return fail("no example documents found");

  for (const std::filesystem::path& path : examples) {
    const std::string text = read(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<callable::ast::Program>)
      return fail(path.filename().string() + " built no program");

    const auto& program = *static_cast<const callable::ast::Program*>(root);
    if (program.lines.empty())
      return fail(path.filename().string() + " built an empty program");
  }

  std::printf("callable: %zu examples parse\n", examples.size());
  return 0;
}
