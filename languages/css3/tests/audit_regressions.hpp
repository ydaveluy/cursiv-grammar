#pragma once

#include <cstdio>
#include <string_view>
#include <utility>

#include "css3/core/ast.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace css3_audit {

inline bool run(const cursiv::Parser& parser) {
  using namespace css3::ast;
  int failures = 0;
  const auto check = [&](bool ok) { if (!ok) ++failures; };
  const auto parse = [&](std::string_view text) {
    auto result = parser.parse(text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    return result;
  };
  for (const auto& [source, expected] : {
           std::pair<std::string_view, std::string_view>{R"(.x { content: "\41 \1f600 "; })", "A😀"},
           {".x { content: \"a\\\r\nb\"; }", "ab"},
           {R"(.x { content: "\0 \d800 \110000 "; })", "���"}}) {
    const auto result = parse(source);
    unsigned strings = 0;
    for (const auto* node : result.nodes)
      if (const auto* string = cursiv::ast_cast<StringLit>(node)) {
        ++strings;
        check(string->value == expected);
      }
    check(strings == 1);
  }
  {
    const auto result = parse(".--x { --:; --1:var(--x, blue); y:var(--x,); }");
    unsigned declarations = 0, variables = 0, classes = 0;
    for (const auto* node : result.nodes) {
      if (const auto* declaration = cursiv::ast_cast<Declaration>(node)) {
        ++declarations;
        if (declaration->property == "--") check(declaration->value.empty());
      }
      if (const auto* var = cursiv::ast_cast<VarRef>(node)) {
        ++variables;
        check(var->name == "--x" && var->hasFallback);
        const auto* parent = cursiv::ast_cast<Declaration>(var->parent);
        check(parent != nullptr);
        if (parent) check(var->fallback.size() == (parent->property == "y" ? 0 : 1));
      }
      if (const auto* cls = cursiv::ast_cast<ClassSelector>(node)) {
        ++classes;
        check(cls->name == "--x");
      }
    }
    check(declarations == 3 && variables == 2 && classes == 1);
  }
  {
    const auto result = parse("a[data-x=\"A\" i] { color:red; }");
    unsigned attributes = 0;
    for (const auto* node : result.nodes)
      if (const auto* attribute = cursiv::ast_cast<AttribSelector>(node)) {
        ++attributes;
        check(attribute->name == "data-x" && attribute->value == "A" && attribute->modifier == "i");
      }
    check(attributes == 1);
  }
  for (const std::string_view source : {":is(.a,.b) { color:red; }", ":not(.a > .b) { color:red; }"}) {
    const auto result = parse(source);
    unsigned pseudos = 0;
    for (const auto* node : result.nodes)
      if (const auto* pseudo = cursiv::ast_cast<PseudoSelector>(node)) {
        ++pseudos;
        check(pseudo->args.size() == 5);
      }
    check(pseudos == 1);
  }
  {
    const auto result = parse("a { --x:; color:; }");
    unsigned declarations = 0;
    for (const auto* node : result.nodes)
      if (const auto* declaration = cursiv::ast_cast<Declaration>(node)) {
        ++declarations;
        check(declaration->value.empty());
      }
    check(declarations == 2);
  }
  {
    const auto result = parse("a { x:var(--x, var(--y, red)); }");
    unsigned variables = 0;
    for (const auto* node : result.nodes)
      if (const auto* var = cursiv::ast_cast<VarRef>(node)) {
        ++variables;
        check(var->hasFallback && var->fallback.size() == 1);
        if (var->name == "--x" && !var->fallback.empty())
          check(cursiv::ast_cast<VarRef>(var->fallback[0]) != nullptr);
      }
    check(variables == 2);
  }

  {
    const auto result = parse(R"css(.a/* x y */.b, .a /* comment */ .b {
      empty:url(); escaped:url(a\20 b); spaced:url (x);
      signed:- 1px; tight:-1px; block:{foo:1; nested:{bar:2;}};
      fn:calc (1 + 2); variable:var (--x);
      actual:calc(1px /* x */ + /* y */ 2px);
      comments:calc(1px/* x */+/* y */2px);
    })css");
    const auto* sheet = result.value_as<Stylesheet>();
    const auto* rule = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<Ruleset>(sheet->statements[0]) : nullptr;
    check(rule && rule->selectors && rule->selectors->selectors.size() == 2 &&
          rule->declarations.size() == 10);
    if (rule && rule->selectors && rule->selectors->selectors.size() == 2) {
      const auto* compound = rule->selectors->selectors[0];
      const auto* descendant = rule->selectors->selectors[1];
      check(compound->sequences.size() == 1 && compound->sequences[0]->items.size() == 2);
      check(descendant->sequences.size() == 2 &&
            descendant->sequences[1]->combinator == Combinator::descendant);
    }
    if (rule && rule->declarations.size() == 10) {
      const auto* empty = rule->declarations[0]->value.size() == 1
          ? cursiv::ast_cast<UrlRef>(rule->declarations[0]->value[0]) : nullptr;
      const auto* url = rule->declarations[1]->value.size() == 1
          ? cursiv::ast_cast<UrlRef>(rule->declarations[1]->value[0]) : nullptr;
      check(empty && empty->value.empty() && url && url->value == "a b");
      for (const unsigned i : {2u, 6u, 7u}) {
        const auto* declaration = rule->declarations[i];
        check(declaration->value.size() == 2 &&
              cursiv::ast_cast<IdentValue>(declaration->value[0]) &&
              cursiv::ast_cast<GroupValue>(declaration->value[1]));
      }
      const auto* signed_value = rule->declarations[3];
      check(signed_value->value.size() == 2 &&
            cursiv::ast_cast<PunctValue>(signed_value->value[0]) &&
            cursiv::ast_cast<DimensionLit>(signed_value->value[1]));
      const auto* tight = rule->declarations[4]->value.size() == 1
          ? cursiv::ast_cast<DimensionLit>(rule->declarations[4]->value[0]) : nullptr;
      check(tight && tight->negative && tight->text == "1px");
      const auto* raw = rule->declarations[5]->value.size() == 1
          ? cursiv::ast_cast<RawBlock>(rule->declarations[5]->value[0]) : nullptr;
      check(raw && raw->items.size() == 2);
      check(rule->declarations[8]->value.size() == 1 &&
            cursiv::ast_cast<CalcExpr>(rule->declarations[8]->value[0]));
      check(rule->declarations[9]->value.size() == 1 &&
            cursiv::ast_cast<FunctionCall>(rule->declarations[9]->value[0]));
    }
  }
  {
    const auto result = parse(R"css(@page Chapter:left:first, :right {
      size:A4;
      @top-center {content:"Title";}
    })css");
    const auto* sheet = result.value_as<Stylesheet>();
    const auto* page = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<AtPage>(sheet->statements[0]) : nullptr;
    check(page && page->selectors.size() == 2 && page->declarations.size() == 1 &&
          page->margins.size() == 1);
    if (page && page->selectors.size() == 2 && page->margins.size() == 1) {
      check(page->selectors[0]->name == "Chapter" && page->selectors[0]->pseudos.size() == 2 &&
            page->selectors[0]->pseudos[0] == "left" && page->selectors[0]->pseudos[1] == "first");
      check(page->selectors[1]->name.empty() && page->selectors[1]->pseudos.size() == 1 &&
            page->selectors[1]->pseudos[0] == "right");
      check(page->margins[0]->name == "@top-center" && page->margins[0]->declarations.size() == 1);
    }
  }
  for (const std::string_view source : {"@supports future(foo) {}", "@supports not(display:block) {}"}) {
    const auto result = parse(source);
    const auto* sheet = result.value_as<Stylesheet>();
    const auto* supports = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<AtSupports>(sheet->statements[0]) : nullptr;
    const auto* general = supports ? cursiv::ast_cast<SupportsGeneral>(supports->condition) : nullptr;
    check(general && general->items.size() == 1 &&
          cursiv::ast_cast<FunctionCall>(general->items[0]));
  }
  {
    const auto result = parse("@mediaé(foo);");
    const auto* sheet = result.value_as<Stylesheet>();
    const auto* unknown = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<UnknownAtRule>(sheet->statements[0]) : nullptr;
    check(unknown && unknown->name == "@mediaé");
  }
  {
    const auto result = parse(R"css(@keyframes "slide" {from {opacity:0;} to {opacity:1;}})css");
    const auto* sheet = result.value_as<Stylesheet>();
    const auto* frames = sheet && sheet->statements.size() == 1
        ? cursiv::ast_cast<AtKeyframes>(sheet->statements[0]) : nullptr;
    check(frames && frames->name == "slide" && frames->blocks.size() == 2);
  }
  {
    const auto result = parse("a {x:U+12??; y:U+?F; z:U+1?-AF;}");
    unsigned ranges = 0, tails = 0;
    for (const auto* node : result.nodes) {
      if (const auto* range = cursiv::ast_cast<UnicodeRangeLit>(node)) {
        check(range->text == "U+12??" || range->text == "U+?" || range->text == "U+1?");
        ++ranges;
      }
      if (const auto* tail = cursiv::ast_cast<IdentValue>(node))
        if (tail->name == "F" || tail->name == "-AF") ++tails;
    }
    check(ranges == 3 && tails == 2);
  }
  if (failures) std::fprintf(stderr, "css3: %d AST audit failures\n", failures);
  return failures == 0;
}

}  // namespace css3_audit
