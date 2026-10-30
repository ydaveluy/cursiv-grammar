// The pddl language: its grammar -- PDDL (Planning Domain Definition
// Language) domain and problem files, an s-expression syntax.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pddl/core/ast.hpp"

namespace pddl {

struct Pddl : cursiv::GrammarSpec {
  // PDDL names carry '-' and '_' beyond letters and digits, so the keyword
  // boundary must include them too, or `ikw<"end">` would fire inside a name
  // like `end-effector`.
  static constexpr auto NameCont = cr<"a-zA-Z0-9_-">;
  static constexpr auto word_boundary = NameCont;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // The trailing newline is optional, so a file ending in a comment with no
  // final newline still lexes.
  struct LineComment : terminal<lit<";"> + many(cr<R"(^\r\n)">) + option(eol)> {};

  struct NAME : terminal<cr<"a-zA-Z"> + many(NameCont)> {};
  struct VARIABLE : terminal<lit<"?"> + cr<"a-zA-Z"> + many(NameCont)> {};
  // Keep numeric spelling: integers need not fit double's exact range.
  struct NUMBER : terminal<some(d) + option(lit<"."> + some(d))> {};

  struct REQUIRE_KEY
      : terminal<ikw<":strips"> | ikw<":typing"> |
                 ikw<":negative-preconditions"> |
                 ikw<":disjunctive-preconditions"> | ikw<":equality"> |
                 ikw<":existential-preconditions"> |
                 ikw<":universal-preconditions"> |
                 ikw<":quantified-preconditions"> | ikw<":conditional-effects"> |
                 ikw<":fluents"> | ikw<":numeric-fluents"> |
                 ikw<":action-costs"> | ikw<":non-deterministic"> |
                 ikw<":adl"> | ikw<":durative-actions"> |
                 ikw<":derived-predicates"> | ikw<":timed-initial-literals"> |
                 ikw<":preferences"> | ikw<":constraints">> {};

  static constexpr auto OPEN = lit<"(">;
  static constexpr auto CLOSE = lit<")">;

  static constexpr auto BinaryComp =
      as<ast::CompareOp::Ge>(lit<">=">) | as<ast::CompareOp::Le>(lit<"<=">) | as<ast::CompareOp::Gt>(lit<">">) | as<ast::CompareOp::Lt>(lit<"<">) | as<ast::CompareOp::Eq>(lit<"=">);
  // A duration constraint's comparator: only <=, >= and =, all already
  // spelled above.
  static constexpr auto DurOp = as<ast::CompareOp::Ge>(lit<">=">) | as<ast::CompareOp::Le>(lit<"<=">) | as<ast::CompareOp::Eq>(lit<"=">);

  static constexpr auto BinaryOp =
      as<ast::ArithOp::Mul>(lit<"*">) | as<ast::ArithOp::Add>(lit<"+">) | as<ast::ArithOp::Sub>(lit<"-">) | as<ast::ArithOp::Div>(lit<"/">);

  static constexpr auto AssignOp = as<ast::AssignKind::Assign>(ikw<"assign">) | as<ast::AssignKind::ScaleUp>(ikw<"scale-up">) |
                                    as<ast::AssignKind::ScaleDown>(ikw<"scale-down">) | as<ast::AssignKind::Increase>(ikw<"increase">) |
                                    as<ast::AssignKind::Decrease>(ikw<"decrease">);

  struct TimeStart : terminal<ikw<"start">> {};
  struct TimeEnd : terminal<ikw<"end">> {};
  static constexpr auto TimeSpecifier = ref<TimeStart> | ref<TimeEnd>;
  struct IntervalAll : terminal<ikw<"all">> {};
  static constexpr auto Interval = ref<IntervalAll>;
  struct MinimizeKw : terminal<ikw<"minimize">> {};
  struct MaximizeKw : terminal<ikw<"maximize">> {};
  static constexpr auto Optimization = ref<MinimizeKw> | ref<MaximizeKw>;

  // Forward declarations: the mutually recursive core of the language.
  struct GoalDesc;
  struct FExp;
  struct FExpDA;
  struct MetricFExp;
  struct TypedVarList;
  struct Constraint;
  struct DaGoal;
  struct DaEffect;
  struct Effect;

  struct Term
      : rule<assign<&ast::Term::name>(ref<NAME> | ref<VARIABLE>)> {};
  // A primitive type name, a by-name predicate/function argument, or a
  // metric's function-application argument: a name only, never a variable.
  struct NameTerm : rule<assign<&ast::Term::name>(ref<NAME>)> {};

  struct Type
      : rule<create<ast::Type> +
             ((OPEN + ikw<"either"> +
               some(append<&ast::Type::names>(ref<NAME>)) + CLOSE) |
              append<&ast::Type::names>(ref<NAME>))> {};

  struct TypedNameGroup
      : rule<some(append<&ast::TypedGroup::names>(ref<NAME>)) + lit<"-"> +
             assign<&ast::TypedGroup::type>(ref<Type>)> {};
  struct TypedVarGroup
      : rule<some(append<&ast::TypedGroup::names>(ref<VARIABLE>)) +
             lit<"-"> + assign<&ast::TypedGroup::type>(ref<Type>)> {};

  // A typed list of names: typed groups (if any) always come first, then
  // untyped names.
  struct TypedNameList
      : rule<create<ast::TypedList> +
             many(append<&ast::TypedList::groups>(ref<TypedNameGroup>)) +
             many(append<&ast::TypedList::untyped>(ref<NAME>))> {};
  struct TypedVarList
      : rule<create<ast::TypedList> +
             many(append<&ast::TypedList::groups>(ref<TypedVarGroup>)) +
             many(append<&ast::TypedList::untyped>(ref<VARIABLE>))> {};

  // `at` is also a keyword -- a durative goal's/effect's `(at start|end
  // ...)`, a constraint's `(at end ...)`, a problem's timed literal -- so a
  // predicate literally named `at` (rover-style domains routinely declare
  // one) needs the soft-keyword spelling: `ref<NAME> | ikw<"at">`.
  struct AtomicTermFormula
      : rule<OPEN +
             assign<&ast::AtomicFormula::predicate>(ref<NAME> | ikw<"at">) +
             many(append<&ast::AtomicFormula::args>(ref<Term>)) +
             CLOSE> {};
  // Same soft-keyword spelling, for the by-name form.
  struct AtomicNameFormula
      : rule<OPEN +
             assign<&ast::AtomicFormula::predicate>(ref<NAME> | ikw<"at">) +
             many(append<&ast::AtomicFormula::args>(ref<NameTerm>)) +
             CLOSE> {};
  struct AtomicFormulaSkeleton
      : rule<OPEN + assign<&ast::PredicateDecl::name>(ref<NAME>) +
             assign<&ast::PredicateDecl::parameters>(ref<TypedVarList>) +
             CLOSE> {};
  struct AtomicFunctionSkeleton
      : rule<OPEN + assign<&ast::FunctionDecl::name>(ref<NAME>) +
             assign<&ast::FunctionDecl::parameters>(ref<TypedVarList>) +
             CLOSE> {};
  struct TypedFunctionGroup
      : rule<some(append<&ast::FunctionGroup::functions>(
                 ref<AtomicFunctionSkeleton>)) +
             flag<&ast::FunctionGroup::isNumberTyped>(lit<"-"> +
                                                            ikw<"number">)> {};
  struct UntypedFunctionGroup
      : rule<some(append<&ast::FunctionGroup::functions>(
            ref<AtomicFunctionSkeleton>))> {};

  struct FHead
      : rule<(OPEN + assign<&ast::AtomicFormula::predicate>(ref<NAME>) +
              many(append<&ast::AtomicFormula::args>(ref<Term>)) +
              CLOSE) |
             assign<&ast::AtomicFormula::predicate>(ref<NAME>)> {};
  struct MetricFHead
      : rule<(OPEN + assign<&ast::AtomicFormula::predicate>(ref<NAME>) +
              many(append<&ast::AtomicFormula::args>(ref<NameTerm>)) +
              CLOSE) |
             assign<&ast::AtomicFormula::predicate>(ref<NAME>)> {};

  // Parse the complete operand list; the validator checks operator arity.
  // Addition and multiplication may have more than two operands.
  template <class Operand>
  static constexpr auto Arithmetic =
      OPEN + assign<&ast::Expr::arith>(BinaryOp) +
      many(append<&ast::Expr::operands>(ref<Operand>)) + CLOSE;

  struct FExp
      : rule<assign<&ast::Expr::number>(ref<NUMBER>) |
             Arithmetic<FExp> |
             assign<&ast::Expr::head>(ref<FHead>) |
             assign<&ast::Expr::variable>(ref<VARIABLE>)> {};

  // Self-contained rather than falling back to `ref<FExp>`: every
  // shape a numeric expression admits is spelled again here, so
  // `?duration` can nest at any depth, not only at the top.
  struct FExpDA
      : rule<flag<&ast::Expr::isDuration>(ikw<"?duration">) |
             assign<&ast::Expr::number>(ref<NUMBER>) |
             assign<&ast::Expr::head>(ref<FHead>) |
             Arithmetic<FExpDA>> {};

  struct MetricFExp
      : rule<assign<&ast::Expr::number>(ref<NUMBER>) |
             flag<&ast::Expr::isTotalTime>(ikw<"total-time">) |
             (OPEN + ikw<"is-violated"> +
              assign<&ast::Expr::violatedName>(ref<NAME>) + CLOSE) |
             Arithmetic<MetricFExp> |
             assign<&ast::Expr::head>(ref<MetricFHead>)> {};

  struct GoalDesc
      : rule<(OPEN + ikw<"and"> +
              many(append<&ast::GoalFormula::operands>(ref<GoalDesc>)) +
              CLOSE) |
             (OPEN + flag<&ast::GoalFormula::isOr>(ikw<"or">) +
              many(append<&ast::GoalFormula::operands>(ref<GoalDesc>)) +
              CLOSE) |
             (OPEN + ikw<"not"> +
              assign<&ast::GoalFormula::notBody>(ref<GoalDesc>) +
              CLOSE) |
             (OPEN + ikw<"imply"> +
              assign<&ast::GoalFormula::left>(ref<GoalDesc>) +
              assign<&ast::GoalFormula::right>(ref<GoalDesc>) + CLOSE) |
             (OPEN + ikw<"exists"> + OPEN +
              assign<&ast::GoalFormula::parameters>(ref<TypedVarList>) +
              CLOSE + assign<&ast::GoalFormula::quantBody>(ref<GoalDesc>) +
              CLOSE) |
             (OPEN + flag<&ast::GoalFormula::isForall>(ikw<"forall">) +
              OPEN +
              assign<&ast::GoalFormula::parameters>(ref<TypedVarList>) +
              CLOSE + assign<&ast::GoalFormula::quantBody>(ref<GoalDesc>) +
              CLOSE) |
             (OPEN + assign<&ast::GoalFormula::compareOp>(BinaryComp) +
              assign<&ast::GoalFormula::leftExpr>(ref<FExp>) +
              assign<&ast::GoalFormula::rightExpr>(ref<FExp>) +
              CLOSE) |
             assign<&ast::GoalFormula::atom>(ref<AtomicTermFormula>)> {};

  // `op` names which of the twelve operators an alternative is: several
  // pairs (AtEnd/Always/Sometime/AtMostOnce; Within/HoldAfter;
  // SometimeAfter/SometimeBefore) would otherwise write the same fields.

  struct Constraint
      : rule<(OPEN + ikw<"and"> +
              many(append<&ast::Constraint::operands>(ref<Constraint>)) +
              CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::Forall>(ikw<"forall">)) +
              OPEN +
              assign<&ast::Constraint::parameters>(ref<TypedVarList>) +
              CLOSE + assign<&ast::Constraint::body>(ref<Constraint>) +
              CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::AtEnd>(ikw<"at"> + ikw<"end">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::Always>(ikw<"always">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::Sometime>(ikw<"sometime">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::Within>(ikw<"within">)) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(as<ast::ConstraintOp::AtMostOnce>(ikw<"at-most-once">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(as<ast::ConstraintOp::SometimeAfter>(ikw<"sometime-after">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) +
              assign<&ast::Constraint::goal2>(ref<GoalDesc>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(as<ast::ConstraintOp::SometimeBefore>(ikw<"sometime-before">)) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) +
              assign<&ast::Constraint::goal2>(ref<GoalDesc>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(as<ast::ConstraintOp::AlwaysWithin>(ikw<"always-within">)) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) +
              assign<&ast::Constraint::goal2>(ref<GoalDesc>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(as<ast::ConstraintOp::HoldDuring>(ikw<"hold-during">)) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::num2>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(as<ast::ConstraintOp::HoldAfter>(ikw<"hold-after">)) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDesc>) +
              CLOSE)> {};

  struct PrefConstraint
      : rule<(OPEN + ikw<"and"> +
              many(append<&ast::PrefConstraint::operands>(
                  ref<PrefConstraint>)) +
              CLOSE) |
             (OPEN + ikw<"forall"> + OPEN +
              assign<&ast::PrefConstraint::parameters>(
                  ref<TypedVarList>) +
              CLOSE +
              assign<&ast::PrefConstraint::body>(ref<PrefConstraint>) +
              CLOSE) |
             (OPEN +
              flag<&ast::PrefConstraint::isPreference>(
                  ikw<"preference">) +
              option(assign<&ast::PrefConstraint::name>(ref<NAME>)) +
              assign<&ast::PrefConstraint::constraint>(ref<Constraint>) +
              CLOSE) |
             assign<&ast::PrefConstraint::constraint>(ref<Constraint>)> {
  };

  static constexpr auto TimedGDBody =
      (OPEN + ikw<"at"> + assign<&ast::DaGoal::timeSpecifier>(TimeSpecifier) +
       assign<&ast::DaGoal::goal>(ref<GoalDesc>) + CLOSE) |
      (OPEN + ikw<"over"> + assign<&ast::DaGoal::interval>(Interval) +
       assign<&ast::DaGoal::goal>(ref<GoalDesc>) + CLOSE);
  struct TimedGD : rule<TimedGDBody> {};

  struct DaGoal
      : rule<TimedGDBody |
             (OPEN + ikw<"and"> +
              many(append<&ast::DaGoal::operands>(ref<DaGoal>)) +
              CLOSE) |
             (OPEN + ikw<"forall"> + OPEN +
              assign<&ast::DaGoal::parameters>(ref<TypedVarList>) +
              CLOSE + assign<&ast::DaGoal::body>(ref<DaGoal>) + CLOSE) |
             (OPEN + ikw<"preference"> +
              option(assign<&ast::DaGoal::name>(ref<NAME>)) +
              assign<&ast::DaGoal::body>(ref<TimedGD>) + CLOSE)> {};

  // Shared by an ordinary and a durative action's effect: both can add or
  // delete a predicate, not only reassign a numeric fluent.
  static constexpr auto AtomicEffectBody =
      (OPEN + flag<&ast::Effect::negated>(ikw<"not">) +
       assign<&ast::Effect::head>(ref<AtomicTermFormula>) + CLOSE) |
      assign<&ast::Effect::head>(ref<AtomicTermFormula>);

  static constexpr auto PEffectBody =
      (OPEN + assign<&ast::Effect::assignOp>(AssignOp) +
       assign<&ast::Effect::head>(ref<FHead>) +
       assign<&ast::Effect::value>(ref<FExp>) + CLOSE) |
      AtomicEffectBody;
  struct PEffect : rule<PEffectBody> {};

  static constexpr auto CondEffectBody =
      (OPEN + ikw<"and"> +
       many(append<&ast::Effect::operands>(ref<PEffect>)) + CLOSE) |
      PEffectBody;
  struct CondEffect : rule<CondEffectBody> {};

  static constexpr auto CEffectBody =
      (OPEN + ikw<"forall"> + OPEN +
       assign<&ast::Effect::parameters>(ref<TypedVarList>) + CLOSE +
       assign<&ast::Effect::body>(ref<Effect>) + CLOSE) |
      (OPEN + ikw<"when"> +
       assign<&ast::Effect::condition>(ref<GoalDesc>) +
       assign<&ast::Effect::consequence>(ref<CondEffect>) + CLOSE) |
      (OPEN + flag<&ast::Effect::isOneOf>(ikw<"oneof">) +
       many(append<&ast::Effect::operands>(ref<Effect>)) + CLOSE) |
      PEffectBody;
  struct CEffect : rule<CEffectBody> {};

  struct Effect
      : rule<(OPEN + ikw<"and"> +
              many(append<&ast::Effect::operands>(ref<CEffect>)) +
              CLOSE) |
             CEffectBody> {};

  static constexpr auto TimedEffectBody =
      OPEN + ikw<"at"> + assign<&ast::Effect::timeSpecifier>(TimeSpecifier) +
      (assign<&ast::Effect::body>(ref<DaEffect>) |
       (assign<&ast::Effect::assignOp>(AssignOp) +
        assign<&ast::Effect::head>(ref<FHead>) +
        assign<&ast::Effect::value>(ref<FExpDA>))) +
      CLOSE;
  struct TimedEffect : rule<TimedEffectBody> {};

  struct DaEffect
      : rule<TimedEffectBody |
             AtomicEffectBody |
             (OPEN + ikw<"and"> +
              many(append<&ast::Effect::operands>(ref<DaEffect>)) +
              CLOSE) |
             (OPEN + ikw<"forall"> + OPEN +
              assign<&ast::Effect::parameters>(ref<TypedVarList>) +
              CLOSE + assign<&ast::Effect::body>(ref<DaEffect>) +
              CLOSE) |
             (OPEN + ikw<"when"> +
              assign<&ast::Effect::daCondition>(ref<DaGoal>) +
              assign<&ast::Effect::consequence>(ref<TimedEffect>) +
              CLOSE) |
             (OPEN + assign<&ast::Effect::assignOp>(AssignOp) +
              assign<&ast::Effect::head>(ref<FHead>) +
              assign<&ast::Effect::value>(ref<FExpDA>) + CLOSE)> {};

  struct DurationRule
      : rule<(OPEN + ikw<"and"> +
              some(append<&ast::Duration::operands>(ref<DurationRule>)) +
              CLOSE) |
             (create<ast::Duration> + OPEN + CLOSE) |
             (OPEN + assign<&ast::Duration::compareOp>(DurOp) +
              ikw<"?duration"> +
              assign<&ast::Duration::value>(ref<FExp>) + CLOSE) |
             (OPEN + ikw<"at"> +
              assign<&ast::Duration::timeSpecifier>(TimeSpecifier) +
              assign<&ast::Duration::body>(ref<DurationRule>) + CLOSE)> {};

  struct ActionDef
      : rule<OPEN + ikw<":action"> +
             assign<&ast::ActionDef::name>(ref<NAME>) + ikw<":parameters"> +
             OPEN +
             assign<&ast::ActionDef::parameters>(ref<TypedVarList>) +
             CLOSE +
             option(flag<&ast::ActionDef::hasPrecondition>(ikw<":precondition">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::ActionDef::precondition>(
                         ref<GoalDesc>))) +
             option(flag<&ast::ActionDef::hasEffect>(ikw<":effect">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::ActionDef::effect>(ref<Effect>))) +
             CLOSE> {};

  // Duration, condition and effect are optional and independent, exactly as
  // they are for a plain action, not mutually exclusive.
  struct DurativeActionDef
      : rule<OPEN + ikw<":durative-action"> +
             assign<&ast::DurativeActionDef::name>(ref<NAME>) +
             ikw<":parameters"> + OPEN +
             assign<&ast::DurativeActionDef::parameters>(
                 ref<TypedVarList>) +
             CLOSE +
             option(ikw<":duration"> +
                    assign<&ast::DurativeActionDef::duration>(
                        ref<DurationRule>)) +
             option(flag<&ast::DurativeActionDef::hasCondition>(ikw<":condition">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::DurativeActionDef::condition>(
                         ref<DaGoal>))) +
             option(flag<&ast::DurativeActionDef::hasEffect>(ikw<":effect">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::DurativeActionDef::effect>(
                         ref<DaEffect>))) +
             CLOSE> {};

  struct DerivedDef
      : rule<OPEN + ikw<":derived"> + OPEN +
             assign<&ast::DerivedDef::name>(ref<NAME>) +
             assign<&ast::DerivedDef::parameters>(ref<TypedVarList>) +
             CLOSE + assign<&ast::DerivedDef::body>(ref<GoalDesc>) +
             CLOSE> {};

  static constexpr auto StructureAlt = ref<ActionDef> |
                                        ref<DurativeActionDef> |
                                        ref<DerivedDef>;

  struct DomainRule
      : rule<OPEN + ikw<"define"> + OPEN + ikw<"domain"> +
             assign<&ast::Domain::name>(ref<NAME>) + CLOSE +
             option(OPEN + ikw<":requirements"> +
                    some(append<&ast::Domain::requirements>(
                        ref<REQUIRE_KEY>)) +
                    CLOSE) +
             option(OPEN + ikw<":types"> +
                    assign<&ast::Domain::types>(ref<TypedNameList>) +
                    CLOSE) +
             option(OPEN + ikw<":constants"> +
                    assign<&ast::Domain::constants>(ref<TypedNameList>) +
                    CLOSE) +
             option(OPEN + ikw<":predicates"> +
                    some(append<&ast::Domain::predicates>(
                        ref<AtomicFormulaSkeleton>)) +
                    CLOSE) +
             option(OPEN + ikw<":functions"> +
                    many(append<&ast::Domain::functionGroups>(
                        ref<TypedFunctionGroup>)) +
                    option(append<&ast::Domain::functionGroups>(
                        ref<UntypedFunctionGroup>)) +
                    CLOSE) +
             option(OPEN + ikw<":constraints"> +
                    assign<&ast::Domain::constraints>(ref<Constraint>) +
                    CLOSE) +
             many(append<&ast::Domain::structures>(StructureAlt)) +
             CLOSE> {};

  struct NameLiteral
      : rule<assign<&ast::NameLiteral::atom>(ref<AtomicNameFormula>) |
             (OPEN + flag<&ast::NameLiteral::negated>(ikw<"not">) +
              assign<&ast::NameLiteral::atom>(ref<AtomicNameFormula>) +
              CLOSE)> {};

  struct InitEl
      : rule<assign<&ast::InitElement::literal>(ref<NameLiteral>) |
             (OPEN + lit<"="> +
              assign<&ast::InitElement::head>(ref<FHead>) +
              assign<&ast::InitElement::value>(ref<NUMBER>) + CLOSE) |
             (OPEN + flag<&ast::InitElement::isTimed>(ikw<"at">) +
              assign<&ast::InitElement::timestamp>(ref<NUMBER>) +
              assign<&ast::InitElement::literal>(ref<NameLiteral>) +
              CLOSE)> {};

  struct Problem
      : rule<OPEN + ikw<"define"> + OPEN + ikw<"problem"> +
             assign<&ast::Problem::name>(ref<NAME>) + CLOSE + OPEN +
             ikw<":domain"> +
             assign<&ast::Problem::domainName>(ref<NAME>) + CLOSE +
             option(OPEN + ikw<":requirements"> +
                    some(append<&ast::Problem::requirements>(
                        ref<REQUIRE_KEY>)) +
                    CLOSE) +
             option(OPEN + ikw<":objects"> +
                    assign<&ast::Problem::objects>(ref<TypedNameList>) +
                    CLOSE) +
             OPEN + ikw<":init"> +
             many(append<&ast::Problem::init>(ref<InitEl>)) + CLOSE +
             OPEN + ikw<":goal"> +
             assign<&ast::Problem::goal>(ref<GoalDesc>) + CLOSE +
             option(OPEN + ikw<":constraints"> +
                    assign<&ast::Problem::constraints>(
                        ref<PrefConstraint>) +
                    CLOSE) +
             option(flag<&ast::Problem::hasMetric>(OPEN + ikw<":metric">) +
                    assign<&ast::Problem::metricOptimization>(
                        Optimization) +
                    assign<&ast::Problem::metricExpr>(ref<MetricFExp>) +
                    CLOSE) +
             CLOSE> {};

  struct Document
      : rule<assign<&ast::Document::content>(ref<DomainRule> |
                                              ref<Problem>)> {};

  using entry = Document;
  using ignored = list<WS, LineComment>;
};

}  // namespace pddl
