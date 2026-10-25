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
  if (failures) std::fprintf(stderr, "css3: %d AST audit failures\n", failures);
  return failures == 0;
}

}  // namespace css3_audit
