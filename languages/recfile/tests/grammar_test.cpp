
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "recfile/core/CoreServices.hpp"
#include "recfile/core/ast.hpp"
// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "recfile: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<recfile::RecfileCoreServices,
                              recfile::RecfileSharedCoreServices,
                              recfile::install_core_module>();
  {
    const auto r = language.language.parser->parse("Name: Ada\n\nName: Bob\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<recfile::ast::Document>();
    if (!doc || doc->entries.size() != 2 ||
        !std::holds_alternative<recfile::ast::Record*>(
            doc->entries[0]) ||
        !std::holds_alternative<recfile::ast::Record*>(doc->entries[1]))
      return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      recfile::RecfileCoreServices, recfile::RecfileSharedCoreServices,
      recfile::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".rec")
      continue;
    ++total;
    const std::filesystem::path path = entry.path();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<recfile::ast::Document>) {
      fail(path, "the parse built no Document");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "recfile: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("recfile: %d example(s) parse\n", total);
  return 0;
}
