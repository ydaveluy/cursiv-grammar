#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "html/core/ast.hpp"
#include "html/generated/Html/Module.hpp"

namespace html_raw_text_regressions {
inline bool run() {
  auto parser = html::generated::Html::create_parser();
  bool ok = true;
  auto check = [&](bool value, const char *message) {
    if (!value) {
      std::fprintf(stderr, "html raw text: %s\n", message);
      ok = false;
    }
  };
  auto raw = [&](std::string_view source, bool script) {
    auto result = parser->parse(source);
    const auto *doc = result.value_as<html::ast::Document>();
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && doc && doc->content.size() == 1;
    if (correct && script) {
      auto *const *node = std::get_if<html::ast::Script *>(&doc->content[0]);
      correct = node && *node && (*node)->text == source && (*node)->parent == doc;
    } else if (correct) {
      auto *const *node = std::get_if<html::ast::Style *>(&doc->content[0]);
      correct = node && *node && (*node)->text == source && (*node)->parent == doc;
    }
    check(correct, "clean raw node must preserve the entire source");
  };
  for (const char *source : {
           "<ScRiPt>if (a < b) x()</sCrIpT>",
           "<script>if (a < b) x()</script >",
           "<script>if (a < b) x()</SCRIPT\n>",
           "<script data-x=\"> </script>\">a < b</script>",
           "<script>\"</scriptx>\"; a < b</script>",
           "<script>\"</script-widget>\"; a < b</script>",
           "<script>a < b</>"})
    raw(source, true);
  for (const char *source : {
           "<StYlE>a::before { content: \"<\"; }</sTyLe>",
           "<style>a<b</style\n>",
           "<style data-x='> </style>'>a<b</style>",
           "<style>\"</stylesheet>\"; a<b</style>",
           "<style>a<b</>"})
    raw(source, false);

  for (const char *name : {"scriptable", "script-widget", "stylesheet"}) {
    const bool script = std::string_view(name) != "stylesheet";
    const std::string raw_source = script ? "<script>ok</script>" : "<style>ok</style>";
    const std::string source = "<" + std::string(name) + ">x</" + name + ">" + raw_source;
    auto result = parser->parse(source);
    const auto *doc = result.value_as<html::ast::Document>();
    bool correct = result.accepted && !result.cst().recovered() &&
                   result.diagnostics.empty() && doc && doc->content.size() == 2;
    if (correct) {
      auto *const *element = std::get_if<html::ast::Element *>(&doc->content[0]);
      correct = element && *element && (*element)->name == name;
      if (script) {
        auto *const *node = std::get_if<html::ast::Script *>(&doc->content[1]);
        correct = correct && node && *node && (*node)->text == raw_source;
      } else {
        auto *const *node = std::get_if<html::ast::Style *>(&doc->content[1]);
        correct = correct && node && *node && (*node)->text == raw_source;
      }
    }
    check(correct, "a raw-text name prefix must remain an ordinary element");
  }

  auto adjacent_result = parser->parse("<style>one</STYLE><style>two</style>");
  const auto *adjacent = adjacent_result.value_as<html::ast::Document>();
  bool adjacent_ok = adjacent_result.accepted && !adjacent_result.cst().recovered() &&
                     adjacent_result.diagnostics.empty() && adjacent && adjacent->content.size() == 2;
  if (adjacent_ok) {
    auto *const *first = std::get_if<html::ast::Style *>(&adjacent->content[0]);
    auto *const *second = std::get_if<html::ast::Style *>(&adjacent->content[1]);
    adjacent_ok = first && *first && (*first)->text == "<style>one</STYLE>" &&
                  second && *second && (*second)->text == "<style>two</style>";
  }
  check(adjacent_ok, "first matching closer separates adjacent raw-text nodes");

  auto fragment_result = parser->parse("hello <b>world</b> !");
  const auto *fragment = fragment_result.value_as<html::ast::Document>();
  bool fragment_ok = fragment_result.accepted && !fragment_result.cst().recovered() &&
                     fragment_result.diagnostics.empty() && fragment && fragment->content.size() == 3;
  if (fragment_ok) {
    auto *const *first = std::get_if<html::ast::Text *>(&fragment->content[0]);
    auto *const *element = std::get_if<html::ast::Element *>(&fragment->content[1]);
    auto *const *last = std::get_if<html::ast::Text *>(&fragment->content[2]);
    fragment_ok = first && *first && (*first)->text == "hello " &&
                  element && *element && (*element)->name == "b" &&
                  last && *last && (*last)->text == " !" &&
                  (*first)->parent == fragment && (*last)->parent == fragment;
  }
  check(fragment_ok, "document mixed content preserves top-level text around elements");

  auto declaration_result = parser->parse(" \n<!--before--><!DOCTYPE html>hello <br>");
  const auto *declaration = declaration_result.value_as<html::ast::Document>();
  bool declaration_ok = declaration_result.accepted && !declaration_result.cst().recovered() &&
                        declaration_result.diagnostics.empty() && declaration &&
                        declaration->doctypes.size() == 1 &&
                        declaration->doctypes[0] == "<!DOCTYPE html>";
  bool saw_text = false;
  if (declaration_ok) {
    for (const auto &item : declaration->content) {
      auto *const *text = std::get_if<html::ast::Text *>(&item);
      saw_text |= text && *text && (*text)->text == "hello ";
    }
  }
  check(declaration_ok && saw_text, "a comment before the doctype preserves metadata and following text");

  auto duplicate_result = parser->parse("<!DOCTYPE html><!DOCTYPE html><br>");
  const auto *duplicate = duplicate_result.value_as<html::ast::Document>();
  check(duplicate_result.accepted && !duplicate_result.cst().recovered() &&
            duplicate_result.diagnostics.empty() && duplicate &&
            duplicate->doctypes.size() == 2 &&
            duplicate->doctypes[0] == "<!DOCTYPE html>" &&
            duplicate->doctypes[1] == "<!DOCTYPE html>",
        "duplicate declarations remain available for runtime validation");

  auto xml_result = parser->parse("<?xml version='1.0'?><?xml version='1.1'?><br>");
  const auto *xml_doc = xml_result.value_as<html::ast::Document>();
  check(xml_result.accepted && !xml_result.cst().recovered() &&
            xml_result.diagnostics.empty() && xml_doc &&
            xml_doc->xmlDeclarations.size() == 2 &&
            xml_doc->xmlDeclarations[0] == "<?xml version='1.0'?>" &&
            xml_doc->xmlDeclarations[1] == "<?xml version='1.1'?>",
        "XML declaration extension retains repeated values for validation");

  auto cdata_result = parser->parse("<![CDATA[<tag>]]>");
  const auto *cdata_doc = cdata_result.value_as<html::ast::Document>();
  bool cdata_ok = cdata_result.accepted && !cdata_result.cst().recovered() &&
                  cdata_result.diagnostics.empty() && cdata_doc &&
                  cdata_doc->content.size() == 1;
  if (cdata_ok) {
    auto *const *cdata = std::get_if<html::ast::CData *>(&cdata_doc->content[0]);
    cdata_ok = cdata && *cdata && (*cdata)->text == "<![CDATA[<tag>]]>" &&
               (*cdata)->parent == cdata_doc;
  }
  check(cdata_ok, "CDATA extension retains document-level raw text and parent");
  auto source_result = parser->parse("<div title='&amp;\r\nx'> \r\n&amp;<br> </div>");
  const auto* source_doc = source_result.value_as<html::ast::Document>();
  bool source_ok = source_result.accepted && !source_result.cst().recovered() &&
                   source_result.diagnostics.empty() && source_doc && source_doc->content.size() == 1;
  if (source_ok) {
    auto* const* div = std::get_if<html::ast::Element*>(&source_doc->content[0]);
    source_ok = div && *div && (*div)->attributes.size() == 1 &&
                (*div)->attributes[0]->value == "&amp;\nx" && (*div)->content.size() == 3;
    if (source_ok) {
      auto* const* first = std::get_if<html::ast::Text*>(&(*div)->content[0]);
      auto* const* last = std::get_if<html::ast::Text*>(&(*div)->content[2]);
      source_ok = first && *first && (*first)->text == " \n&amp;" &&
                  last && *last && (*last)->text == " ";
    }
  }
  check(source_ok, "source AST keeps references and whitespace while normalizing CRLF");

  raw("<ScRiPt>" + std::string(100000, 'x') + "</SCRIPT >", true);
  return ok;
}
} // namespace html_raw_text_regressions
