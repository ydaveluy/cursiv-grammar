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

  // Native DCM 2.x matrix syntax and numeric spellings. A dot is a comment
  // marker only at the beginning of a line, never inside a WERT line.
  {
    const std::string source =
        "* initial comment\nKONSERVIERUNG_FORMAT\t 2.0\n"
        "FESTWERTEBLOCK matrix 03 @ +2\n"
        "WERT .5 +1 012\n. comment between rows\n"
        "WERT -2 1e20 123456789012345678901234567890\nEND";
    const auto result = language.language.parser->parse(source);
    const auto* doc = cursiv::ast_cast<dcm::ast::Document>(result.value);
    const auto* block = doc && doc->items.size() == 1
                            ? cursiv::ast_cast<dcm::ast::Kennwerteblock>(doc->items[0])
                            : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !block || block->count != 3 ||
        !block->countY || *block->countY != 2 || block->blocks.size() != 2 ||
        block->blocks[0]->numbers.size() != 3 ||
        block->blocks[0]->numbers[0] != .5 ||
        block->blocks[0]->numbers[1] != 1 ||
        block->blocks[0]->numbers[2] != 12 ||
        block->blocks[1]->numbers.size() != 3 ||
        block->blocks[1]->numbers[2] <= 1e29)
      failures += fail("native-matrix", "matrix dimensions or values were lost");
  }
  {
    const auto result = language.language.parser->parse(
        "KONSERVIERUNG_FORMAT 2.0\n"
        "FESTKENNLINIE curve 2\nWERT 1 2\nEND\n"
        "GRUPPENKENNFELD map 2 2\n*SSTX external_distribution\n"
        "WERT 1 2\nWERT 3 4\nEND\n! final comment");
    const auto* doc = cursiv::ast_cast<dcm::ast::Document>(result.value);
    const auto* curve = doc && doc->items.size() == 2
                            ? cursiv::ast_cast<dcm::ast::Kennlinie>(doc->items[0])
                            : nullptr;
    const auto* map = doc && doc->items.size() == 2
                          ? cursiv::ast_cast<dcm::ast::Kennfeld>(doc->items[1])
                          : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !curve || !map ||
        curve->kind != dcm::ast::CurveKind::Fixed || !curve->supportX.empty() ||
        map->kind != dcm::ast::CurveKind::Group || map->rows.size() != 1 ||
        map->rows[0]->y || map->rows[0]->rows.size() != 2)
      failures += fail("native-fixed", "native fixed/group rows were lost");
  }

  if (count == 0) {
    std::fprintf(stderr, "dcm: no .dcm file found under tests/\n");
    return 1;
  }
  if (failures > 0) return 1;

  std::printf("dcm: %d example documents parse clean\n", count);
  return 0;
}
