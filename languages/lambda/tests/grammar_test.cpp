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
#include "lambda/core/CoreServices.hpp"
#include "lambda/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "lambda: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      lambda::LambdaCoreServices, lambda::LambdaSharedCoreServices,
      lambda::install_core_module>();

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

    // The root is one of Variable, Function or Application -- whichever the
    // top-level term is -- so the test asks the family, not the exact class.
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        !root->type->derivesFrom(&cursiv::ast_type_tag<lambda::ast::Expression>)) {
      fail(path, "the parse built no Expression");
      ++failures;
    }
  }

  for (const char* invalid : {"a\v", "a\f", "(a b) extra"}) {
    const auto result = language.language.parser->parse(invalid);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty()) {
      std::fprintf(stderr, "lambda: native whitespace or trailing-text restriction lost\n");
      return 1;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "lambda: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("lambda: %d example(s) parse\n", total);
  return 0;
}
