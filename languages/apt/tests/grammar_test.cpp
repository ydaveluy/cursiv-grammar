// The smoke test of the language: parse every file in example/, and check each
// tree.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "apt/core/CoreServices.hpp"
#include "apt/core/ast.hpp"
#include "apt/core/Word.hpp"
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

void fail(const std::filesystem::path& file, const char* what) {
  std::fprintf(stderr, "apt: %s: %s\n", file.string().c_str(), what);
}

}  // namespace

int main() {
  const auto language = cursiv::create_language<
      apt::AptCoreServices, apt::AptSharedCoreServices,
      apt::install_core_module>();

  // APT 3.0.3 treats CR as field whitespace, while only LF ends a line.
  // ParseQuoteWord also keeps CR inside quotes and dequotes option names.
  const struct {
    const char* source;
    std::string_view optionName;
    std::string_view optionValue;
  } nativeCases[] = {
      {"deb https://example.org\rsuite main\n", {}, {}},
      {"\rdeb https://example.org suite main\n", {}, {}},
      {"deb [\"custom key\"=\"two words\"] https://example.org suite main\n",
       "custom key", "two words"},
      {"deb [label=\"a\rb\"] https://example.org suite main\n", "label", "a\rb"}};
  for (const auto& c : nativeCases) {
    const auto result = language.language.parser->parse(c.source);
    const auto* doc = result.value_as<apt::ast::Document>();
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !doc || doc->records.size() != 1)
      return (fail("native whitespace/quoting", "not one clean record"), 1);
    const auto* record = doc->records[0];
    if (record->uri != "https://example.org" ||
        record->distribution != "suite" || record->components.size() != 1 ||
        record->components[0] != "main")
      return (fail("native whitespace/quoting", "incorrect record fields"), 1);
    if (!c.optionName.empty()) {
      if (record->options.size() != 1 ||
          apt::word_value(record->options[0]->name) != c.optionName ||
          apt::word_value(record->options[0]->value) != c.optionValue)
        return (fail("native quoting", "incorrect option fields"), 1);
    }
  }
  {
    const auto result = language.language.parser->parse(
        "deb\rhttps://example.org suite main\n");
    if (result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty())
      return (fail("native type", "CR alone cannot terminate a record type"), 1);
  }

  int total = 0;
  int failures = 0;
  for (const char* const dir : {CURSIV_EXAMPLE_DIR, CURSIV_TEST_DIR})
  for (const auto& entry : std::filesystem::directory_iterator(dir)) {
    if (!entry.is_regular_file() || entry.path().extension() != ".list")
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

    const cursiv::ParseResult result = language.language.parser->parse(text);
    if (result.cst().recovered()) {
      fail(path, "the parse was recovered");
      ++failures;
      continue;
    }
    if (!result.diagnostics.empty()) {
      fail(path, "the parse left diagnostics");
      ++failures;
      continue;
    }

    const cursiv::AstNode* const root = result.value;
    if (root == nullptr ||
        root->type != &cursiv::ast_type_tag<apt::ast::Document>) {
      fail(path, "the parse built no Document");
      ++failures;
    }
  }

  if (total == 0) {
    std::fprintf(stderr, "apt: no example files found\n");
    return 1;
  }
  if (failures > 0) return 1;
  std::printf("apt: %d example(s) parse\n", total);
  return 0;
}
