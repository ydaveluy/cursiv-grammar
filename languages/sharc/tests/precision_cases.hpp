#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreServices.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "sharc/core/ast.hpp"

namespace sharc_precision_cases {
inline bool run(const cursiv::CoreServices& language) {
  namespace ast = sharc::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* why) {
    if (!value) { std::fprintf(stderr, "sharc precision: %s\n", why); ok = false; }
  };
  const auto clean = [](const auto& result) {
    return result.accepted && !result.cst().recovered() && result.diagnostics.empty();
  };
  // VisualDSP++ 5.0 pp1-60/1-115: preserve numeric spelling while checking
  // the integer setting. No proprietary assembler oracle was available.
  struct Case { std::string_view value; bool valid; };
  constexpr Case cases[] = {
      {"32", true}, {"40", true}, {"032", true}, {"00040", true},
      {"0x20", true}, {"0X28", true}, {"B#100000", true}, {"b#101000", true},
      {"24", false}, {"0x18", false}, {"B#11000", false}, {"0", false},
      {"64", false}, {"4294967296", false}, {"32r", false},
      {"40.0", false}, {"3.2e1", false}};
  for (const auto& test : cases) {
    for (const auto* prefix : {".precision ", ".PrEcIsIoN = "}) {
      const std::string source = std::string(prefix) + std::string(test.value) + ";";
      const auto parsed = language.parser->parse(source);
      const auto* root = parsed.value_as<ast::Program>();
      const auto* statement = root && root->statements.size() == 1 ? root->statements[0] : nullptr;
      const auto* directive = statement ? cursiv::ast_cast<ast::Directive>(statement->atom) : nullptr;
      const auto* number = directive && directive->values.size() == 1
          ? cursiv::ast_cast<ast::NumberLit>(directive->values[0]) : nullptr;
      check(clean(parsed) && parsed.cst().text() == source && statement &&
            statement->parent == root && directive && directive->parent == statement &&
            directive->keyword == std::string_view(prefix).substr(0, 10) &&
            directive->attributes.empty() && directive->names.empty() && directive->strings.empty() &&
            number && number->value == test.value && number->parent == directive,
            "numeric spelling, directive identity or AST ownership lost");
      const auto checked = cursiv::test::validationHelper(language, source);
      check(test.valid ? checked.diagnostics.empty() :
            checked.diagnostics.size() == 1 && checked.diagnostics[0].code == "sharc.precision",
            "integer precision validation must accept only the values 32 and 40");
    }
  }
  for (const auto* source : {".precision;", ".precision 16+16;", ".precision (32);",
                            ".precision setting;", ".precision -32;"})
    check(!clean(language.parser->parse(source)),
          "precision still requires a literal rather than a missing value, expression or symbol");
  check(cursiv::test::validationHelper(language, ".align 24;").diagnostics.empty(),
        "precision restriction must not apply to another directive");
  return ok;
}
}  // namespace sharc_precision_cases
