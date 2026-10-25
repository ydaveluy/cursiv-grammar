// The grammar's smoke test: every document of example/ parses clean (no
// recovery, no diagnostics) and builds a Model.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "alpaca/core/CoreServices.hpp"
#include "alpaca/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "alpaca: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      alpaca::AlpacaCoreServices, alpaca::AlpacaSharedCoreServices,
      alpaca::install_core_module>();

  int seen = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (entry.path().extension() != ".alp") continue;
    ++seen;

    std::ifstream file(entry.path());
    if (!file) return fail(entry.path(), "not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result =
        language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(entry.path(), "did not parse clean");
    if (!result.diagnostics.empty())
      return fail(entry.path(), "left diagnostics behind");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<alpaca::ast::Model>)
      return fail(entry.path(), "built no Model");
  }
  if (seen == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  std::puts("alpaca: the examples parse");
  return 0;
}
