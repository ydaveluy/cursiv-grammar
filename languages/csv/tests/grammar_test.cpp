#include "value_regressions.hpp"
// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "csv/core/CoreServices.hpp"
#include "csv/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "csv: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      csv::CsvCoreServices, csv::CsvSharedCoreServices,
      csv::install_core_module>();

  if (value_regressions::run(language)) return 1;

  int total = 0;
  int failures = 0;
  for (const auto& entry :
      std::filesystem::directory_iterator(CURSIV_EXAMPLE_DIR)) {
    if (!entry.is_regular_file()) continue;
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
        root->type != &cursiv::ast_type_tag<csv::ast::Doc>) {
      fail(path, "the parse built no Doc");
      ++failures;
    }
  }

  // A document does not imply a header or a final line ending. Blank
  // fields and actual blank lines still have their own positions.
  for (const auto &[text, rowCount] :
       {std::pair<std::string_view, std::size_t>{"", 0},
        {"a,b", 1},
        {"a,b\n", 1},
        {"a,b\r\nc,d", 2},
        {",,\n\n", 2}}) {
    const auto result = language.language.parser->parse(text);
    const auto *doc = result.value && result.value->type ==
                                          &cursiv::ast_type_tag<csv::ast::Doc>
                          ? static_cast<const csv::ast::Doc *>(result.value)
                          : nullptr;
    if (result.cst().recovered() || !doc || doc->rows.size() != rowCount ||
        (text == ",,\n\n" && (doc->rows[0]->fields.size() != 3 ||
                              doc->rows[1]->fields.size() != 1))) {
      std::fprintf(stderr, "csv: row/field positions changed for '%.*s'\n",
                   static_cast<int>(text.size()), text.data());
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "csv: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("csv: %d example(s) parse\n", total);
  return 0;
}
