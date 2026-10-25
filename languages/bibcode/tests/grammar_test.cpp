// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "bibcode/core/CoreServices.hpp"
#include "bibcode/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "bibcode: %s: %s\n", file.string().c_str(), what);
  return 1;
}

bool regressions(const auto& language) {
  const auto check = [](bool value, const char* message) {
    if (!value) std::fprintf(stderr, "bibcode regression: %s\n", message);
    return value;
  };
  for (const std::string_view text : {"2020A&A...641A...6P", "2020A&AS..641....6.",
                                    "2020ApJ...900....13", "2020ApJ...900....1_"}) {
    const auto parsed = language.language.parser->parse(text);
    const auto* root = cursiv::ast_cast<bibcode::ast::Bibcode>(parsed.value);
    if (!check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && root,
               "ADS journal punctuation and author initials parse cleanly")) return false;
    const std::array<std::string_view, 6> fields = {
        root->year, root->publish, root->volume, root->section, root->page, root->author};
    constexpr std::array<std::size_t, 7> positions = {0, 4, 9, 13, 14, 18, 19};
    for (std::size_t i = 0; i < fields.size(); ++i)
      if (!check(fields[i] == text.substr(positions[i], positions[i + 1] - positions[i]),
                 "all six fields retain their exact columns")) return false;
    if (!check(cursiv::test::validationHelper(language.language, std::string(text)).diagnostics.empty(),
               "native ADS identifiers have no validation errors")) return false;
  }
  {
    const auto parsed = language.language.parser->parse(" \t2020A&A...641A...6P\r\n");
    const auto* root = cursiv::ast_cast<bibcode::ast::Bibcode>(parsed.value);
    if (!check(parsed.accepted && !parsed.cst().recovered() && root &&
                   root->year == "2020" && root->publish == "A&A.." && root->author == "P",
               "surrounding whitespace does not change field positions")) return false;
  }
  struct Invalid { std::string text; std::string_view code; std::size_t begin, end; };
  for (const auto& item : {Invalid{"20X0ApJ...900....1A", "bibcode.year", 0, 4},
                           Invalid{"2020A A...900....1A", "bibcode.field", 4, 9},
                           Invalid{"2020ApJ...900....1\t", "bibcode.field", 18, 19},
                           Invalid{std::string("2020ApJ...900....1\0", 19), "bibcode.field", 18, 19}}) {
    const auto parsed = language.language.parser->parse(item.text);
    if (!check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
               "invalid field content remains parseable")) return false;
    const auto diagnostics = cursiv::test::validationHelper(language.language, item.text).diagnostics;
    if (!check(diagnostics.size() == 1 && diagnostics[0].code && diagnostics[0].code->isString() &&
                   diagnostics[0].code->string() == item.code &&
                   diagnostics[0].range.begin == item.begin && diagnostics[0].range.end == item.end,
               "runtime errors point to the invalid field")) return false;
  }
  return true;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      bibcode::BibcodeCoreServices, bibcode::BibcodeSharedCoreServices,
      bibcode::install_core_module>();

  if (!regressions(language)) return 1;

  int total = 0;
  int failures = 0;
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::filesystem::path& file = entry.path();
    ++total;

    std::ifstream in(file);
    if (!in) {
      failures += fail(file, "the document is not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      failures += fail(file, "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bibcode::ast::Bibcode>)
      failures += fail(file, "the parse built no Bibcode");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("bibcode: %d example(s) parse\n", total);
  return 0;
}
