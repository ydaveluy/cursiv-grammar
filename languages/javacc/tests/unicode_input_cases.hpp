#pragma once
#include <string_view>
#include <unordered_set>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "javacc/core/UnicodeEscapes.hpp"
#include "javacc/core/ast.hpp"
namespace javacc_native_followup {
template<class Language> bool unicode_input(const Language& language) {
  struct Case { std::string_view source, text; bool invalid = false; };
  // Expected UTF-16 units recorded from native JavaCC7 JavaCharStream,
  // represented as UTF-8 scalars, or preserved escapes for lone units.
  for (const auto& c : {
      Case{R"(\u0041)", "A"}, Case{R"(\uuuu0041)", "A"},
      Case{R"(\\u0041)", R"(\\u0041)"}, Case{R"(\\\u0041)", R"(\\A)"},
      Case{R"(\\\\u0041)", R"(\\\\u0041)"},
      Case{R"(\u005cu0041)", R"(\u0041)"},
      Case{R"(\u005c\u0041)", R"(\A)"},
      Case{R"(\u005c\\u0041)", R"(\\\u0041)"},
      Case{R"(\u000a)", "\n"}, Case{R"(\uD835\uDC9C)", "𝒜"},
      Case{R"(\uD800)", R"(\uD800)"}, Case{R"(\uDC00)", R"(\uDC00)"},
      Case{R"(\u12x4)", "", true}, Case{R"(\u1)", "", true}, Case{R"(\u٠٠٤١)", "", true}}) {
    cursiv::SourceTranslation out(c.source);
    javacc::translate_unicode_escapes(c.source, out);
    out.finish();
    if (c.invalid ? out.errors().size() != 1 : !out.errors().empty() || out.text() != c.text) return false;
    for (const auto& d : out.errors()) if (d.begin > d.end || d.end > c.source.size()) return false;
  }
  constexpr std::string_view source = R"jj(\u0050ARSER_BEGIN(P)
cl\u0061ss P { int \u0078=1; }
PARSER_END(P)
// comment\u000avoid S():{}{<EOF>}
)jj";
  const auto result = language.language.parser->parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !result.value ||
      result.cst().text() != source || !result.cst().engine_stamped()) return false;
  bool name = false;
  for (const auto* node : result.nodes)
    if (const auto* p = cursiv::ast_cast<javacc::ast::VariableDeclaratorId>(node))
      name |= p->name == "x" && result.cst().text(p->cstNodeId).find(R"(\u0078)") != std::string_view::npos;
  if (!name) return false;
  std::unordered_set<const cursiv::AstNode*> seen;
  bool owned = true;
  auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (!seen.insert(&node).second) { owned = false; return; }
    language.language.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
          owned &= child.parent == &node && result.cst().contains(node.cstNodeId, child.cstNodeId);
          self(self, child);
        });
  };
  walk(walk, *result.value);
  if (!owned || seen.size() != result.nodes.size()) return false;
  for (cursiv::NodeId id = 0; id < result.cst().size(); ++id) {
    const auto& n = result.cst()[id];
    if (n.begin > n.end || n.end > source.size() || (id && n.end < result.cst()[id - 1].end)) return false;
  }
  const auto malformed = language.language.parser->parse(R"(\u12x4PARSER_BEGIN(P) class P{} PARSER_END(P) void S():{}{<EOF>})");
  bool error = false;
  for (const auto& d : malformed.diagnostics)
    error |= d.message == "Malformed JavaCC Unicode escape" && d.begin == 0 && d.end == 5;
  return error;
}
} // namespace javacc_native_followup
