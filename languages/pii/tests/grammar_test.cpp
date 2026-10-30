// The smoke test of the language: build the container the way a workspace
// does, parse every document under example/, and check the tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "pii/core/CoreServices.hpp"
#include "pii/core/ast.hpp"

namespace {

int fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "pii: %s: %s\n", file.string().c_str(), what);
  return 1;
}

}  // namespace

int main() {
  // The whole language in one call: the defaults, then `install_core_module`.
  const auto language = cursiv::create_language<
      pii::PiiCoreServices, pii::PiiSharedCoreServices,
      pii::install_core_module>();

  int total = 0;
  int failures = 0;
  struct Case {
    const char* source;
    const char* identifier;
    const char* year;
    const char* item;
    const char* check;
  };
  // Publisher XML examples exercise source forms the teaching grammar missed.
  for (const Case& test : {
           Case{"S0012365X15000898", "0012365X", "15", "00089", "8"},
           Case{"S0168-583X(16)91021-8", "0168583X", "16", "91021", "8"},
           Case{"S9999-9994(03)X7607-2", "99999994", "03", "X7607", "2"},
           Case{"S0092-8674(03)00309-X", "00928674", "03", "00309", "X"},
           Case{"B978-0-323-01679-7.10003-8", "9780323016797", "", "10003", "8"},
           Case{"B0-323-01679-0/10027-7", "0323016790", "", "10027", "7"},
           Case{"B0-8044-2957-X/00001-0", "080442957X", "", "00001", "0"}}) {
    const auto result = language.language.parser->parse(test.source);
    const auto* doc = result.value_as<pii::ast::Doc>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc || !doc->id ||
        doc->item != test.item || doc->check != test.check ||
        result.cst().text() != test.source) {
      failures += fail(test.source, "native identifier was lost or rejected");
      continue;
    }
    if (test.source[0] == 'S') {
      if (doc->id->type != &cursiv::ast_type_tag<pii::ast::Issn>) {
        failures += fail(test.source, "wrong journal identifier type");
        continue;
      }
      const auto& id = *static_cast<const pii::ast::Issn*>(doc->id);
      if (id.digits != test.identifier || id.year != test.year)
        failures += fail(test.source, "wrong ISSN/year capture");
    } else {
      if (doc->id->type != &cursiv::ast_type_tag<pii::ast::Isbn> ||
          static_cast<const pii::ast::Isbn*>(doc->id)->digits != test.identifier)
        failures += fail(test.source, "wrong ISBN capture");
    }
    if (doc->parent || doc->id->parent != doc || result.nodes.size() != 2)
      failures += fail(test.source, "wrong identifier ownership");
  }
  for (const char* source : {"S001236AX15000898", "S0012365x15000898",
                             "S99999994031X6072", "B978032301679X100038",
                             "B0323016790/10027"}) {
    const auto result = language.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty())
      failures += fail(source, "malformed identifier parsed cleanly");
  }
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const std::filesystem::directory_entry& entry :
       std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".pii")
      continue;
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
    if (root == nullptr || root->type != &cursiv::ast_type_tag<pii::ast::Doc>)
      failures += fail(file, "the parse built no Doc");
  }

  if (total == 0) return fail(CURSIV_EXAMPLE_DIR, "no example documents");
  if (failures > 0) return 1;

  std::printf("pii: %d example(s) parse\n", total);
  return 0;
}
