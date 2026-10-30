// The smoke test of the language: every document under example/ parses clean
// and builds the tree it declares.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/workspace/DocumentFactory.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "bdf/core/CoreServices.hpp"
#include "bdf/core/ast.hpp"

namespace {

int fail(const char* path, const char* what) {
  std::fprintf(stderr, "bdf: %s: %s\n", path, what);
  return 1;
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      bdf::BdfCoreServices,
      bdf::BdfSharedCoreServices,
      bdf::install_core_module>();

  int failures = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
    const std::string path = entry.path().string();

    std::ifstream file(entry.path());
    if (!file) {
      failures += fail(path.c_str(), "not readable");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered() || !result.diagnostics.empty()) {
      failures += fail(path.c_str(), "the document does not parse clean");
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<bdf::ast::Font>) {
      failures += fail(path.c_str(), "the parse built no font");
      continue;
    }
  }

  {
    const std::string source =
        "STARTFONT 2.2\nFONT vertical\nSIZE 8 75 75\n"
        "FONTBOUNDINGBOX 8 1 0 0\nCONTENTVERSION 3\nMETRICSSET 1\n"
        "SWIDTH1 0 -1000.25\nDWIDTH1 0 -8\nVVECTOR 4 8\nCHARS 1\n"
        "STARTCHAR a\nENCODING -1 65\nSWIDTH1 0 -999.5\n"
        "DWIDTH1 0 -7\nVVECTOR +3 +7\nBBX 8 1 0 0\nBITMAP\n80\nENDCHAR\nENDFONT\n";
    auto document = language.shared->documentFactory->fromString(
        source, "file:///vertical.bdf");
    const auto& result = document->parseResult;
    const auto* font = result.value_as<bdf::ast::Font>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !font)
      return fail("native-vertical", "BDF 2.2 vertical metrics need repair");
    bool global_width = false, glyph_width = false;
    for (const auto& part : font->parts) {
      if (const auto* line = std::get_if<bdf::ast::Directive*>(&part))
        global_width |= (*line)->keyword == "SWIDTH1" &&
            (*line)->arguments.size() == 2 && (*line)->arguments[1] == "-1000.25";
      if (const auto* glyph = std::get_if<bdf::ast::Char*>(&part))
        for (const auto& glyph_part : (*glyph)->parts)
          if (const auto* line = std::get_if<bdf::ast::Directive*>(&glyph_part))
            glyph_width |= (*line)->keyword == "SWIDTH1" &&
                (*line)->arguments.size() == 2 && (*line)->arguments[1] == "-999.5";
    }
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    if (!global_width || !glyph_width || !diagnostics.empty())
      return fail("native-vertical", "native numeric metrics or AST were lost");
  }

  {
    auto document = language.shared->documentFactory->fromString(
        "STARTFONT 2.2\nFONT missing\nSIZE 8 75 75\nFONTBOUNDINGBOX 8 1 0 0\n"
        "METRICSSET 1\nCHARS 1\nSTARTCHAR a\nENCODING 65\nBBX 8 1 0 0\n"
        "BITMAP\n80\nENDCHAR\nENDFONT\n", "file:///missing-metrics.bdf");
    if (!document->parseResult.accepted || document->parseResult.cst().recovered())
      return fail("native-metrics", "invalid glyph must retain its AST");
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(*document, diagnostics);
    std::size_t missing = 0;
    for (const auto& diagnostic : diagnostics)
      missing += diagnostic.code && *diagnostic.code == "bdf.required-metric";
    if (missing != 3)
      return fail("native-metrics", "vertical inherited metrics were not checked");
  }

  if (failures != 0) return 1;
  std::puts("bdf: every example parses");
  return 0;
}
