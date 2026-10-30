#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace vhdl::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Item : cursiv::Extends<cursiv::AstNode> { string_view endName; };
struct ConcStmt : cursiv::Extends<cursiv::AstNode> { string_view endLabel; };
struct SeqStmt : cursiv::Extends<cursiv::AstNode> { string_view endLabel; };

// ---------- names and expressions ----------
// A name is a postfix chain over an atom: selection, call/index/slice,
// attribute and range-constraint suffixes all nest the same way a C postfix
// expression does, which is why VHDL's separate name/name_part/constraint
// rules collapse into one chain here.
struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// Numeric, string, bit-string, character and null literals: kept as the text
// written, since the value's type depends on context a parser doesn't know.
struct Literal : cursiv::Extends<Expr> {
  string_view text;
  Expr* unit;
};

struct OpenMark : cursiv::Extends<Expr> {};

struct SelectedName : cursiv::Extends<Expr> {
  Expr* prefix;
  string_view suffix;
  boolean all;
};

struct Assoc : cursiv::Extends<cursiv::AstNode> {
  Expr* formal;
  Expr* actual;
  boolean inertial;
};

// `prefix(args)`: indexed name, slice, function call and type conversion are
// one shape in the grammar and one shape here.
struct CallName : cursiv::Extends<Expr> {
  Expr* prefix;
  vector<Assoc*> args;
};

struct Signature;

struct AttributeName : cursiv::Extends<Expr> {
  Expr* prefix;
  string_view designator;
  Signature* signature;
  Expr* arg;
};

struct RangeExpr : cursiv::Extends<Expr> {
  Expr* low;
  string_view direction;
  Expr* high;
};

// A name followed by `range <range>`, the constraint form written without
// parentheses.
struct RangeConstraintName : cursiv::Extends<Expr> {
  Expr* prefix;
  Expr* range;
};

// `resolution? type_mark`: any trailing constraint already lives on
// `typeMark` through the postfix chain above.
struct SubtypeIndication : cursiv::Extends<Expr> {
  Expr* resolution;
  Expr* typeMark;
};

struct ExternalName : cursiv::Extends<Expr> {
  string_view kind;
  string_view root;
  Expr* path;
  SubtypeIndication* subtype;
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

struct CondOpExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct ElementAssoc : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> choices;
  Expr* value;
};

struct Aggregate : cursiv::Extends<Expr> {
  vector<ElementAssoc*> elements;
};

struct QualifiedExpr : cursiv::Extends<Expr> {
  Expr* typeMark;
  Expr* value;
};

struct AllocatorExpr : cursiv::Extends<Expr> {
  Expr* target;
};

struct CondExprEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  Expr* condition;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  vector<CondExprEntry*> entries;
  Expr* elseValue;
};

struct SelExprEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> choices;
  Expr* value;
};

struct SelectedExpr : cursiv::Extends<Expr> {
  vector<SelExprEntry*> entries;
};

// ---------- waveforms and delay ----------
struct WaveformElement : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  Expr* afterTime;
  boolean isNull;
};

struct Waveform : cursiv::Extends<cursiv::AstNode> {
  vector<WaveformElement*> elements;
  boolean unaffected;
};

struct CondWaveEntry : cursiv::Extends<cursiv::AstNode> {
  Waveform* wave;
  Expr* condition;
};

// A plain (non-conditional) waveform is this same shape, one entry with no
// condition.
struct CondWaveAssign : cursiv::Extends<cursiv::AstNode> {
  vector<CondWaveEntry*> entries;
};

struct SelWaveEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> choices;
  Waveform* wave;
};

struct SelectedWaveforms : cursiv::Extends<cursiv::AstNode> {
  vector<SelWaveEntry*> entries;
};

struct DelayMechanism : cursiv::Extends<cursiv::AstNode> {
  boolean transport;
  boolean inertial;
  Expr* rejectExpr;
};

// ---------- declarative items ----------
struct LibraryClause : cursiv::Extends<Item> {
  vector<string_view> names;
};

struct UseClause : cursiv::Extends<Item> {
  vector<Expr*> names;
};

struct ContextRef : cursiv::Extends<Item> {
  vector<Expr*> names;
};

struct ContextDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> context;
};

// One class for every interface_object_declaration kind: the grammar
// distinguishes them only by an optional leading keyword.
struct InterfaceObject : cursiv::Extends<Item> {
  string_view kind;  // "constant" | "signal" | "variable" | "file"
  vector<string_view> names;
  string_view mode;
  SubtypeIndication* subtype;
  Expr* defaultValue;
  boolean bus;
};

struct InterfaceType : cursiv::Extends<Item> {
  string_view name;
};

struct InterfaceSubprogram : cursiv::Extends<Item> {
  string_view kind;  // "procedure" | "function"
  string_view name;
  vector<Item*> params;
  boolean pure;
  boolean impure;
  Expr* returnType;
  Expr* defaultName;
  boolean defaultBox;
};

struct InterfacePackage : cursiv::Extends<Item> {
  string_view name;
  Expr* uninstantiated;
  vector<Assoc*> generics;
  boolean genericBox;
  boolean genericDefault;
};

struct TypeDecl : cursiv::Extends<Item> {
  string_view name;
  Item* definition;  // null: incomplete_type_declaration
};

struct EnumTypeDef : cursiv::Extends<Item> {
  vector<Expr*> literals;
};

struct RangeTypeDef : cursiv::Extends<Item> {
  Expr* range;
};

struct UnitDecl : cursiv::Extends<Item> {
  string_view name;
  Expr* value;  // null for the primary unit
};

struct PhysicalTypeDef : cursiv::Extends<Item> {
  Expr* range;
  vector<UnitDecl*> units;
};

struct ArrayTypeDef : cursiv::Extends<Item> {
  boolean unbounded;
  vector<Expr*> indexTypes;    // unbounded_array_definition index subtypes
  vector<Expr*> indexRanges;   // constrained_array_definition ranges
  SubtypeIndication* element;
};

struct RecordElementDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  SubtypeIndication* subtype;
};

struct RecordTypeDef : cursiv::Extends<Item> {
  vector<RecordElementDecl*> elements;
};

struct AccessTypeDef : cursiv::Extends<Item> {
  SubtypeIndication* subtype;
};

struct FileTypeDef : cursiv::Extends<Item> {
  SubtypeIndication* subtype;
};

struct ProtectedTypeDef : cursiv::Extends<Item> {
  boolean isBody;
  vector<Item*> decls;
};

struct SubtypeDecl : cursiv::Extends<Item> {
  string_view name;
  SubtypeIndication* subtype;
};

struct FileOpenInfo : cursiv::Extends<Item> {
  Expr* openKind;
  Expr* logicalName;
};

// constant / signal / variable / shared variable / file declarations: one
// class, the keyword told apart by `kind`.
struct ObjectDecl : cursiv::Extends<Item> {
  string_view kind;
  boolean shared;
  vector<string_view> names;
  SubtypeIndication* subtype;
  Expr* defaultValue;
  string_view signalKind;  // "register" | "bus"
  FileOpenInfo* fileOpen;
};

struct Signature : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> params;
  Expr* returnType;
};

struct AliasDecl : cursiv::Extends<Item> {
  string_view name;
  SubtypeIndication* subtype;
  Expr* target;
  Signature* signature;
};

struct ComponentDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> generics;
  vector<Item*> ports;
};

struct AttributeDecl : cursiv::Extends<Item> {
  string_view name;
  Expr* typeMark;
};

struct EntityDesignator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Signature* signature;
};

struct AttributeSpec : cursiv::Extends<Item> {
  string_view designator;
  vector<EntityDesignator*> entities;
  boolean allEntities;
  boolean othersEntities;
  string_view entityClass;
  Expr* value;
};

struct BindingIndication : cursiv::Extends<Item> {
  string_view aspectKind;  // "entity" | "configuration" | "open" | ""
  Expr* aspectName;
  string_view archId;
  vector<Assoc*> generics;
  vector<Assoc*> ports;
};

struct ConfigSpec : cursiv::Extends<Item> {
  vector<string_view> instances;
  boolean allInstances;
  boolean othersInstances;
  Expr* componentName;
  BindingIndication* binding;
};

struct DisconnectSpec : cursiv::Extends<Item> {
  vector<Expr*> signals;
  boolean allSignals;
  boolean othersSignals;
  Expr* typeMark;
  Expr* afterTime;
};

struct GroupTemplateEntry : cursiv::Extends<Item> {
  string_view entityClass;
  boolean repeated;
};

struct GroupTemplateDecl : cursiv::Extends<Item> {
  string_view name;
  vector<GroupTemplateEntry*> entries;
};

struct GroupDecl : cursiv::Extends<Item> {
  string_view name;
  Expr* templateName;
  vector<Expr*> constituents;
};

struct SubprogramSpec : cursiv::Extends<Item> {
  string_view kind;  // "procedure" | "function"
  string_view name;
  vector<Item*> headerGenerics;
  vector<Assoc*> headerGenericMap;
  vector<Item*> params;
  boolean pure;
  boolean impure;
  Expr* returnType;
};

struct SubprogramDecl : cursiv::Extends<Item> {
  SubprogramSpec* spec;
};

struct SubprogramBody : cursiv::Extends<Item> {
  SubprogramSpec* spec;
  string_view endKind;  // explicit closing procedure/function, or empty
  vector<Item*> decls;
  vector<SeqStmt*> statements;
};

struct SubprogramInstDecl : cursiv::Extends<Item> {
  string_view kind;
  string_view name;
  Expr* uninstantiated;
  vector<Assoc*> generics;
};

struct PackageDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> headerGenerics;
  vector<Assoc*> headerGenericMap;
  vector<Item*> decls;
};

struct PackageBody : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> decls;
};

struct PackageInstDecl : cursiv::Extends<Item> {
  string_view name;
  Expr* uninstantiated;
  vector<Assoc*> generics;
};

struct EntityDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> generics;
  vector<Item*> ports;
  vector<Item*> decls;
  vector<ConcStmt*> statements;
};

struct ArchitectureBody : cursiv::Extends<Item> {
  string_view name;
  Expr* entityName;
  vector<Item*> decls;
  vector<ConcStmt*> statements;
};

struct BlockConfig : cursiv::Extends<Item> {
  Expr* target;
  vector<Item*> uses;
  vector<Item*> items;  // nested BlockConfig / ComponentConfig
};

struct ComponentConfig : cursiv::Extends<Item> {
  vector<string_view> instances;
  boolean allInstances;
  boolean othersInstances;
  Expr* componentName;
  BindingIndication* binding;
  BlockConfig* block;
};

struct ConfigurationDecl : cursiv::Extends<Item> {
  string_view name;
  Expr* entityName;
  vector<Item*> decls;
  BlockConfig* body;
};

struct DesignUnit : cursiv::Extends<Item> {
  vector<Item*> context;
  Item* unit;
};

struct DesignFile : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> units;
};

// ---------- concurrent statements ----------
struct GenBody : cursiv::Extends<Item> {
  vector<Item*> decls;
  vector<ConcStmt*> statements;
};

struct BlockStmt : cursiv::Extends<ConcStmt> {
  string_view label;
  Expr* guardExpr;
  vector<Item*> generics;
  vector<Assoc*> genericMap;
  vector<Item*> ports;
  vector<Assoc*> portMap;
  vector<Item*> decls;
  vector<ConcStmt*> statements;
};

struct ProcessStmt : cursiv::Extends<ConcStmt> {
  string_view label;
  boolean postponed;
  boolean endPostponed;
  vector<Expr*> sensitivity;
  boolean allSensitivity;
  vector<Item*> decls;
  vector<SeqStmt*> statements;
};

struct ConcProcCall : cursiv::Extends<ConcStmt> {
  string_view label;
  boolean postponed;
  Expr* call;
};

struct ConcAssert : cursiv::Extends<ConcStmt> {
  string_view label;
  boolean postponed;
  Expr* condition;
  Expr* report;
  Expr* severity;
};

// concurrent (and, via SeqSignalAssign below, sequential) signal assignment:
// simple, conditional, selected and their force/release variants share one
// shape, told apart by which optional field is set.
struct SignalAssign : cursiv::Extends<ConcStmt> {
  string_view label;
  boolean postponed;
  Expr* target;
  Expr* selector;  // set: `with <selector> select`
  boolean guarded;
  boolean questioned;
  DelayMechanism* delay;
  boolean force;
  boolean release;
  string_view forceMode;
  Expr* forceValue;  // `force`'s operand: a plain expression only
  CondWaveAssign* condWave;
  SelectedWaveforms* selWave;
};

struct ComponentInst : cursiv::Extends<ConcStmt> {
  string_view label;
  string_view kind;  // "component" | "entity" | "configuration" | ""
  Expr* name;
  string_view archId;
  vector<Assoc*> generics;
  vector<Assoc*> ports;
};

// "for"/"if"/"case" generate, told apart by which fields are set:
// `paramName`, `conditions`, `selector` respectively.
struct GenerateBranch : cursiv::Extends<Item> {
  string_view label;
  Expr* condition;
  GenBody* body;
};

struct GenerateStmt : cursiv::Extends<ConcStmt> {
  string_view label;
  string_view paramName;
  Expr* paramRange;
  GenBody* body;
  vector<GenerateBranch*> branches;
  Expr* selector;
  vector<Item*> caseAlts;  // CaseGenAlt list
};

struct CaseGenAlt : cursiv::Extends<Item> {
  string_view label;
  vector<Expr*> choices;
  GenBody* body;
};

// ---------- sequential statements ----------
struct WaitStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  vector<Expr*> sensitivity;
  Expr* untilCondition;
  Expr* forExpr;
};

struct AssertStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* condition;
  Expr* report;
  Expr* severity;
};

struct ReportStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* report;
  Expr* severity;
};

struct SeqSignalAssign : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* target;
  Expr* selector;
  boolean questioned;
  DelayMechanism* delay;
  boolean force;
  boolean release;
  string_view forceMode;
  Expr* forceValue;
  CondWaveAssign* condWave;
  SelectedWaveforms* selWave;
};

struct SeqVarAssign : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* target;
  Expr* selector;
  boolean questioned;
  Expr* simpleValue;
  ConditionalExpr* condExpr;
  SelectedExpr* selExpr;
};

struct ProcCallStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* call;
};

struct IfBranch : cursiv::Extends<Item> {
  Expr* condition;
  vector<SeqStmt*> body;
};

struct IfStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  vector<IfBranch*> branches;
  vector<SeqStmt*> elseBody;
};

struct CaseAlt : cursiv::Extends<Item> {
  vector<Expr*> choices;
  vector<SeqStmt*> body;
};

struct CaseStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  boolean questioned;
  boolean endQuestioned;
  Expr* selector;
  vector<CaseAlt*> alternatives;
};

struct LoopStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  string_view kind;  // "" | "while" | "for"
  Expr* whileCond;
  string_view forParamName;
  Expr* forRange;
  vector<SeqStmt*> body;
};

struct NextStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  string_view target;
  Expr* condition;
};

struct ExitStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  string_view target;
  Expr* condition;
};

struct ReturnStmt : cursiv::Extends<SeqStmt> {
  string_view label;
  Expr* value;
};

struct NullStmt : cursiv::Extends<SeqStmt> {
  string_view label;
};

}  // namespace vhdl::ast
