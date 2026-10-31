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
#include "moo/core/CoreServices.hpp"
#include "moo/core/MooGrammar.hpp"

namespace moo_conditional_fork_cases {
struct Case { const char* name; std::string_view source; bool valid; unsigned conditionals; unsigned forks; };
inline constexpr Case cases[] = {
  {"conditional-basic", R"moo(@program obj:run
return 1 ? 2 | 3;
.)moo", true, 1, 0},
  {"conditional-precedence", R"moo(@program obj:run
return 1 || 0 ? 2+3 | 4*5;
.)moo", true, 1, 0},
  {"conditional-in-condition", R"moo(@program obj:run
return (1?2|3)?4|5;
.)moo", true, 2, 0},
  {"conditional-in-consequent", R"moo(@program obj:run
return 1?(2?3|4)|5;
.)moo", true, 2, 0},
  {"conditional-in-alternate", R"moo(@program obj:run
return 1?2|(3?4|5);
.)moo", true, 2, 0},
  {"conditional-string", R"moo(@program obj:run
return 0 ? "a?x|y" | "z";
.)moo", true, 1, 0},
  {"conditional-list", R"moo(@program obj:run
return {1?2|3,4};
.)moo", true, 1, 0},
  {"conditional-assignment", R"moo(@program obj:run
x=1?2|3;
.)moo", true, 1, 0},
  {"conditional-call", R"moo(@program obj:run
notify(player,1?2|3);
.)moo", true, 1, 0},
  {"conditional-if", R"moo(@program obj:run
if (1?2|3) return 4; endif
.)moo", true, 1, 0},
  {"fork-empty", R"moo(@program obj:run
fork (0) endfork
.)moo", true, 0, 1},
  {"fork-named", R"moo(@program obj:run
fork task (5) return task; endfork
.)moo", true, 0, 1},
  {"fork-case", R"moo(@program obj:run
FORK Task (1) RETURN 2; ENDFORK
.)moo", true, 0, 1},
  {"fork-binary-delay", R"moo(@program obj:run
fork task (1+2) return 1?2|3; endfork
.)moo", true, 1, 1},
  {"fork-nested", R"moo(@program obj:run
fork outer (0) fork inner (1) return inner; endfork return outer; endfork
.)moo", true, 0, 2},
  {"fork-conditional-delay", R"moo(@program obj:run
fork (1?2|3) endfork
.)moo", true, 1, 1},
  {"fork-in-if", R"moo(@program obj:run
if (1) fork (0) return 2; endfork endif
.)moo", true, 0, 1},
  {"fork-siblings", R"moo(@program obj:run
fork a (0) endfork fork b (1) endfork
.)moo", true, 0, 2},
  {"conditional-chain-left", R"moo(@program obj:run
return 1?2|3?4|5;
.)moo", false, 0, 0},
  {"conditional-chain-middle", R"moo(@program obj:run
return 1?2?3|4|5;
.)moo", false, 0, 0},
  {"conditional-missing-consequent", R"moo(@program obj:run
return 1?|3;
.)moo", false, 0, 0},
  {"conditional-missing-alternate", R"moo(@program obj:run
return 1?2|;
.)moo", false, 0, 0},
  {"conditional-wrong-colon", R"moo(@program obj:run
return 1?2:3;
.)moo", false, 0, 0},
  {"fork-missing-open", R"moo(@program obj:run
fork task 1) endfork
.)moo", false, 0, 0},
  {"fork-empty-delay", R"moo(@program obj:run
fork () endfork
.)moo", false, 0, 0},
  {"fork-missing-close", R"moo(@program obj:run
fork (1 endfork
.)moo", false, 0, 0},
  {"fork-missing-end", R"moo(@program obj:run
fork (0) return 1;
.)moo", false, 0, 0},
  {"fork-reserved-name", R"moo(@program obj:run
fork if (1) endfork
.)moo", false, 0, 0},
};
inline bool run(const moo::MooCoreServices& services) {
  namespace a = moo::ast;
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* what) {
    if (!ok) { std::fprintf(stderr, "moo %s: %s\n", name, what); ++failures; }
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.valid, test.name, "native syntax verdict");
    if (!clean || !test.valid) continue;
    check(parsed.cst().text() == test.source, test.name, "full source retained");
    unsigned conditionals = 0, forks = 0;
    const a::ConditionalExpr* conditional = nullptr;
    const a::ForkStmt* fork = nullptr;
    for (const auto* node : parsed.nodes) {
      if (const auto* value = cursiv::ast_cast<a::ConditionalExpr>(node)) {
        ++conditionals; conditional = value;
        check(value->condition && value->consequent && value->alternate &&
              value->condition->parent == value && value->consequent->parent == value &&
              value->alternate->parent == value, test.name, "all three conditional roles owned");
      }
      if (const auto* value = cursiv::ast_cast<a::ForkStmt>(node)) {
        ++forks; fork = value;
        check(value->delay && value->delay->parent == value, test.name, "fork owns its delay");
      }
    }
    check(conditionals == test.conditionals && forks == test.forks, test.name, "typed node counts");
    const std::string_view name = test.name;
    if (name == "conditional-basic" && conditional) {
      const auto* c = cursiv::ast_cast<a::IntLit>(conditional->condition);
      const auto* t = cursiv::ast_cast<a::IntLit>(conditional->consequent);
      const auto* f = cursiv::ast_cast<a::IntLit>(conditional->alternate);
      check(c && t && f && c->value == 1 && t->value == 2 && f->value == 3,
            test.name, "condition/consequent/alternate values do not swap");
    }
    if (name == "conditional-precedence" && conditional) {
      const auto* c = cursiv::ast_cast<a::BinaryExpr>(conditional->condition);
      const auto* t = cursiv::ast_cast<a::BinaryExpr>(conditional->consequent);
      const auto* f = cursiv::ast_cast<a::BinaryExpr>(conditional->alternate);
      check(c && t && f && c->op == a::Operator::logical_or && t->op == a::Operator::add &&
            f->op == a::Operator::multiply, test.name, "binary operators bind inside all conditional roles");
    }
    const auto* root = parsed.value_as<a::Prog>();
    const auto* program = root && root->declarations.size() == 1 ?
        cursiv::ast_cast<a::ProgramDecl>(root->declarations[0]) : nullptr;
    if (name == "fork-empty" && fork)
      check(fork->name.empty() && fork->body.empty(), test.name, "unnamed empty task distinct");
    if (name == "fork-named" && fork)
      check(fork->name == "task" && fork->body.size() == 1, test.name, "task identifier and body retained");
    if (name == "fork-case" && fork)
      check(fork->name == "Task", test.name, "identifier source case retained");
    if (name == "fork-nested") {
      const auto* outer = program && program->body.size() == 1 ? cursiv::ast_cast<a::ForkStmt>(program->body[0]) : nullptr;
      const auto* inner = outer && outer->body.size() == 2 ? cursiv::ast_cast<a::ForkStmt>(outer->body[0]) : nullptr;
      check(outer && inner && outer->name == "outer" && inner->name == "inner" &&
            inner->parent == outer && inner->body.size() == 1, test.name, "nested task order and owners retained");
    }
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique AST owner"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "member parent and CST containment");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "complete tree reachability");
  }
  // Total conversion is a direct contract, without claiming that recovery
  // ever supplies one of these malformed fragments to the converter.
  std::pmr::monotonic_buffer_resource memory;
  for (std::string_view malformed : {"", "a", "\"", "\"a", "a\"", "'a'"}) {
    const auto converted = moo::strip_quotes(malformed, &memory);
    check(!converted && !converted.error.message.empty(), "quote-totality", "malformed fragment returns an error");
  }
  for (const auto& pair : {
      std::pair<std::string_view, std::string_view>{R"moo("")moo", ""},
      {R"moo("x\ny")moo", "xny"}, {R"moo("a\"b")moo", "a\"b"}}) {
    const auto converted = moo::strip_quotes(pair.first, &memory);
    check(converted && *converted.value == pair.second, "quote-totality", "native backslash erasure retained");
  }
  return !failures;
}
} // namespace moo_conditional_fork_cases
