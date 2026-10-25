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
#include "gff3/core/CoreServices.hpp"
#include "gff3/core/ast.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "gff3: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  {
    const auto audit_language = cursiv::create_language<
      gff3::Gff3CoreServices, gff3::Gff3SharedCoreServices,
      gff3::install_core_module>();
    const auto audit_result = audit_language.language.parser->parse("##gff-version 3\nchr1\t.\tgene\t1\t2\t.\t+\t.\t.\n");
    if (!audit_result.accepted || audit_result.cst().recovered() || !audit_result.diagnostics.empty() || audit_result.value == nullptr) {
      std::fprintf(stderr, "audit regression: clean parse expected\n");
      return 1;
    }
  }

  const auto language = cursiv::create_language<
      gff3::Gff3CoreServices, gff3::Gff3SharedCoreServices,
      gff3::install_core_module>();

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (!entry.is_regular_file() || entry.path().extension() != ".gff")
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
          root->type != &cursiv::ast_type_tag<gff3::ast::Document>) {
        fail(path, "the parse built no Document");
        ++failures;
      }
    }

  if (total == 0) {
    std::fprintf(stderr, "gff3: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("gff3: %d example(s) parse\n", total);
  return 0;
}
