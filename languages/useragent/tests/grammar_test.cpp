#include "audit_regressions.hpp"
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
#include "useragent/core/CoreServices.hpp"
#include "useragent/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "useragent: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      useragent::UseragentCoreServices, useragent::UseragentSharedCoreServices,
      useragent::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const auto& entry :
      std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
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

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<useragent::ast::Document>) {
      fail(path, "the parse built no Document");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "useragent: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("useragent: %d example(s) parse\n", total);
  return 0;
}
