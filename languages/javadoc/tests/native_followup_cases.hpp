#pragma once
#include <cstdio>
#include <string_view>
#include <variant>
#include "cursiv/parser/Parser.hpp"
#include "cursiv/workspace/Document.hpp"
#include "cursiv/ValidationHelper.hpp"
#include "cursiv/ValidationResult.hpp"
#include "javadoc/Content.hpp"
#include "javadoc/ast.hpp"
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
    const auto* doc = result.template valueAs<javadoc::ast::Documentation>();
    bool inlineTag = false;
    if (doc) for (const auto& item : doc->description)
      if (const auto* tag = std::get_if<javadoc::ast::InlineTag*>(&item))
        inlineTag |= *tag && (*tag)->parent == doc && !(*tag)->content.empty();
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && inlineTag,
          "balanced wrapped tags and bare-body comment text retain raw AST content");
  }
  struct Margin { std::string_view source; bool startsTag; };
  for (const auto m : {Margin{"/** text\f * @since next */", false},
                       Margin{"/** text\f @since next */", true},
                       Margin{"/** text\r\n \f  ** @since next */", true},
                       Margin{"/** \t* @since next */", true}}) {
    const auto r = language.language.parser->parse(m.source);
    const auto* doc = r.template valueAs<javadoc::ast::Documentation>();
    check(doc && r.accepted && r.diagnostics.empty() &&
              (m.startsTag ? doc->tags.size() == 1 && doc->tags[0]->name == "since"
                           : doc->tags.empty()),
          "only physical line margins strip decoration before a block tag");
  }
  {
    const auto r = language.language.parser->parse("/** {@code A\n \f  \n\t\f * B} */");
    const auto* doc = r.template valueAs<javadoc::ast::Documentation>();
    bool found = false;
    if (doc) for (const auto& item : doc->description)
      if (const auto* tag = std::get_if<javadoc::ast::InlineTag*>(&item))
        found |= *tag && javadoc::inline_content(**tag) == "A\n\n B";
    check(found && r.diagnostics.empty(), "code preserves the two physical newlines");
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
