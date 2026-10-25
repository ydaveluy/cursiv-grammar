#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/DocumentValidator.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "graphql/core/CoreServices.hpp"

namespace graphql_resume {
inline bool run(const graphql::GraphqlCoreServices& language) {
  namespace a = graphql::ast;
  bool good = true;
  const auto check = [&](bool ok, const char* what) {
    if (!ok) std::fprintf(stderr, "graphql native regression: %s\n", what);
    good &= ok;
  };
  const auto positive = [&](std::string_view source) {
    auto result = language.parser->parse(source);
    check(result.accepted && !result.cst().recovered() &&
              result.diagnostics.empty() && result.value, "clean source");
    check(result.cst().text() == source, "exact source bytes");
    std::unordered_set<const cursiv::AstNode*> seen;
    const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
      if (!seen.insert(&node).second) { check(false, "duplicate AST child"); return; }
      check(node.cstNodeId < result.cst().size(), "CST node");
      if (node.cstNodeId < result.cst().size()) {
        const auto span = result.cst()[node.cstNodeId];
        check(span.begin <= span.end && span.end <= source.size(), "source bounds");
      }
      language.astReflection->visitMembers(node, [&](auto, auto, auto& child) {
        check(child.parent == &node, "AST parent");
        check(result.cst().contains(node.cstNodeId, child.cstNodeId), "CST containment");
        self(self, child);
      });
    };
    if (result.value) {
      check(result.value->parent == nullptr, "root parent");
      walk(walk, *result.value);
    }
    check(seen.size() == result.nodes.size(), "all allocations are reachable");
    return result;
  };
  for (std::string_view number : {
           "-0", "9223372036854775808", "-9223372036854775809",
           "9007199254740993", "1234567890123456789012345678901234567890",
           "-0.00", "1e4000", "1e-4000", "1.234567890123456789"}) {
    const std::string source = "{f(x:" + std::string(number) + ")}";
    const auto result = positive(source);
    unsigned found = 0;
    for (const auto* node : result.nodes) {
      if (const auto* n = cursiv::ast_cast<a::IntLit>(node)) {
        ++found;
        check(n->value == number && result.cst().text(n->cstNodeId) == number,
              "exact integer spelling and span");
      }
      if (const auto* n = cursiv::ast_cast<a::FloatLit>(node)) {
        ++found;
        check(n->value == number && result.cst().text(n->cstNodeId) == number,
              "exact float spelling and span");
      }
    }
    check(found == 1, "one numeric node");
  }
  {
    const std::string source = "query Q($,#comment\nx:Int){f(x:$ x)}";
    const auto result = positive(source);
    unsigned variables = 0, definitions = 0;
    for (const auto* node : result.nodes) {
      if (const auto* n = cursiv::ast_cast<a::Variable>(node))
        variables += n->name == "x";
      if (const auto* n = cursiv::ast_cast<a::VariableDefinition>(node))
        definitions += n->name == "x";
    }
    check(variables == 1 && definitions == 1, "ignored tokens after dollar");
  }
  {
    const std::string source = R"({f(x:"\u{00000041}\uD83D\uDE00\u{10FFFF}")})";
    const auto result = positive(source);
    bool found = false;
    for (const auto* node : result.nodes)
      if (const auto* arg = cursiv::ast_cast<a::Argument>(node))
        if (const auto* value = std::get_if<std::string_view>(&arg->value))
          found = *value == "A😀\xF4\x8F\xBF\xBF";
    check(found, "Unicode scalar boundary and paired surrogate");
  }
  const auto negative = [&](std::string_view source, std::string_view code) {
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///native.graphql", "graphql", 0, std::string(source)));
    document.language = &language;
    document.parseResult = language.parser->parse(source);
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.documentValidator->validateDocument(document, diagnostics);
    bool found = !document.parseResult.diagnostics.empty() || document.parseRecovered();
    if (!code.empty()) {
      found = false;
      for (const auto& d : diagnostics)
        if (d.code && d.code->isString() && d.code->string() == code) {
          found = true;
          check(d.range.begin <= d.range.end && d.range.end <= source.size(),
                "diagnostic range");
        }
    } else found |= !diagnostics.empty();
    check(found, "native-invalid syntax must have a diagnostic");
  };
  for (std::string_view source : {
           R"({f(x:"\uD800")})", R"({f(x:"\uDC00")})",
           R"({f(x:"\uD800\u0041")})", R"({f(x:"\u{D800}")})",
           R"({f(x:"\u{110000}")})", R"({f(x:"\u{FFFFFFFF}")})",
           R"({f(x:"\u{000000041}")})", "{f(x:\"a\nb\")}",
           "{f(x:[123abc])}", "{f(x:[01])}", "{f(x:[1.2.3])}"})
    negative(source, {});
  for (std::string_view source : {
           "query Q($x:Int=$y){f}", "query Q($x:[Int]=[1,$y]){f}",
           "query Q($x:Int @d(a:$y)){f}", "type T @d(a:$x){f:Int}",
           "input I{f:Int={a:$x}}", "directive @d(a:Int=$x) on FIELD",
           "schema @d(a:$x){query:T}"})
    negative(source, "graphql.constant-value");
  for (std::string_view source : {
           "extend schema", "extend scalar S", "extend type T",
           "extend interface I", "extend union U", "extend enum E", "extend input I"})
    negative(source, "graphql.empty-extension");
  negative("fragment on on T{f}", "graphql.fragment-name");
  negative("{...on}", {});
  for (std::string_view source : {"enum E{true}", "enum E{false}", "enum E{null}"})
    negative(source, "graphql.enum-value");
  for (std::string_view source : {
           "{f(x:$y) @d(a:$z)}", "query Q($x:Int=0){f(x:$x)}",
           "input I{f:Int=null}", "fragment F on T{on type true}",
           "extend scalar S @d", "extend type T implements I"}) {
    const auto result = positive(source);
    cursiv::Document document(cursiv::TextDocument::create(
        "file:///valid.graphql", "graphql", 0, std::string(source)));
    document.language = &language;
    document.parseResult = language.parser->parse(source);
    if (document.parseResult.value) document.parseResult.value->parent = &document;
    std::vector<cursiv::Diagnostic> diagnostics;
    language.documentValidator->validateDocument(document, diagnostics);
    check(diagnostics.empty(), "valid runtime control");
  }
  return good;
}
} // namespace graphql_resume
