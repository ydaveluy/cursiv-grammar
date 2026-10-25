// The arden language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace arden::ast {

// The expression family: an empty base so every operand slot below can be
// typed Expr* regardless of which alternative built it.
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

// ---- Atoms ----

struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// `truth value 0.667`: an explicit fuzzy scalar, distinct from a crisp
// BoolLit even when spelled `truth value true`.
struct TruthValueLit : cursiv::Extends<Expr> {
  float64 value;
};

// now/today/tomorrow, the event/trigger/current-time keywords, a bare
// weekday name, a clock time, or an ISO date or date-time: one
// moment-in-time atom, kept as the text it was spelled with.
struct TimeLit : cursiv::Extends<Expr> {
  string_view text;
};

struct IdentExpr : cursiv::Extends<Expr> {
  string_view name;
};

// `it` / `they`: the implicit subject of a WHERE clause.
struct ItExpr : cursiv::Extends<Expr> {};

struct LocalizedExpr : cursiv::Extends<Expr> {
  string_view term;
  Expr* by;  // optional
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* base;
  Expr* index;
};

struct FieldExpr : cursiv::Extends<Expr> {
  Expr* base;
  string_view field;
};

// The comma-level list every `expr` production is: a lone value is a
// one-item list, `()` an empty one. A leading comma (`, 3`) marks an omitted
// first item; the omission itself is not represented.
struct ExprList : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// ---- Binary operators (MERGE, OR, AND, +-, *`/`, DOR/FORMATTED WITH) ----

enum class BinOp : uint8_t {
  merge,
  logical_or,
  logical_and,
  add,
  subtract,
  multiply,
  divide,
  power,
  concat,          // ||
  formatted_with,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

// ---- String prefix operators ----

struct TrimExpr : cursiv::Extends<Expr> {
  string_view side;  // "left" / "right", empty when unspecified
  Expr* value;
};

struct CaseConvExpr : cursiv::Extends<Expr> {
  boolean upper;  // false: lowercase
  Expr* value;
};

struct SubstringExpr : cursiv::Extends<Expr> {
  Expr* length;
  Expr* startingAt;  // optional
  Expr* source;
};

struct UnaryNot : cursiv::Extends<Expr> {
  Expr* operand;
};

// ---- Comparisons ----

enum class CompareOp : uint8_t {
  eq,
  lt,
  gt,
  le,
  ge,
  ne,
  less_than,
  less_or_equal,
  greater_than,
  greater_or_equal,
  in_list,
  occurs_equal,
  occurs_before,
  occurs_after,
  occurs_within_past,
  occurs_within_sameday,
  occurs_at,
  before_rel,
  after_rel,
  from_rel,
};

struct BinaryComparison : cursiv::Extends<Expr> {
  Expr* left;
  CompareOp op;
  boolean negated;
  Expr* right;
};

enum class TypeCheck : uint8_t {
  present,
  is_null,
  boolean_,
  truth_value,
  number_,
  string_,
  time_,
  duration_,
  list_,
  object_,
  fuzzy_,
  crisp_,
  time_of_day,
  linguistic_variable,
};

struct TypeCheckExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  TypeCheck check;
};

// `x is SomeUserType` / `x is linguistic variable`: an open-ended type name,
// so it cannot join the closed TypeCheck set above.
struct TypeNameCheckExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  string_view typeName;
};

enum class RangeEdge : uint8_t { to, preceding, following, surrounding };

struct RangeComparison : cursiv::Extends<Expr> {
  Expr* subject;
  boolean occurs;   // "occur within" rather than "is within"
  boolean negated;  // "occur not within"
  RangeEdge edge;
  Expr* lo;
  Expr* hi;
};

struct FindInString : cursiv::Extends<Expr> {
  Expr* needle;
  Expr* haystack;
  Expr* startingAt;  // optional
};

struct MatchesPattern : cursiv::Extends<Expr> {
  Expr* value;
  Expr* pattern;
};

// `expr AS NUMBER|TIME|STRING|TRUTH VALUE`
struct CastExpr : cursiv::Extends<Expr> {
  Expr* value;
  string_view asType;
};

// A dozen named-function shapes share one node. `func` selects the operation;
// the other fields retain its operand, source, secondary argument and flags.
struct FuncApply : cursiv::Extends<Expr> {
  string_view func;
  string_view part;  // REPLACE's time part
  Expr* operand;
  Expr* from;
  Expr* extra;    // USING comparator, WITH replacement, or STARTING AT offset
  boolean flag;   // AT LEAST/MOST's ISTRUE|ARETRUE marker
  boolean index;  // an INDEX ... form: the position, not the value
};

struct ObjectInitElement : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct NewObjectExpr : cursiv::Extends<Expr> {
  string_view typeName;
  Expr* args;  // positional constructor args, optional
  vector<ObjectInitElement*> fields;
};

struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  Expr* args;  // optional
};

struct FuzzySetPoint : cursiv::Extends<cursiv::AstNode> {
  Expr* x;
  Expr* y;
};

struct FuzzySetLit : cursiv::Extends<Expr> {
  vector<FuzzySetPoint*> points;
};

struct FuzzifiedExpr : cursiv::Extends<Expr> {
  Expr* value;
  Expr* width;
};

// ---- data-slot read/mapping forms ----

struct ReadExpr : cursiv::Extends<Expr> {
  string_view aggregator;  // the read function's name, empty if none
  Expr* count;             // "LAST n FROM {..}" count, optional
  string_view mapping;     // raw {...} content
  Expr* where;             // optional
  string_view resultName;  // "READ AS name", optional
};

struct MlmRefExpr : cursiv::Extends<Expr> {
  boolean self;
  string_view name;         // TERM text, empty when `self`
  string_view institution;  // optional STRING text
};

struct MappingExpr : cursiv::Extends<Expr> {
  string_view kind;        // interface/event/message/destination
  string_view resultName;  // "AS name", optional
  string_view mapping;     // optional
};

struct ArgumentExpr : cursiv::Extends<Expr> {};

struct ObjectDefExpr : cursiv::Extends<Expr> {
  boolean linguistic;
  vector<string_view> fields;
};

struct FhirObjectRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> vars;  // optional [a, b, ...] destructure
};

struct FhirAccessExpr : cursiv::Extends<cursiv::AstNode> {
  boolean isFind;  // false: `it.path op value`; true: `find value in it.path`
  vector<string_view> path;
  CompareOp op;    // unused when isFind
  boolean negated;
  Expr* value;
};

// ---- Statements (shared across the data/logic/action slot grammars) ----

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  boolean isTime;
  boolean isApplicability;
  // Expr*, NewObjectExpr*, CallExpr*, FuzzySetLit* or FuzzifiedExpr*: read
  // node->type to tell them apart.
  Expr* value;
  boolean isConclude;  // action's `x := conclude`; value is unused
};

struct DestructureAssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
  Expr* value;  // ReadExpr*, ArgumentExpr* or CallExpr*
};

struct IdentReadAssignStmt : cursiv::Extends<Stmt> {
  string_view target;
  // ReadExpr*, MlmRefExpr*, MappingExpr*, ArgumentExpr* or ObjectDefExpr*.
  Expr* value;
};

struct FhirAssignStmt : cursiv::Extends<Stmt> {
  FhirObjectRef* target;
  boolean latest;
  FhirObjectRef* resource;
  vector<FhirAccessExpr*> conditions;
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Stmt*> body;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<ElseIfClause*> elseIfs;
  vector<Stmt*> elseBody;
  boolean hasElse;
  boolean aggregate;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Expr* var;
  Expr* iterable;
  vector<Stmt*> body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  vector<Stmt*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<CaseClause*> cases;
  vector<Stmt*> defaultBody;
  boolean hasDefault;
  boolean aggregate;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct IncludeStmt : cursiv::Extends<Stmt> {
  string_view name;
};
struct ConcludeStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct WriteStmt : cursiv::Extends<Stmt> {
  Expr* value;
  string_view target;  // "AT name", optional
};
struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct CallStmt : cursiv::Extends<Stmt> {
  CallExpr* call;
  Expr* delay;  // optional
};

// ---- Evoke slot ----

struct EvokeTrigger : cursiv::Extends<cursiv::AstNode> {};

struct EventTrigger : cursiv::Extends<EvokeTrigger> {
  vector<string_view> events;
  boolean any;  // `ANY OF (a, b, c)` spelling
};

struct TimeOr : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> times;  // TimeLit* or a duration NumberLit-shaped Expr*
};

struct DelayedEventTrigger : cursiv::Extends<EvokeTrigger> {
  TimeOr* delay;
  string_view event;
};

struct ConstantTimeTrigger : cursiv::Extends<EvokeTrigger> {
  TimeOr* times;        // the bare "at these times" form, optional
  Expr* afterDuration;  // "duration AFTER time" form, optional
  Expr* afterTime;
};

// Extends EvokeTrigger (not a trigger on its own) so it shares `starting`'s
// type below with DelayedEventTrigger and ConstantTimeTrigger.
struct TimeOfEvent : cursiv::Extends<EvokeTrigger> {
  string_view event;
};

struct EveryTrigger : cursiv::Extends<EvokeTrigger> {
  Expr* interval;
  Expr* forDuration;
  EvokeTrigger* starting;  // DelayedEventTrigger*, TimeOfEvent* or ConstantTimeTrigger*
  Expr* until;             // optional
};

// ---- MLM structure ----

struct LangMessage : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view text;
};

struct LanguageSlot : cursiv::Extends<cursiv::AstNode> {
  string_view code;
  vector<LangMessage*> messages;
};

struct Maintenance : cursiv::Extends<cursiv::AstNode> {
  string_view title;
  string_view mlmName;
  string_view version;
  string_view institution;
  string_view author;
  string_view specialist;
  string_view date;
  string_view validation;
};

struct Library : cursiv::Extends<cursiv::AstNode> {
  string_view purpose;
  string_view explanation;
  string_view keywords;
  string_view citations;  // optional
  string_view links;      // optional
};

struct Knowledge : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  vector<Stmt*> dataStatements;
  optional<float64> priority;
  vector<EvokeTrigger*> evokeStatements;
  vector<Stmt*> logicStatements;
  vector<Stmt*> actionStatements;
  Expr* urgency;  // optional
};

struct Resources : cursiv::Extends<cursiv::AstNode> {
  string_view defaultLanguage;
  vector<LanguageSlot*> languages;
};

struct Mlm : cursiv::Extends<cursiv::AstNode> {
  Maintenance* maintenance;
  Library* library;
  Knowledge* knowledge;
  Resources* resources;
};

}  // namespace arden::ast
