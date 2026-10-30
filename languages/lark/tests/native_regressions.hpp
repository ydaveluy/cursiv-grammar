#pragma once

#include <cstdio>
#include <string>
#include <string_view>

#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "lark/core/ast.hpp"

inline bool native_regressions(const auto& services) {
  namespace ast = lark::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) {std::fprintf(stderr, "lark: %s\n", message);ok = false;}
  };
  const char* source = R"lark(# grammar
!?start{item}.123456789012345678901234567890: item \
    "\u0061"
 # alternative
 | ["b"]~٢..٤ -> choice
%import .foo.bar (A, b)
%declare X Y
)lark";
  const auto parsed = services.parser->parse(source);
  check(parsed.accepted && !parsed.cst().recovered() &&
            parsed.diagnostics.empty(), "native grammar parses cleanly");
  const auto* root = cursiv::ast_cast<ast::Start>(parsed.value);
  check(root && root->items.size() == 3, "three statements");
  if (root && root->items.size() == 3) {
    const auto* d = cursiv::ast_cast<ast::Declaration>(root->items[0]);
    check(d && d->name == "start" && d->modifiers == "!?" &&
              !d->terminal && d->params.size() == 1 && d->params[0] == "item" &&
              d->priority == "123456789012345678901234567890", "declaration fields");
    check(d && d->body && d->body->alternatives.size() == 2,
          "commented continuation retains both alternatives");
    if (d && d->body && d->body->alternatives.size() == 2) {
      const auto* first = d->body->alternatives[0]->expansion;
      check(first && first->items.size() == 2, "backslash continues a sequence");
      if (first && first->items.size() == 2) {
        const auto* value = std::get_if<ast::Value*>(&first->items[1]->atom->content);
        const auto* literal = value ? cursiv::ast_cast<ast::Literal>(*value) : nullptr;
        check(literal && literal->text == R"("\u0061")", "literal spelling");
      }
      const auto* second = d->body->alternatives[1];
      check(second->label == "choice" && second->expansion->items.size() == 1,
            "alias and grouped expression");
      if (second->expansion->items.size() == 1) {
        const auto* e = second->expansion->items[0];
        check(e->atom->maybe && e->repeatMin == "٢" && e->repeatMax == "٤",
              "Unicode repeat bounds");
      }
    }
    const auto* import = cursiv::ast_cast<ast::ImportStmt>(root->items[1]);
    check(import && import->relative && import->path.size() == 2 &&
              import->path[0] == "foo" && import->path[1] == "bar" &&
              import->names.size() == 2 && import->names[0] == "A" &&
              import->names[1] == "b", "relative multiple import");
    const auto* declared = cursiv::ast_cast<ast::DeclareStmt>(root->items[2]);
    check(declared && declared->names.size() == 2 && declared->names[0] == "X" &&
              declared->names[1] == "Y", "declared terminals");
  }
  struct RuntimeCase {const char* text;const char* message;};
  for (const auto& c : {
      RuntimeCase{"start{x,x}: x\n", "repeated"},
      {"?_start: \"x\"\n", "question mark"},
      {"start: \"x\"~4..2\n", "upper bound"},
      {"start: \"x\"~-2..2\n", "negative lower"},
      {"start: \"x\"~٢..١\n", "upper bound"},
      {"start: \"\"\n", "empty terminal"},
      {"start: /a\nb/\n", "x flag"},
      {"A{item}: item\n", "template parameters"},
      {"start: A{\"x\"}\n", "template must"},
      {"start: \"x\" -> A\n", "alias must"},
      {"!!start: \"x\"\n", "only once"},
      {"!A: \"x\"\n", "Only rules"},
      {"start{A}: A\n", "parameter must"}}) {
    const auto p = services.parser->parse(c.text);
    check(p.accepted && !p.cst().recovered() && p.diagnostics.empty(),
          "invalid form reaches runtime validation");
    const auto result = cursiv::test::validationHelper(services, c.text);
    check(result.diagnostics.size() == 1 &&
              result.diagnostics[0].message.find(c.message) != std::string::npos,
          c.message);
  }
  for (std::string text : {"start: /a\nb/x\n", "start: \"x\"~٠..٢\n",
                           "start.99999999999999999999999999999999: \"x\"\n"})
    check(cursiv::test::validationHelper(services, text).diagnostics.empty(),
          "valid native form has no runtime error");
  const auto adjacent = services.parser->parse("start: \"x\"?_item\n_item: \"y\"\n");
  check(!adjacent.accepted || adjacent.cst().recovered() || !adjacent.diagnostics.empty(),
        "a question-mark operator cannot run into a lowercase/underscore rule name");
  const auto spaced = services.parser->parse("start: \"x\"? _item\n_item: \"y\"\n");
  check(spaced.accepted && !spaced.cst().recovered() && spaced.diagnostics.empty(),
        "a spaced question-mark operator and following rule remain valid");
  const std::string text = "start: \"x\"~4..2\n";
  const auto precise = cursiv::test::validationHelper(services, text);
  check(precise.diagnostics.size() == 1 &&
            precise.diagnostics[0].range.begin == text.find('2') &&
            precise.diagnostics[0].range.end == text.find('2') + 1,
        "range error selects its upper bound");
  return ok;
}
