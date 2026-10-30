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
#include "tinybasic/core/CoreServices.hpp"
#include "tinybasic/core/ast.hpp"
#include "native_followup_cases.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "tinybasic: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      tinybasic::TinybasicCoreServices, tinybasic::TinybasicSharedCoreServices,
      tinybasic::install_core_module>();
  if (!tinybasic_native_followup::run(language)) return fail("native followup", "arithmetic, digits or AST mismatch");

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<tinybasic::ast::Program>)
      failures += fail(file, "the parse built no Program");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");

  // coverage.tinybasic: my own document, exercising constructs example1
  // does not. Never counted in `total`: only example/ is the upstream
  // corpus.
  int testDocs = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".tinybasic")
      continue;
    const std::filesystem::path& file = entry.path();
    ++testDocs;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<tinybasic::ast::Program>)
      failures += fail(file, "the parse built no Program");
  }
  if (testDocs == 0) return fail(CURSIV_TEST_DIR, "no test documents");

  if (failures > 0) return 1;

  std::printf("tinybasic: %d example(s), %d test document(s) parse\n", total,
             testDocs);
  return 0;
}
