#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "dif/core/CoreServices.hpp"
#include "dif/core/ast.hpp"

namespace audit_regressions {
inline bool run() {
  namespace ast = dif::ast;
  const auto language = cursiv::create_language<dif::DifCoreServices,
      dif::DifSharedCoreServices, dif::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "dif regression: %s\n", message); ok = false; }
  };
  const std::string header =
      "TABLE\n0,1\n\"sample\"\nTUPLES\n0,1\n\"\"\n"
      "LABEL\n1,2\n\"column\"\nVECTORS\n0,4\n\"\"\nDATA\n0,0\n\"\"\n";
  const auto document = [&](std::string_view records) {
    return header + "-1,0\nBOT\n" + std::string(records) + "-1,0\nEOD\n";
  };
  const std::string text = document(
      "0,-3.75e+8\nV\n0,1\nTRUE\n0,0\nFALSE\n1,0\n\"a\"\"b\nline two\"\n");
  const auto parsed = language.language.parser->parse(text);
  const auto* root = cursiv::ast_cast<ast::Document>(parsed.value);
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
        "numeric, boolean and multiline records parse");
  check(root && root->header && root->header->sections.size() == 5 &&
            root->header->sections[1]->topic == "TUPLES" &&
            root->header->sections[2]->topic == "LABEL" &&
            root->header->sections[2]->label == "column",
        "header order and additional topics survive");
  check(root && root->items.size() == 6 &&
            root->items[1]->id->second == "-3.75e+8" &&
            root->items[1]->value == "V" && !root->items[1]->quoted &&
            root->items[2]->value == "TRUE" && root->items[3]->value == "FALSE" &&
            root->items[4]->quoted && root->items[4]->value == "a\"b\nline two",
        "record types, exact numbers and decoded quotes survive");
  check(cursiv::test::validationHelper(language.language, text).diagnostics.empty(),
        "valid records validate");
  const auto hugeText = document("0,999999999999999999999999999999999999999\nV\n");
  const auto huge = language.language.parser->parse(hugeText);
  const auto* hugeRoot = cursiv::ast_cast<ast::Document>(huge.value);
  check(huge.accepted && !huge.cst().recovered() && hugeRoot &&
            hugeRoot->items[1]->id->second == "999999999999999999999999999999999999999",
        "unbounded numeric spelling stays exact");

  struct Invalid { std::string text; std::string_view code; std::string_view span; };
  for (const auto& item : {
      Invalid{document("999999999999999999999999,0\nV\n"), "dif.type", "999999999999999999999999"},
      Invalid{document("2,0\nV\n"), "dif.type", "2"},
      Invalid{document("0,1\nUNKNOWN\n"), "dif.indicator", "UNKNOWN"},
      Invalid{document("1,0\nnot quoted\n"), "dif.string", "not quoted"},
      Invalid{document("-1,0\nWRONG\n"), "dif.command", "WRONG"},
      Invalid{std::string("TABLE\n0,2\n\"\"\nDATA\n0,0\n\"\"\n"), "dif.headerValue", "2"}}) {
    const auto result = language.language.parser->parse(item.text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
          "invalid record remains parseable");
    const auto validation = cursiv::test::validationHelper(language.language, item.text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() &&
          diagnostic.code->string() == item.code &&
          std::string_view(item.text).substr(diagnostic.range.begin,
              diagnostic.range.end - diagnostic.range.begin) == item.span) found = true;
    check(found, "runtime error points to the invalid value");
  }
  return ok;
}
}  // namespace audit_regressions
