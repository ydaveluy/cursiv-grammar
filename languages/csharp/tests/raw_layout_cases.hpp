#pragma once
#include <cstdio>
#include <initializer_list>
#include <string>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "csharp/core/ast.hpp"
namespace csharp_raw_layout {
inline void run(const auto& language) {
  struct Case { const char* source; const char* literal; bool valid; };
  for (const Case& test : std::initializer_list<Case>{
      {"class C { string s = \"\"\"text\"\"\"; }", "\"\"\"text\"\"\"", true},
      {"class C { string s = \"\"\"   \"\"\"; }", "\"\"\"   \"\"\"", true},
      {"class C { string s = \"\"\"\n\"\"\"; }", "\"\"\"\n\"\"\"", false},
      {"class C { string s = \"\"\"\n\n\"\"\"; }", "\"\"\"\n\n\"\"\"", true},
      {"class C { string s = \"\"\"\n  text\n  \"\"\"; }", "\"\"\"\n  text\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n text\n  \"\"\"; }", "\"\"\"\n text\n  \"\"\"", false},
      {"class C { string s = \"\"\"\n   text\n  \"\"\"; }", "\"\"\"\n   text\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n  text\"\"\"; }", "\"\"\"\n  text\"\"\"", false},
      {"class C { string s = \"\"\"text\n  \"\"\"; }", "\"\"\"text\n  \"\"\"", false},
      {"class C { string s = \"\"\" \t\n  text\n  \"\"\"; }", "\"\"\" \t\n  text\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n  a\n \n  b\n  \"\"\"; }", "\"\"\"\n  a\n \n  b\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n  a\n   \n  b\n  \"\"\"; }", "\"\"\"\n  a\n   \n  b\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n  a\n\n  b\n  \"\"\"; }", "\"\"\"\n  a\n\n  b\n  \"\"\"", true},
      {"class C { string s = \"\"\"\n  a\n\t\n  b\n  \"\"\"; }", "\"\"\"\n  a\n\t\n  b\n  \"\"\"", false},
      {"class C { string s = \"\"\"\n\ta\n\t\"\"\"; }", "\"\"\"\n\ta\n\t\"\"\"", true},
      {"class C { string s = \"\"\"\n\t a\n\t \"\"\"; }", "\"\"\"\n\t a\n\t \"\"\"", true},
      {"class C { string s = \"\"\"\n\ta\n  \"\"\"; }", "\"\"\"\n\ta\n  \"\"\"", false},
      {"class C { string s = \"\"\"\n  \"abc\"\n  \"\"\"; }", "\"\"\"\n  \"abc\"\n  \"\"\"", true},
      {"class C { string s = \"\"\"\"\n  a\"\"\"b\n  \"\"\"\"; }", "\"\"\"\"\n  a\"\"\"b\n  \"\"\"\"", true},
      {"class C { string s = \"\"\"\r  a\r  \"\"\"; }", "\"\"\"\r  a\r  \"\"\"", true},
      {"class C { string s = \"\"\"\r\n  a\r\n  \"\"\"; }", "\"\"\"\r\n  a\r\n  \"\"\"", true},
      {"class C { string s = \"\"\"\u0085  a\u0085  \"\"\"; }", "\"\"\"\u0085  a\u0085  \"\"\"", true},
      {"class C { string s = \"\"\"\u2028  a\u2028  \"\"\"; }", "\"\"\"\u2028  a\u2028  \"\"\"", true},
      {"class C { string s = \"\"\"\u2029  a\u2029  \"\"\"; }", "\"\"\"\u2029  a\u2029  \"\"\"", true},
      {"class C { string s = \"\"\"\u000b\n\u000ba\n\u000b\"\"\"; }", "\"\"\"\u000b\n\u000ba\n\u000b\"\"\"", true},
      {"class C { string s = \"\"\"\f\n\fa\n\f\"\"\"; }", "\"\"\"\f\n\fa\n\f\"\"\"", true},
      {"class C { string s = \"\"\"\u00a0\n\u00a0a\n\u00a0\"\"\"; }", "\"\"\"\u00a0\n\u00a0a\n\u00a0\"\"\"", true},
      {"class C { string s = \"\"\"\ufeff\n\ufeffa\n\ufeff\"\"\"; }", "\"\"\"\ufeff\n\ufeffa\n\ufeff\"\"\"", true},
      {"class C { string s = \"\"\"\u2003\n\u2003a\n\u2003\"\"\"; }", "\"\"\"\u2003\n\u2003a\n\u2003\"\"\"", true},
      {"class C { string s = \"\"\"\u001a\n\u001aa\n\u001a\"\"\"; }", "\"\"\"\u001a\n\u001aa\n\u001a\"\"\"", true},
  }) {
    const std::string source = test.source;
    const auto parsed = language.parser->parse(source);
    CHECK(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty());
    const auto validation = cursiv::test::validationHelper(language, source);
    if (validation.diagnostics.empty() != test.valid) std::fprintf(stderr, "raw layout: %s\n", test.source);
    CHECK(validation.diagnostics.empty() == test.valid);
    if (!test.valid) {
      CHECK(validation.diagnostics.size() == 1);
      if (validation.diagnostics.size() == 1) {
        const auto& d = validation.diagnostics[0];
        CHECK(d.code && d.code->isString() && d.code->string() == "csharp.raw-layout");
        CHECK(d.range.begin == source.find(test.literal) &&
              d.range.end == d.range.begin + std::string_view(test.literal).size());
      }
    }
    std::size_t literals = 0;
    for (const auto* node : parsed.nodes)
      if (const auto* literal = cursiv::ast_cast<csharp::ast::LiteralExpr>(node)) {
        ++literals;
        CHECK(literal->text == test.literal);
      }
    CHECK(literals == 1);
    std::unordered_set<const cursiv::AstNode*> visited;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      CHECK(visited.insert(&node).second);
      language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        CHECK(child.parent == &node);
        CHECK(parsed.cst().contains(node.cstNodeId, child.cstNodeId));
        if (!visited.contains(&child)) self(self, child);
      });
    };
    CHECK(parsed.value && !parsed.value->parent);
    if (parsed.value) visit(visit, *parsed.value);
    CHECK(visited.size() == parsed.nodes.size());
  }
}
}  // namespace csharp_raw_layout
