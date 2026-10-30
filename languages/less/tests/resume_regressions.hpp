#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "less/core/ast.hpp"
#include "less/core/CoreServices.hpp"

namespace less_resume {
inline bool run(const less::LessCoreServices& language) {
  namespace a = less::ast;
  bool good = true;
  const auto check = [&](bool ok, const char* what) {
    if (!ok) std::fprintf(stderr, "less native regression: %s\n", what);
    good &= ok;
  };
  const auto verify = [&](const cursiv::ParseResult& result, std::string_view source) {
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && result.value,
          "expected a clean AST");
    check(result.cst().text() == source, "the CST must retain the original bytes");
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { check(false, "duplicate child"); return; }
      check(node.cstNodeId < result.cst().size(), "node must have a CST span");
      if (node.cstNodeId < result.cst().size()) {
        const auto span = result.cst()[node.cstNodeId];
        check(span.begin <= span.end && span.end <= source.size(), "source bounds");
      }
      language.astReflection->visitMembers(node, [&](auto, auto, auto& child) {
        check(child.parent == &node, "child parent");
        check(result.cst().contains(node.cstNodeId, child.cstNodeId), "child span");
        self(self, child);
      });
    };
    if (result.value) {
      check(result.value->parent == nullptr, "root parent");
      walk(walk, *result.value);
    }
    check(seen.size() == result.nodes.size(), "all allocations must be reachable");
  };
  struct StringCase { std::string_view raw, value; bool escaped; };
  for (const auto& c : {
           StringCase{R"("a\"b")", R"(a\"b)", false},
           StringCase{R"('a\'b')", R"(a\'b)", false},
           StringCase{R"("a\\b")", R"(a\\b)", false},
           StringCase{"\"a\r\nb\"", "a\nb", false},
           StringCase{"\"a\\\r\nb\"", "a\\\nb", false},
           StringCase{R"(~"a b")", "a b", true},
           StringCase{R"(~'a\'b')", R"(a\'b)", true}}) {
    const std::string source = "@v: " + std::string(c.raw) + ";";
    const auto result = language.parser->parse(source);
    verify(result, source);
    unsigned found = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::StringLit>(node)) {
        ++found;
        check(value->value == c.value && value->escaped == c.escaped,
              "quoted value and escaped flag");
        check(result.cst().text(value->cstNodeId) == c.raw, "raw quoted source");
      }
    check(found == 1, "one quoted value");
  }
  struct DimensionCase { std::string_view raw, unit; double value; };
  for (const auto& c : {DimensionCase{"+2px", "px", 2},
                       DimensionCase{"1rem", "rem", 1},
                       DimensionCase{"-.5dvh", "dvh", -.5},
                       DimensionCase{"1VH", "VH", 1},
                       DimensionCase{"1custom_unit", "custom_unit", 1}}) {
    const std::string source = "@v: " + std::string(c.raw) + ";";
    const auto result = language.parser->parse(source);
    verify(result, source);
    unsigned found = 0;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::Measurement>(node)) {
        ++found;
        check(value->value == c.value && value->unit == c.unit,
              "dimension number, open unit and unit case");
      }
    check(found == 1, "one dimension");
  }
  for (std::string_view gap : {" ", "/*comment*/"}) {
    const std::string source = "@v: 1" + std::string(gap) + "px; .a" +
                               std::string(gap) + ".b { v: foo bar; }";
    const auto result = language.parser->parse(source);
    verify(result, source);
    bool unit = false, selector = false, words = false;
    for (const auto* node : result.nodes) {
      if (const auto* value = cursiv::ast_cast<a::Measurement>(node))
        unit = value->unit.empty();
      if (const auto* value = cursiv::ast_cast<a::Selector>(node))
        selector = value->elements.size() == 2;
      if (const auto* value = cursiv::ast_cast<a::MulTerm>(node))
        words |= value->items.size() == 2;
    }
    check(unit && selector && words, "layout must separate values and selector elements");
  }
  for (std::string_view separator : {",", ";"}) {
    const std::string source = ".m(@a:1px" + std::string(separator) +
                               "@b:2rem){v:@a + @b;}";
    const auto result = language.parser->parse(source);
    verify(result, source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::MixinDef>(node))
        found = value->params.size() == 2;
    check(found, "comma and semicolon preserve two default parameters");
  }
  {
    const std::string source = ".m(@a:1,2;@b:3){v:1;}";
    const auto result = language.parser->parse(source);
    verify(result, source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* value = cursiv::ast_cast<a::MixinDef>(node))
        if (value->params.size() == 2)
          if (const auto* first = std::get_if<a::VariableDecl*>(&value->params[0]))
            found = *first && (*first)->value && (*first)->value->items.size() == 2;
    check(found, "semicolon mode retains the comma list in a default");
  }
  {
    const std::string source = ".m(@a:1,@b:2;@c:3){v:1;}";
    const auto result = language.parser->parse(source);
    check(result.cst().recovered() || !result.diagnostics.empty(),
          "mixed named-parameter delimiters need a diagnostic");
  }
  {
    const auto result = language.parser->parse("@@x:1;");
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "ordinary indirect declaration is rejected by native syntax");
  }
  for (std::string_view source : {"@name:base;@v:@@name;", ".m(@@x:1){v:1;}"}) {
    const auto result = language.parser->parse(source);
    verify(result, source);
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///valid.less", "less", 0, std::string(source)));
    document.language = &language;
    document.parseResult = language.parser->parse(source);
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.documentValidator->validateDocument(document, diagnostics);
    check(diagnostics.empty(), "indirection in a value or parameter stays valid");
  }
  return good;
}
} // namespace less_resume
