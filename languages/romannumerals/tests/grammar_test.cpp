#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "romannumerals/core/CoreServices.hpp"
#include "romannumerals/core/ast.hpp"
// The smoke test of the language: parse every document of example/ and check
// its root type.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "romannumerals: %s\n", what.c_str());
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<romannumerals::RomannumeralsCoreServices,
                              romannumerals::RomannumeralsSharedCoreServices,
                              romannumerals::install_core_module>();
  for (const char* spelling : {"I", "IV", "IX", "MCMLXXII", "MMMMCMXCIX",
                               "iv", "mcmxcix", "mIx", "N", "n", "MMXXVI",
                               "DCCC", "XLII", "IIV", "IL", "NN", "NI"}) {
    const std::string source(spelling);
    const auto result = language.language.parser->parse(source);
    const auto* node = cursiv::ast_cast<romannumerals::ast::RomanNumeral>(result.value);
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !node || node->text != source || node->parent != nullptr)
      return 1;
    auto document = language.shared->documentFactory->fromString(
        source, "file:///audit.romannumerals");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    const bool invalid = source == "IIV" || source == "IL" || source == "NN" || source == "NI";
    if (diagnostics.empty() == invalid) return 1;
  }
  return 0;
}

int main() {
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      romannumerals::RomannumeralsCoreServices,
      romannumerals::RomannumeralsSharedCoreServices,
      romannumerals::install_core_module>();

  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() ||
          entry.path().extension() != ".romannumerals")
        continue;
      const std::string path = entry.path().string();
      std::ifstream file(path);
      if (!file) return fail(path + ": not readable");
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) return fail(path + ": did not parse clean");
      if (!result.diagnostics.empty()) return fail(path + ": left diagnostics");

      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type !=
              &cursiv::ast_type_tag<romannumerals::ast::RomanNumeral>)
        return fail(path + ": built no RomanNumeral");
      ++count;
    }
  if (count == 0) return fail("no example documents found");

  std::printf("romannumerals: %d examples parse\n", count);
  return 0;
}
