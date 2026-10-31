// The pddl language: its AST -- PDDL domain and problem files, an
// s-expression syntax for classical and durative planning.
//
// Several rules that share a shape merge into one class here: the
// grammar keeps each context's exact keyword set (a domain `:constraints`
// cannot admit `preference`, which only a problem's form allows), the AST
// only keeps the shape. Where two alternatives of one rule would otherwise
// write the same fields, one small boolean tells them apart; where every
// alternative writes a distinct field, no tag is needed at all.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pddl::ast {

// A term: a constant/object name, or a `?`-prefixed variable -- the raw text
// tells which, so one class covers both.
struct Term : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// `(either a b ...)`, or a single name: both read as a list of primitive
// type names, one element for the plain form.
struct Type : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
};

// One `name+ '-' type` group inside a typed list.
struct TypedGroup : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type = nullptr;
};

// A typed list of names or variables: typed groups first (if any), then any
// number of untyped names -- one shape for both a name list and a variable
// list, only the element (a name or a `?`-variable) differs.
struct TypedList : cursiv::Extends<cursiv::AstNode> {
  vector<TypedGroup*> groups;
  vector<string_view> untyped;
};

// A predicate or function applied to terms, by variable or by name alike:
// `predicate` doubles as the function symbol where the head is a function.
struct AtomicFormula : cursiv::Extends<cursiv::AstNode> {
  string_view predicate;
  vector<Term*> args;
};

struct PredicateDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypedList* parameters = nullptr;
};

struct FunctionDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypedList* parameters = nullptr;
};

// `atomicFunctionSkeleton+ ('-' functionType)?`: a run of function decls
// sharing one optional trailing type (only `number` exists).
struct FunctionGroup : cursiv::Extends<cursiv::AstNode> {
  vector<FunctionDecl*> functions;
  boolean isNumberTyped;
};

struct NameLiteral : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  AtomicFormula* atom = nullptr;
};

// `isTimed` is needed because a literal timed at 0 would otherwise leave
// `timestamp` indistinguishable from an untimed literal's default.
struct InitElement : cursiv::Extends<cursiv::AstNode> {
  NameLiteral* literal = nullptr;  // Literal, TimedLiteral
  boolean isTimed;
  string_view timestamp;           // TimedLiteral, exact numeric spelling
  AtomicFormula* head = nullptr;   // Assign: a function head
  string_view value;               // Assign, exact numeric spelling
};

enum class ArithOp : std::uint8_t { Mul, Add, Sub, Div };

// An expression, including object terms in equality, in an ordinary,
// durative-action or metric context alike, merges into one shape -- only a
// durative action's expression reaches `Duration`, and only a metric's reaches
// `NaryOp`, `TotalTime` and `IsViolated`.
struct Expr : cursiv::Extends<cursiv::AstNode> {
  string_view number;             // exact NUMBER; empty for other expressions
  boolean isDuration;              // '?duration'
  boolean isTotalTime;             // 'total-time'
  string_view violatedName;        // '(is-violated NAME)'
  ArithOp arith{};                 // BinaryOp, NaryOp, Negate
  vector<Expr*> operands;          // BinaryOp (2), NaryOp (2+), Negate (1)
  AtomicFormula* head = nullptr;   // a function application or bare symbol
  string_view variable;            // an object variable, including its ? prefix
};

enum class CompareOp : std::uint8_t { Gt, Lt, Eq, Ge, Le };

// A goal description: `atom` identifies the atomic-formula leaf, `leftExpr`
// the comparison leaf; `isOr`/`isForall` split the two pairs of
// alternatives (and/or, exists/forall) that would otherwise write the same
// fields.
struct GoalFormula : cursiv::Extends<cursiv::AstNode> {
  AtomicFormula* atom = nullptr;     // a bare predicate
  boolean isOr;                      // false: and, true: or
  vector<GoalFormula*> operands;     // and, or
  GoalFormula* notBody = nullptr;    // not
  GoalFormula* left = nullptr;       // imply
  GoalFormula* right = nullptr;      // imply
  boolean isForall;                  // false: exists, true: forall
  TypedList* parameters = nullptr;   // exists, forall
  GoalFormula* quantBody = nullptr;  // exists, forall
  boolean isPreference;
  string_view preferenceName;       // optional ordinary pre-GD preference name
  GoalFormula* preferenceBody = nullptr;
  CompareOp compareOp{};             // a comparison
  Expr* leftExpr = nullptr;          // a comparison
  Expr* rightExpr = nullptr;         // a comparison
};

// A domain-level constraint: every alternative names distinct operators,
// but several share the same field shape (`AtEnd`/`Always`/
// `Sometime`/`AtMostOnce` all take one goal; `Within`/`HoldAfter` a number
// and a goal; `SometimeAfter`/`SometimeBefore` two goals), so `op` says
// which.
enum class ConstraintOp : std::uint8_t {
  And,
  Forall,
  AtEnd,
  Always,
  Sometime,
  Within,
  AtMostOnce,
  SometimeAfter,
  SometimeBefore,
  AlwaysWithin,
  HoldDuring,
  HoldAfter
};

struct Constraint : cursiv::Extends<cursiv::AstNode> {
  ConstraintOp op{};
  vector<Constraint*> operands;     // And
  TypedList* parameters = nullptr;  // Forall
  Constraint* body = nullptr;       // Forall
  string_view num1;  // Within, AlwaysWithin, HoldDuring, HoldAfter
  string_view num2;  // HoldDuring's second bound
  GoalFormula* goal1 = nullptr;
  GoalFormula* goal2 = nullptr;  // SometimeAfter, SometimeBefore, AlwaysWithin
};

// A constraint, optionally named as a preference -- kept apart from
// `Constraint` so a bare domain `:constraints` cannot admit
// `preference`, which only the problem's form allows. `isPreference` is
// needed because an unnamed preference and a plain constraint would
// otherwise both leave only `constraint` set.
struct PrefConstraint : cursiv::Extends<cursiv::AstNode> {
  vector<PrefConstraint*> operands;  // And
  TypedList* parameters = nullptr;   // Forall
  PrefConstraint* body = nullptr;    // Forall
  boolean isPreference;
  string_view name;                  // Preference: the optional NAME
  Constraint* constraint = nullptr;  // Preference, Plain
};

// A durative goal description, in a plain, preference-named or timed form
// alike, merged into one class. `Preference` is told apart from `Forall`
// by having no `parameters`; `AtTime` and `Over` each set a field the
// other never touches.
struct DaGoal : cursiv::Extends<cursiv::AstNode> {
  vector<DaGoal*> operands;         // And
  TypedList* parameters = nullptr;  // Forall
  DaGoal* body = nullptr;           // Forall, Preference
  string_view name;                 // Preference: the optional NAME
  string_view timeSpecifier;        // AtTime: "start" or "end"
  string_view interval;             // Over: "all"
  GoalFormula* goal = nullptr;      // AtTime, Over
};

enum class AssignKind : std::uint8_t {
  Assign,
  ScaleUp,
  ScaleDown,
  Increase,
  Decrease
};

// An effect, at every level of an ordinary or durative action alike,
// merges into one shape; `AtTime` only exists on the durative side.
// `negated` tells a predicate effect from its `(not ...)` twin, which
// would otherwise both leave only `head` set.
struct Effect : cursiv::Extends<cursiv::AstNode> {
  vector<Effect*> operands;          // And, OneOf
  boolean isOneOf;
  TypedList* parameters = nullptr;   // Forall
  Effect* body = nullptr;            // Forall, AtTime
  GoalFormula* condition = nullptr;  // When (plain effect)
  DaGoal* daCondition = nullptr;     // When (durative effect): a durative goal, not a plain one
  Effect* consequence = nullptr;     // When
  AssignKind assignOp{};             // set when `value` is (an Assign)
  AtomicFormula* head = nullptr;     // Assign, NotAtom, Atom
  Expr* value = nullptr;             // Assign
  boolean negated;                   // NotAtom vs. Atom
  string_view timeSpecifier;         // AtTime: "start" or "end"
};

// A duration constraint, merged from its `and`/comparison/timed/empty
// forms: `And`, `Compare` and `AtTime` each set a field the others never
// touch, so no tag is needed; the bare `()` form leaves every field at its
// default.
struct Duration : cursiv::Extends<cursiv::AstNode> {
  vector<Duration*> operands;  // And
  CompareOp compareOp{};       // Compare: always <=, >= or =
  Expr* value = nullptr;       // Compare: durValue
  string_view timeSpecifier;   // AtTime: "start" or "end"
  Duration* body = nullptr;    // AtTime
};

struct ActionDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypedList* parameters = nullptr;
  boolean hasPrecondition;
  boolean hasEffect;
  GoalFormula* precondition = nullptr;  // optional; empty `()` leaves null
  Effect* effect = nullptr;             // optional; empty `()` leaves null
};

struct DurativeActionDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypedList* parameters = nullptr;
  boolean hasCondition;
  boolean hasEffect;
  Duration* duration = nullptr;
  DaGoal* condition = nullptr;
  Effect* effect = nullptr;
};

// A derived predicate keeps its head name, typed parameters and defining goal.
struct DerivedDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypedList* parameters = nullptr;
  GoalFormula* body = nullptr;
};

using StructureItem =
    std::variant<ActionDef*, DurativeActionDef*, DerivedDef*>;

struct Domain : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> requirements;
  TypedList* types = nullptr;
  TypedList* constants = nullptr;
  vector<PredicateDecl*> predicates;
  vector<FunctionGroup*> functionGroups;
  Constraint* constraints = nullptr;
  vector<StructureItem> structures;
};

struct Problem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view domainName;
  vector<string_view> requirements;
  TypedList* objects = nullptr;
  vector<InitElement*> init;
  GoalFormula* goal = nullptr;
  PrefConstraint* constraints = nullptr;
  boolean hasMetric;
  string_view metricOptimization;  // "minimize" or "maximize"
  Expr* metricExpr = nullptr;
};

// The entry: one file holds exactly one domain or one problem.
struct Document : cursiv::Extends<cursiv::AstNode> {
  std::variant<std::monostate, Domain*, Problem*> content;
};

}  // namespace pddl::ast
