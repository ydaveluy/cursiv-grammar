#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "xml/core/CoreServices.hpp"
#include "xml/core/ast.hpp"
#include "xml/generated/Xml/Module.hpp"
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>
namespace audit_regressions {
template <class T> T *as(cursiv::AstNode *p) {
  return p && p->type == &cursiv::ast_type_tag<T> ? static_cast<T *>(p)
                                                  : nullptr;
}
inline bool run() {
  auto parser = xml::generated::Xml::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "audit regression: %s\n", message);
      ok = false;
    }
  };
  namespace ast = xml::ast;
  auto r = parser->parse("<!DOCTYPE é [<!ENTITY a \"x>y\">]><é>&a;</é>");
  check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
        "xml valid syntax regression");
  const auto* dtd_doc = r.value_as<ast::Document>();
  check(dtd_doc && dtd_doc->doctypes.size() == 1 &&
            dtd_doc->doctypes[0] == "<!DOCTYPE é [<!ENTITY a \"x>y\">]>",
        "xml preserves the doctype for runtime entity validation");

  // Processing instructions in the internal subset are opaque, including
  // brackets, quotes and '>' that do not end their surrounding DOCTYPE.
  for (const char *source : {
           "<!DOCTYPE a [<?p ] >?> <!ENTITY q \"abc\">]><a/>",
           "<!DOCTYPE a [<?p [ ] > '\" ?> <!ELEMENT a ANY>]><a/>",
           "<!DOCTYPE a [<!-- ] > --><?p ] >?><!ELEMENT a ANY>]><a/>",
           "\xEF\xBB\xBF<a/>",
           "\xEF\xBB\xBF<?xml version=\"1.0\"?><a/>"}) {
    auto doc_result = parser->parse(source);
    const auto *doc = doc_result.value_as<ast::Document>();
    check(doc_result.accepted && !doc_result.cst().recovered() &&
              doc_result.diagnostics.empty() && doc && doc->root &&
              doc->root->name == "a" && doc->root->children.empty(),
          "xml opaque DTD instruction or leading UTF-8 BOM");
  }

  // Text inside elements remains source text, including whitespace-only
  // children; a BOM there is content rather than document trivia.
  auto mixed_result = parser->parse("<a> \t<b/>\xEF\xBB\xBF </a>");
  const auto *mixed = mixed_result.value_as<ast::Document>();
  bool mixed_ok = mixed_result.accepted && !mixed_result.cst().recovered() &&
                  mixed_result.diagnostics.empty() && mixed && mixed->root &&
                  mixed->root->children.size() == 3;
  if (mixed_ok) {
    const auto &children = mixed->root->children;
    const auto *left = std::get_if<std::string_view>(&children[0]);
    auto *const *element = std::get_if<ast::Element *>(&children[1]);
    const auto *right = std::get_if<std::string_view>(&children[2]);
    mixed_ok = left && *left == " \t" && element && *element &&
               (*element)->name == "b" && right && *right == "\xEF\xBB\xBF ";
  }
  check(mixed_ok, "xml mixed content preserves whitespace and embedded BOM");

  for (const char* source : {
           "<a x='&amp;&#65;&#x41;' y=\"a&amp;b\"/>",
           "<a>&#x1FFFE;&#x10FFFF;&#0000000000000000000065;</a>",
           "<a><![CDATA[&#0;]]></a>",
           "<!DOCTYPE a SYSTEM 'a>b'><a/>",
           "<!DOCTYPE a PUBLIC '-//Example//DTD Test//EN' 'test.dtd'><a/>",
           "<?p?><a><!--a-b--><?xml-stylesheet href='x'?><![CDATA[]]>x]]</a>",
           "<?xml version='1.0' encoding='UTF-8' standalone='no'?><a />"}) {
    const auto valid = parser->parse(source);
    check(valid.accepted && !valid.cst().recovered() && valid.diagnostics.empty(),
          "XML references, comments, targets and tag spacing remain valid");
  }
  for (const char* source : {
           "< a/>", "<a></ a>", "<a x='1'y='2'/>", "<a x='&broken'/>",
           "<a x='a&b'/>", "<a>]]></a>", "<!--a--b--><a/>",
           "<!--a---><a/>", "<?XML?><a/>", "<?\?><a/>", "<?1p?><a/>",
           "<?xml?><a/>", " <?xml version='1.0'?><a/>",
           "<?xml version='1.0'encoding='UTF-8'?><a/>"}) {
    const auto invalid = parser->parse(source);
    check(!invalid.accepted || invalid.cst().recovered() || !invalid.diagnostics.empty(),
          "malformed XML must not parse without a diagnostic");
  }
  for (const std::string_view source : std::initializer_list<std::string_view>{
           std::string_view{"<a>&#0;</a>"}, "<a>&#xD800;</a>",
           "<a>&#xFFFE;</a>", "<a>&#x110000;</a>", "<a x='&#0;'/>",
           "<a>\x01</a>", "<a x='\x0b'/>", "<?pi \x01?><a/>",
           "<!BAD thing><a/>", "<!DOCTYPE a foo><a/>",
           "<!DOCTYPE a><!DOCTYPE a><a/>",
           std::string_view{"<a>\0</a>", 8},
           std::string_view{"<!--\0--><a/>", 12},
           std::string_view{"<a><![CDATA[\0]]></a>", 20}}) {
    const auto invalid = parser->parse(source);
    check(!invalid.accepted || invalid.cst().recovered() ||
              !invalid.diagnostics.empty(),
          "XML character and document-type constraints");
  }

  const auto language = cursiv::create_language<xml::XmlCoreServices,
      xml::XmlSharedCoreServices, xml::install_core_module>();
  for (const auto& [source, code] : {
           std::pair{"<a x='1' x='2'/>", "xml.duplicate-attribute"},
           {"<?xml encoding='UTF-8'?><a/>", "xml.declaration-order"},
           {"<?xml version='1.0' version='1.0'?><a/>", "xml.declaration-order"},
           {"<?xml version='1.0' standalone='maybe'?><a/>", "xml.declaration-value"},
           {"<?xml version='1.0' encoding='8bit'?><a/>", "xml.declaration-value"}}) {
    cursiv::Document document(
        cursiv::TextDocument::create("memory:xml", "xml", 0, source));
    document.language = &language.language;
    document.parseResult = language.language.parser->parse(document.textDocument().getText());
    std::vector<cursiv::Diagnostic> diagnostics;
    language.language.documentValidator->validateDocument(document, diagnostics);
    bool found = false;
    for (const auto& diagnostic : diagnostics)
      found |= diagnostic.code && diagnostic.code->isString() &&
               diagnostic.code->string() == code;
    check(found, "XML well-formedness constraints are diagnosed during validation");
  }

  return ok;
}
} // namespace audit_regressions
