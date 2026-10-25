// The gdscript language: its AST, covering GDScript 1.x class bodies
// (declarations), statements and expressions.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace gdscript::ast {

struct TopLevelDecl : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Pattern : cursiv::Extends<cursiv::AstNode> {};

// `extends Base ('.' member)*`; Base is an IDENTIFIER or a STRING (a path).
struct Inheritance : cursiv::Extends<TopLevelDecl> {
  string_view base;
  vector<string_view> path;
};

// EXPRESSIONS
// ===========

struct Name : cursiv::Extends<Expr> {
  string_view id;
};

// Spelling kept verbatim: base prefix and digit runs are for a later pass.
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

// Verbatim including its quotes.
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};
struct NodePathLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NullLit : cursiv::Extends<Expr> {};
struct SelfExpr : cursiv::Extends<Expr> {};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> elts;
};

// One item of a dict literal: `key: value` (key set) or `name = value`
// (name set) -- exactly one of the two is non-empty/non-null.
struct DictItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  string_view name;
  Expr* value;
};

struct DictExpr : cursiv::Extends<Expr> {
  vector<DictItem*> items;
};

struct Subscript : cursiv::Extends<Expr> {
  Expr* value;
  Expr* index;
};

struct Attribute : cursiv::Extends<Expr> {
  Expr* value;
  string_view attr;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* func;
  vector<Expr*> args;
};

// `.method(args)`: calls the base/parent class's own method of the same
// name -- GDScript's stand-in for `super`.
struct SuperCall : cursiv::Extends<Expr> {
  string_view method;
  vector<Expr*> args;
};

// `$path`, `$"path"`: a scene-tree node lookup. `path` is the raw text
// after `$` -- a slash-separated identifier chain, or a string's contents
// including its quotes.
struct GetNodeExpr : cursiv::Extends<Expr> {
  string_view path;
};

// `value is type`.
struct IsExpr : cursiv::Extends<Expr> {
  Expr* value;
  Expr* type;
};

// `~x`, `-x`, `+x`, `!x`/`not x`.
struct UnaryOp : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// Every binary level from `*` to `or`/`||`: the operator's text, so a
// consumer does not have to know the set.
struct BinOp : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `body if test else orelse`.
struct CondExpr : cursiv::Extends<Expr> {
  Expr* body;
  Expr* test;
  Expr* orelse;
};

// `value as type`.
struct CastExpr : cursiv::Extends<Expr> {
  Expr* value;
  string_view type;
};

// PATTERNS (match statement)
// ===========================

struct LiteralPattern : cursiv::Extends<Pattern> {
  Expr* value;
};

struct WildcardPattern : cursiv::Extends<Pattern> {};

// `var name`.
struct BindingPattern : cursiv::Extends<Pattern> {
  string_view name;
};

struct ArrayPattern : cursiv::Extends<Pattern> {
  vector<Pattern*> items;
  boolean openEnded;  // a trailing `..`
};

struct DictKeyValuePattern : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Pattern* value;  // null when the entry names no pattern
};

struct DictPattern : cursiv::Extends<Pattern> {
  vector<DictKeyValuePattern*> items;
  boolean openEnded;
};

struct MatchCase : cursiv::Extends<cursiv::AstNode> {
  vector<Pattern*> patterns;
  vector<Stmt*> body;
};

struct MatchStmt : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<MatchCase*> cases;
};

// STATEMENTS
// ==========

struct LocalVarDecl : cursiv::Extends<Stmt> {
  string_view name;
  string_view typeHint;
  boolean isInferred;
  Expr* value;  // null when there is no initializer
};

struct ElifClause : cursiv::Extends<cursiv::AstNode> {
  Expr* test;
  vector<Stmt*> body;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* test;
  vector<Stmt*> body;
  vector<ElifClause*> elifs;
  vector<Stmt*> orelse;  // an `else` body, empty if absent
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* test;
  vector<Stmt*> body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view target;
  Expr* iter;
  vector<Stmt*> body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};
struct BreakStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;  // null when bare `return`
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  string_view op;  // "=", "+=", "-=", ...
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* test;
  Expr* msg;
};

struct BreakpointStmt : cursiv::Extends<Stmt> {};
struct PassStmt : cursiv::Extends<Stmt> {};

// TOP-LEVEL DECLARATIONS (class members)
// ========================================

struct ExportDecl : cursiv::Extends<cursiv::AstNode> {
  string_view hint;  // a type name or hint identifier; empty for bare `export`
  vector<Expr*> args;  // enum/range hint literals following `hint,`
};

struct SetGet : cursiv::Extends<cursiv::AstNode> {
  string_view setter;
  string_view getter;
};

struct ClassVarDecl : cursiv::Extends<TopLevelDecl> {
  boolean isOnready;
  string_view rpcMode;
  ExportDecl* exportDecl;  // null if absent
  string_view name;
  string_view typeHint;  // empty if absent
  boolean isInferred;    // the `:=` form
  Expr* value;           // null if absent
  SetGet* setget;        // null if absent
};

struct ConstDecl : cursiv::Extends<TopLevelDecl> {
  boolean isInferred;
  string_view name;
  string_view typeHint;  // empty if absent
  Expr* value;
};

struct SignalDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  vector<string_view> params;  // empty when the signal has no parameter list
};

struct EnumItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;  // null if absent
};

struct EnumDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;  // empty for an anonymous enum
  vector<EnumItem*> items;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean hasVarKeyword;
  boolean isInferred;  // a leading `var` on the parameter
  string_view name;
  string_view typeHint;  // empty if absent
  Expr* defaultValue;    // null if absent
};

struct MethodDecl : cursiv::Extends<TopLevelDecl> {
  string_view rpcMode;  // "remote"/"master"/... ; empty if absent
  boolean isStatic;
  string_view name;
  vector<Param*> params;
  boolean hasSuperCall;
  vector<Expr*> superArgs;
  string_view returnType;  // empty if absent
  vector<Stmt*> body;
};

struct InnerClassDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  Inheritance* headerInherits;  // `class Name extends Base:`; null if absent
  vector<TopLevelDecl*> body;
};

struct ToolDecl : cursiv::Extends<TopLevelDecl> {};

struct ClassNameDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  string_view icon;
};
struct PassDecl : cursiv::Extends<TopLevelDecl> {};
struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<TopLevelDecl*> body;
};

}  // namespace gdscript::ast
