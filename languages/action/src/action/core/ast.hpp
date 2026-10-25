// The Action! language: its AST.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace action::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Raw numeric or character text: Action! numbers are decimal, `$`-prefixed
// hex, or a bare character constant, and their meaning depends on context, so
// the spelling is kept rather than folded to one representation.
struct NumberLiteral : cursiv::Extends<Expr> {
  string_view text;
};

// The base of every memory reference: a plain variable, an array element, a
// pointer dereference, a record field, or an `@` address-of. All four are
// also expressions (they read a value) and, outside expressions, the target
// of an assignment.
struct MemRef : cursiv::Extends<Expr> {};

struct FundRef : cursiv::Extends<MemRef> {
  string_view name;
};

struct ArrRef : cursiv::Extends<MemRef> {
  string_view name;
  Expr* index = nullptr;
};

struct PtrRef : cursiv::Extends<MemRef> {
  string_view name;
};

struct RecRef : cursiv::Extends<MemRef> {
  string_view name;
  string_view field;
};

struct AddrRef : cursiv::Extends<MemRef> {
  string_view name;
};

struct GroupedExpr : cursiv::Extends<Expr> {
  Expr* expr = nullptr;
};

struct UnaryMinus : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
};

struct StrLiteral : cursiv::Extends<Expr> {
  string_view text;
};

enum class ArithOp : std::uint8_t { add, sub, mul, div, mod, lsh, rsh, bxor };

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  ArithOp op{};
  Expr* right = nullptr;
};

// A run of the same items a compile-time constant address or an initializer
// list is made of: identifiers, numbers, `^` and `*`, kept as text because
// their arithmetic is a linker concern, not a parse one.
struct RawValues : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> items;
};

// Any condition an `IF`/`WHILE` reads.
struct Cond : cursiv::Extends<cursiv::AstNode> {};

enum class RelOp : std::uint8_t {
  eq,
  not_equal,
  lt,
  le,
  gt,
  ge,
};

// A single comparison. Also the base a boolean chain (`AND`/`OR`) links, and,
// read alone with no chain, a whole condition by itself.
struct Compare : cursiv::Extends<Cond> {
  Expr* left = nullptr;
  RelOp op{};
  Expr* right = nullptr;
};

enum class BoolOp : std::uint8_t { and_, or_, amp, pct };

struct BoolChain : cursiv::Extends<Cond> {
  Cond* left = nullptr;
  BoolOp op{};
  Cond* right = nullptr;
};

// `IF flags(i) THEN`: a bare arithmetic expression read as a truth value,
// with no comparison at all.
struct ArithCond : cursiv::Extends<Cond> {
  Expr* expr = nullptr;
};

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

enum class AssignOp : std::uint8_t { set, inc, dec };

struct AssignStmt : cursiv::Extends<Stmt> {
  MemRef* target = nullptr;
  AssignOp op{};
  Expr* value = nullptr;
};

struct ExitStmt : cursiv::Extends<Stmt> {};

struct RoutineCallStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> args;
};

struct ElseIf : cursiv::Extends<cursiv::AstNode> {
  Cond* cond = nullptr;
  vector<Stmt*> body;
};

struct ElseBranch : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Cond* cond = nullptr;
  vector<Stmt*> body;
  vector<ElseIf*> elseifs;
  ElseBranch* else_branch = nullptr;
};

struct DoLoop : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
  Cond* until_cond = nullptr;
};

struct DoLoopStmt : cursiv::Extends<Stmt> {
  DoLoop* loop = nullptr;
};

struct WhileLoop : cursiv::Extends<Stmt> {
  Cond* cond = nullptr;
  DoLoop* body = nullptr;
};

struct ForLoop : cursiv::Extends<Stmt> {
  string_view var;
  Expr* start = nullptr;
  Expr* finish = nullptr;
  Expr* step = nullptr;
  DoLoop* body = nullptr;
};

struct CodeBlock : cursiv::Extends<Stmt> {
  RawValues* values = nullptr;
};

// DEFINE/TYPE/variable declarations. A declaration's own repetition (one or
// more of the same shape, no separator) is flattened here: each entry below
// appears directly, as many times as written, instead of nested inside an
// extra grouping node.
struct Decl : cursiv::Extends<cursiv::AstNode> {};

struct DefineEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct DefineDecl : cursiv::Extends<Decl> {
  vector<DefineEntry*> defs;
};

struct VarIdent : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  RawValues* init = nullptr;
};

struct FieldDecl : cursiv::Extends<cursiv::AstNode> {
  string_view fundtype;
  vector<VarIdent*> idents;
};

struct TypeRecord : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<FieldDecl*> fields;
};

struct TypeDecl : cursiv::Extends<Decl> {
  vector<TypeRecord*> records;
};

struct FundDecl : cursiv::Extends<Decl> {
  string_view fundtype;
  vector<VarIdent*> idents;
};

struct PtrIdent : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  RawValues* init = nullptr;
};

struct PointerDecl : cursiv::Extends<Decl> {
  string_view ptrtype;
  vector<PtrIdent*> idents;
};

struct ArrIdent : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* dim = nullptr;
  RawValues* init = nullptr;
  string_view str;
};

struct ArrayDecl : cursiv::Extends<Decl> {
  string_view fundtype;
  vector<ArrIdent*> idents;
};

struct RecIdent : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  RawValues* address = nullptr;
};

struct RecordDecl : cursiv::Extends<Decl> {
  string_view type_name;
  vector<RecIdent*> idents;
};

struct Routine : cursiv::Extends<cursiv::AstNode> {};

struct ProcRoutine : cursiv::Extends<Routine> {
  string_view name;
  RawValues* addr = nullptr;
  vector<Decl*> params;
  vector<Decl*> decls;
  vector<Stmt*> body;
};

struct FuncRoutine : cursiv::Extends<Routine> {
  string_view fundtype;
  string_view name;
  RawValues* addr = nullptr;
  vector<Decl*> params;
  vector<Decl*> decls;
  vector<Stmt*> body;
  Expr* return_expr = nullptr;
};

struct ProgModule : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  vector<Routine*> routines;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<ProgModule*> modules;
};

}  // namespace action::ast
