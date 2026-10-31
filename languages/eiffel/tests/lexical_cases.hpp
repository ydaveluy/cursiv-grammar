#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "eiffel/core/ast.hpp"

namespace eiffel_lexical {
inline bool fail(const char* message, std::string_view source) {
  std::fprintf(stderr, "eiffel lexical: %s\n%.*s\n", message,
               static_cast<int>(source.size()), source.data());
  return false;
}
inline bool parents(const cursiv::AstReflection& reflection,
                    const cursiv::ParseResult& parsed) {
  if (!parsed.value || parsed.value->parent) return false;
  bool okay = true;
  std::unordered_set<const cursiv::AstNode*> seen;
  const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { okay = false; return; }
    reflection.visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          okay &= child.parent == &node && parsed.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  visit(visit, *parsed.value);
  return okay && seen.size() == parsed.nodes.size();
}
inline bool run(const cursiv::Parser& parser, const cursiv::AstReflection& reflection) {
  struct Positive { const char* literal; const char* type; };
  const Positive valid[] = {
      {"1__234", "INTEGER"}, {"0b10__01", "INTEGER"}, {"0c7_7", "INTEGER"},
      {"0xA__F", "INTEGER"}, {"123456789012345678901234567890", "INTEGER"},
      {"1_000.2_5E+1_0", "REAL"}, {".5_0", "REAL"}, {"1_0.", "REAL"},
      {"1.E-2", "REAL"}, {"0.0", "REAL"},
      {"'%/0x3B/'", "CHARACTER"}, {"'%/0b111/'", "CHARACTER"},
      {"'%/0c77/'", "CHARACTER"}, {"'%N'", "CHARACTER"}, {"'%%'", "CHARACTER"},
      {R"e("a%Nb%/0x3B/%%")e", "STRING"}, {R"e("a%"b")e", "STRING"},
      {"\"a% \r\n %b\"", "STRING"}, {"\"a% \r %b\"", "STRING"},
      {R"e("x[{[
text ]x[{" occurs inside a line
]x[{")e", "STRING"},
      {R"e("tag{part{
body
}tag{part")e", "STRING"},
      {"\"tag[\nbody\n\t]tag\"", "STRING"},
      {"\"[\n]\"", "STRING"}, {"\"{\r\nbody\r\n}\"", "STRING"}};
  for (const auto& test : valid) {
    const std::string source = std::string("class C feature value: ") + test.type +
                               " = " + test.literal + " end";
    const auto parsed = parser.parse(source);
    const auto* type = cursiv::ast_cast<eiffel::ast::ClassDecl>(parsed.value);
    const auto* declaration = type && type->features.size() == 1 &&
                                     type->features[0]->features.size() == 1
        ? type->features[0]->features[0] : nullptr;
    if (!parsed.accepted || parsed.cst().recovered() || !parsed.diagnostics.empty() ||
        !declaration || !declaration->value || declaration->value->explicitValue != test.literal ||
        !parents(reflection, parsed)) return fail("native literal spelling/ownership", source);
  }
  for (const char* literal : {"0b2", "0c8", "0xG", "12AF", "123_", "0x_AF", "1e2",
                              "1_.0", "1.0_", "1.0e_2", "1.0e2_", "'%?'", "'%Z'",
                              "'%/0b2/'", "'%/0x_F/'", "\"bad%?\"", "\"bad%Z\"",
                              "\"raw\nline\"", "\"tag[\nbody ]tag\"",
                              "\"tag[\nbody\n]other\"", "\"tag{\nbody\n]tag\"",
                              "\"tag name[\nbody\n]tag name\""}) {
    const std::string source = std::string("class C feature value: STRING = ") + literal + " end";
    const auto parsed = parser.parse(source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty())
      return fail("invalid lexical form accepted clean", source);
  }
  return true;
}
}  // namespace eiffel_lexical
