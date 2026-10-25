#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreServices.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "postalcode/core/ast.hpp"

inline int postalcode_regressions(const cursiv::CoreServices& language) {
  int failures = 0;
  const auto check = [&](std::string_view source, bool valid) {
    cursiv::Document document(cursiv::TextDocument::create(
        "memory:postalcode", "postalcode", 0, std::string(source)));
    document.language = &language;
    document.parseResult = language.parser->parse(source);
    const auto& result = document.parseResult;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !result.value ||
        result.value->type !=
            &cursiv::ast_type_tag<postalcode::ast::PostalCode>) {
      ++failures;
      return;
    }
    const auto& code =
        *static_cast<const postalcode::ast::PostalCode*>(result.value);
    if (code.forwardSortationArea != source.substr(0, 3) ||
        code.localDeliveryUnit != source.substr(source.size() - 3) ||
        code.parent || result.nodes.size() != 1 ||
        result.cst().text() != source)
      ++failures;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.documentValidator->validateDocument(document, diagnostics);
    if (diagnostics.empty() != valid) ++failures;
  };
  for (const std::string_view source :
       {"k1a0b1", "k1a-0b1", "K1A\t0B1", "K1A\v0B1", "K1A\f0B1",
        "K1A\xC2\xA0"
        "0B1",
        "K1A\xEF\xBB\xBF"
        "0B1",
        "K1A\xE2\x80\xA8"
        "0B1",
        "K1A\xE2\x80\xA9"
        "0B1"})
    check(source, true);
  for (const std::string_view source :
       {"d1a0b1", "w1a0b1", "Z1A0B1", "K1D0B1", "K1A0d1"})
    check(source, false);
  cursiv::Document partial(cursiv::TextDocument::create(
      "memory:partial-postalcode", "postalcode", 0, "K1A"));
  partial.language = &language;
  partial.parseResult = language.parser->parse("K1A");
  std::vector<cursiv::Diagnostic> diagnostics;
  language.documentValidator->validateDocument(partial, diagnostics);
  if (diagnostics.empty()) ++failures;
  if (failures)
    std::fprintf(stderr, "postalcode: %d regression failure(s)\n", failures);
  return failures;
}
