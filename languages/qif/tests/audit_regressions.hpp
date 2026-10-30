#pragma once

#include <string>
#include <utility>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/Check.hpp"
#include "qif/core/CoreServices.hpp"
#include "qif/core/ast.hpp"

inline void qif_regressions(const qif::QifCoreServices &services) {
  namespace ast = qif::ast;
  const std::string source =
      "!Account\r\nN0001: Main/日本\r\nDBank description\r\nTBank\r\n^\r\n"
      "!Type:Bank\r\nD10/10'01\r\nT12345678901234567890.0012300\r\n"
      "N00001-REF\r\nM\r\nSFood:Lunch\r\nEcomplete memo\r\n$-12.50\r\n"
      "S[Cash]\r\n%50\r\n$-12.50\r\n^\r\n"
      "!Type:Cat\nNExpenses\nE\nT\n^";
  const auto result = services.parser->parse(source);
  CHECK(result.accepted && !result.cst().recovered() &&
        result.diagnostics.empty());
  const auto *root = cursiv::ast_cast<ast::Document>(result.value);
  CHECK(root && root->entries.size() == 6);
  if (root && root->entries.size() == 6) {
    const auto *header = cursiv::ast_cast<ast::Header>(root->entries[0]);
    const auto *account = cursiv::ast_cast<ast::Record>(root->entries[1]);
    const auto *transaction = cursiv::ast_cast<ast::Record>(root->entries[3]);
    const auto *category = cursiv::ast_cast<ast::Record>(root->entries[5]);
    CHECK(header && header->text == "Account" && header->parent == root);
    CHECK(account && account->terminated && account->fields.size() == 3);
    CHECK(transaction && transaction->terminated &&
          transaction->fields.size() == 10);
    CHECK(category && category->terminated && category->fields.size() == 3);
    if (account && account->fields.size() == 3) {
      CHECK(account->fields[0]->code == "N" &&
            account->fields[0]->text == "0001: Main/日本");
      CHECK(account->fields[1]->code == "D" &&
            account->fields[1]->text == "Bank description");
      CHECK(account->fields[2]->code == "T" &&
            account->fields[2]->text == "Bank");
    }
    if (transaction && transaction->fields.size() == 10) {
      const auto &f = transaction->fields;
      CHECK(f[0]->text == "10/10'01" &&
            f[1]->text == "12345678901234567890.0012300" && f[2]->code == "N" &&
            f[2]->text == "00001-REF");
      CHECK(f[3]->code == "M" && f[3]->text.empty());
      CHECK(f[4]->code == "S" && f[4]->text == "Food:Lunch" &&
            f[5]->text == "complete memo" && f[6]->text == "-12.50" &&
            f[7]->text == "[Cash]" && f[8]->code == "%" && f[8]->text == "50");
      const auto span = result.cst()[f[2]->cstNodeId];
      CHECK(span.begin == source.find("N00001-REF") &&
            span.end == source.find("M\r\n"));
    }
    std::size_t count = 1;
    for (const auto *entry : root->entries) {
      CHECK(entry && entry->parent == root);
      ++count;
      if (const auto *record = cursiv::ast_cast<ast::Record>(entry))
        for (const auto *field : record->fields) {
          CHECK(field && field->parent == record);
          ++count;
        }
    }
    CHECK(count == result.nodes.size());
  }
  for (const auto [text, code] :
       {std::pair{"Nname\n^\n", "qif.header"},
        std::pair{"!Type:Bank\nT1", "qif.record-end"},
        std::pair{"!Type:Bank\nEorphan\n^\n", "qif.split"},
        std::pair{"!Type:CCard\nEorphan\n^\n", "qif.split"}}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:qif", "qif", 0, text));
    document.language = &services;
    document.parseResult = services.parser->parse(text);
    std::vector<cursiv::Diagnostic> diagnostics;
    services.documentValidator->validateDocument(document, diagnostics);
    CHECK(document.parseResult.accepted &&
          !document.parseResult.cst().recovered() &&
          document.parseResult.diagnostics.empty());
    CHECK(diagnostics.size() == 1 && diagnostics[0].code &&
          *diagnostics[0].code == code);
    if (diagnostics.size() == 1 && std::string_view(text).starts_with("!Type:Bank") &&
        std::string_view(code) == "qif.split")
      CHECK(diagnostics[0].range.begin == 11 && diagnostics[0].range.end == 19);
  }
  for (const auto *text : {"!Type:Bank\nT1\n^!Type:Cash\nT2\n^\n",
                           "!Type:Bank\nT1\n^D01/01/26\n^\n"}) {
    const auto invalid = services.parser->parse(text);
    CHECK(!invalid.accepted || invalid.cst().recovered() ||
          !invalid.diagnostics.empty());
  }
}
