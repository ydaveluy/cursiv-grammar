#include "audit_regressions.hpp"
// The smoke test of the language: build the container the way a workspace
// does, parse every example, and check each tree.

#include <cstdio>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "dif/core/CoreServices.hpp"
#include "dif/core/ast.hpp"

namespace {

int fail(const char* what) {
  std::fprintf(stderr, "dif: %s\n", what);
  return 1;
}

std::string read_file(const std::string& path) {
  std::ifstream file(path);
  if (!file) return {};
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

struct Sample {
  const char* file;
  std::size_t items;
};

constexpr Sample kSamples[] = {
    {"/example1.dif", 10},
    {"/example2.dif", 10},
};

}  // namespace

int main() {
  if (!audit_regressions::run()) return 1;
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      dif::DifCoreServices, dif::DifSharedCoreServices,
      dif::install_core_module>();

  for (const Sample& sample : kSamples) {
    const std::string path = std::string(CURSIV_EXAMPLE_DIR) + sample.file;
    const std::string text = read_file(path);
    if (text.empty()) return fail("an example is not readable");

    const cursiv::ParseResult result = language.language.parser->parse(text);
    // `recovered()` is set when the parser had to repair the input to keep
    // going: a clean document leaves it false.
    if (result.cst().recovered()) return fail("an example does not parse");

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<dif::ast::Document>)
      return fail("the parse built no document");

    const auto& doc = *static_cast<const dif::ast::Document*>(root);
    if (!doc.header || doc.header->sections.size() != 4)
      return fail("the document is missing a header section");
    const std::string_view topics[]{"TABLE", "VECTORS", "TUPLES", "DATA"};
    for (std::size_t i = 0; i < 4; ++i)
      if (!doc.header->sections[i] || doc.header->sections[i]->topic != topics[i])
        return fail("the document is missing a header section");
    if (doc.items.size() != sample.items)
      return fail("the document does not hold every data item");
  }

  std::puts("dif: every example parses");
  return 0;
}
