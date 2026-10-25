#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "abnf/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

inline bool native_regressions(const auto& services) {
  namespace ast = abnf::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {std::fprintf(stderr, "abnf: %s\n", message);ok = false;}
  };
  const auto parsed = services.parser->parse(
      "Foo = 123456789012345678901234567890*223456789012345678901234567890%s\"X\"\r\n"
      "fOO =/ %B10.11 / [<prose>]\r\n");
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(),
        "unbounded repetition counts parse cleanly");
  const auto* root = cursiv::ast_cast<ast::Rulelist>(parsed.value);
  check(root && root->rules.size() == 2, "both definitions retained");
  if (root && root->rules.size() == 2) {
    const auto* first = root->rules[0];
    check(first->name == "Foo" && !first->incremental && first->parent == root,
          "basic definition");
    const auto* second = root->rules[1];
    check(second->name == "fOO" && second->incremental && second->parent == root &&
              second->elements->concatenations.size() == 2, "incremental alternatives");
    const auto* repetition = first->elements->concatenations[0]->repetitions[0];
    check(repetition->hasStar && !repetition->exact &&
              repetition->min == "123456789012345678901234567890" &&
              repetition->max == "223456789012345678901234567890", "exact count spellings");
    const auto* text = cursiv::ast_cast<ast::CharVal>(repetition->element);
    check(text && text->text == "%s\"X\"" && text->parent == repetition,
          "case-sensitive string");
    const auto* number = cursiv::ast_cast<ast::NumVal>(
        second->elements->concatenations[0]->repetitions[0]->element);
    check(number && number->text == "%B10.11", "numeric spelling");
    const auto* option = cursiv::ast_cast<ast::OptionElem>(
        second->elements->concatenations[1]->repetitions[0]->element);
    const auto* prose = option ? cursiv::ast_cast<ast::ProseVal>(
        option->alt->concatenations[0]->repetitions[0]->element) : nullptr;
    check(prose && prose->text == "<prose>", "nested prose");
  }
  struct Case {const char* text;const char* message;};
  for (const auto& c : {
      Case{"a = 3*2\"x\"\r\n", "repetition maximum"},
      {"a = %b2\r\n", "outside its base"},
      {"a = %dA\r\n", "outside its base"},
      {"a = %x41-42.43\r\n", "two bounds"},
      {"a = %x41-42-43\r\n", "two bounds"},
      {"some_name = \"a\"\r\n", "rule name"},
      {"a = some_name\r\n", "rule name"},
      {"a = \"é\"\r\n", "printable ASCII"},
      {"a = \"a\tb\"\r\n", "printable ASCII"},
      {"a = <é>\r\n", "printable ASCII"},
      {"a = \"x\"\"y\"\r\n", "Separate concatenated"},
      {"a = \"x\"\r\n/ \"y\"\r\n", "must be indented"},
      {"a = 2 \"x\"\r\n", "attached"},
      {"a = 1 *2\"x\"\r\n", "attached"}}) {
    const auto r = services.parser->parse(c.text);
    check(r.accepted && !r.cst().recovered() && r.diagnostics.empty(),
          "invalid form reaches runtime validation");
    const auto checked = cursiv::test::validationHelper(services, c.text);
    check(checked.diagnostics.size() == 1 &&
              checked.diagnostics[0].message.find(c.message) != std::string::npos,
          c.message);
  }
  for (std::string text : {
           "a = \"x\" ; comment\r\n  \"y\"\r\n",
           "a = \"x\"\r\n / \"y\"\r\n",
           "a = %xFF-00\r\n", "a = \"\" <>\r\n",
           "a = 123456789012345678901234567890\"x\"\r\n"})
    check(cursiv::test::validationHelper(services, text).diagnostics.empty(),
          "valid source retains native semantics");
  const std::string bad = "a = 3*2\"x\"\r\n";
  const auto precise = cursiv::test::validationHelper(services, bad);
  check(precise.diagnostics.size() == 1 && precise.diagnostics[0].range.begin == bad.find('2') &&
            precise.diagnostics[0].range.end == bad.find('2') + 1,
        "repetition error selects its maximum");
  return ok;
}
