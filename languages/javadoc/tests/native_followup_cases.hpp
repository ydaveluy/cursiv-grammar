#pragma once
#include <cstdio>
#include <string_view>
#include <variant>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "javadoc/core/Content.hpp"
#include "javadoc/core/ast.hpp"
namespace javadoc_native_followup {
template<class Language> bool run(const Language& language) {
  bool ok = true;
  auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "javadoc native: %s\n", message); ok = false; }
  };
  for (const auto bad : {"/** {@code */} */", "/** {@code { */ }} */",
                          "/** {@snippet :\n x /* comment */\n } */"}) {
    const auto result = language.language.parser->parse(bad);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "comment terminator cannot occur inside a wrapped inline tag");
  }
  for (const auto source : {"{@code */}", "/** {@code {x:{y}}} */",
                             "/** {@snippet file=\"Example.java\"} */"}) {
    const auto result = language.language.parser->parse(source);
    const auto* doc = result.template value_as<javadoc::ast::Documentation>();
    bool inlineTag = false;
    if (doc) for (const auto& item : doc->description)
      if (const auto* tag = std::get_if<javadoc::ast::InlineTag*>(&item))
        inlineTag |= *tag && (*tag)->parent == doc && !(*tag)->content.empty();
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && inlineTag,
          "balanced wrapped tags and bare-body comment text retain raw AST content");
  }
  for (const auto source : {"/** {@link} */", "/** {@linkplain} */"}) {
    const auto validation = cursiv::test::validationHelper(language.language, source);
    check(validation.document->parseResult.accepted && !validation.document->parseResult.cst().recovered() &&
          validation.diagnostics.size() == 1 && validation.diagnostics[0].code == "javadoc.missing-reference",
          "native doclet missing-reference warning");
  }
  return ok;
}
} // namespace javadoc_native_followup
