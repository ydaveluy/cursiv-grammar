#include "code_audit.hpp"
// The smoke test of the language: build the container the way a workspace
// does, and check that every document under example/ parses clean into a Fen.

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
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "fen/core/CoreServices.hpp"
#include "fen/core/ast.hpp"

namespace {

void fail(const std::string& path, const char* what) {
  std::fprintf(stderr, "fen: %s: %s\n", path.c_str(), what);
}

}  // namespace

int main() {
  if (!code_audit::run()) return 1;
  {
    const auto audit_language =
        cursiv::create_language<fen::FenCoreServices,
                                fen::FenSharedCoreServices,
                                fen::install_core_module>();
    const auto audit_result =
        audit_language.language.parser->parse("8/8/8/8/8/8/8/8 w KK - 0 1");
    if (!audit_result.accepted || audit_result.cst().recovered() ||
        !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
    const auto* fen = audit_result.value_as<fen::ast::Fen>();
    if (!fen || fen->castling != "KK") return 1;
    const auto checked = cursiv::test::validationHelper(
        audit_language.language, "8/8/8/8/8/8/8/8 w KK - 0 1");
    if (checked.diagnostics.size() != 1 ||
        checked.diagnostics[0].message.find("repeated") == std::string::npos)
      return 1;
  }

  {
    const auto language = cursiv::create_language<fen::FenCoreServices,
        fen::FenSharedCoreServices, fen::install_core_module>();
    const auto parsed = language.language.parser->parse("8/8/8/8/8/8/8/8 w - - +0 -0");
    const auto* value = parsed.value_as<fen::ast::Fen>();
    const auto checked = cursiv::test::validationHelper(language.language,
        "8/8/8/8/8/8/8/8 w - - +0 -0");
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !value || value->halfmoveClock != "+0" || value->fullmoveClock != "-0" ||
        !checked.diagnostics.empty()) return 1;
  }

  std::vector<std::filesystem::path> examples;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir))
      if (entry.is_regular_file() && entry.path().extension() == ".fen")
        examples.push_back(entry.path());
  std::ranges::sort(examples);
  if (examples.empty()) {
    std::fprintf(stderr, "fen: no example documents found\n");
    return 1;
  }

  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language =
      cursiv::create_language<fen::FenCoreServices, fen::FenSharedCoreServices,
                              fen::install_core_module>();

  int failures = 0;
  for (const std::filesystem::path& example : examples) {
    const std::string path = example.string();
    std::ifstream file(path);
    if (!file) {
      fail(path, "not readable");
      ++failures;
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) {
      fail(path, "does not parse clean");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr || root->type != &cursiv::ast_type_tag<fen::ast::Fen>) {
      fail(path, "the parse built no Fen");
      ++failures;
      continue;
    }

    const auto& record = *static_cast<const fen::ast::Fen*>(root);
    if (record.placement == nullptr || record.placement->ranks.size() != 8) {
      fail(path, "the board does not hold eight ranks");
      ++failures;
    }
  }
  if (failures != 0) return 1;

  std::printf("fen: %zu examples parse\n", examples.size());
  return 0;
}
