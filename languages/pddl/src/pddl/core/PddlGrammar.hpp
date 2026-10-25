// The pddl language: its grammar -- PDDL (Planning Domain Definition
// Language) domain and problem files, an s-expression syntax.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pddl/core/ast.hpp"

namespace pddl {

struct Pddl : cursiv::GrammarSpec {
  // PDDL names carry '-' and '_' beyond letters and digits, so the keyword
  // boundary must include them too, or `kw<"end">` would fire inside a name
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
      : terminal<kw<":strips"> | kw<":typing"> |
                 kw<":negative-preconditions"> |
                 kw<":disjunctive-preconditions"> | kw<":equality"> |
                 kw<":existential-preconditions"> |
                 kw<":universal-preconditions"> |
                 kw<":quantified-preconditions"> | kw<":conditional-effects"> |
                 kw<":fluents"> | kw<":numeric-fluents"> |
                 kw<":action-costs"> | kw<":non-deterministic"> |
                 kw<":adl"> | kw<":durative-actions"> |
                 kw<":derived-predicates"> | kw<":timed-initial-literals"> |
                 kw<":preferences"> | kw<":constraints">> {};

  static constexpr auto OPEN = lit<"(">;
  static constexpr auto CLOSE = lit<")">;

  struct GE : terminal<lit<">=">, value<ast::CompareOp::Ge>> {};
  struct LE : terminal<lit<"<=">, value<ast::CompareOp::Le>> {};
  struct GT : terminal<lit<">">, value<ast::CompareOp::Gt>> {};
  struct LT : terminal<lit<"<">, value<ast::CompareOp::Lt>> {};
  struct EQ : terminal<lit<"=">, value<ast::CompareOp::Eq>> {};
  static constexpr auto BinaryComp =
      ref<GE> | ref<LE> | ref<GT> | ref<LT> | ref<EQ>;
  // A duration constraint's comparator: only <=, >= and =, all already
  // spelled above.
  static constexpr auto DurOp = ref<GE> | ref<LE> | ref<EQ>;

  struct MUL : terminal<lit<"*">, value<ast::ArithOp::Mul>> {};
  struct ADD : terminal<lit<"+">, value<ast::ArithOp::Add>> {};
  struct SUB : terminal<lit<"-">, value<ast::ArithOp::Sub>> {};
  struct DIV : terminal<lit<"/">, value<ast::ArithOp::Div>> {};
  static constexpr auto BinaryOp =
      ref<MUL> | ref<ADD> | ref<SUB> | ref<DIV>;

  struct ASSIGN_OP : terminal<kw<"assign">, value<ast::AssignKind::Assign>> {
  };
  struct SCALE_UP_OP
      : terminal<kw<"scale-up">, value<ast::AssignKind::ScaleUp>> {};
  struct SCALE_DOWN_OP
      : terminal<kw<"scale-down">, value<ast::AssignKind::ScaleDown>> {};
  struct INCREASE_OP
      : terminal<kw<"increase">, value<ast::AssignKind::Increase>> {};
  struct DECREASE_OP
      : terminal<kw<"decrease">, value<ast::AssignKind::Decrease>> {};
  static constexpr auto AssignOp = ref<ASSIGN_OP> | ref<SCALE_UP_OP> |
                                    ref<SCALE_DOWN_OP> | ref<INCREASE_OP> |
                                    ref<DECREASE_OP>;

  struct TimeStart : terminal<kw<"start">> {};
  struct TimeEnd : terminal<kw<"end">> {};
  static constexpr auto TimeSpecifier = ref<TimeStart> | ref<TimeEnd>;
  struct IntervalAll : terminal<kw<"all">> {};
  static constexpr auto Interval = ref<IntervalAll>;
  struct MinimizeKw : terminal<kw<"minimize">> {};
  struct MaximizeKw : terminal<kw<"maximize">> {};
  static constexpr auto Optimization = ref<MinimizeKw> | ref<MaximizeKw>;

  // Forward declarations: the mutually recursive core of the language.
  struct GoalDescRule;
  struct FExpRule;
  struct FExpDARule;
  struct MetricFExpRule;
  struct TypedVarListRule;
  struct ConstraintRule;
  struct DaGoalRule;
  struct DaEffectRule;
  struct EffectRule;

  struct TermRule
      : rule<assign<&ast::Term::name>(ref<NAME> | ref<VARIABLE>)> {};
  // A primitive type name, a by-name predicate/function argument, or a
  // metric's function-application argument: a name only, never a variable.
  struct NameTermRule : rule<assign<&ast::Term::name>(ref<NAME>)> {};

  struct TypeRule
      : rule<create<ast::Type> +
             ((OPEN + kw<"either"> +
               some(append<&ast::Type::names>(ref<NAME>)) + CLOSE) |
              append<&ast::Type::names>(ref<NAME>))> {};

  struct TypedNameGroupRule
      : rule<some(append<&ast::TypedGroup::names>(ref<NAME>)) + lit<"-"> +
             assign<&ast::TypedGroup::type>(ref<TypeRule>)> {};
  struct TypedVarGroupRule
      : rule<some(append<&ast::TypedGroup::names>(ref<VARIABLE>)) +
             lit<"-"> + assign<&ast::TypedGroup::type>(ref<TypeRule>)> {};

  // A typed list of names: typed groups (if any) always come first, then
  // untyped names.
  struct TypedNameListRule
      : rule<create<ast::TypedList> +
             many(append<&ast::TypedList::groups>(ref<TypedNameGroupRule>)) +
             many(append<&ast::TypedList::untyped>(ref<NAME>))> {};
  struct TypedVarListRule
      : rule<create<ast::TypedList> +
             many(append<&ast::TypedList::groups>(ref<TypedVarGroupRule>)) +
             many(append<&ast::TypedList::untyped>(ref<VARIABLE>))> {};

  // `at` is also a keyword -- a durative goal's/effect's `(at start|end
  // ...)`, a constraint's `(at end ...)`, a problem's timed literal -- so a
  // predicate literally named `at` (rover-style domains routinely declare
  // one) needs the soft-keyword spelling: `ref<NAME> | kw<"at">`.
  struct AtomicTermFormulaRule
      : rule<OPEN +
             assign<&ast::AtomicFormula::predicate>(ref<NAME> | kw<"at">) +
             many(append<&ast::AtomicFormula::args>(ref<TermRule>)) +
             CLOSE> {};
  // Same soft-keyword spelling, for the by-name form.
  struct AtomicNameFormulaRule
      : rule<OPEN +
             assign<&ast::AtomicFormula::predicate>(ref<NAME> | kw<"at">) +
             many(append<&ast::AtomicFormula::args>(ref<NameTermRule>)) +
             CLOSE> {};
  struct AtomicFormulaSkeletonRule
      : rule<OPEN + assign<&ast::PredicateDecl::name>(ref<NAME>) +
             assign<&ast::PredicateDecl::parameters>(ref<TypedVarListRule>) +
             CLOSE> {};
  struct AtomicFunctionSkeletonRule
      : rule<OPEN + assign<&ast::FunctionDecl::name>(ref<NAME>) +
             assign<&ast::FunctionDecl::parameters>(ref<TypedVarListRule>) +
             CLOSE> {};
  struct TypedFunctionGroupRule
      : rule<some(append<&ast::FunctionGroup::functions>(
                 ref<AtomicFunctionSkeletonRule>)) +
             flag<&ast::FunctionGroup::isNumberTyped>(lit<"-"> +
                                                            kw<"number">)> {};
  struct UntypedFunctionGroupRule
      : rule<some(append<&ast::FunctionGroup::functions>(
            ref<AtomicFunctionSkeletonRule>))> {};

  struct FHeadRule
      : rule<(OPEN + assign<&ast::AtomicFormula::predicate>(ref<NAME>) +
              many(append<&ast::AtomicFormula::args>(ref<TermRule>)) +
              CLOSE) |
             assign<&ast::AtomicFormula::predicate>(ref<NAME>)> {};
  struct MetricFHeadRule
      : rule<(OPEN + assign<&ast::AtomicFormula::predicate>(ref<NAME>) +
              many(append<&ast::AtomicFormula::args>(ref<NameTermRule>)) +
              CLOSE) |
             assign<&ast::AtomicFormula::predicate>(ref<NAME>)> {};

  // Parse the complete operand list; the validator checks operator arity.
  // Addition and multiplication may have more than two operands.
  template <class Operand>
  static constexpr auto Arithmetic =
      OPEN + assign<&ast::Expr::arith>(BinaryOp) +
      many(append<&ast::Expr::operands>(ref<Operand>)) + CLOSE;

  struct FExpRule
      : rule<assign<&ast::Expr::number>(ref<NUMBER>) |
             Arithmetic<FExpRule> |
             assign<&ast::Expr::head>(ref<FHeadRule>) |
             assign<&ast::Expr::variable>(ref<VARIABLE>)> {};

  // Self-contained rather than falling back to `ref<FExpRule>`: every
  // shape a numeric expression admits is spelled again here, so
  // `?duration` can nest at any depth, not only at the top.
  struct FExpDARule
      : rule<flag<&ast::Expr::isDuration>(kw<"?duration">) |
             assign<&ast::Expr::number>(ref<NUMBER>) |
             assign<&ast::Expr::head>(ref<FHeadRule>) |
             Arithmetic<FExpDARule>> {};

  struct MetricFExpRule
      : rule<assign<&ast::Expr::number>(ref<NUMBER>) |
             flag<&ast::Expr::isTotalTime>(kw<"total-time">) |
             (OPEN + kw<"is-violated"> +
              assign<&ast::Expr::violatedName>(ref<NAME>) + CLOSE) |
             Arithmetic<MetricFExpRule> |
             assign<&ast::Expr::head>(ref<MetricFHeadRule>)> {};

  struct GoalDescRule
      : rule<assign<&ast::GoalFormula::atom>(ref<AtomicTermFormulaRule>) |
             (OPEN + kw<"and"> +
              many(append<&ast::GoalFormula::operands>(ref<GoalDescRule>)) +
              CLOSE) |
             (OPEN + flag<&ast::GoalFormula::isOr>(kw<"or">) +
              many(append<&ast::GoalFormula::operands>(ref<GoalDescRule>)) +
              CLOSE) |
             (OPEN + kw<"not"> +
              assign<&ast::GoalFormula::notBody>(ref<GoalDescRule>) +
              CLOSE) |
             (OPEN + kw<"imply"> +
              assign<&ast::GoalFormula::left>(ref<GoalDescRule>) +
              assign<&ast::GoalFormula::right>(ref<GoalDescRule>) + CLOSE) |
             (OPEN + kw<"exists"> + OPEN +
              assign<&ast::GoalFormula::parameters>(ref<TypedVarListRule>) +
              CLOSE + assign<&ast::GoalFormula::quantBody>(ref<GoalDescRule>) +
              CLOSE) |
             (OPEN + flag<&ast::GoalFormula::isForall>(kw<"forall">) +
              OPEN +
              assign<&ast::GoalFormula::parameters>(ref<TypedVarListRule>) +
              CLOSE + assign<&ast::GoalFormula::quantBody>(ref<GoalDescRule>) +
              CLOSE) |
             (OPEN + assign<&ast::GoalFormula::compareOp>(BinaryComp) +
              assign<&ast::GoalFormula::leftExpr>(ref<FExpRule>) +
              assign<&ast::GoalFormula::rightExpr>(ref<FExpRule>) +
              CLOSE)> {};

  // `op` names which of the twelve operators an alternative is: several
  // pairs (AtEnd/Always/Sometime/AtMostOnce; Within/HoldAfter;
  // SometimeAfter/SometimeBefore) would otherwise write the same fields.
  struct ConstraintForall
      : terminal<kw<"forall">, value<ast::ConstraintOp::Forall>> {};
  struct ConstraintAtEnd
      : rule<kw<"at"> + kw<"end">, value<ast::ConstraintOp::AtEnd>> {};
  struct ConstraintAlways
      : terminal<kw<"always">, value<ast::ConstraintOp::Always>> {};
  struct ConstraintSometime
      : terminal<kw<"sometime">, value<ast::ConstraintOp::Sometime>> {};
  struct ConstraintWithin
      : terminal<kw<"within">, value<ast::ConstraintOp::Within>> {};
  struct ConstraintAtMostOnce
      : terminal<kw<"at-most-once">, value<ast::ConstraintOp::AtMostOnce>> {
  };
  struct ConstraintSometimeAfter
      : terminal<kw<"sometime-after">,
                 value<ast::ConstraintOp::SometimeAfter>> {};
  struct ConstraintSometimeBefore
      : terminal<kw<"sometime-before">,
                 value<ast::ConstraintOp::SometimeBefore>> {};
  struct ConstraintAlwaysWithin
      : terminal<kw<"always-within">,
                 value<ast::ConstraintOp::AlwaysWithin>> {};
  struct ConstraintHoldDuring
      : terminal<kw<"hold-during">, value<ast::ConstraintOp::HoldDuring>> {};
  struct ConstraintHoldAfter
      : terminal<kw<"hold-after">, value<ast::ConstraintOp::HoldAfter>> {};

  struct ConstraintRule
      : rule<(OPEN + kw<"and"> +
              many(append<&ast::Constraint::operands>(ref<ConstraintRule>)) +
              CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintForall>) +
              OPEN +
              assign<&ast::Constraint::parameters>(ref<TypedVarListRule>) +
              CLOSE + assign<&ast::Constraint::body>(ref<ConstraintRule>) +
              CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintAtEnd>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintAlways>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintSometime>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintWithin>) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(ref<ConstraintAtMostOnce>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(ref<ConstraintSometimeAfter>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) +
              assign<&ast::Constraint::goal2>(ref<GoalDescRule>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(ref<ConstraintSometimeBefore>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) +
              assign<&ast::Constraint::goal2>(ref<GoalDescRule>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(ref<ConstraintAlwaysWithin>) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) +
              assign<&ast::Constraint::goal2>(ref<GoalDescRule>) + CLOSE) |
             (OPEN +
              assign<&ast::Constraint::op>(ref<ConstraintHoldDuring>) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::num2>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) + CLOSE) |
             (OPEN + assign<&ast::Constraint::op>(ref<ConstraintHoldAfter>) +
              assign<&ast::Constraint::num1>(ref<NUMBER>) +
              assign<&ast::Constraint::goal1>(ref<GoalDescRule>) +
              CLOSE)> {};

  struct PrefConstraintRule
      : rule<(OPEN + kw<"and"> +
              many(append<&ast::PrefConstraint::operands>(
                  ref<PrefConstraintRule>)) +
              CLOSE) |
             (OPEN + kw<"forall"> + OPEN +
              assign<&ast::PrefConstraint::parameters>(
                  ref<TypedVarListRule>) +
              CLOSE +
              assign<&ast::PrefConstraint::body>(ref<PrefConstraintRule>) +
              CLOSE) |
             (OPEN +
              flag<&ast::PrefConstraint::isPreference>(
                  kw<"preference">) +
              option(assign<&ast::PrefConstraint::name>(ref<NAME>)) +
              assign<&ast::PrefConstraint::constraint>(ref<ConstraintRule>) +
              CLOSE) |
             assign<&ast::PrefConstraint::constraint>(ref<ConstraintRule>)> {
  };

  static constexpr auto TimedGDBody =
      (OPEN + kw<"at"> + assign<&ast::DaGoal::timeSpecifier>(TimeSpecifier) +
       assign<&ast::DaGoal::goal>(ref<GoalDescRule>) + CLOSE) |
      (OPEN + kw<"over"> + assign<&ast::DaGoal::interval>(Interval) +
       assign<&ast::DaGoal::goal>(ref<GoalDescRule>) + CLOSE);
  struct TimedGDRule : rule<TimedGDBody> {};

  struct DaGoalRule
      : rule<TimedGDBody |
             (OPEN + kw<"and"> +
              many(append<&ast::DaGoal::operands>(ref<DaGoalRule>)) +
              CLOSE) |
             (OPEN + kw<"forall"> + OPEN +
              assign<&ast::DaGoal::parameters>(ref<TypedVarListRule>) +
              CLOSE + assign<&ast::DaGoal::body>(ref<DaGoalRule>) + CLOSE) |
             (OPEN + kw<"preference"> +
              option(assign<&ast::DaGoal::name>(ref<NAME>)) +
              assign<&ast::DaGoal::body>(ref<TimedGDRule>) + CLOSE)> {};

  // Shared by an ordinary and a durative action's effect: both can add or
  // delete a predicate, not only reassign a numeric fluent.
  static constexpr auto AtomicEffectBody =
      (OPEN + flag<&ast::Effect::negated>(kw<"not">) +
       assign<&ast::Effect::head>(ref<AtomicTermFormulaRule>) + CLOSE) |
      assign<&ast::Effect::head>(ref<AtomicTermFormulaRule>);

  static constexpr auto PEffectBody =
      (OPEN + assign<&ast::Effect::assignOp>(AssignOp) +
       assign<&ast::Effect::head>(ref<FHeadRule>) +
       assign<&ast::Effect::value>(ref<FExpRule>) + CLOSE) |
      AtomicEffectBody;
  struct PEffectRule : rule<PEffectBody> {};

  static constexpr auto CondEffectBody =
      (OPEN + kw<"and"> +
       many(append<&ast::Effect::operands>(ref<PEffectRule>)) + CLOSE) |
      PEffectBody;
  struct CondEffectRule : rule<CondEffectBody> {};

  static constexpr auto CEffectBody =
      (OPEN + kw<"forall"> + OPEN +
       assign<&ast::Effect::parameters>(ref<TypedVarListRule>) + CLOSE +
       assign<&ast::Effect::body>(ref<EffectRule>) + CLOSE) |
      (OPEN + kw<"when"> +
       assign<&ast::Effect::condition>(ref<GoalDescRule>) +
       assign<&ast::Effect::consequence>(ref<CondEffectRule>) + CLOSE) |
      (OPEN + flag<&ast::Effect::isOneOf>(kw<"oneof">) +
       many(append<&ast::Effect::operands>(ref<EffectRule>)) + CLOSE) |
      PEffectBody;
  struct CEffectRule : rule<CEffectBody> {};

  struct EffectRule
      : rule<(OPEN + kw<"and"> +
              many(append<&ast::Effect::operands>(ref<CEffectRule>)) +
              CLOSE) |
             CEffectBody> {};

  static constexpr auto TimedEffectBody =
      OPEN + kw<"at"> + assign<&ast::Effect::timeSpecifier>(TimeSpecifier) +
      (assign<&ast::Effect::body>(ref<DaEffectRule>) |
       (assign<&ast::Effect::assignOp>(AssignOp) +
        assign<&ast::Effect::head>(ref<FHeadRule>) +
        assign<&ast::Effect::value>(ref<FExpDARule>))) +
      CLOSE;
  struct TimedEffectRule : rule<TimedEffectBody> {};

  struct DaEffectRule
      : rule<TimedEffectBody |
             AtomicEffectBody |
             (OPEN + kw<"and"> +
              many(append<&ast::Effect::operands>(ref<DaEffectRule>)) +
              CLOSE) |
             (OPEN + kw<"forall"> + OPEN +
              assign<&ast::Effect::parameters>(ref<TypedVarListRule>) +
              CLOSE + assign<&ast::Effect::body>(ref<DaEffectRule>) +
              CLOSE) |
             (OPEN + kw<"when"> +
              assign<&ast::Effect::daCondition>(ref<DaGoalRule>) +
              assign<&ast::Effect::consequence>(ref<TimedEffectRule>) +
              CLOSE) |
             (OPEN + assign<&ast::Effect::assignOp>(AssignOp) +
              assign<&ast::Effect::head>(ref<FHeadRule>) +
              assign<&ast::Effect::value>(ref<FExpDARule>) + CLOSE)> {};

  struct DurationRule
      : rule<(OPEN + kw<"and"> +
              some(append<&ast::Duration::operands>(ref<DurationRule>)) +
              CLOSE) |
             (create<ast::Duration> + OPEN + CLOSE) |
             (OPEN + assign<&ast::Duration::compareOp>(DurOp) +
              kw<"?duration"> +
              assign<&ast::Duration::value>(ref<FExpRule>) + CLOSE) |
             (OPEN + kw<"at"> +
              assign<&ast::Duration::timeSpecifier>(TimeSpecifier) +
              assign<&ast::Duration::body>(ref<DurationRule>) + CLOSE)> {};

  struct ActionDefRule
      : rule<OPEN + kw<":action"> +
             assign<&ast::ActionDef::name>(ref<NAME>) + kw<":parameters"> +
             OPEN +
             assign<&ast::ActionDef::parameters>(ref<TypedVarListRule>) +
             CLOSE +
             option(flag<&ast::ActionDef::hasPrecondition>(kw<":precondition">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::ActionDef::precondition>(
                         ref<GoalDescRule>))) +
             option(flag<&ast::ActionDef::hasEffect>(kw<":effect">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::ActionDef::effect>(ref<EffectRule>))) +
             CLOSE> {};

  // Duration, condition and effect are optional and independent, exactly as
  // they are for a plain action, not mutually exclusive.
  struct DurativeActionDefRule
      : rule<OPEN + kw<":durative-action"> +
             assign<&ast::DurativeActionDef::name>(ref<NAME>) +
             kw<":parameters"> + OPEN +
             assign<&ast::DurativeActionDef::parameters>(
                 ref<TypedVarListRule>) +
             CLOSE +
             option(kw<":duration"> +
                    assign<&ast::DurativeActionDef::duration>(
                        ref<DurationRule>)) +
             option(flag<&ast::DurativeActionDef::hasCondition>(kw<":condition">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::DurativeActionDef::condition>(
                         ref<DaGoalRule>))) +
             option(flag<&ast::DurativeActionDef::hasEffect>(kw<":effect">) +
                    ((OPEN + CLOSE) |
                     assign<&ast::DurativeActionDef::effect>(
                         ref<DaEffectRule>))) +
             CLOSE> {};

  struct DerivedDefRule
      : rule<OPEN + kw<":derived"> + OPEN +
             assign<&ast::DerivedDef::name>(ref<NAME>) +
             assign<&ast::DerivedDef::parameters>(ref<TypedVarListRule>) +
             CLOSE + assign<&ast::DerivedDef::body>(ref<GoalDescRule>) +
             CLOSE> {};

  static constexpr auto StructureAlt = ref<ActionDefRule> |
                                        ref<DurativeActionDefRule> |
                                        ref<DerivedDefRule>;

  struct DomainRule
      : rule<OPEN + kw<"define"> + OPEN + kw<"domain"> +
             assign<&ast::Domain::name>(ref<NAME>) + CLOSE +
             option(OPEN + kw<":requirements"> +
                    some(append<&ast::Domain::requirements>(
                        ref<REQUIRE_KEY>)) +
                    CLOSE) +
             option(OPEN + kw<":types"> +
                    assign<&ast::Domain::types>(ref<TypedNameListRule>) +
                    CLOSE) +
             option(OPEN + kw<":constants"> +
                    assign<&ast::Domain::constants>(ref<TypedNameListRule>) +
                    CLOSE) +
             option(OPEN + kw<":predicates"> +
                    some(append<&ast::Domain::predicates>(
                        ref<AtomicFormulaSkeletonRule>)) +
                    CLOSE) +
             option(OPEN + kw<":functions"> +
                    many(append<&ast::Domain::functionGroups>(
                        ref<TypedFunctionGroupRule>)) +
                    option(append<&ast::Domain::functionGroups>(
                        ref<UntypedFunctionGroupRule>)) +
                    CLOSE) +
             option(OPEN + kw<":constraints"> +
                    assign<&ast::Domain::constraints>(ref<ConstraintRule>) +
                    CLOSE) +
             many(append<&ast::Domain::structures>(StructureAlt)) +
             CLOSE> {};

  struct NameLiteralRule
      : rule<assign<&ast::NameLiteral::atom>(ref<AtomicNameFormulaRule>) |
             (OPEN + flag<&ast::NameLiteral::negated>(kw<"not">) +
              assign<&ast::NameLiteral::atom>(ref<AtomicNameFormulaRule>) +
              CLOSE)> {};

  struct InitElRule
      : rule<assign<&ast::InitElement::literal>(ref<NameLiteralRule>) |
             (OPEN + lit<"="> +
              assign<&ast::InitElement::head>(ref<FHeadRule>) +
              assign<&ast::InitElement::value>(ref<NUMBER>) + CLOSE) |
             (OPEN + flag<&ast::InitElement::isTimed>(kw<"at">) +
              assign<&ast::InitElement::timestamp>(ref<NUMBER>) +
              assign<&ast::InitElement::literal>(ref<NameLiteralRule>) +
              CLOSE)> {};

  struct ProblemRule
      : rule<OPEN + kw<"define"> + OPEN + kw<"problem"> +
             assign<&ast::Problem::name>(ref<NAME>) + CLOSE + OPEN +
             kw<":domain"> +
             assign<&ast::Problem::domainName>(ref<NAME>) + CLOSE +
             option(OPEN + kw<":requirements"> +
                    some(append<&ast::Problem::requirements>(
                        ref<REQUIRE_KEY>)) +
                    CLOSE) +
             option(OPEN + kw<":objects"> +
                    assign<&ast::Problem::objects>(ref<TypedNameListRule>) +
                    CLOSE) +
             OPEN + kw<":init"> +
             many(append<&ast::Problem::init>(ref<InitElRule>)) + CLOSE +
             OPEN + kw<":goal"> +
             assign<&ast::Problem::goal>(ref<GoalDescRule>) + CLOSE +
             option(OPEN + kw<":constraints"> +
                    assign<&ast::Problem::constraints>(
                        ref<PrefConstraintRule>) +
                    CLOSE) +
             option(flag<&ast::Problem::hasMetric>(OPEN + kw<":metric">) +
                    assign<&ast::Problem::metricOptimization>(
                        Optimization) +
                    assign<&ast::Problem::metricExpr>(ref<MetricFExpRule>) +
                    CLOSE) +
             CLOSE> {};

  struct DocumentRule
      : rule<assign<&ast::Document::content>(ref<DomainRule> |
                                              ref<ProblemRule>)> {};

  using entry = DocumentRule;
  using ignore = list<WS, LineComment>;
};

}  // namespace pddl
