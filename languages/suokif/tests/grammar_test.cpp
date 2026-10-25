// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "suokif/core/CoreServices.hpp"
#include "suokif/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "suokif: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  if (!native_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      suokif::SuokifCoreServices, suokif::SuokifSharedCoreServices,
      suokif::install_core_module>();

  int total = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".kif")
      continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) return fail(path, "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(path, "did not parse clean");
    if (!result.diagnostics.empty())
      return fail(path, "left diagnostics behind");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<suokif::ast::Document>)
      return fail(path, "built no Document");
  }
  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  std::printf("suokif: %d example(s) parse\n", total);
  return 0;
}
