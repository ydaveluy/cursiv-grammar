// The grammar end to end: every document under `tests/` parses clean --
// no recovery, no diagnostics -- and builds a Document at the root.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "dcm/core/CoreServices.hpp"
#include "dcm/core/ast.hpp"

namespace {

namespace fs = std::filesystem;

int fail(const fs::path& file, const char* what) {
  std::fprintf(stderr, "dcm: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      dcm::DcmCoreServices, dcm::DcmSharedCoreServices,
      dcm::install_core_module>();

  int failures = 0;
  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".dcm")
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
        root->type != &cursiv::ast_type_tag<dcm::ast::Document>) {
      failures += fail(entry.path(), "the parse built no Document");
      continue;
    }
  }

  if (count == 0) {
    std::fprintf(stderr, "dcm: no .dcm file found under tests/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("dcm: %d example documents parse clean\n", count);
  return 0;
}
