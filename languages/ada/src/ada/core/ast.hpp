// The Ada language: its AST. No cross-reference resolution -- an Ada
// document routinely names units and entities outside itself (with-ed
// packages, predefined types), so names are kept as text, never
// `reference<>`. Constraints, type definitions, ranges and choices are all
// `Expr` nodes rather than a separate hierarchy, to keep member types
// uniform across the ~150 constructs the grammar distinguishes.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ada::ast {

struct Decl : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Aspect : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value;
};
struct GroupedExpr : cursiv::Extends<Expr> {
  Expr *value;
};

// One writer name, one payload: a discriminant, a parameter, a generic
// formal object, all share this shape.
struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean nullExclusion;
  boolean reverse_;
  boolean isOf;
  vector<string_view> names;
  boolean aliased_;
  boolean constant_;
  string_view mode;  // "in", "out", "in out", or empty
  Expr* type;
  Expr* def;
};

struct Params : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> items;
};

// A record component, mixed with representation clauses in a component
// list -- kept apart from Param so that list can hold one Decl vector.
struct ComponentDecl : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  vector<string_view> names;
  boolean aliased_;
  Expr* type;
  Expr* def;
};

// One positional or named entry: an aggregate item, a call argument, a
// discriminant or generic association. `<>` is a `BoxExpr` value, so no
// separate flag is needed for it.
struct Choice : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> tests;  // labels/choices; empty means positional
  Expr* value;
};

// -- Expressions and names --------------------------------------------

struct Ident : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NumericLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NullLit : cursiv::Extends<Expr> {};
struct OthersExpr : cursiv::Extends<Expr> {};
struct BoxExpr : cursiv::Extends<Expr> {};

struct AllExpr : cursiv::Extends<Expr> {
  Expr* prefix;
};

struct SelectExpr : cursiv::Extends<Expr> {
  Expr* prefix;
  string_view selector;
};

// Indexing, slicing, a call, a generalized indexing or a type conversion:
// telling them apart needs a name's declared kind, which this parser has
// no symbol table to ask, so these forms share one node.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Choice*> args;
};

struct AttrExpr : cursiv::Extends<Expr> {
  Expr* prefix;
  string_view attr;
  Expr* arg;  // null when the attribute takes none
};

struct QualifiedExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

struct RangeExpr : cursiv::Extends<Expr> {
  Expr* low;
  Expr* high;
};

struct SubtypeIndication : cursiv::Extends<Expr> {
  boolean nullExclusion;
  string_view mark;
  Expr* constraint;  // RangeExpr, DigitsConstraint, DeltaConstraint,
                      // IndexConstraint, DiscriminantConstraint or null
};

struct DigitsConstraint : cursiv::Extends<Expr> {
  Expr* digits_;
  Expr* range;
};

struct DeltaConstraint : cursiv::Extends<Expr> {
  Expr* delta_;
  Expr* range;
};

struct IndexConstraint : cursiv::Extends<Expr> {
  vector<Expr*> ranges;
};

struct DiscriminantConstraint : cursiv::Extends<Expr> {
  vector<Choice*> associations;
};

// Every access-type shape collapses into one: a plain access-to-object
// mark, or an access-to-subprogram profile.
struct AccessDef : cursiv::Extends<Expr> {
  boolean all_;
  boolean returnNullExclusion;
  boolean nullExclusion;
  boolean constant_;
  boolean protected_;
  boolean isFunction;
  boolean isSubprogram;
  string_view mark;
  Expr* constraint;  // access-to-object only
  Params* params;
  Expr* returnType;
};

struct Aggregate : cursiv::Extends<Expr> {
  Expr* ancestorOrTag;  // an extension aggregate's ancestor, or a qualified
                        // expression's target type
  boolean isExtension;
  boolean isNullRecord;
  vector<Choice*> items;
};

struct Allocator : cursiv::Extends<Expr> {
  Expr* subpool;
  Expr* target;  // SubtypeIndication or QualifiedExpr
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

enum class Operator : std::uint8_t {
  power,
  abs_,
  not_,
  multiply,
  divide,
  mod_,
  rem_,
  add,
  subtract,
  concat,
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  and_,
  and_then,
  or_,
  or_else,
  xor_,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct MembershipExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  vector<Expr*> choices;
};

struct RaiseExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* message;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  vector<Choice*> elsifs;  // tests[0]=cond, value=then-value
  Expr* elseValue;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr* selector;
  vector<Choice*> alts;  // tests=choices, value=result
};

struct QuantifiedExpr : cursiv::Extends<Expr> {
  boolean forAll;
  Param* var;  // names[0]=loop var; type/def carry the range, or the name
  Expr* predicate;
};

// -- Type definitions ---------------------------------------------------

struct EnumTypeDef : cursiv::Extends<Expr> {
  vector<Expr*> literals;
};

struct IntegerTypeDef : cursiv::Extends<Expr> {
  Expr* range;
};

struct ModularTypeDef : cursiv::Extends<Expr> {
  Expr* modulus;
};

struct FloatTypeDef : cursiv::Extends<Expr> {
  Expr* digits_;
  Expr* range;
};

struct OrdinaryFixedDef : cursiv::Extends<Expr> {
  Expr* delta_;
  Expr* range;
};

struct DecimalFixedDef : cursiv::Extends<Expr> {
  Expr* delta_;
  Expr* digits_;
  Expr* range;
};

struct ArrayTypeDef : cursiv::Extends<Expr> {
  boolean constrained;
  vector<Expr*> indexes;
  boolean aliased_;
  Expr* componentType;
};

struct VariantPart;

struct Variant : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> choices;
  vector<Decl*> components;
  VariantPart* nested;
};

struct VariantPart : cursiv::Extends<cursiv::AstNode> {
  string_view discriminant;
  vector<Variant*> variants;
};

struct RecordTypeDef : cursiv::Extends<Expr> {
  boolean abstract_;
  boolean tagged_;
  boolean limited_;
  boolean isNullRecord;
  vector<Decl*> components;
  VariantPart* variant;
};

struct DerivedTypeDef : cursiv::Extends<Expr> {
  boolean abstract_;
  boolean limited_;
  Expr* base;
  vector<Expr*> interfaces;
  Expr* extension;  // RecordTypeDef or null
};

struct InterfaceTypeDef : cursiv::Extends<Expr> {
  string_view kind;  // "limited"/"task"/"protected"/"synchronized"/""
  vector<Expr*> interfaces;
};

struct PrivateTypeDef : cursiv::Extends<Expr> {
  boolean abstract_;
  boolean tagged_;
  boolean limited_;
};

struct PrivateExtensionDef : cursiv::Extends<Expr> {
  boolean abstract_;
  string_view kind;  // "limited"/"synchronized"/""
  Expr* base;
  vector<Expr*> interfaces;
};

// A generic formal type's own definition, for the shapes with no payload
// beyond a box (`range <>`, `mod <>`, `digits <>`, `delta <>`, `(<>)`).
struct FormalTypeDef : cursiv::Extends<Expr> {
  boolean decimalFixed;
  string_view kind;
};

struct FormalDerivedDef : cursiv::Extends<Expr> {
  boolean synchronized_;
  boolean abstract_;
  boolean limited_;
  Expr* base;
  vector<Expr*> interfaces;
  boolean withPrivate;
};

struct TypeDecl : cursiv::Extends<Decl> {
  boolean unknownDiscriminants;
  vector<Aspect *> aspects;
  string_view name;
  Params* discriminants;
  Expr* def;
  boolean incomplete;
  boolean isTagged;
  boolean private_;
  boolean privateExtension;
};

// -- Declarations ---------------------------------------------------------

struct ObjectDecl : cursiv::Extends<Decl> {
  boolean nullExclusion;
  vector<Aspect *> aspects;
  vector<string_view> names;
  string_view mode;  // formal object declarations only
  boolean aliased_;
  boolean constant_;
  Expr* type;
  Expr* init;
};

struct ExceptionDecl : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  vector<string_view> names;
};

struct RenameDecl : cursiv::Extends<Decl> {
  boolean nullExclusion;
  vector<Aspect *> aspects;
  string_view name;
  Expr* type;   // object/exception renaming's type mark or access definition
  Expr* spec;   // a subprogram specification, passed through
  Expr* target;
  boolean isException;
  boolean isPackage;
  string_view genericKind;  // "package"/"procedure"/"function" or empty
};

struct SubprogramSpec : cursiv::Extends<Expr> {
  boolean returnNullExclusion;
  boolean isFunction;
  boolean overriding_;
  boolean notOverriding;
  string_view name;
  Params* params;
  Expr* returnType;
};

struct SubprogramDecl : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  Expr* spec;
  boolean abstract_;
  boolean isNullProc;
  Expr* exprBody;
};

struct StmtSeq;

struct SubprogramBody : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  Expr* spec;
  vector<Decl*> decls;
  StmtSeq* body;
  string_view endName;
};

struct PackageDecl : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  string_view name;
  vector<Decl*> visible;
  vector<Decl*> private_;
  string_view endName;
};

struct PackageBody : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  string_view name;
  vector<Decl*> decls;
  StmtSeq* body;
  string_view endName;
};

struct GenericDecl : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  vector<Decl*> formals;
  Expr* subprogSpec;
  Decl* packageSpec;
};

struct GenericFormalSubprogram : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  Expr* spec;
  Expr* default_;
  boolean isBox;
  boolean abstract_;
};

struct GenericFormalPackage : cursiv::Extends<Decl> {
  boolean othersBox;
  vector<Aspect *> aspects;
  string_view name;
  string_view instantiated;
  vector<Choice*> actuals;
  boolean isBox;
};

struct GenericInstantiation : cursiv::Extends<Decl> {
  boolean overriding_;
  boolean notOverriding;
  vector<Aspect *> aspects;
  string_view kind;  // "package"/"procedure"/"function"
  string_view name;
  string_view generic;
  vector<Choice*> actuals;
};

struct WithClause : cursiv::Extends<cursiv::AstNode> {
  boolean limited_;
  boolean private_;
  vector<string_view> names;
};

struct UseClause : cursiv::Extends<Decl> {
  boolean isType;
  boolean all_;
  vector<string_view> names;
};

struct RecordCompClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* position;
  Expr* first;
  Expr* last;
};

struct AspectClause : cursiv::Extends<Decl> {
  boolean isEnumRep;
  boolean isRecordRep;
  boolean isAt;
  string_view name;
  string_view attr;
  Expr* value;
  Expr* modClause;
  vector<RecordCompClause*> components;
};

struct EntryDecl : cursiv::Extends<Decl> {
  boolean overriding_;
  boolean notOverriding;
  vector<Aspect *> aspects;
  string_view name;
  Expr* familyRange;
  Params* params;
};

struct EntryBody : cursiv::Extends<Decl> {
  string_view endName;
  string_view name;
  Param* index;
  Params* params;
  Expr* barrier;
  vector<Decl*> decls;
  StmtSeq* body;
};

struct TaskSpec : cursiv::Extends<Decl> {
  string_view endName;
  vector<Aspect *> aspects;
  string_view name;
  boolean isType;
  Params* discriminants;
  vector<Expr*> interfaces;
  boolean hasDef;
  vector<Decl*> items;
  vector<Decl*> privateItems;
};

struct TaskBody : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  string_view name;
  vector<Decl*> decls;
  StmtSeq* body;
  string_view endName;
};

struct ProtectedSpec : cursiv::Extends<Decl> {
  string_view endName;
  vector<Aspect *> aspects;
  string_view name;
  boolean isType;
  Params* discriminants;
  vector<Expr*> interfaces;
  vector<Decl*> items;
  vector<Decl*> privateItems;
};

struct ProtectedBody : cursiv::Extends<Decl> {
  string_view endName;
  vector<Aspect *> aspects;
  string_view name;
  vector<Decl*> items;
};

struct BodyStub : cursiv::Extends<Decl> {
  vector<Aspect *> aspects;
  Expr* spec;  // subprogram spec, null for package/task/protected stubs
  string_view name;
};

struct Subunit : cursiv::Extends<Decl> {
  string_view parent;
  Decl* body;
};

struct PragmaDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Choice*> args;
};

struct PragmaStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Choice*> args;
};

// -- Statements -------------------------------------------------------

struct CondBlock : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Stmt*> body;
};

struct ExcHandler : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> choices;
  vector<Stmt*> body;
};

struct StmtSeq : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
  vector<ExcHandler*> handlers;
};

struct NullStmt : cursiv::Extends<Stmt> {};

struct LabeledStmt : cursiv::Extends<Stmt> {
  vector<string_view> labels;
  Stmt* inner;  // null for labels at the end of a statement sequence
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<CondBlock*> elsifs;
  vector<Stmt*> elseBody;
};

struct CaseAlt : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> choices;
  vector<Stmt*> body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* selector;
  vector<CaseAlt*> alts;
};

struct ForSpec : cursiv::Extends<cursiv::AstNode> {
  string_view var;
  boolean reverse_;
  boolean isIterator;
  boolean isOf;
  Expr* range;
  Expr* subtypeIndication;
};

struct LoopStmt : cursiv::Extends<Stmt> {
  string_view label;
  Expr* whileCond;
  ForSpec* forSpec;
  vector<Stmt*> body;
  string_view endLabel;
};

struct BlockStmt : cursiv::Extends<Stmt> {
  string_view endName;
  string_view label;
  vector<Decl*> decls;
  StmtSeq* body;
};

struct ExitStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* cond;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ExtendedReturnStmt : cursiv::Extends<Stmt> {
  Decl* obj;
  StmtSeq* body;
};

struct ProcCallStmt : cursiv::Extends<Stmt> {
  Expr* target;
  vector<Choice*> args;
};

struct RequeueStmt : cursiv::Extends<Stmt> {
  Expr* target;
  boolean abort_;
};

struct DelayStmt : cursiv::Extends<Stmt> {
  Expr* value;
  boolean until_;
};

struct AbortStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct RaiseStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* message;
};

struct AcceptStmt : cursiv::Extends<Stmt> {
  string_view endName;
  string_view name;
  Expr* index;
  Params* params;
  StmtSeq* body;
};

// The four select forms share enough shape (guarded alternatives, an else
// body) to fit one node: a triggering accept/delay/entry-call/terminate
// starts each alternative's body, followed by the rest of its statements.
struct SelectStmt : cursiv::Extends<Stmt> {
  boolean isTimed;
  boolean isConditional;
  boolean isAsynchronous;
  vector<CondBlock*> alts;
  vector<Stmt*> elseBody;  // ELSE body, or the abortable statements of THEN ABORT
};

struct TerminateStmt : cursiv::Extends<Stmt> {};

// -- Compilation ----------------------------------------------------------

struct Unit : cursiv::Extends<cursiv::AstNode> {
  vector<WithClause*> withClauses;
  vector<UseClause*> useClauses;
  vector<Decl*> pragmas;  // among the context items, not the unit's own
  boolean private_;
  Decl* body;
};

struct Compilation : cursiv::Extends<cursiv::AstNode> {
  vector<Unit*> units;
};

}  // namespace ada::ast
