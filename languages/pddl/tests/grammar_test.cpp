
#include <cstdio>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "pddl/core/CoreServices.hpp"
#include "pddl/core/ast.hpp"
#include "native_regressions.hpp"
// The grammar's smoke test: every document under tests/ (the upstream
// corpus is empty) parses clean, and real field values read back --  a
// converter that silently gave the wrong value would still pass a
// structural-only check.

#include <fstream>
#include <iterator>
#include <string>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace {

namespace ast = pddl::ast;

int fail(const char* what) {
  std::fprintf(stderr, "pddl: %s\n", what);
  return 1;
}

std::string read(const char* path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

template <class T>
const T* as(const cursiv::AstNode* node) {
  if (node == nullptr || node->type != &cursiv::ast_type_tag<T>) return nullptr;
  return static_cast<const T*>(node);
}

}  // namespace

static int audit_regression() {
  const auto language = cursiv::create_language<pddl::PddlCoreServices,
                                                pddl::PddlSharedCoreServices,
                                                pddl::install_core_module>();
  {
    const auto r = language.language.parser->parse(
        "(define (domain d) (:predicates (p ?x) (q ?x)) (:derived (p ?x) (q "
        "?x)))");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<pddl::ast::Document>();
    const auto* domain =
        doc ? std::get_if<pddl::ast::Domain*>(&doc->content) : nullptr;
    const auto* derived =
        domain && *domain && (*domain)->structures.size() == 1
            ? std::get_if<pddl::ast::DerivedDef*>(&(*domain)->structures[0])
            : nullptr;
    if (!derived || !*derived || (*derived)->name != "p" ||
        !(*derived)->parameters ||
        (*derived)->parameters->untyped.size() != 1 ||
        (*derived)->parameters->untyped[0] != "?x")
      return 1;
  }
  {
    const auto r = language.language.parser->parse(
        "(define (domain d) (:requirements :equality) (:predicates (p)) "
        "(:action a :parameters (?x ?y) :precondition (= ?x ?y) :effect (p)))");
    if (!r.accepted || r.cst().recovered() || !r.diagnostics.empty() ||
        r.value == nullptr)
      return 1;
    const auto* doc = r.value_as<pddl::ast::Document>();
    const auto* domain =
        doc ? std::get_if<pddl::ast::Domain*>(&doc->content) : nullptr;
    const auto* action =
        domain && *domain && (*domain)->structures.size() == 1
            ? std::get_if<pddl::ast::ActionDef*>(&(*domain)->structures[0])
            : nullptr;
    const auto* equality =
        action && *action ? (*action)->precondition : nullptr;
    if (!equality || !equality->leftExpr || !equality->rightExpr ||
        equality->leftExpr->variable != "?x" ||
        equality->rightExpr->variable != "?y" || equality->leftExpr->head ||
        equality->rightExpr->head)
      return 1;
  }
  return 0;
}

int main() {
  if (native_regressions()) return 1;
  if (audit_regression()) {
    std::fputs("audit regression failed\n", stderr);
    return 1;
  }
  const auto language = cursiv::create_language<
      pddl::PddlCoreServices, pddl::PddlSharedCoreServices,
      pddl::install_core_module>();
  const cursiv::Parser& parser = *language.language.parser;

  const char* const docs[] = {
      CURSIV_TEST_DIR "/blocksworld-domain.pddl",
      CURSIV_TEST_DIR "/blocksworld-problem.pddl",
      CURSIV_TEST_DIR "/rover-domain.pddl",
      CURSIV_TEST_DIR "/rover-problem.pddl",
  };
  std::string texts[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) texts[i] = read(docs[i]);

  cursiv::ParseResult results[std::size(docs)];
  for (std::size_t i = 0; i < std::size(docs); ++i) {
    if (texts[i].empty()) return fail("a document is not readable");
    results[i] = parser.parse(texts[i]);
    if (results[i].cst().recovered())
      return fail("a document does not parse clean");
    if (!results[i].diagnostics.empty())
      return fail("a document carries an unexpected diagnostic");
    const auto checked =
        cursiv::test::validationHelper(language.language, texts[i]);
    if (!checked.diagnostics.empty())
      return fail("an existing document carries a validation diagnostic");
  }

  // blocksworld-domain.pddl
  {
    const auto* doc = as<ast::Document>(results[0].value);
    if (doc == nullptr) return fail("blocksworld-domain: no document");
    const auto* domain_p = std::get_if<ast::Domain*>(&doc->content);
    if (domain_p == nullptr || *domain_p == nullptr)
      return fail("blocksworld-domain: content is not a domain");
    const ast::Domain& domain = **domain_p;
    if (domain.name != "blocksworld") return fail("blocksworld-domain: wrong name");
    if (domain.requirements.size() != 12)
      return fail("blocksworld-domain: wrong requirement count");
    if (domain.types == nullptr || domain.types->groups.size() != 1 ||
        domain.types->groups[0]->names.size() != 2 ||
        domain.types->groups[0]->names[0] != "block" ||
        domain.types->groups[0]->type == nullptr ||
        domain.types->groups[0]->type->names.size() != 1 ||
        domain.types->groups[0]->type->names[0] != "object")
      return fail("blocksworld-domain: types did not read back");
    if (domain.constants == nullptr || domain.constants->groups.size() != 1 ||
        domain.constants->groups[0]->names[0] != "ground")
      return fail("blocksworld-domain: constants did not read back");
    if (domain.predicates.size() != 5 ||
        domain.predicates[3]->name != "movable" ||
        domain.predicates[3]->parameters == nullptr ||
        domain.predicates[3]->parameters->groups[0]->type->names.size() != 2)
      return fail("blocksworld-domain: the `either` predicate type did not read back");
    if (domain.functionGroups.size() != 1 ||
        domain.functionGroups[0]->functions.size() != 2 ||
        domain.functionGroups[0]->isNumberTyped)
      return fail("blocksworld-domain: functions did not read back");
    if (domain.structures.size() != 3)
      return fail("blocksworld-domain: wrong structure count");

    const auto* pick_up = std::get_if<ast::ActionDef*>(&domain.structures[0]);
    if (pick_up == nullptr || *pick_up == nullptr ||
        (*pick_up)->name != "pick-up")
      return fail("blocksworld-domain: pick-up did not read back");
    if ((*pick_up)->precondition == nullptr ||
        (*pick_up)->precondition->operands.size() != 7 ||
        (*pick_up)->precondition->isOr)
      return fail("blocksworld-domain: pick-up's precondition did not read back");
    if ((*pick_up)->effect == nullptr || (*pick_up)->effect->operands.size() != 6)
      return fail("blocksworld-domain: pick-up's effect did not read back");

    const auto* derived = std::get_if<ast::DerivedDef*>(&domain.structures[2]);
    if (derived == nullptr || *derived == nullptr ||
        (*derived)->parameters == nullptr ||
        (*derived)->parameters->groups[0]->names[0] != "?x" ||
        (*derived)->body == nullptr || (*derived)->body->operands.size() != 2)
      return fail("blocksworld-domain: the derived predicate did not read back");
  }

  // blocksworld-problem.pddl
  {
    const auto* doc = as<ast::Document>(results[1].value);
    const auto* problem_p = std::get_if<ast::Problem*>(&doc->content);
    if (problem_p == nullptr || *problem_p == nullptr)
      return fail("blocksworld-problem: content is not a problem");
    const ast::Problem& problem = **problem_p;
    if (problem.name != "blocksworld-1" || problem.domainName != "blocksworld")
      return fail("blocksworld-problem: names did not read back");
    if (problem.objects == nullptr || problem.objects->groups[0]->names.size() != 3)
      return fail("blocksworld-problem: objects did not read back");
    if (problem.init.size() != 8) return fail("blocksworld-problem: wrong init count");
    const ast::InitElement& negated = *problem.init[4];
    if (negated.literal == nullptr || !negated.literal->negated ||
        negated.literal->atom == nullptr ||
        negated.literal->atom->predicate != "holding")
      return fail("blocksworld-problem: the negated literal did not read back");
    const ast::InitElement& assign_el = *problem.init[5];
    if (assign_el.head == nullptr || assign_el.head->predicate != "weight" ||
        assign_el.head->args.empty() || assign_el.head->args[0]->name != "a" ||
        assign_el.value != "3")
      return fail("blocksworld-problem: the numeric fact did not read back");
    if (problem.goal == nullptr || problem.goal->operands.size() != 3)
      return fail("blocksworld-problem: the goal did not read back");
    if (!problem.hasMetric || problem.metricOptimization != "minimize" ||
        problem.metricExpr == nullptr || problem.metricExpr->operands.size() != 2)
      return fail("blocksworld-problem: the metric did not read back");
  }

  // rover-domain.pddl
  {
    const auto* doc = as<ast::Document>(results[2].value);
    const auto* domain_p = std::get_if<ast::Domain*>(&doc->content);
    if (domain_p == nullptr || *domain_p == nullptr)
      return fail("rover-domain: content is not a domain");
    const ast::Domain& domain = **domain_p;
    if (domain.name != "rover-transport" || domain.requirements.size() != 6)
      return fail("rover-domain: name/requirements did not read back");
    if (domain.constraints == nullptr ||
        domain.constraints->op != ast::ConstraintOp::And ||
        domain.constraints->operands.size() != 2)
      return fail("rover-domain: domain-level constraints did not read back");
    if (domain.structures.size() != 4)
      return fail("rover-domain: wrong structure count");

    const auto* move = std::get_if<ast::DurativeActionDef*>(&domain.structures[0]);
    if (move == nullptr || *move == nullptr || (*move)->name != "move")
      return fail("rover-domain: move did not read back");
    if ((*move)->duration == nullptr || (*move)->duration->operands.size() != 2)
      return fail("rover-domain: move's duration did not read back");
    if ((*move)->condition == nullptr || (*move)->condition->operands.size() != 4)
      return fail("rover-domain: move's condition did not read back");
    if ((*move)->effect == nullptr || (*move)->effect->operands.size() != 5)
      return fail("rover-domain: move's effect did not read back");

    const auto* no_op = std::get_if<ast::DurativeActionDef*>(&domain.structures[2]);
    if (no_op == nullptr || *no_op == nullptr || (*no_op)->name != "no-op")
      return fail("rover-domain: no-op did not read back");
    if ((*no_op)->condition != nullptr)
      return fail("rover-domain: no-op's empty condition should stay null");
    if ((*no_op)->effect == nullptr || (*no_op)->effect->value == nullptr ||
        !(*no_op)->effect->value->isDuration)
      return fail("rover-domain: no-op's `?duration` effect did not read back");

    const auto* derived = std::get_if<ast::DerivedDef*>(&domain.structures[3]);
    if (derived == nullptr || *derived == nullptr ||
        (*derived)->parameters->groups[0]->names.size() != 2 ||
        (*derived)->body == nullptr || (*derived)->body->atom == nullptr ||
        (*derived)->body->atom->predicate != "far")
      return fail("rover-domain: the second derived predicate did not read back");
  }

  // rover-problem.pddl
  {
    const auto* doc = as<ast::Document>(results[3].value);
    const auto* problem_p = std::get_if<ast::Problem*>(&doc->content);
    if (problem_p == nullptr || *problem_p == nullptr)
      return fail("rover-problem: content is not a problem");
    const ast::Problem& problem = **problem_p;
    if (problem.init.size() != 6) return fail("rover-problem: wrong init count");
    if (!problem.init[5]->isTimed || problem.init[5]->timestamp != "5")
      return fail("rover-problem: the timed literal did not read back");

    if (problem.constraints == nullptr || problem.constraints->operands.size() != 11)
      return fail("rover-problem: constraints did not read back");
    const ast::PrefConstraint& reach = *problem.constraints->operands[0];
    if (!reach.isPreference || reach.name != "reach-site1" ||
        reach.constraint == nullptr ||
        reach.constraint->op != ast::ConstraintOp::Sometime)
      return fail("rover-problem: the preference did not read back");
    const ast::PrefConstraint& always = *problem.constraints->operands[2];
    if (always.isPreference || always.constraint == nullptr ||
        always.constraint->op != ast::ConstraintOp::Always)
      return fail("rover-problem: the plain constraint did not read back");

    if (problem.metricExpr == nullptr || problem.metricExpr->operands.size() != 2)
      return fail("rover-problem: the metric did not read back");
    const ast::Expr& nary = *problem.metricExpr->operands[0];
    if (nary.operands.size() != 3)
      return fail("rover-problem: the n-ary `*` did not read back");
    const ast::Expr& violated = *problem.metricExpr->operands[1];
    if (violated.violatedName != "reach-site1")
      return fail("rover-problem: `is-violated` did not read back");
  }

  // An unterminated document must not parse clean.
  {
    const std::string broken = "(define (domain broken) (:action";
    const cursiv::ParseResult result = parser.parse(broken);
    if (!result.cst().recovered() && result.diagnostics.empty())
      return fail("an unterminated document was accepted as clean");
  }

  std::puts("pddl: every document parses, field values read back correctly");
  return 0;
}
