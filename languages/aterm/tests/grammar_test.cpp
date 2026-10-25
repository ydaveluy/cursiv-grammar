// The smoke test of the language: build the container the way a workspace
// does, parse every file under example/ and tests/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "aterm/core/CoreServices.hpp"
#include "aterm/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "aterm: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      aterm::AtermCoreServices, aterm::AtermSharedCoreServices,
      aterm::install_core_module>();

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".aterm")
        continue;

      std::ifstream file(entry.path());
      if (!file) return fail("cannot read " + entry.path().string());
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered())
        return fail(entry.path().filename().string() + " does not parse clean");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<aterm::ast::Doc>)
        return fail(entry.path().filename().string() + " built no Doc root");

      ++count;
    }
  }
  if (count == 0) return fail("no example found under " CURSIV_EXAMPLE_DIR);

  std::printf("aterm: %d example(s) parse clean\n", count);
  return 0;
}
