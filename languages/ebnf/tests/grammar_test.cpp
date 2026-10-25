// The smoke test of the language: parse every file in example/, and check each
// one builds a clean Document.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "ebnf/core/CoreServices.hpp"
#include "ebnf/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& path, const char* what) {
  std::fprintf(stderr, "ebnf: %s: %s\n", path.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      ebnf::EbnfCoreServices,
      ebnf::EbnfSharedCoreServices,
      ebnf::install_core_module>();

  int failures = 0;
  int parsed = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".ebnf")
      continue;

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(entry.path(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      failures += fail(entry.path(), "does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<ebnf::ast::Document>) {
      failures += fail(entry.path(), "built no Document");
      continue;
    }

    const auto& document = *static_cast<const ebnf::ast::Document*>(root);
    if (document.rules.empty()) {
      failures += fail(entry.path(), "declares no rule");
      continue;
    }

    ++parsed;
  }

  if (parsed == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");
  if (failures > 0) return 1;

  std::printf("ebnf: %d example(s) parse\n", parsed);
  return 0;
}
