#pragma once
#include "cursiv/core/parser/Parser.hpp"
#include "xml/core/ast.hpp"
#include "xml/generated/Xml/Module.hpp"
#include <cstdio>
#include <string>
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

  return ok;
}
} // namespace audit_regressions
