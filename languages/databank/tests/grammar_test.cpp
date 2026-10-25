// The smoke test of the language: every file of example/ parses clean and
// builds a Series.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "databank/core/CoreServices.hpp"
#include "databank/core/ast.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "databank: %s\n", what.c_str());
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      databank::DatabankCoreServices, databank::DatabankSharedCoreServices,
      databank::install_core_module>();

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR}) {
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".databank")
        continue;
      const std::string path = entry.path().string();

      std::ifstream file(path);
      if (!file) return fail(path + ": not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) return fail(path + ": did not parse clean");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<databank::ast::Series>)
        return fail(path + ": did not build a Series");
      ++count;
    }
  }

  if (count == 0) return fail("no examples found");
  std::printf("databank: %d examples parse\n", count);
  return 0;
}
