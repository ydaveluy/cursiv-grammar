#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace verilog::ast {

struct Expr;
struct AttributeSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};
struct AttributeInstance : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpec*> specs;
};
struct Item : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeInstance*> attributes;
};
struct Expr : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeInstance*> attributes;
};
struct Stmt : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeInstance*> attributes;
};

struct PreprocessorItem : cursiv::Extends<Item> { string_view text; };
struct PreprocessorStmt : cursiv::Extends<Stmt> { string_view text; };
struct PreprocessorExpr : cursiv::Extends<Expr> { string_view text; };

// Expressions.

struct HierSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> indices;
};

struct HierIdent : cursiv::Extends<Expr> {
  vector<HierSegment*> prefix;
  string_view name;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* base;
  Expr* index;
};

struct RangeSelectExpr : cursiv::Extends<Expr> {
  Expr* base;
  Expr* msb;
  Expr* lsb;
};

struct PartSelectExpr : cursiv::Extends<Expr> {
  Expr* base;
  Expr* offset;
  string_view op;
  Expr* width;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

// A based number, `8'hFF`: size and base kept as written since their meaning
// (width, signedness, radix) is a semantic reading a parser doesn't do.
struct NumberLit : cursiv::Extends<Expr> {
  string_view size;
  string_view base;
  string_view value;
};

struct RealLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ConcatExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ReplicateExpr : cursiv::Extends<Expr> {
  Expr* count;
  vector<Expr*> items;
};

struct OmittedArg : cursiv::Extends<Expr> {};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct TimingLimits : cursiv::Extends<Expr> {
  vector<Expr*> values;
};

struct MinTypMaxExpr : cursiv::Extends<Expr> {
  Expr* min;
  Expr* typ;
  Expr* max;
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

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

// Declarations.

// `[msb:lsb]`, shared by a range and an array dimension: same syntax.
struct DimRange : cursiv::Extends<cursiv::AstNode> {
  Expr* msb;
  Expr* lsb;
};

struct VarItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<DimRange*> dims;
  Expr* init;
};

struct NetDecl : cursiv::Extends<Item> {
  string_view strength;
  string_view delay;
  string_view expansion;
  string_view netType;
  boolean isSigned;
  DimRange* range;
  vector<VarItem*> items;
};

struct RegDecl : cursiv::Extends<Item> {
  boolean isSigned;
  DimRange* range;
  vector<VarItem*> items;
};

// integer, real, time, realtime.
struct VarDecl : cursiv::Extends<Item> {
  string_view kind;
  vector<VarItem*> items;
};

struct EventDecl : cursiv::Extends<Item> {
  vector<VarItem*> items;
};

struct GenvarDecl : cursiv::Extends<Item> {
  vector<string_view> names;
};

// parameter, localparam, specparam.
struct ParamDecl : cursiv::Extends<Item> {
  string_view kind;
  boolean isSigned;
  string_view type;
  DimRange* range;
  vector<VarItem*> items;
};

struct Assign;

// A defparam assignment reuses Assign: the lvalue is the hierarchical path
// (`inst.WIDTH`), which VarItem's single-name shape cannot hold.
struct ParamOverride : cursiv::Extends<Item> {
  vector<Assign*> items;
};

struct Assign : cursiv::Extends<cursiv::AstNode> {
  Expr* lvalue;
  string_view op;
  Expr* value;
};

struct ContinuousAssign : cursiv::Extends<Item> {
  string_view strength;
  string_view delay;
  vector<Assign*> items;
};

// assign/deassign/force/release on a variable.
struct ProcAssignStmt : cursiv::Extends<Stmt> {
  string_view kind;
  Expr* lvalue;
  Expr* value;
};

// input/output/inout, at module, function, task, or UDP scope alike.
struct PortDecl : cursiv::Extends<Item> {
  string_view direction;
  string_view typeKind;
  boolean isSigned;
  DimRange* range;
  vector<VarItem*> items;
};

// Non-ANSI module port: `.name(expr, ...)` or a bare (possibly braced)
// reference list.
struct PortItem : cursiv::Extends<Item> {
  string_view explicitName;
  vector<Expr*> refs;
};

// Statements.

struct NullStmt : cursiv::Extends<Stmt> {};

struct BlockStmt : cursiv::Extends<Stmt> {
  boolean isFork;
  string_view label;
  vector<Item*> decls;
  vector<Stmt*> stmts;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  string_view control;
  boolean isBlocking;
  Expr* lvalue;
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct CaseItem : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> labels;
  boolean isDefault;
  Stmt* body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  string_view kind;
  Expr* value;
  vector<CaseItem*> items;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Assign* init;
  Expr* cond;
  Assign* step;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct RepeatStmt : cursiv::Extends<Stmt> {
  Expr* count;
  Stmt* body;
};

struct ForeverStmt : cursiv::Extends<Stmt> {
  Stmt* body;
};

struct WaitStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DisableStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct EventTriggerStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

// A task or system-task call, as a statement: a bare reference or a call.
struct TaskEnableStmt : cursiv::Extends<Stmt> {
  Expr* call;
};

// Timing control and the guarded statement are both retained.
struct TimingStmt : cursiv::Extends<Stmt> {
  string_view control;
  Stmt* body;
};

struct InitialConstruct : cursiv::Extends<Item> {
  Stmt* body;
};

struct AlwaysConstruct : cursiv::Extends<Item> {
  Stmt* body;
};

// Functions and tasks.

struct FunctionDecl : cursiv::Extends<Item> {
  boolean automatic;
  boolean isSigned;
  string_view typeKind;
  DimRange* range;
  string_view name;
  vector<PortDecl*> ansiPorts;
  vector<Item*> items;
  Stmt* body;
};

struct TaskDecl : cursiv::Extends<Item> {
  boolean automatic;
  string_view name;
  vector<PortDecl*> ansiPorts;
  vector<Item*> items;
  Stmt* body;
};

// Instantiation.

// An ordered argument has an empty `name`; shared by parameter value
// assignments and port connections, both ordered-or-named lists.
struct NamedArg : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeInstance*> attributes;
  string_view name;
  Expr* value;
};

struct ModuleInstItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  DimRange* range;
  vector<NamedArg*> ports;
};

struct ModuleInst : cursiv::Extends<Item> {
  string_view moduleName;
  vector<NamedArg*> params;
  vector<ModuleInstItem*> instances;
};

struct GateInstItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  DimRange* range;
  vector<Expr*> terminals;
};

struct GateInst : cursiv::Extends<Item> {
  string_view strength;
  string_view delay;
  string_view gateType;
  vector<GateInstItem*> instances;
};

// Generate constructs.

struct GenerateRegion : cursiv::Extends<Item> {
  vector<Item*> items;
};

struct GenerateBlock : cursiv::Extends<Item> {
  string_view label;
  vector<Item*> items;
};

struct ForGenerate : cursiv::Extends<Item> {
  Assign* init;
  Expr* cond;
  Assign* step;
  Item* body;
};

struct IfGenerate : cursiv::Extends<Item> {
  Expr* cond;
  Item* thenBranch;
  Item* elseBranch;
};

struct GenCaseItem : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> labels;
  boolean isDefault;
  Item* body;
};

struct CaseGenerate : cursiv::Extends<Item> {
  Expr* value;
  vector<GenCaseItem*> items;
};

// UDP declaration.

struct UdpRow : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  vector<string_view> inputs;
  string_view current;
  string_view next;
};

struct UdpDecl : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> portNames;
  string_view initName;
  vector<Item*> portDecls;
  Expr* initVal;
  vector<UdpRow*> rows;
};

// Specify block.

struct SpecifyBlock : cursiv::Extends<Item> {
  vector<Item*> items;
};

struct PulseStyleDecl : cursiv::Extends<Item> {
  string_view kind;
  vector<Expr*> outputs;
};

// Covers simple, edge-sensitive and state-dependent path declarations: the
// state-dependent `if (cond)`/`ifnone` guard and the edge-sensitive data
// source and edge are all optional fields of one shape.
struct PathDecl : cursiv::Extends<Item> {
  boolean ifnone;
  Expr* condition;
  string_view edge;
  vector<Expr*> inputs;
  string_view pathOp;  // parallel => or full *>
  string_view polarity;
  vector<Expr*> outputs;
  Expr* dataSource;
  vector<Expr*> delays;
};

struct TimingCheckArg : cursiv::Extends<cursiv::AstNode> {
  string_view control;
  Expr* value;
  Expr* condition;
};

struct SystemTimingCheck : cursiv::Extends<Item> {
  string_view kind;
  vector<TimingCheckArg*> args;
};

// Top level.

struct ModuleDecl : cursiv::Extends<Item> {
  boolean macromodule;
  string_view name;
  vector<Item*> params;
  vector<Item*> ports;
  vector<Item*> items;
};

struct ConfigRule;

struct ConfigDecl : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> designCells;
  vector<ConfigRule*> rules;
};

struct ConfigRule : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  string_view target;
  vector<string_view> liblist;
  string_view useCell;
  boolean useConfig;  // :config explicitly selects a configuration
};

struct SourceText : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> descriptions;
};

}  // namespace verilog::ast
