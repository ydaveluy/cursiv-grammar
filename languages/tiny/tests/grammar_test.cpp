// The smoke test of the language: build the container the way a workspace
// does, and check that every document in example/ parses clean into a Program.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "tiny/core/CoreServices.hpp"
#include "tiny/core/ast.hpp"

#include "native_cases.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "tiny: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  namespace fs = std::filesystem;

  const auto language = cursiv::create_language<
      tiny::TinyCoreServices,
      tiny::TinySharedCoreServices,
      tiny::install_core_module>();

  if (!audit_regressions::run(language.language, *language.shared)) return fail("native AST or runtime mismatch");

  int parsed = 0;
  // lists.tiny lives beside this test, not in the corpus: it covers the
  // fused-sign integer terminal (`a - -5`), which no upstream example needs.
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".tiny")
      continue;
    const fs::path path = entry.path();

    std::ifstream file(path);
    if (!file) return fail(path.string() + ": not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.string() + ": did not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<tiny::ast::Program>)
      return fail(path.string() + ": built no Program");

    ++parsed;
  }

  if (parsed == 0) return fail("no examples found");

  std::printf("tiny: %d example(s) parse clean\n", parsed);
  return 0;
}
