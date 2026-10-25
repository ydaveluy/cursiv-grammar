
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "morsecode/core/CoreServices.hpp"
#include "morsecode/core/ast.hpp"
#include "table_regressions.hpp"
// The grammar end to end: every document under `example/` parses clean --
// no recovery, no diagnostics -- and builds a Message at the root.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "morsecode: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<morsecode::MorsecodeCoreServices,
                              morsecode::MorsecodeSharedCoreServices,
                              morsecode::install_core_module>();
  {
    const auto r = language.language.parser->parse(".");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
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
      morsecode::MorsecodeCoreServices, morsecode::MorsecodeSharedCoreServices,
      morsecode::install_core_module>();

  int failures = morse_table_regressions(language.language);
  int count = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR "/example", CURSIV_TEST_DIR})
    for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".morsecode")
        continue;
      ++count;

      std::ifstream file(entry.path());
      if (!file) {
        failures += fail(entry.path(), "not readable");
        continue;
      }
      const std::string text((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

      const cursiv::ParseResult result = language.language.parser->parse(text);
      if (result.cst().recovered()) {
        failures += fail(entry.path(), "the document does not parse clean");
        continue;
      }
      if (!result.diagnostics.empty()) {
        failures += fail(entry.path(), "the document carries diagnostics");
        continue;
      }
      const cursiv::AstNode* const root = result.value;
      if (root == nullptr ||
          root->type != &cursiv::ast_type_tag<morsecode::ast::Message>) {
        failures += fail(entry.path(), "the parse built no Message");
        continue;
      }
    }

  if (count == 0) {
    std::fprintf(stderr, "morsecode: no .morsecode file found under example/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("morsecode: %d example documents parse clean\n", count);
  return 0;
}
