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
#include "arithmetic/core/CoreServices.hpp"
#include "arithmetic/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "arithmetic: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      arithmetic::ArithmeticCoreServices, arithmetic::ArithmeticSharedCoreServices,
      arithmetic::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() ||
        entry.path().extension() != ".arithmetic")
      continue;
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
        root->type != &cursiv::ast_type_tag<arithmetic::ast::File>) {
      fail(path, "the parse built no File");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "arithmetic: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;

  // `^` is right-associative: `2^3^2` is `2^(3^2)`, not `(2^3)^2`.
  {
    const cursiv::ParseResult result =
        language.language.parser->parse("2^3^2=1");
    const auto* const file =
        static_cast<const arithmetic::ast::File*>(result.value);
    if (file == nullptr || file->equations.size() != 1) {
      std::fprintf(stderr, "arithmetic: 2^3^2=1: expected one equation\n");
      return 1;
    }
    const auto* const outer = static_cast<const arithmetic::ast::BinaryExpression*>(
        file->equations[0]->left);
    const auto* const rightOperand =
        static_cast<const arithmetic::ast::BinaryExpression*>(outer->right);
    if (outer->op != arithmetic::ast::Operator::exponent ||
        rightOperand->type !=
            &cursiv::ast_type_tag<arithmetic::ast::BinaryExpression> ||
        rightOperand->op != arithmetic::ast::Operator::exponent) {
      std::fprintf(stderr, "arithmetic: 2^3^2=1: expected 2^(3^2)\n");
      return 1;
    }
  }

  std::printf("arithmetic: %d example(s) parse\n", total);
  return 0;
}
