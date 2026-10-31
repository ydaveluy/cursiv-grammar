#pragma once
#include <cstdio>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "less/core/CoreServices.hpp"
#include "less/core/LessGrammar.hpp"

namespace less_selector_order_cases {
struct Case { const char* name; std::string_view source; std::vector<std::string_view> labels; std::vector<std::string_view> combinators; bool guard; };
inline const Case cases[] = {
  {"attribute-pseudo-attribute", "a[x]:hover[y]{}", {"a", "[x]", ":hover", "[y]"}, {"", "", "", ""}, false},
  {"pseudo-two-attributes", "a:hover[x][y]{}", {"a", ":hover", "[x]", "[y]"}, {"", "", "", ""}, false},
  {"attribute-name-attribute", "[x]a[y]{}", {"[x]", "a", "[y]"}, {"", "", ""}, false},
  {"two-attributes", "[x][y]{}", {"[x]", "[y]"}, {"", ""}, false},
  {"child-attribute", "a > [x]{}", {"a", "[x]"}, {"", ">"}, false},
  {"sibling-attribute", "a + [x]{}", {"a", "[x]"}, {"", "+"}, false},
  {"general-sibling-attribute", "a ~ [x]{}", {"a", "[x]"}, {"", "~"}, false},
  {"attribute-child-name", "[x] > a{}", {"[x]", "a"}, {"", ">"}, false},
  {"space-attribute", "a [x]{}", {"a", "[x]"}, {"", " "}, false},
  {"comment-attribute", "a/**/[x]{}", {"a", "[x]"}, {"", ""}, false},
  {"spaced-comment-attribute", "a /*x*/ [x]:focus{}", {"a", "[x]", ":focus"}, {"", " ", ""}, false},
  {"guarded-order", "a[x]:hover when (1=1) {}", {"a", "[x]", ":hover"}, {"", "", ""}, true},
  {"child-id", "a > #x{}", {"a", "#x"}, {"", ">"}, false},
  {"child-pseudo", "a > :hover{}", {"a", ":hover"}, {"", ">"}, false},
  {"child-wildcard", "a > *{}", {"a", "*"}, {"", ">"}, false},
  {"child-parent", "a > &{}", {"a", "&"}, {"", ">"}, false},
  {"space-pseudo", "a :hover{}", {"a", ":hover"}, {"", " "}, false},
  {"comment-before-child", "a/**/>[x]{}", {"a", "[x]"}, {"", ">"}, false},
};
inline std::string name(const less::ast::Name* value) {
  std::string text;
  if (value) for (const auto& part : value->parts)
    if (const auto* raw = std::get_if<std::string_view>(&part)) text += *raw;
  return text;
}
inline std::string label(const less::ast::Element* element) {
  namespace a = less::ast;
  if (const auto* v = cursiv::ast_cast<a::Attrib>(element)) return "[" + std::string(v->name) + "]";
  if (const auto* v = cursiv::ast_cast<a::NamedElement>(element)) return name(v->name);
  if (const auto* v = cursiv::ast_cast<a::IdElement>(element)) return "#" + name(v->name);
  if (const auto* v = cursiv::ast_cast<a::Pseudo>(element)) return (v->isClass ? "::" : ":") + std::string(v->name);
  if (cursiv::ast_cast<a::ParentRefElement>(element)) return "&";
  if (cursiv::ast_cast<a::WildcardElement>(element)) return "*";
  return "<unexpected component>";
}
inline std::string_view combinator(const less::ast::Element& element) {
  if (element.prefix) switch (*element.prefix) {
    case less::ast::SelectorPrefix::child: return ">";
    case less::ast::SelectorPrefix::sibling: return "+";
    case less::ast::SelectorPrefix::general_sibling: return "~";
  }
  return element.descendant ? " " : "";
}
inline bool run(const less::LessCoreServices& services) {
  namespace a = less::ast;
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* what) {
    if (!ok) { std::fprintf(stderr, "less selector %s: %s\n", name, what); ++failures; }
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const auto* sheet = parsed.value_as<a::Stylesheet>();
    const auto* rule = sheet && sheet->statements.size() == 1 ? cursiv::ast_cast<a::Ruleset>(sheet->statements[0]) : nullptr;
    const auto* selector = rule && rule->selectors && rule->selectors->items.size() == 1 ? rule->selectors->items[0] : nullptr;
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && sheet;
    check(clean && selector && selector->elements.size() == test.labels.size(), test.name, "one complete ordered selector");
    if (!clean || !selector) continue;
    check(parsed.cst().text() == test.source, test.name, "complete source preserved");
    check((selector->guard != nullptr) == test.guard, test.name, "guard presence preserved");
    if (selector->elements.size() == test.labels.size())
      for (std::size_t i = 0; i < test.labels.size(); ++i) {
        const auto* element = selector->elements[i];
        check(label(element) == test.labels[i], test.name, "all attributes/pseudos/names retain source order");
        check(element && combinator(*element) == test.combinators[i] &&
              !(element->prefix && element->descendant), test.name, "exclusive native signed/descendant combinator");
        check(element && element->parent == selector, test.name, "component has one immediate selector owner");
      }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique ownership"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "every parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "every allocated node reachable once");
  }
  std::pmr::monotonic_buffer_resource memory;
  for (std::string_view bad : {"", "a", "'", "'a", "a'", "'a\""}) {
    const auto converted = less::unquote(bad, &memory);
    check(!converted && !converted.error.message.empty(), "unquote-totality", "malformed fragment returns error");
  }
  for (const auto& pair : {
      std::pair<std::string_view, std::string_view>{"\"\"", ""},
      {"\"a\r\nb\"", "a\nb"}, {R"less('a\'b')less", R"less(a\'b)less"}}) {
    const auto converted = less::unquote(pair.first, &memory);
    check(converted && *converted.value == pair.second, "unquote-totality", "raw escapes and native line normalization preserved");
  }
  const std::string lineSource = "@x: \"a\r\nb\";";
  check(lineSource.find('\r') != std::string::npos, "quoted-CRLF", "runtime source includes physical CR");
  const auto lineResult = services.parser->parse(lineSource);
  const a::StringLit* value = nullptr;
  for (const auto* node : lineResult.nodes) if (const auto* v = cursiv::ast_cast<a::StringLit>(node)) value = v;
  check(lineResult.accepted && !lineResult.cst().recovered() && lineResult.diagnostics.empty() &&
        value && value->value == "a\nb", "quoted-CRLF", "parsed value normalizes CRLF with source retained");
  return !failures;
}
} // namespace less_selector_order_cases
