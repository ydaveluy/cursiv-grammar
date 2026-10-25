// The grammar's own corpus: every document under example/ parses clean, with
// no diagnostics and a Document root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/Check.hpp"
#include "pest/core/CoreServices.hpp"
#include "pest/core/ast.hpp"
#include "native_regressions.hpp"

namespace {

namespace fs = std::filesystem;

std::string read_file(const fs::path& path) {
  std::ifstream file(path);
  return {std::istreambuf_iterator<char>(file),
          std::istreambuf_iterator<char>()};
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      pest::PestCoreServices, pest::PestSharedCoreServices,
      pest::install_core_module>();

  std::vector<fs::path> examples;
  for (const auto& entry : fs::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.path().extension() == ".pest") examples.push_back(entry.path());
  CHECK(!examples.empty());

  for (const fs::path& path : examples) {
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      std::printf("FAIL %s: did not parse clean\n", path.c_str());
      ++cursiv::test::failures;
      continue;
    }
    if (!result.diagnostics.empty()) {
      std::printf("FAIL %s: %zu diagnostic(s)\n", path.c_str(),
                  result.diagnostics.size());
      ++cursiv::test::failures;
      continue;
    }
    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<pest::ast::Document>) {
      std::printf("FAIL %s: no Document root\n", path.c_str());
      ++cursiv::test::failures;
      continue;
    }
    const auto& doc = *static_cast<const pest::ast::Document*>(root);
    if (doc.rules.empty()) {
      std::printf("FAIL %s: no rule declared\n", path.c_str());
      ++cursiv::test::failures;
    }
  }

  // features.pest: tests/, not example/ (this port's own document). Its
  // `mixed` rule reads `"a"* ~ "b"? | &"b"+ ~ "a"` back as two Sequence
  // alternatives -- sequence bound tighter than choice, not left flat.
  {
    const std::string text =
        read_file(fs::path(CURSIV_TEST_DIR) / "features.pest");
    const cursiv::ParseResult result = language.language.parser->parse(text);
    CHECK(!result.cst().recovered());
    CHECK(result.diagnostics.empty());
    const auto* const doc =
        result.value != nullptr &&
                result.value->type == &cursiv::ast_type_tag<pest::ast::Document>
            ? static_cast<const pest::ast::Document*>(result.value)
            : nullptr;
    const pest::ast::RuleDecl* mixed = nullptr;
    if (doc != nullptr)
      for (pest::ast::RuleDecl* const r : doc->rules)
        if (r != nullptr && r->name == "mixed") mixed = r;
    CHECK(mixed != nullptr);
    const pest::ast::Expression* const expr =
        mixed != nullptr ? mixed->expression : nullptr;
    CHECK(expr != nullptr && expr->alternatives.size() == 2);
    if (expr != nullptr && expr->alternatives.size() == 2) {
      CHECK(expr->alternatives[0]->terms.size() == 2);
      CHECK(expr->alternatives[1]->terms.size() == 2);
      CHECK(!expr->alternatives[1]->terms.empty() &&
            !expr->alternatives[1]->terms[0]->prefixes.empty() &&
            expr->alternatives[1]->terms[0]->prefixes[0] ==
                pest::ast::PrefixOp::positive_predicate);
    }
  }

  CHECK(native_regressions(language.language));
  return cursiv::test::report("pest_grammar_test");
}
