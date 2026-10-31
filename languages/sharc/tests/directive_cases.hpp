#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "sharc/core/ast.hpp"

namespace sharc_directive_cases {
inline bool run(const cursiv::Parser& parser) {
  namespace ast = sharc::ast;
  bool ok = true;
  const auto check = [&](bool valid, const char* why) {
    if (!valid) { std::fprintf(stderr, "sharc directives: %s\n", why); ok = false; }
  };
  const auto clean = [](const auto& parsed) {
    return parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty();
  };
  // VisualDSP++5.0 assembler manual p1-90: names, optional values and lists.
  {
    constexpr auto text = ".FILE_ATTR at101=a123,at102,at103=\"999\",empty=\"\";";
    const auto parsed = parser.parse(text);
    const auto* root = parsed.value_as<ast::Program>();
    const auto* statement = root && root->statements.size() == 1 ? root->statements[0] : nullptr;
    const auto* directive = statement ? cursiv::ast_cast<ast::Directive>(statement->atom) : nullptr;
    check(clean(parsed) && parsed.cst().text() == text && directive &&
          statement->parent == root && directive->parent == statement &&
          directive->keyword == ".FILE_ATTR" && directive->attributes.size() == 4 &&
          directive->names.empty() && directive->strings.empty() && directive->values.empty(),
          "file attributes retain their structured list and exact source");
    if (directive && directive->attributes.size() == 4) {
      constexpr std::string_view names[] = {"at101", "at102", "at103", "empty"};
      constexpr std::string_view values[] = {"a123", "", "\"999\"", "\"\""};
      for (unsigned i = 0; i < 4; ++i) {
        const auto* attribute = directive->attributes[i];
        check(attribute && attribute->parent == directive && attribute->name == names[i] &&
              attribute->value == values[i], "attribute identity, value presence or owner lost");
      }
    }
  }
  for (const std::string_view value : {"32", "40"}) {
    const std::string source = std::string(".precision = ") + std::string(value) + ";";
    const auto parsed = parser.parse(source);
    const auto* root = parsed.value_as<ast::Program>();
    const auto* directive = root && root->statements.size() == 1
        ? cursiv::ast_cast<ast::Directive>(root->statements[0]->atom) : nullptr;
    const auto* number = directive && directive->values.size() == 1
        ? cursiv::ast_cast<ast::NumberLit>(directive->values[0]) : nullptr;
    check(clean(parsed) && number && number->value == value && number->parent == directive,
          "documented precision setting and its value AST");
  }
  // Grouping directives by operand shape must preserve their original fields.
  for (const auto* source : {".COMPRESS;", ".FORCECOMPRESS;", ".LIST;", ".NOLIST;",
                            ".LIST_DATA;", ".LIST_DATFILE;", ".LIST_WRAPDATA;",
                            ".NEWPAGE;", ".NOCOMPRESS;", ".NOLIST_DATA;",
                            ".NOLIST_DATFILE;", ".NOLIST_WRAPDATA;", ".ROUND_MINUS;",
                            ".ROUND_NEAREST;", ".ROUND_PLUS;", ".ROUND_ZERO;", ".PREVIOUS;"}) {
    const auto parsed = parser.parse(source);
    const auto* root = parsed.value_as<ast::Program>();
    const auto* directive = root && root->statements.size() == 1
        ? cursiv::ast_cast<ast::Directive>(root->statements[0]->atom) : nullptr;
    check(clean(parsed) && directive && directive->keyword == std::string_view(source).substr(0, std::string_view(source).size()-1) &&
          directive->values.empty() && directive->names.empty() && directive->strings.empty() &&
          directive->attributes.empty(), "no-argument directive changed during factoring");
  }
  {
    const auto parsed = parser.parse("first:\nsecond: r0=1;\nfinish:");
    const auto* root = parsed.value_as<ast::Program>();
    const auto* first = root && root->statements.size() == 2 ? root->statements[0] : nullptr;
    const auto* last = root && root->statements.size() == 2 ? root->statements[1] : nullptr;
    check(clean(parsed) && first && last && first->labels.size() == 2 &&
          first->labels[0] == "first" && first->labels[1] == "second" && first->atom &&
          first->parent == root && first->atom->parent == first && last->parent == root &&
          last->labels.size() == 1 && last->labels[0] == "finish" && !last->atom,
          "preceding-line labels and a final label retain their statement positions");
  }
  for (const auto* source : {".file_attr;", ".file_attr a=;", ".file_attr \"name\";",
                            ".file_attr a=123;", ".file_attr a='x';", ".file_attr a,;",
                            "r0=1"})
    check(!clean(parser.parse(source)), "native malformed directive or missing terminator accepted");
  return ok;
}
}  // namespace sharc_directive_cases
