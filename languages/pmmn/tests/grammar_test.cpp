// The smoke test of the language: the upstream corpus ships no examples/ at
// all, so every document lives under tests/ instead; parse each and check
// its tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "pmmn/core/CoreServices.hpp"
#include "pmmn/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "pmmn: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      pmmn::PmmnCoreServices, pmmn::PmmnSharedCoreServices,
      pmmn::install_core_module>();

  std::vector<std::filesystem::path> files;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".pmmn")
      files.push_back(entry.path());
  if (files.empty()) return fail("no example document found");

  int parsed = 0;
  for (const std::filesystem::path& path : files) {
    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered())
      return fail(path.string() + ": did not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pmmn::ast::Program>)
      return fail(path.string() + ": the parse built no Program");

    ++parsed;
  }

  std::printf("pmmn: %d example(s) parse\n", parsed);
  return 0;
}
