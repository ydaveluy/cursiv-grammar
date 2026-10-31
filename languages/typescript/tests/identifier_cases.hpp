#pragma once

#include <cstdio>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "typescript/core/CoreServices.hpp"
#include "typescript/core/ast.hpp"

namespace typescript_identifier_cases {
inline bool run(const typescript::TypeScriptCoreServices& services) {
  struct Case { const char* name; const char* source; bool accepted; const char* raw; };
  const Case cases[] = {
    {"letter", "const \\u0061=1;", true, "\\u0061"},
    {"continue-digit", "const a\\u0030=1;", true, "a\\u0030"},
    {"braced", "const \\u{61}=1;", true, "\\u{61}"},
    {"astral", "const \\u{1D49C}=1;", true, "\\u{1D49C}"},
    {"leading-zeros", "const \\u{00000061}=1;", true, "\\u{00000061}"},
    {"joiner-continue", "const a\\u200C=1;", true, "a\\u200C"},
    {"underscore", "const \\u005F=1;", true, "\\u005F"},
    {"dollar", "const \\u0024=1;", true, "\\u0024"},
    {"digit-start", "const \\u0030=1;", false, ""},
    {"hyphen-continue", "const a\\u002D=1;", false, ""},
    {"surrogate", "const \\uD800=1;", false, ""},
    {"overflow", "const \\u{110000}=1;", false, ""},
    {"emoji", "const \\u{1F600}=1;", false, ""},
    {"joiner-start", "const \\u200C=1;", false, ""},
    {"braced-surrogate", "const \\u{D800}=1;", false, ""},
    {"private-digit", "class C{#\\u0030=1;}", false, ""},
    {"property-digit", "const x={\\u0030:1};", false, ""},
    {"member-hyphen", "const x=obj.\\u002D;", false, ""},
    {"surrogate-pair", "const \\uD835\\uDC9C=1;", false, ""},
    {"mark-continue", "const a\\u0345=1;", true, "a\\u0345"},
    {"mark-start", "const \\u0345=1;", false, ""},
    {"private-letter", "class C{#\\u0061=1;}", true, "#\\u0061"},
    {"type-digit", "type T=\\u0030;", false, ""},
    {"type-keyword", "type T=\\u0069f;", true, "\\u0069f"},
  };
  bool ok = true;
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() &&
                       parsed.diagnostics.empty() && parsed.value;
    const auto validation = cursiv::test::validationHelper(services, std::string(test.source));
    const auto check = [&](bool passed, const char* message) {
      if (!passed) {
        std::fprintf(stderr, "typescript identifier %s: %s; source=%s\n",
                     test.name, message, test.source);
        ok = false;
      }
    };
    check((clean && validation.diagnostics.empty()) == test.accepted,
          "native identifier character/position verdict");
    if (!test.accepted) continue;
    if (!clean) {
      for (const auto& diagnostic : parsed.diagnostics)
        std::fprintf(stderr, "kind=%u bytes=%u..%u %s\n", unsigned(diagnostic.kind),
                     diagnostic.begin, diagnostic.end, diagnostic.message.c_str());
      continue;
    }
    check(parsed.cst().text() == test.source, "source text retained");
    bool rawSeen = false;
    for (const auto* node : parsed.nodes) {
      if (const auto* n = cursiv::ast_cast<typescript::ast::Ident>(node))
        rawSeen |= n->name == test.raw;
      if (const auto* n = cursiv::ast_cast<typescript::ast::TypeRef>(node))
        rawSeen |= n->name == test.raw;
    }
    check(rawSeen, "typed name retains its exact escaped spelling");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back();
      work.pop_back();
      if (!seen.insert(node).second) { check(false, "unique ownership"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() &&
              parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              "parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent,
          "all allocations reachable once");
  }
  std::printf("typescript identifier: %zu native character controls\n", std::size(cases));
  return ok;
}
}  // namespace typescript_identifier_cases
