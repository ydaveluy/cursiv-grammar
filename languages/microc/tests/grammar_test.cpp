#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, read every sample under example/, and check each tree.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "microc/core/CoreServices.hpp"
#include "microc/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "microc: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR))
    if (entry.is_regular_file()) files.push_back(entry.path());
  std::sort(files.begin(), files.end());
  if (files.empty()) return fail(CURSIV_EXAMPLE_DIR, "no example files");

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      microc::MicrocCoreServices,
      microc::MicrocSharedCoreServices,
      microc::install_core_module>();
  if (!audit_regressions::run(language.language, *language.shared)) return 1;

  for (const std::filesystem::path& file : files) {
    std::ifstream stream(file);
    if (!stream) return fail(file, "not readable");
    const std::string text((std::istreambuf_iterator<char>(stream)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail(file, "does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<microc::ast::Program>)
      return fail(file, "built no program");

    const auto& program = *static_cast<const microc::ast::Program*>(root);
    if (program.statements.empty())
      return fail(file, "program holds no statement");
  }

  std::printf("microc: %zu samples parse\n", files.size());
  return 0;
}
