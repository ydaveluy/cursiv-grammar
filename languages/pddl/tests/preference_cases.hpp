#pragma once
#include <algorithm>
#include <cstdio>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "pddl/core/CoreServices.hpp"
#include "pddl/core/ast.hpp"

namespace pddl_preference_cases {
struct Case { const char* name; std::string_view source; bool valid; std::vector<std::string_view> names; };
inline const Case cases[] = {
  {"action-named", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference stay (p)) :effect (p))))pddl", true, {R"pddl(stay)pddl"}},
  {"action-unnamed", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference (p)) :effect (p))))pddl", true, {R"pddl()pddl"}},
  {"empty-preference-body", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference empty (and)) :effect (p))))pddl", true, {R"pddl(empty)pddl"}},
  {"and-plain-preferences", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (and (p) (preference a (q)) (preference (not (p)))) :effect (p))))pddl", true, {R"pddl(a)pddl", R"pddl()pddl"}},
  {"forall-preference", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (forall (?x) (preference all (p))) :effect (p))))pddl", true, {R"pddl(all)pddl"}},
  {"forall-and-nested", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (forall (?x) (and (preference first (p)) (forall (?y) (preference second (q))))) :effect (p))))pddl", true, {R"pddl(first)pddl", R"pddl(second)pddl"}},
  {"ordinary-gd-in-preference", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference complex (and (or (p) (q)) (exists (?x) (not (p))))) :effect (p))))pddl", true, {R"pddl(complex)pddl"}},
  {"mixedcase", R"pddl((DEFINE (DOMAIN D) (:REQUIREMENTS :STRIPS :PREFERENCES :UNIVERSAL-PRECONDITIONS) (:PREDICATES (P) (Q)) (:ACTION A :PARAMETERS () :PRECONDITION (PREFERENCE Mixed-Name (AND)) :EFFECT (P))))pddl", true, {R"pddl(Mixed-Name)pddl"}},
  {"problem-named", R"pddl((define (problem x) (:domain d) (:requirements :preferences) (:init (p)) (:goal (preference goal (p)))))pddl", true, {R"pddl(goal)pddl"}},
  {"problem-multiple", R"pddl((define (problem x) (:domain d) (:requirements :preferences) (:init (p)) (:goal (and (preference a (p)) (preference b (q)))) (:metric minimize (+ (is-violated a) (is-violated b)))))pddl", true, {R"pddl(a)pddl", R"pddl(b)pddl"}},
  {"classical-control", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (and (p) (forall (?x) (q))) :effect (p))))pddl", true, {}},
  {"no-under-or", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (or (preference a (p)) (q)) :effect (p))))pddl", false, {}},
  {"no-under-not", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (not (preference a (p))) :effect (p))))pddl", false, {}},
  {"no-under-exists", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (exists (?x) (preference a (p))) :effect (p))))pddl", false, {}},
  {"no-under-imply", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (imply (p) (preference a (q))) :effect (p))))pddl", false, {}},
  {"no-nested-preference", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference a (preference b (p))) :effect (p))))pddl", false, {}},
  {"extra-preference-body", R"pddl((define (domain d) (:requirements :strips :preferences :universal-preconditions) (:predicates (p) (q)) (:action a :parameters () :precondition (preference a (p) (q)) :effect (p))))pddl", false, {}},
  {"derived-gd-no-preference", R"pddl((define (domain d) (:requirements :preferences :derived-predicates) (:predicates (p)) (:derived (p) (preference a (p)))))pddl", false, {}},
  {"when-gd-no-preference", R"pddl((define (domain d) (:requirements :preferences :conditional-effects) (:predicates (p)) (:action a :parameters () :effect (when (preference a (p)) (p)))))pddl", false, {}},
  {"effect-no-preference", R"pddl((define (domain d) (:requirements :preferences) (:predicates (p)) (:action a :parameters () :effect (preference a (p)))))pddl", false, {}},
};
inline bool run(const pddl::PddlCoreServices& services) {
  unsigned failures = 0;
  auto check = [&](bool ok, const char* name, const char* what) {
    if (!ok) { std::fprintf(stderr, "pddl preferences %s: %s\n", name, what); ++failures; }
  };
  for (const auto& test : cases) {
    const auto parsed = services.parser->parse(test.source);
    const bool clean = parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() && parsed.value;
    check(clean == test.valid, test.name, "author pre-GD context verdict");
    if (!clean || !test.valid) continue;
    check(parsed.cst().text() == test.source, test.name, "source retained exactly");
    std::vector<std::string_view> names;
    for (const auto* node : parsed.nodes)
      if (const auto* goal = cursiv::ast_cast<pddl::ast::GoalFormula>(node)) {
        if (goal->isPreference) {
          names.push_back(goal->preferenceName);
          check(goal->preferenceBody && goal->preferenceBody->parent == goal &&
                !goal->atom && goal->operands.empty() && !goal->quantBody,
                test.name, "preference keeps its own GD role and name");
          if (std::string_view(test.name) == "empty-preference-body")
            check(goal->preferenceBody && !goal->preferenceBody->atom &&
                  goal->preferenceBody->operands.empty() && !goal->preferenceBody->isOr &&
                  !goal->preferenceBody->isPreference, test.name, "empty and is a real ordinary GD node");
        } else {
          check(goal->preferenceName.empty() && !goal->preferenceBody,
                test.name, "ordinary goals do not acquire preference fields");
        }
      }
    auto expected = test.names;
    std::sort(names.begin(), names.end()); std::sort(expected.begin(), expected.end());
    check(names == expected, test.name, "all named and unnamed preferences retained");
    if (std::string_view(test.name) == "and-plain-preferences")
      for (const auto* node : parsed.nodes)
        if (const auto* action = cursiv::ast_cast<pddl::ast::ActionDef>(node))
          check(action->hasPrecondition && action->precondition &&
                action->precondition->operands.size() == 3 &&
                action->precondition->operands[0]->atom &&
                action->precondition->operands[1]->isPreference &&
                action->precondition->operands[1]->preferenceName == "a" &&
                action->precondition->operands[2]->isPreference &&
                action->precondition->operands[2]->preferenceName.empty(),
                test.name, "plain and preference children preserve source order");
    if (std::string_view(test.name) == "forall-preference")
      for (const auto* node : parsed.nodes)
        if (const auto* action = cursiv::ast_cast<pddl::ast::ActionDef>(node))
          check(action->precondition && action->precondition->isForall &&
                action->precondition->parameters && action->precondition->quantBody &&
                action->precondition->quantBody->isPreference,
                test.name, "forall flag, binder and preference child retained");
    std::unordered_set<const cursiv::AstNode*> seen;
    std::vector<const cursiv::AstNode*> work = {parsed.value};
    while (!work.empty()) {
      const auto* node = work.back(); work.pop_back();
      if (!seen.insert(node).second) { check(false, test.name, "unique owner"); continue; }
      services.astReflection->visitMembers(*node, [&](auto, auto, const auto& child) {
        check(child.parent == node && node->cstNodeId < parsed.cst().size() &&
              child.cstNodeId < parsed.cst().size() && parsed.cst().contains(node->cstNodeId, child.cstNodeId),
              test.name, "all immediate parents and CST spans");
        work.push_back(&child);
      });
    }
    check(seen.size() == parsed.nodes.size() && !parsed.value->parent, test.name, "complete tree reachability");
  }
  return !failures;
}
} // namespace pddl_preference_cases
