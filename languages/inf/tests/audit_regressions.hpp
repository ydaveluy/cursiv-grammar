#pragma once
#include <string>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "inf/core/CoreServices.hpp"
#include "inf/core/Values.hpp"
namespace audit_regressions {
inline void run() {
  const auto language =
      cursiv::create_language<inf::InfCoreServices, inf::InfSharedCoreServices,
                              inf::install_core_module>();
  const std::string source =
      "\ufeff[ spaced ] ignored\n"
      "a,b=c,,\n"
      "=,\n"
      "\"a,b\"=pre\"mid\"post,\"a\"\"b\",a \"b\" c\n"
      "k=one \\ ;comment\r\n  two\n"
      "q=\"unterminated\n"
      "unicode=é測試😀\n"
      "empty=;comment\n"
      "[ spaced ]\nx=1\n"
      "\x1a"
      "[Ignored]\ny=2\n";
  cursiv::Document document(
      cursiv::TextDocument::create("memory:inf", "inf", 0, source));
  document.language = &language.language;
  document.parseResult =
      language.language.parser->parse(document.textDocument().getText());
  const auto& parsed = document.parseResult;
  CHECK(parsed.accepted && !parsed.cst().recovered() &&
        parsed.diagnostics.empty());
  const auto* root = cursiv::ast_cast<inf::ast::Document>(parsed.value);
  CHECK(root && root->has_bom && root->preamble.empty() &&
        root->sections.size() == 2);
  if (root && root->sections.size() == 2) {
    const auto* section = root->sections[0];
    CHECK(section->name == " spaced " && section->closing == "] ignored" &&
          section->parent == root);
    CHECK(section->lines.size() == 7 && root->sections[1]->lines.size() == 1);
    CHECK(root->trailer ==
          "\x1a"
          "[Ignored]\ny=2\n");
    if (section->lines.size() == 7) {
      const auto value = [&](const inf::ast::Item* item) {
        return inf::decode_value(*item, parsed.cst());
      };
      const auto* plain = section->lines[0];
      CHECK(!plain->key && plain->fields.size() == 4 &&
            plain->parent == section);
      if (plain->fields.size() == 4) {
        CHECK(value(plain->fields[0]) == "a" &&
              value(plain->fields[1]) == "b=c" &&
              value(plain->fields[2]).empty() &&
              value(plain->fields[3]).empty());
        CHECK(plain->fields[1]->raw == "b=c" &&
              plain->fields[1]->parent == plain);
      }
      const auto* emptyKey = section->lines[1];
      CHECK(emptyKey->key && value(emptyKey->key).empty() &&
            emptyKey->fields.size() == 2);
      const auto* mixed = section->lines[2];
      CHECK(mixed->key && value(mixed->key) == "a,b" &&
            mixed->fields.size() == 3);
      if (mixed->fields.size() == 3)
        CHECK(value(mixed->fields[0]) == "premidpost" &&
              value(mixed->fields[1]) == "a\"b" &&
              value(mixed->fields[2]) == "ab c");
      CHECK(value(section->lines[3]->fields[0]) == "onetwo");
      CHECK(value(section->lines[4]->fields[0]) == "unterminated");
      CHECK(value(section->lines[5]->fields[0]) == "é測試😀");
      CHECK(value(section->lines[6]->fields[0]).empty());
    }
  }
  std::vector<cursiv::Diagnostic> diagnostics;
  language.language.documentValidator->validateDocument(document, diagnostics);
  CHECK(diagnostics.empty());
  for (const std::string invalid : std::vector<std::string>{
       "before=value\n[S]\nk=v\n", "[" + std::string(256, 's') + "]\n"}) {
    cursiv::Document bad(
        cursiv::TextDocument::create("memory:inf", "inf", 0, invalid));
    bad.language = &language.language;
    bad.parseResult =
        language.language.parser->parse(bad.textDocument().getText());
    CHECK(bad.parseResult.accepted && !bad.parseResult.cst().recovered() &&
          bad.parseResult.diagnostics.empty());
    diagnostics.clear();
    language.language.documentValidator->validateDocument(bad, diagnostics);
    const auto expected =
        invalid[0] == '[' ? "inf.section-length" : "inf.section";
    bool found = false;
    for (const auto& d : diagnostics)
      if (d.code && d.code->string() == expected) {
        found = true;
        CHECK(d.range.begin < d.range.end && d.range.end <= invalid.size());
      }
    CHECK(found);
  }
}
}  // namespace audit_regressions
