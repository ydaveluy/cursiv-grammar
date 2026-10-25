// The smoke test of the language: every file under example/ parses cleanly and
// builds a Proposition.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ltl/core/CoreServices.hpp"
#include "ltl/core/ast.hpp"

#include "audit_regressions.hpp"

namespace {

std::string read(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// UNTIL/WEAK/RELEASE bind tighter than AND/OR/ARROW: `a U b∧c` is
// `(a U b)∧c`, `a∧b U c` is `a∧(b U c)`.
bool check_until_and_precedence(ltl::LtlCoreServices &language) {
  const cursiv::ParseResult r1 = language.parser->parse("a U b\xe2\x88\xa7"
                                                        "c");
  const auto *top1 = cursiv::ast_cast<ltl::ast::BinaryExpr>(r1.value);
  if (top1 == nullptr || top1->op != ltl::ast::BinaryOp::land) {
    std::fprintf(stderr, "ltl: 'a U b and c' is not top-level AND\n");
    return false;
  }
  const auto *left1 = cursiv::ast_cast<ltl::ast::BinaryExpr>(top1->left);
  if (left1 == nullptr || left1->op != ltl::ast::BinaryOp::until) {
    std::fprintf(stderr, "ltl: 'a U b and c' left operand is not U\n");
    return false;
  }

  const cursiv::ParseResult r2 = language.parser->parse("a\xe2\x88\xa7"
                                                        "b U c");
  const auto *top2 = cursiv::ast_cast<ltl::ast::BinaryExpr>(r2.value);
  if (top2 == nullptr || top2->op != ltl::ast::BinaryOp::land) {
    std::fprintf(stderr, "ltl: 'a and b U c' is not top-level AND\n");
    return false;
  }
  const auto *right2 = cursiv::ast_cast<ltl::ast::BinaryExpr>(top2->right);
  if (right2 == nullptr || right2->op != ltl::ast::BinaryOp::until) {
    std::fprintf(stderr, "ltl: 'a and b U c' right operand is not U\n");
    return false;
  }
  return true;
}

} // namespace

int main() {
  const auto language =
      cursiv::create_language<ltl::LtlCoreServices, ltl::LtlSharedCoreServices,
                              ltl::install_core_module>();

  ltl_regressions(language.language);

  if (!check_until_and_precedence(language.language))
    return 1;

  int checked = 0;
  int failed = 0;
  for (const char *const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".ltl")
        continue;
      const std::filesystem::path path = entry.path();
      const std::string name = path.filename().string();
      ++checked;

      const std::string text = read(path);
      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        std::fprintf(stderr, "ltl: %s does not parse cleanly\n", name.c_str());
        ++failed;
        continue;
      }
      if (cursiv::ast_cast<ltl::ast::Proposition>(result.value) == nullptr) {
        std::fprintf(stderr, "ltl: %s built no proposition\n", name.c_str());
        ++failed;
      }
    }

  if (checked == 0) {
    std::fprintf(stderr, "ltl: no examples found in %s\n", CURSIV_EXAMPLE_DIR);
    return 1;
  }
  if (failed != 0) {
    std::fprintf(stderr, "ltl: %d/%d example(s) failed\n", failed, checked);
    return 1;
  }
  std::printf("ltl: %d example(s) parse\n", checked);
  return cursiv::test::report("ltl_grammar_test");
}
