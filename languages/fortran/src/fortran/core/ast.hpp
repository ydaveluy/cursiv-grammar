// The fortran language: its AST. Fortran keywords are never reserved, so
// program units, specifications and executable statements share broad,
// generic node shapes rather than one class per sub-rule.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fortran::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct LabelledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* statement;
};
struct ProgramUnit : cursiv::Extends<cursiv::AstNode> {};

// -- expressions ----

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
  string_view kind;  // a trailing `_kind`, empty when absent
};

struct RealLit : cursiv::Extends<Expr> {
  string_view text;
  string_view kind;
};

struct BozLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
  string_view kind;
};

struct HollerithLit : cursiv::Extends<Expr> {
  string_view text;
};

// The bare `*`: an assumed-size dimension, a list-directed unit or format,
// an alternate-return dummy argument.
struct StarExpr : cursiv::Extends<Expr> {};

struct LogicalLit : cursiv::Extends<Expr> {
  boolean value;
  string_view kind;
};

// A subscript, a section (`lower:upper:stride`, any part optional) or a
// keyword argument (`name = value`, `value` reused for the value): array
// indexing, structure components and calls share one shape without a symbol
// table to tell them apart.
struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
  Expr* upper;
  Expr* stride;
  boolean isTriplet;
};

struct FieldAccess : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Argument*> args;
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
  vector<Argument*> args;
  vector<FieldAccess*> fields;
};

struct TypeSpec;

struct ComplexLit : cursiv::Extends<Expr> {
  Expr* real;
  Expr* imag;
};

struct ArrayCtor : cursiv::Extends<Expr> {
  TypeSpec* type;
  vector<Expr*> items;
};

struct ImpliedDo : cursiv::Extends<Expr> {
  vector<Expr*> items;
  string_view var;
  Expr* start;
  Expr* stop;
  Expr* step;
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

// -- shared shapes ----

struct TypeParameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // KIND, LEN, *, or empty for positional selectors
  Expr* value;
};

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {
  vector<TypeParameter*> parameters;
  string_view keyword;
  string_view typeName;
};

struct Dim : cursiv::Extends<cursiv::AstNode> {
  Expr* lower;
  Expr* upper;
  boolean assumedSize;
};

struct ArraySpec : cursiv::Extends<cursiv::AstNode> {
  vector<Dim*> dims;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ArraySpec* shape;
  Expr* charLength;
  Expr* init;
  boolean pointerInit;  // `=>` rather than `=`
};

// A control spec of an I/O statement (`UNIT=`, `FMT=`, `IOSTAT=`, a bare
// positional unit or format, ...): OPEN/CLOSE/READ/WRITE/PRINT/INQUIRE/
// BACKSPACE/REWIND/ENDFILE share this shape.
struct IoSpec : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  Expr* value;
};

// -- specification statements ----

struct TypeDecl : cursiv::Extends<Stmt> {
  TypeSpec* type;
  vector<string_view> attrs;
  ArraySpec* dimAttr;
  string_view intentAttr;
  vector<Declarator*> declarators;
};

struct ParameterDecl : cursiv::Extends<Stmt> {
  vector<Declarator*> items;
};

struct DimensionDecl : cursiv::Extends<Stmt> {
  vector<Declarator*> items;
};

struct CommonGroup : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Declarator*> items;
};

struct CommonDecl : cursiv::Extends<Stmt> {
  vector<CommonGroup*> groups;
};

// DATA's `repeat * constant`, kept without expanding the value list.
struct DataRepeat : cursiv::Extends<Expr> {
  Expr* count;
  Expr* value;
};

struct DataSet : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> objects;
  vector<Expr*> values;
};

struct DataDecl : cursiv::Extends<Stmt> {
  vector<DataSet*> sets;
};

// SAVE, ACCESS (PRIVATE/PUBLIC), ALLOCATABLE, OPTIONAL, POINTER, TARGET,
// EXTERNAL, INTRINSIC and INTENT share this shape: a keyword, an optional
// parenthesised qualifier (INTENT's IN/OUT/INOUT) and a name list.
struct AttrDecl : cursiv::Extends<Stmt> {
  string_view keyword;
  string_view qualifier;
  vector<string_view> items;
};

struct NamelistGroup : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> items;
};

struct NamelistDecl : cursiv::Extends<Stmt> {
  vector<NamelistGroup*> groups;
};

struct EquivSet : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> items;
};

struct EquivalenceDecl : cursiv::Extends<Stmt> {
  vector<EquivSet*> sets;
};

struct ImplicitDecl : cursiv::Extends<Stmt> {
  boolean none;
  vector<string_view> noneOptions;
  vector<TypeSpec*> specs;
  vector<string_view> letters;
};

struct Rename : cursiv::Extends<cursiv::AstNode> {
  string_view local;
  string_view use;
};

struct UseDecl : cursiv::Extends<Stmt> {
  string_view nature;
  string_view moduleName;
  boolean onlyMode;
  vector<Rename*> items;
};

struct IncludeDecl : cursiv::Extends<Stmt> {
  string_view path;
};

struct FormatToken;

struct FormatDecl : cursiv::Extends<Stmt> {
  string_view label;
  FormatToken* spec;
};

struct EntryDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<string_view> params;
  string_view result;
};

struct ModuleProcRef : cursiv::Extends<ProgramUnit> {
  vector<string_view> names;
};

struct InterfaceDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<ProgramUnit*> procedures;
};

struct Component : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* type;
  vector<string_view> attrs;
  ArraySpec* dimAttr;
  vector<Declarator*> declarators;
};

struct DerivedTypeDecl : cursiv::Extends<Stmt> {
  string_view name;
  string_view access;
  boolean isSequence;
  boolean isPrivate;
  vector<Component*> components;
};

// -- executable statements ----

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct PointerAssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct LabelAssignStmt : cursiv::Extends<Stmt> {
  string_view label;
  string_view variable;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ComputedGotoStmt : cursiv::Extends<Stmt> {
  vector<string_view> labels;
  Expr* selector;
};

struct AssignedGotoStmt : cursiv::Extends<Stmt> {
  string_view variable;
  vector<string_view> labels;
};

struct ArithmeticIfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  string_view negLabel;
  string_view zeroLabel;
  string_view posLabel;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* action;
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Stmt*> body;
};

struct BlockIfStmt : cursiv::Extends<Stmt> {
  string_view name;
  Expr* cond;
  vector<Stmt*> thenBody;
  vector<ElseIfClause*> elseIfs;
  vector<Stmt*> elseBody;
  boolean hasElse;
};

struct DoStmt : cursiv::Extends<Stmt> {
  string_view name;
  string_view endLabel;
  string_view var;
  Expr* start;
  Expr* stop;
  Expr* step;
  Expr* whileCond;
  vector<Stmt*> body;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<Argument*> values;
  boolean isDefault;
  vector<Stmt*> body;
};

struct SelectCaseStmt : cursiv::Extends<Stmt> {
  string_view name;
  Expr* selector;
  vector<CaseClause*> cases;
};

struct WhereBlock : cursiv::Extends<cursiv::AstNode> {
  Expr* mask;
  vector<Stmt*> body;
};

struct WhereStmt : cursiv::Extends<Stmt> {
  vector<WhereBlock*> blocks;
};

struct CallStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Argument*> args;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct StopStmt : cursiv::Extends<Stmt> {
  string_view code;
  boolean isPause;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct CycleStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct ExitStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct NullifyStmt : cursiv::Extends<Stmt> {
  vector<Expr*> items;
};

struct AllocateStmt : cursiv::Extends<Stmt> {
  boolean isAllocate;
  vector<Expr*> items;
  Expr* stat;
};

struct StmtFunctionStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<string_view> params;
  Expr* value;
};

// READ, WRITE, PRINT, OPEN, CLOSE, INQUIRE, BACKSPACE, REWIND, ENDFILE: one
// keyword, a control-spec list and an item list.
struct IoStmt : cursiv::Extends<Stmt> {
  string_view keyword;
  vector<IoSpec*> specs;
  vector<Expr*> items;
};

// -- program units ----

struct FunctionUnit : cursiv::Extends<ProgramUnit> {
  boolean recursive;
  vector<string_view> qualifiers;
  TypeSpec* returnType;
  string_view name;
  vector<string_view> params;
  string_view resultName;
  vector<Stmt*> body;
  vector<ProgramUnit*> internals;
};

struct SubroutineUnit : cursiv::Extends<ProgramUnit> {
  boolean recursive;
  vector<string_view> qualifiers;
  string_view name;
  vector<string_view> params;
  vector<Stmt*> body;
  vector<ProgramUnit*> internals;
};

struct MainProgramUnit : cursiv::Extends<ProgramUnit> {
  string_view name;
  vector<Stmt*> body;
  vector<ProgramUnit*> internals;
};

struct ModuleUnit : cursiv::Extends<ProgramUnit> {
  string_view name;
  vector<Stmt*> body;
  vector<ProgramUnit*> subprograms;
};

struct BlockDataUnit : cursiv::Extends<ProgramUnit> {
  string_view name;
  vector<Stmt*> body;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  vector<ProgramUnit*> units;
};

// FORMAT edit descriptors, separators and parenthesized sublists in source
// order.
struct FormatToken : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<string_view, FormatToken*>> parts;
};

}  // namespace fortran::ast
