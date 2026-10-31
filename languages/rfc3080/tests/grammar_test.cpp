#include <cstdio>

#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "rfc3080/core/CoreServices.hpp"
#include "rfc3080/core/ast.hpp"
// The grammar's end to end check: every sample under tests/ parses
// clean, with no recovery, and builds a Frame root.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

int fail(const std::string& what) {
  std::fprintf(stderr, "rfc3080: %s\n", what.c_str());
  return 1;
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<rfc3080::Rfc3080CoreServices,
                              rfc3080::Rfc3080SharedCoreServices,
                              rfc3080::install_core_module>();
  {
    const auto r =
        language.language.parser->parse("MSG 0 0 . 0 3\r\nENDEND\r\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* frame = r.value_as<rfc3080::ast::Frame>();
    if (!frame || frame->payload != "END" || frame->size != "3") return 1;
  }
  {
    auto doc = language.shared->documentFactory->fromString(
        "MSG 0 0 . 0 2\r\nxEND\r\n", "file:///audit.beep");
    if (!doc->parseResult.accepted || doc->parseResult.cst().recovered() ||
        !doc->parseResult.diagnostics.empty())
      return 1;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*doc, diagnostics);
    if (diagnostics.empty()) return 1;
  }
  return 0;
}

#include "native_regressions.hpp"

int main() {
  if (native_regressions()) return 1;
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  namespace fs = std::filesystem;

  const auto language =
      cursiv::create_language<rfc3080::Rfc3080CoreServices,
                              rfc3080::Rfc3080SharedCoreServices,
                              rfc3080::install_core_module>();

  int count = 0;
  for (const fs::directory_entry& entry :
       fs::directory_iterator(CURSIV_TEST_DIR)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".beep")
      continue;
    const fs::path path = entry.path();

    std::ifstream file(path, std::ios::binary);
    if (!file) return fail(path.filename().string() + " is not readable");
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered())
      return fail(path.filename().string() + " does not parse clean");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<rfc3080::ast::Frame>)
      return fail(path.filename().string() + " built no frame");
    ++count;
  }
  if (count == 0) return fail("no sample documents found");

  std::printf("rfc3080: %d examples parse\n", count);
  return 0;
}
