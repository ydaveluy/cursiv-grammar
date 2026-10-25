#include "value_regressions.hpp"
// The smoke test of the language: parse every file in example/, fail on a
// recovered parse or the wrong root type.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "tsv/core/CoreServices.hpp"
#include "tsv/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "tsv: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      tsv::TsvCoreServices,
      tsv::TsvSharedCoreServices,
      tsv::install_core_module>();

  if (value_regressions::run(language)) return 1;

  std::vector<std::filesystem::path> files;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".tsv")
        files.push_back(entry.path());
  if (files.empty()) {
    std::fprintf(stderr, "tsv: no example files found\n");
    return 1;
  }

  for (const std::filesystem::path& file : files) {
    std::ifstream in(file);
    if (!in) return fail(file, "the file is not readable");
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) return fail(file, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<tsv::ast::Document>)
      return fail(file, "the parse built no document");
  }

  std::printf("tsv: %zu example(s) parse\n", files.size());
  return 0;
}
