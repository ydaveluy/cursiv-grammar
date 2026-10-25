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
#include "gedcom/core/CoreServices.hpp"
#include "gedcom/core/ast.hpp"
#include "native_cases.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "gedcom: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      gedcom::GedcomCoreServices, gedcom::GedcomSharedCoreServices,
      gedcom::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("0 HEAD\n1 NOTE caf\u00e9\n1 NOTE @I@\n0 TRLR\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      gedcom::GedcomCoreServices, gedcom::GedcomSharedCoreServices,
      gedcom::install_core_module>();

  int total = 0;
  int failures = native_regressions::run(language);
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".ged")
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
        root->type != &cursiv::ast_type_tag<gedcom::ast::Document>) {
      fail(path, "the parse built no Document");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "gedcom: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("gedcom: %d example(s) parse\n", total);
  return 0;
}
