#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>

#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "sparql/core/CodepointEscapes.hpp"

namespace sparql_uchar_tests {
namespace ast = sparql::ast;

template <class Services>
bool run(const Services& services) {
  struct Positive { const char* id; const char* source; const char* decoded; };
  const Positive positives[] = {
      {"keyword-select", "SEL\\u0045CT * WHERE {}", "SELECT * WHERE {}"},
      {"keyword-ask-braces", "\\u0041SK \\u007B\\u007D", "ASK {}"},
      {"encoded-space", "SELECT\\u0020*\\u0020WHERE{}", "SELECT * WHERE{}"},
      {"comment-opener", "\\u0023 note\nASK{}", "# note\nASK{}"},
      {"comment-newline", "# note\\u000AASK{}", "# note\nASK{}"},
      {"prefix-colon", "PREFIX ex\\u003A <urn:e:> ASK { ex\\u003As ex:p ex:o }", "PREFIX ex: <urn:e:> ASK { ex:s ex:p ex:o }"},
      {"variable-u4", "SELECT ?\\u0061 WHERE {}", "SELECT ?a WHERE {}"},
      {"variable-U8", "SELECT ?\\U00000061 WHERE {}", "SELECT ?a WHERE {}"},
      {"fixed-hex-width", "SELECT ?\\u00611 WHERE {}", "SELECT ?a1 WHERE {}"},
      {"rdf-type-keyword", "ASK { <urn:s> \\u0061 <urn:C> }", "ASK { <urn:s> a <urn:C> }"},
      {"language-tag", "ASK { <urn:s> <urn:p> \"x\"@\\u0065n }", "ASK { <urn:s> <urn:p> \"x\"@en }"},
      {"IRI-codepoint", "ASK { <urn:\\u03BB> <urn:p> <urn:o> }", "ASK { <urn:λ> <urn:p> <urn:o> }"},
      {"string-codepoints", "ASK { <urn:s> <urn:p> \"A\\u0042\\U0001F600\" }", "ASK { <urn:s> <urn:p> \"AB😀\" }"},
      {"plain-ECHAR", "ASK { <urn:s> <urn:p> \"A\\nB\" }", "ASK { <urn:s> <urn:p> \"A\\nB\" }"},
      {"substitution-before-ECHAR", "ASK { <urn:s> <urn:p> \"\\u005Cn\" }", "ASK { <urn:s> <urn:p> \"\\n\" }"},
      {"plain-escaped-backslash", "ASK { <urn:s> <urn:p> \"a\\\\b\" }", "ASK { <urn:s> <urn:p> \"a\\\\b\" }"},
      {"supplementary-bound", "ASK { <urn:s> <urn:p> \"\\U0010FFFF\" }", "ASK { <urn:s> <urn:p> \"􏿿\" }"},
  };
  for (const auto& c : positives) {
    const std::string source(c.source);
    const auto parsed = services.parser->parse(source);
    const auto* query = parsed.template value_as<ast::Query>();
    const auto* translation = parsed.cst().translation();
    bool okay = parsed.accepted && !parsed.cst().recovered() &&
                parsed.diagnostics.empty() && query &&
                parsed.cst().text() == source &&
                (translation ? translation->text() == c.decoded
                             : source == c.decoded);
    if (query) {
      okay &= !query->parent;
      std::unordered_set<const cursiv::AstNode*> seen;
      const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
        if (!seen.insert(&node).second || node.cstNodeId >= parsed.cst().size()) {
          okay = false;
          return;
        }
        const auto& span = parsed.cst()[node.cstNodeId];
        okay &= span.begin <= span.end && span.end <= source.size();
        services.astReflection->visitMembers(
            node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
              okay &= child.parent == &node &&
                      parsed.cst().contains(node.cstNodeId, child.cstNodeId);
              self(self, child);
            });
      };
      walk(walk, *query);
      okay &= seen.size() == parsed.nodes.size();
    }
    if (!okay) {
      std::fprintf(stderr, "sparql UCHAR: %s: positive parse/translation/ownership failed\n%s\n",
                   c.id, c.source);
      return false;
    }
  }
  struct Negative { const char* id; const char* source; };
  const Negative negatives[] = {
      {"malformed-keyword", "SEL\\u004GCT * WHERE {}"},
      {"variable-encoded-space", "SELECT ?x\\u0020y WHERE {}"},
      {"variable-encoded-hyphen", "SELECT ?a\\u002Db WHERE {}"},
      {"keyword-boundary", "SEL\\u0045CTx * WHERE {}"},
      {"short-string-newline", "ASK { <urn:s> <urn:p> \"a\\u000Ab\" }"},
      {"IRI-space", "ASK { <urn:a\\u0020b> <urn:p> <urn:o> }"},
      {"short-string-quote", "ASK { <urn:s> <urn:p> \"a\\u0022b\" }"},
      {"IRI-NUL", "ASK { <urn:a\\u0000b> <urn:p> <urn:o> }"},
      {"out-of-range", "ASK { <urn:s> <urn:p> \"\\U00110000\" }"},
      {"surrogate", "ASK { <urn:s> <urn:p> \"\\uD800\" }"},
  };
  for (const auto& c : negatives) {
    const auto parsed = services.parser->parse(c.source);
    if (parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty()) {
      std::fprintf(stderr, "sparql UCHAR: %s: invalid form accepted\n%s\n", c.id, c.source);
      return false;
    }
    if (std::string_view(c.id) == "out-of-range" || std::string_view(c.id) == "surrogate") {
      const std::string_view source(c.source);
      const auto at = source.find('\\');
      const auto width = source[at + 1] == 'u' ? 6u : 10u;
      bool mapped = false;
      for (const auto& d : parsed.diagnostics)
        mapped |= d.kind == cursiv::DiagnosticKind::conversion_error &&
                  d.begin == at && d.end == at + width;
      if (!mapped) {
        std::fprintf(stderr, "sparql UCHAR: %s: scalar diagnostic lost source range\n", c.id);
        return false;
      }
    }
  }
  {
    const std::string source = R"rq(SEL\u0045CT ?\u0061 WHERE {})rq";
    const auto parsed = services.parser->parse(source);
    const auto* query = parsed.template value_as<ast::Query>();
    const auto* form = query ? std::get_if<ast::SelectQuery*>(&query->form) : nullptr;
    const auto* var = form && *form && (*form)->vars.size() == 1
        ? cursiv::ast_cast<ast::Var>((*form)->vars[0]) : nullptr;
    if (!var || var->name != "a" || var->parent != *form ||
        parsed.cst().text(var->cstNodeId) != R"rq(?\u0061)rq") return false;
    bool keyword_range = false;
    parsed.cst().visit_leaves([&](const cursiv::CstLeaf& leaf) {
      if (leaf.begin == 0 && leaf.end == std::string_view(R"rq(SEL\u0045CT)rq").size())
        keyword_range = true;
    });
    if (!keyword_range) return false;
  }
  for (const auto& sample : {std::pair{R"rq(ASK { <urn:s> <urn:p> "A\u0042\U0001F600" })rq",
                                      std::string_view("AB😀")},
                             std::pair{R"rq(ASK { <urn:s> <urn:p> "\u005Cn" })rq",
                                      std::string_view("\n")}}) {
    const auto parsed = services.parser->parse(sample.first);
    const auto* query = parsed.template value_as<ast::Query>();
    const auto* form = query ? std::get_if<ast::AskQuery*>(&query->form) : nullptr;
    const ast::Triple* triple = form && *form
        ? audit_regressions::first_triple((*form)->where) : nullptr;
    const auto* property = triple && triple->properties && triple->properties->items.size() == 1
        ? triple->properties->items[0] : nullptr;
    const auto* value = property && property->objects.size() == 1
        ? cursiv::ast_cast<ast::StringLit>(property->objects[0]) : nullptr;
    if (!value || value->value != sample.second || value->parent != property) return false;
  }
  return true;
}
}  // namespace sparql_uchar_tests
