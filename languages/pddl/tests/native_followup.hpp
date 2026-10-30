#pragma once
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

namespace pddl_followup {
inline bool run(const pddl::PddlCoreServices& services) {
  const auto source =
      "(DEFINE (DOMAIN D) (:REQUIREMENTS :STRIPS :NON-DETERMINISTIC) "
      "(:PREDICATES (P)) (:ACTION A :PARAMETERS () "
      ":PRECONDITION (AnD) :EFFECT (ONEOF (P))))";
  const auto result = services.parser->parse(source);
  const auto* doc = result.value_as<pddl::ast::Document>();
  const auto* domain = doc ? std::get_if<pddl::ast::Domain*>(&doc->content) : nullptr;
  const auto* action = domain && *domain && (*domain)->structures.size() == 1
      ? std::get_if<pddl::ast::ActionDef*>(&(*domain)->structures[0]) : nullptr;
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
      !domain || !*domain || (*domain)->name != "D" ||
      (*domain)->requirements.size() != 2 ||
      (*domain)->requirements[1] != ":NON-DETERMINISTIC" ||
      !action || !*action || !(*action)->hasPrecondition || !(*action)->precondition ||
      (*action)->precondition->atom || !(*action)->precondition->operands.empty() ||
      (*action)->precondition->isOr || !(*action)->effect ||
      !(*action)->effect->isOneOf || (*action)->effect->operands.size() != 1 ||
      (*action)->precondition->parent != *action)
    return false;
  const auto validation = cursiv::test::validationHelper(services, source);
  if (!validation.diagnostics.empty()) return false;
  const auto disjunction = services.parser->parse(
      "(DEFINE (PROBLEM P) (:DOMAIN D) (:INIT) (:GOAL (OR)))");
  const auto* problemDoc = disjunction.value_as<pddl::ast::Document>();
  const auto* problem = problemDoc
      ? std::get_if<pddl::ast::Problem*>(&problemDoc->content) : nullptr;
  return disjunction.accepted && !disjunction.cst().recovered() &&
      disjunction.diagnostics.empty() && problem && *problem && (*problem)->goal &&
      (*problem)->goal->isOr && !(*problem)->goal->atom &&
      (*problem)->goal->operands.empty() && (*problem)->goal->parent == *problem;
}
} // namespace pddl_followup
