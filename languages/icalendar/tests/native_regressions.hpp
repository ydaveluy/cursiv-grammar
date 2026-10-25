#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "cursiv/test/ValidationHelper.hpp"
#include "icalendar/core/Parameters.hpp"

inline int native_regressions(const auto& language) {
  namespace ast = icalendar::ast;
  const auto clean = [](const auto& p) {
    return p.accepted && !p.cst().recovered() && p.diagnostics.empty() && p.value;
  };
  const std::string source =
      "BEGIN:X-ROOT\nX-BEFORE:a\nBEGIN:X-CHILD\n"
      "S U M M A R Y:folded\n value\nATTENDEE ; C\n N = "
      "\"A^nB^^C^'D^N^x\", b,,\"\" :mailto:t@example.org\n"
      "DESCRIPTION:A\\nB\\,C\nEND:X-CHILD\nX-AFTER:b\nEND:X-ROOT\n"
      "B\n EGIN:X-OTHER\nE\n ND:X-OTHER";
  const auto p = language.parser->parse(source);
  if (!clean(p)) return 1;
  const auto* doc = cursiv::ast_cast<ast::Document>(p.value);
  if (!doc || doc->components.size() != 2 || !doc->properties.empty()) return 2;
  const auto* root = doc->components[0];
  if (root->name != "X-ROOT" || root->closeName != "X-ROOT" ||
      root->properties.size() != 2 || root->properties[0]->value != "a" ||
      root->properties[1]->value != "b" || root->subcomponents.size() != 1)
    return 3;
  const auto* child = root->subcomponents[0];
  if (child->parent != root || child->properties.size() != 3 ||
      child->properties[0]->name != "SUMMARY" ||
      child->properties[0]->value != "foldedvalue" ||
      child->properties[2]->value != "A\\nB\\,C") return 4;
  const auto* attendee = child->properties[1];
  if (attendee->params.size() != 1) return 5;
  const auto* param = attendee->params[0];
  if (param->parent != attendee || param->name != "CN" ||
      icalendar::parameter_values(param->rawValue) !=
          std::vector<std::string>{"A\nB^C\"D^N^x", " b", "", ""}) return 6;
  const auto marked = language.parser->parse("BEGIN;X-P=a:X-A\nEND;X-Q=b:X-A");
  if (!clean(marked)) return 7;
  const auto* component = cursiv::ast_cast<ast::Document>(marked.value)->components[0];
  if (component->params.size() != 1 || component->closeParams.size() != 1 ||
      component->params[0]->name != "X-P" ||
      component->closeParams[0]->rawValue != "b") return 8;
  const auto escapedFold = language.parser->parse(
      "BEGIN:X\nX-P;P=a\\\n ;b;Q=a\\\n :b;R=a\\\n ,b:v\nEND:X");
  if (!clean(escapedFold)) return 14;
  const auto* escapedProperty = cursiv::ast_cast<ast::Document>(escapedFold.value)->components[0]->properties[0];
  if (escapedProperty->params.size() != 3 ||
      icalendar::parameter_values(escapedProperty->params[0]->rawValue) != std::vector<std::string>{"a;b"} ||
      icalendar::parameter_values(escapedProperty->params[1]->rawValue) != std::vector<std::string>{"a:b"} ||
      icalendar::parameter_values(escapedProperty->params[2]->rawValue) != std::vector<std::string>{"a,b"}) return 15;
  const auto unicodeName = language.parser->parse("BEGIN:X\n\u00a0X-P\u00a0:v\nEND:X");
  if (!clean(unicodeName) || cursiv::ast_cast<ast::Document>(unicodeName.value)->components[0]->properties[0]->name != "X-P") return 16;
  const auto noColon = language.parser->parse("BEGIN:X\nX-P;P=x\nEND:X");
  if (!clean(noColon)) return 9;
  const auto* property = cursiv::ast_cast<ast::Document>(noColon.value)->components[0]->properties[0];
  if (property->hasValueDelimiter || !property->value.empty()) return 10;
  struct Invalid { std::string_view text; std::string_view message; };
  for (const auto& c : std::vector<Invalid>{
           {"BEGIN:X-A\nEND:X-B", "different component"},
           {"BEGIN:X\nX-!BAD:a\nEND:X", "Invalid property name"},
           {"BEGIN:X\nX-P;X-!BAD=a:v\nEND:X", "Invalid parameter name"},
           {"BEGIN:X\nX-P;P=\"a\x01" "b\":v\nEND:X", "Invalid parameter value"},
           {"BEGIN:X\nX-P\nEND:X", "value delimiter"},
           {"X-P:v", "belong to a component"},
           {"BEGIN:X\nX-P;P=\"\"a\"\":v\nEND:X", "Invalid parameter value"}}) {
    const auto syntax = language.parser->parse(c.text);
    if (!clean(syntax)) return 11;
    const auto v = cursiv::test::validationHelper(language, std::string(c.text));
    if (v.diagnostics.size() != 1 ||
        v.diagnostics[0].message.find(c.message) == std::string::npos) return 12;
  }
  if (cursiv::test::validationHelper(language,
      "BEGIN:X\nATTENDEE;CN=\"A^nB\":mailto:t@x\nEND:X").diagnostics.size()) return 13;
  return 0;
}
