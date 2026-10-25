#include "audit_regressions.hpp"
// The smoke test of the language: parse every file under example/ (the
// upstream corpus) and tests/ (constructs the corpus does not reach), the
// way a workspace would, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "trapc/core/CoreServices.hpp"
#include "trapc/core/ast.hpp"

namespace {

std::string read_file(const std::filesystem::path& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  const auto language = cursiv::create_language<
      trapc::TrapcCoreServices, trapc::TrapcSharedCoreServices,
      trapc::install_core_module>();

  std::vector<std::filesystem::path> examples;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) examples.push_back(entry.path());
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_TEST_DIR))
    if (entry.is_regular_file() && entry.path().extension() == ".tc")
      examples.push_back(entry.path());

  if (examples.empty()) {
    std::fprintf(stderr, "trapc: no example files found\n");
    return 1;
  }

  int failures = 0;
  for (const std::filesystem::path& path : examples) {
    const std::string text = read_file(path);
    const cursiv::ParseResult result = language.language.parser->parse(text);

    if (result.cst().recovered()) {
      std::fprintf(stderr, "trapc: %s does not parse clean\n",
                   path.filename().c_str());
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<trapc::ast::TranslationUnit>) {
      std::fprintf(stderr, "trapc: %s built no translation unit\n",
                   path.filename().c_str());
      ++failures;
      continue;
    }

    std::printf("trapc: %s parses (%zu items)\n", path.filename().c_str(),
               static_cast<const trapc::ast::TranslationUnit*>(root)
                   ->items.size());
  }

  if (failures != 0) {
    std::fprintf(stderr, "trapc: %d of %zu examples failed\n", failures,
                 examples.size());
    return 1;
  }
  return 0;
}
