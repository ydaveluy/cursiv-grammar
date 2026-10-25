#include "value_regressions.hpp"

#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "properties/core/CoreServices.hpp"
#include "properties/core/ast.hpp"
// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "properties: %s: %s\n", file.string().c_str(), what);
}

// A bare key has no value; a delimiter with nothing after it has an empty one.
template <class Language>
bool checkEmptyValue(const Language& language) {
  using properties::ast::Document;
  using properties::ast::Entry;
  const cursiv::ParseResult result =
      language.language.parser->parse("bare\nempty=\ncolon:\nfull=v\n");
  const Document* const document = result.value_as<Document>();
  if (document == nullptr || document->rows.size() != 4) return false;
  const auto value = [&](std::size_t row) {
    return std::get<Entry*>(document->rows[row])->value;
  };
  return !value(0).has_value() && value(1) == "" && value(2) == "" &&
         value(3) == "v";
}

}  // namespace

static int audit_regression() {
  const auto language =
      cursiv::create_language<properties::PropertiesCoreServices,
                              properties::PropertiesSharedCoreServices,
                              properties::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "key value\nkey\\u0061=one\nkeya=two\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<properties::ast::Document>();
    if (!doc || doc->rows.size() != 3) return 1;
    const auto* one = std::get<properties::ast::Entry*>(doc->rows[0]);
    const auto* two = std::get<properties::ast::Entry*>(doc->rows[1]);
    if (one->key != "key" || one->value != "value" || two->key != "keya")
      return 1;
  }
  {
    const auto r =
        language.language.parser->parse("face=\\uD83D\\uDE00\nlead=   value\n");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty()) return 1;
    const auto* doc = r.value_as<properties::ast::Document>();
    if (!doc || doc->rows.size() != 2 ||
        std::get<properties::ast::Entry*>(doc->rows[0])->value != "😀" ||
        std::get<properties::ast::Entry*>(doc->rows[1])->value != "value")
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
      properties::PropertiesCoreServices, properties::PropertiesSharedCoreServices,
      properties::install_core_module>();

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
        root->type != &cursiv::ast_type_tag<properties::ast::Document>) {
      fail(path, "the parse built no Document");
      ++failures;
    }
  }

  if (!checkEmptyValue(language)) {
    fail("<inline>", "a bare key and an empty value read alike");
    ++failures;
  }

  if (total == 0) {
    std::fprintf(stderr, "properties: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("properties: %d example(s) parse\n", total);
  return 0;
}
