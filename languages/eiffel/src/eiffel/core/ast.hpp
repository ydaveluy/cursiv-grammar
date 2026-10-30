// The eiffel language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace eiffel::ast {

struct NoteEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> values;
};

struct Notes : cursiv::Extends<cursiv::AstNode> {
  vector<NoteEntry*> entries;
};

struct Type : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;  // attached, detachable, separate, expanded
};

// A class name or a formal generic name, with optional actual generics --
// the two share one shape here.
struct ClassType : cursiv::Extends<Type> {
  string_view attachment;  // "?" or "!" or empty
  string_view name;
  vector<Type*> generics;
};

struct TupleType : cursiv::Extends<Type> {
  vector<struct EntityDeclGroup*> params;
};

struct Expr;
struct Assertion;
struct Alias;
struct RenamePair;
struct GenericConstraint;

struct AnchoredType : cursiv::Extends<Type> {
  string_view attachment;
  Expr* anchor;
};

struct EntityDeclGroup : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type;
};

struct FormalGeneric : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;  // frozen, reference
  boolean question;
  string_view name;
  vector<GenericConstraint*> constraints;
  vector<string_view> creators;
};

// ---- Expressions ----

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
  boolean called;  // actuals were present, even if empty
};

struct QualifiedCall : cursiv::Extends<Expr> {
  Expr* target;
  string_view member;
  vector<Expr*> args;
  boolean called;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* target;
  vector<Expr*> indices;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct IntervalExpr : cursiv::Extends<Expr> {
  Expr* first;
  Expr* last;
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

struct ParenTargetExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

struct QuantifierExpr : cursiv::Extends<Expr> {
  string_view kind;  // "forall" | "forsome"
  string_view name;
  Expr* domain;
  Expr* body;
};

struct CurrentExpr : cursiv::Extends<Expr> {};
struct ResultExpr : cursiv::Extends<Expr> {};
struct VoidExpr : cursiv::Extends<Expr> {};

struct AttachedExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* value;
  string_view asName;
};

struct AcrossExpr : cursiv::Extends<Expr> {
  Expr* source;
  string_view varName;
  boolean isAll;  // ALL vs SOME
  Expr* body;
};

struct OnceExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* value;
};

struct CreationExpr : cursiv::Extends<Expr> {
  Type* type;
  string_view member;
  vector<Expr*> args;
  boolean called;
};

struct CondBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  Expr* value;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  vector<CondBranch*> branches;
  Expr* elseValue;
};

struct NonObjectCallExpr : cursiv::Extends<Expr> {
  Type* type;
  string_view member;
  vector<Expr*> args;
  boolean called;
};

struct ManifestConstant : cursiv::Extends<Expr> {
  Type* type;    // optional leading '{' type '}'
  string_view text;
};

struct ManifestArray : cursiv::Extends<Expr> {
  Type* type;
  vector<Expr*> items;
};

struct ManifestTuple : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct AgentActual : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean placeholder;
  Type* placeholderType;
};

struct RoutineBody;

struct Agent : cursiv::Extends<Expr> {
  Expr* target;
  string_view member;
  vector<AgentActual*> args;
  boolean called;
  vector<EntityDeclGroup*> inlineParams;
  Type* inlineType;
  RoutineBody* inlineBody;
};

struct ObjectTest : cursiv::Extends<Expr> {
  string_view name;
  Type* type;
  Expr* value;
};

struct OnceStringExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct AddressExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// ---- Instructions ----

struct Instruction : cursiv::Extends<cursiv::AstNode> {};

struct Compound : cursiv::Extends<cursiv::AstNode> {
  vector<Instruction*> items;
};

struct Precursor : cursiv::Extends<Instruction> {
  string_view parentClass;
  vector<Expr*> args;
  boolean called;
};

struct PrecursorExpr : cursiv::Extends<Expr> {
  Precursor* value;
};

struct ExprInstruction : cursiv::Extends<Instruction> {
  Expr* value;
};

struct Assignment : cursiv::Extends<Instruction> {
  Expr* target;
  Expr* value;
};

struct CreationInstruction : cursiv::Extends<Instruction> {
  string_view passiveRegionClass;
  Type* type;
  Expr* target;
  string_view member;
  vector<Expr*> args;
  boolean called;
};

struct IfBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  Compound* body;
};

struct IfInstruction : cursiv::Extends<Instruction> {
  vector<IfBranch*> branches;
  Compound* elseBody;
};

struct Choice : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;
};

struct WhenBranch : cursiv::Extends<cursiv::AstNode> {
  vector<Choice*> choices;
  Compound* body;
};

struct InspectInstruction : cursiv::Extends<Instruction> {
  Expr* subject;
  vector<WhenBranch*> branches;
  Compound* elseBody;
};

struct LoopInstruction : cursiv::Extends<Instruction> {
  string_view acrossVar;
  Expr* acrossSource;
  Compound* initBody;
  Assertion* invariant;
  Expr* variantBefore;
  string_view variantBeforeTag;
  Expr* untilCond;
  Compound* loopBody;
  Expr* allCond;
  Expr* someCond;
  Expr* variantAfter;
  string_view variantAfterTag;
};

struct DebugInstruction : cursiv::Extends<Instruction> {
  vector<string_view> keys;
  Compound* body;
};

struct AssertionClause : cursiv::Extends<cursiv::AstNode> {
  string_view tag;
  Expr* condition;
  boolean isClassMark;
};

struct Assertion : cursiv::Extends<cursiv::AstNode> {
  vector<AssertionClause*> clauses;
};

struct CheckInstruction : cursiv::Extends<Instruction> {
  Assertion* assertion;
  Notes* notes;
};

struct RetryInstruction : cursiv::Extends<Instruction> {};

struct ConditionalCheckInstruction : cursiv::Extends<Instruction> {
  Assertion* assertion;
  Compound* body;
};

struct InlineSeparateArg : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  string_view name;
};

struct InlineSeparateInstruction : cursiv::Extends<Instruction> {
  vector<InlineSeparateArg*> args;
  Compound* body;
};

struct IteratorInstruction : cursiv::Extends<Instruction> {
  string_view name;
  Expr* source;
  Compound* body;
};

// ---- Feature declarations ----

struct RoutineBody : cursiv::Extends<cursiv::AstNode> {
  bool requireElse = false;
  bool ensureThen = false;
  Assertion* precondition;
  vector<EntityDeclGroup*> locals;
  string_view kind;  // "deferred" | "do" | "once" | "external" | "attribute"
  Compound* body;
  vector<string_view> onceKeys;
  string_view externalLanguage;
  string_view externalAliasName;
  Assertion* postcondition;
  vector<string_view> postOnly;
  Compound* rescueBody;
};

struct Alias : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  boolean convert;
};

struct NewFeatureName : cursiv::Extends<cursiv::AstNode> {
  boolean frozen;
  string_view name;
  vector<Alias*> aliases;
};

struct FeatureValue : cursiv::Extends<cursiv::AstNode> {
  string_view explicitValue;
  string_view obsoleteMessage;
  RoutineBody* body;
  boolean unique;
};

struct FeatureDeclaration : cursiv::Extends<cursiv::AstNode> {
  vector<NewFeatureName*> names;
  vector<EntityDeclGroup*> args;
  Type* type;
  string_view assignerName;
  Notes* notes;
  FeatureValue* value;
};

struct FeatureClause : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> clients;
  vector<FeatureDeclaration*> features;
};

// ---- Inheritance ----

struct RenamePair : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;
  vector<Alias*> toAliases;
};

struct GenericConstraint : cursiv::Extends<cursiv::AstNode> {
  Type* type;
  vector<RenamePair*> renames;
};

struct ExportItem : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> clients;
  vector<string_view> features;
  boolean all;
};

struct Parent : cursiv::Extends<cursiv::AstNode> {
  Type* type;
  vector<string_view> undefineNames;
  vector<string_view> redefineNames;
  vector<RenamePair*> renames;
  vector<ExportItem*> exports;
  vector<string_view> selectNames;
};

struct InheritClause : cursiv::Extends<cursiv::AstNode> {
  string_view nonConformanceClass;
  vector<Parent*> parents;
};

struct CreationClause : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> clients;
  vector<string_view> procedures;
};

struct Converter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Type*> types;
  boolean isQuery;
};

// ---- Class ----

struct ClassDecl : cursiv::Extends<cursiv::AstNode> {
  Notes* notesTop;
  vector<string_view> headerMarks;
  vector<string_view> onceKeys;
  string_view name;
  vector<FormalGeneric*> generics;
  string_view obsoleteMessage;
  vector<InheritClause*> inheritance;
  vector<CreationClause*> creators;
  vector<Converter*> converters;
  vector<FeatureClause*> features;
  Notes* notesMid;
  Assertion* invariant;
  Notes* notesBottom;
};

}  // namespace eiffel::ast
