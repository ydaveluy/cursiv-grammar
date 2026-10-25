// The kuka language: its AST. A robot program is either a routine module
// (one main procedure/function, zero or more subroutines) or a DEFDAT data
// module; declarations, statements and expressions share three empty bases
// (Item, Stmt, Expr) so heterogeneous lists stay plain vector<Base*>.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace kuka::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Item : cursiv::Extends<cursiv::AstNode> {};

enum class Op : std::uint8_t {
  eq, ne, le, ge, lt, gt,
  log_or, bit_or,
  log_exor, bit_exor,
  log_and, bit_and,
  add, sub, mul, div,
  geom,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

// NOT/B_NOT and unary +/- are two distinct nesting layers -- a sign may not
// wrap NOT -- but they build the same shape.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct Assign : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

// IDENTIFIER plus up to three array-index expressions (`a[i,j,k]`); every
// index is independently optional.
struct VarName : cursiv::Extends<cursiv::AstNode> {
  string_view id;
  Expr* index0;
  Expr* index1;
  Expr* index2;
};

// `a.b[1].c(x, y)`: a dotted chain of names, optionally called.
struct VarRef : cursiv::Extends<Expr> {
  vector<VarName*> path;
  boolean called;
  vector<Expr*> args;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct EnumLit : cursiv::Extends<Expr> {
  string_view name;
};

struct StructElement : cursiv::Extends<cursiv::AstNode> {
  VarName* name;
  Expr* value;
};

struct StructLit : cursiv::Extends<Expr> {
  string_view typeName;  // empty when the `{TypeName: ...}` head is absent
  vector<StructElement*> elements;
};

// A primitive (BOOL/CHAR/INT/REAL) or named (struct/enum) type, optionally
// an array of it; `arraySize` is set only when the brackets carry a count.
struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isArray;
  optional<int64> arraySize;
};

struct ParamType : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  string_view callType;  // a `:`-prefixed call type, e.g. `:IN`; empty when absent
};

struct ForwardDecl : cursiv::Extends<Item> {
  boolean isFunction;   // EXTFCT vs EXT
  string_view name;
  TypeRef* returnType;  // null for a procedure
  vector<ParamType*> params;
};

struct SignalDecl : cursiv::Extends<Item> {
  boolean global;
  boolean isConst;
  string_view name;
  Expr* address;
  Expr* to;  // null when the `TO primary` clause is absent
};

struct TypedDecl : cursiv::Extends<Item> {
  boolean global;   // DEFDAT only
  boolean isConst;  // DEFDAT only
  TypeRef* type;
  vector<VarName*> names;
  Expr* init;  // DEFDAT only; null otherwise
};

struct FieldGroup : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  vector<VarName*> names;
};

struct StructDef : cursiv::Extends<Item> {
  boolean global;
  string_view name;
  vector<FieldGroup*> groups;
};

struct EnumDef : cursiv::Extends<Item> {
  boolean global;
  string_view name;
  vector<string_view> values;
};

struct ArrayInit : cursiv::Extends<Item> {
  string_view name;
  Expr* index0;
  Expr* index1;
  Expr* index2;
  Expr* value;
};

struct ImportDecl : cursiv::Extends<Item> {
  TypeRef* type;
  VarName* localName;
  string_view moduleName;
  VarName* remoteName;
};

enum class SimpleKind : std::uint8_t { continue_, exit_, halt_ };

struct SimpleStmt : cursiv::Extends<Stmt> {
  SimpleKind kind;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view var;
  Expr* from;
  Expr* to;
  string_view stepName;  // any identifier, not necessarily the word STEP
  Expr* step;
  vector<Stmt*> body;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view target;
};

struct LabelStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<Stmt*> elseBody;
};

struct LoopStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
};

struct RepeatStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  Expr* cond;
};

struct CaseGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> values;
  vector<Stmt*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<CaseGroup*> cases;
  boolean hasDefault;
  vector<Stmt*> defaultBody;
};

struct WaitStmt : cursiv::Extends<Stmt> {
  boolean seconds;  // WAIT SEC vs WAIT FOR
  Expr* value;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;  // null when bare `RETURN`
};

struct BrakeStmt : cursiv::Extends<Stmt> {
  string_view target;  // empty when bare `BRAKE`
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct InterruptDeclStmt : cursiv::Extends<Stmt> {
  boolean global;
  Expr* number;
  Expr* condition;
  Expr* action;
};

struct InterruptCtrlStmt : cursiv::Extends<Stmt> {
  string_view mode;  // any identifier, e.g. ON/OFF, not a fixed set
  Expr* number;      // null when the optional operand is absent
};

enum class MotionKind : std::uint8_t { ptp, ptp_rel, lin, lin_rel, circ, circ_rel };
enum class MotionMod : std::uint8_t { none, dis, ori, vel };

struct MotionStmt : cursiv::Extends<Stmt> {
  MotionKind kind;
  Expr* target;
  Expr* auxTarget;    // CIRC's second point; null otherwise
  boolean continuous;  // PTP's C_PTP marker
  MotionMod approx;    // C_DIS / C_ORI / C_VEL, none if absent
  string_view auxName;  // CIRC's optional `, IDENTIFIER primary`
  Expr* auxValue;
  EnumLit* status;  // LIN_REL's optional trailing enumElement
};

struct TriggerStmt : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value;
  Expr* delay;
  Expr* action;
  Expr* priority;  // null when the optional PRIO clause is absent
};

struct AnalogParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct AnalogStmt : cursiv::Extends<Stmt> {
  boolean isInput;      // ANIN vs ANOUT
  string_view channel;
  Expr* value;          // set for `ANOUT channel <expr>`; null otherwise
  string_view linkName;  // set for `ANOUT channel otherChannel`; empty otherwise
  vector<AnalogParam*> params;  // ANOUT's extra `IDENTIFIER '=' literal`*
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  VarName* name;
  string_view callType;
};

struct Routine : cursiv::Extends<cursiv::AstNode> {
  boolean isFunction;  // DEFFCT vs DEF
  boolean global;
  TypeRef* returnType;  // set only for a function
  string_view name;
  vector<Param*> params;
  vector<Item*> data;
  vector<Stmt*> body;
};

struct RoutineModule : cursiv::Extends<cursiv::AstNode> {
  Routine* main;
  vector<Routine*> subs;
};

struct DataModule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isPublic;
  vector<Item*> items;
};

// The document root: exactly one of the two is set.
struct Module : cursiv::Extends<cursiv::AstNode> {
  RoutineModule* routines;
  DataModule* data;
};

}  // namespace kuka::ast
