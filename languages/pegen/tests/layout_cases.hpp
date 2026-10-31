#pragma once
#include <cstdio>
#include <initializer_list>
#include <string>
#include <unordered_set>
#include <vector>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/test/Check.hpp"
#include "pegen/core/ast.hpp"
namespace pegen_layout {
inline void run(const auto& language) {
  struct Case { const char* source; bool valid; std::vector<std::size_t> alternatives; };
  for (const Case& test : std::initializer_list<Case>{
      {"start: NAME\n", true, {1}},
      {"start: NAME", true, {1}},
      {"start:\n    | NAME\n", true, {1}},
      {"start: NAME\n    | STRING\n", true, {2}},
      {"start: NAME | STRING\n", true, {2}},
      {"start: NAME\n | STRING\n", true, {2}},
      {"start:\n| NAME\n", false, {}},
      {"start: | NAME\n", false, {}},
      {"start: NAME\n    | STRING\n        | NUMBER\n", false, {}},
      {"start: NAME\n    | STRING\n  | NUMBER\n", false, {}},
      {"start: NAME\n    STRING\n", false, {}},
      {"start: NAME\n| STRING\n", false, {}},
      {"start:\n    NAME\n", false, {}},
      {"  start: NAME\n", false, {}},
      {"@header \"hi\" start: NAME\n", false, {}},
      {"@header\nstart: NAME\n", true, {1}},
      {"@header \"hi\"\nstart: NAME\n", true, {1}},
      {"start: (NAME |\nSTRING)\n", true, {1}},
      {"start: [NAME |\n        STRING]\n", true, {1}},
      {"start: NAME \\\n| STRING\n", true, {2}},
      {"start:\n # comment\nnext: NAME\n", false, {}},
      {"# only comment\n", false, {}},
      {"\n# comment\nstart: NAME\n\n", true, {1}},
      {"start:\n    | NAME # comment\n\n    # blank\n    | STRING\nnext: NUMBER\n", true, {2, 1}},
      {"start:\n\t| NAME\n\t| STRING\n", true, {2}},
      {"start: NAME\r\nnext: STRING\r\n", true, {1, 1}},
      {"start:\n    | NAME", true, {1}},
      {"start: NAME\nnext: STRING", true, {1, 1}},
      {"start: NAME {\n   nested(thing)\n}\n", true, {1}},
      {"start[\n    Token *\n]: NAME\n", true, {1}},
  }) {
    const std::string source = test.source;
    const auto result = language.parser->parse(source);
    const bool valid = result.accepted && !result.cst().recovered() && result.diagnostics.empty();
    if (valid != test.valid) std::fprintf(stderr, "pegen layout: %s\n", test.source);
    CHECK(valid == test.valid);
    if (!valid || !test.valid) continue;
    const auto* grammar = result.template value_as<pegen::ast::Grammar>();
    CHECK(grammar && grammar->rules.size() == test.alternatives.size());
    if (!grammar || grammar->rules.size() != test.alternatives.size()) continue;
    for (std::size_t i = 0; i < grammar->rules.size(); ++i) {
      const auto* rule = grammar->rules[i];
      CHECK(rule && rule->body && rule->body->alts.size() == test.alternatives[i]);
    }
    std::unordered_set<const cursiv::AstNode*> visited;
    const auto visit = [&](auto&& self, const cursiv::AstNode& node) -> void {
      CHECK(visited.insert(&node).second);
      language.astReflection->visitMembers(node, [&](std::string_view, std::uint32_t, cursiv::AstNode& child) {
        CHECK(child.parent == &node);
        CHECK(result.cst().contains(node.cstNodeId, child.cstNodeId));
        if (!visited.contains(&child)) self(self, child);
      });
    };
    CHECK(!grammar->parent);
    visit(visit, *grammar);
    CHECK(visited.size() == result.nodes.size());
  }
}
}  // namespace pegen_layout
