// The smoke test of the language: parse every file under example/, and
// fail on a recovered parse or a wrong root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "lisa/core/CoreServices.hpp"
#include "lisa/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& path, const char* what) {
  std::fprintf(stderr, "lisa: %s: %s\n", path.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      lisa::LisaCoreServices, lisa::LisaSharedCoreServices,
      lisa::install_core_module>();

  std::vector<fs::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : fs::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".lisa")
        files.push_back(entry.path());
  if (files.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example files found");

  for (const fs::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<lisa::ast::Program>)
      return fail(path, "the parse built no Program");
    if (!result.diagnostics.empty())
      return fail(path, "the parse carries diagnostics");
  }

  std::printf("lisa: %zu example files parse clean\n", files.size());
  return 0;
}
