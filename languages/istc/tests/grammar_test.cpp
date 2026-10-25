// The smoke test of the language: build the container the way a workspace
// does, parse every file under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "istc/core/CoreServices.hpp"
#include "istc/core/ast.hpp"

namespace {

int fail(const std::string& file, const char* what) {
  std::fprintf(stderr, "istc: %s: %s\n", file.c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      istc::IstcCoreServices, istc::IstcSharedCoreServices,
      istc::install_core_module>();

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".istc")
        continue;
      const std::string path = entry.path().string();

      std::ifstream file(entry.path());
      if (!file) return fail(path, "the file is not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) return fail(path, "does not parse clean");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<istc::ast::Istc>)
        return fail(path, "the parse built no Istc");

      const auto& node = *static_cast<const istc::ast::Istc*>(root);
      if (node.registration.size() != 3 || node.year.size() != 4 ||
          node.work.size() != 8 || node.check.size() != 1)
        return fail(path, "a field does not hold its expected width");

      ++count;
    }

  if (count == 0) return fail(CURSIV_EXAMPLE_DIR, "no example found");

  std::printf("istc: %d example(s) parse\n", count);
  return 0;
}
