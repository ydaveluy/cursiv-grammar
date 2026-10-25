// The python language: its AST, covering the statement and expression
// grammar of Python 3.14 (PEP 695 generics, f-strings/t-strings, match
// statements and patterns). Targets reuse the expression grammar rather
// than the restricted target grammar.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace python::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// EXPRESSIONS
// ===========

struct Name : cursiv::Extends<Expr> {
  string_view id;
};

// Spelling, not value: underscores, base prefixes and suffix letters stay as
// written, like the numeric literals of examples/c.
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ImagLit : cursiv::Extends<Expr> {
  string_view text;
};

// A plain or bytes string token, verbatim including its prefix and quotes:
// unquoting and prefix semantics (str vs bytes, raw vs escaped) are left to a
// later pass, like examples/c keeps literal text.
struct Str : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct ReplField;

// One literal run inside an f-string/t-string, or a nested `{...}` field.
struct IStr : cursiv::Extends<cursiv::AstNode> {
  boolean isRaw;
  boolean isTemplate;  // t-string (PEP 750) rather than f-string
  vector<variant<string_view, ReplField*>> parts;
};

struct ReplField : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean
      debug;  // Preserve the debug = marker; source spans remain in the CST.
  string_view conversion;  // "r"/"s"/"a" after '!', empty if none
  vector<variant<string_view, ReplField*>> formatSpec;  // empty if no ':'
};

// Adjacent string/f-string/t-string literals, concatenated at parse time:
// one node even for a single piece, so a formatter sees the same shape either
// way.
struct Strings : cursiv::Extends<Expr> {
  vector<variant<Str*, IStr*>> pieces;
};

struct NoneLit : cursiv::Extends<Expr> {};
struct EllipsisLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> elts;
};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> elts;
};

struct SetExpr : cursiv::Extends<Expr> {
  vector<Expr*> elts;
};

struct DictItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;  // null means `**value` unpacking
  Expr* value;
};

struct DictExpr : cursiv::Extends<Expr> {
  vector<DictItem*> items;
};

// `*x` inside a list/tuple/set/call, and as a target.
struct StarredExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct Comprehension : cursiv::Extends<cursiv::AstNode> {
  boolean isAsync;
  Expr* target;
  Expr* iter;
  vector<Expr*> ifs;
};

struct ListComp : cursiv::Extends<Expr> {
  Expr* elt;
  vector<Comprehension*> generators;
};

struct SetComp : cursiv::Extends<Expr> {
  Expr* elt;
  vector<Comprehension*> generators;
};

struct DictComp : cursiv::Extends<Expr> {
  Expr* key;
  Expr* value;
  vector<Comprehension*> generators;
};

struct GeneratorExp : cursiv::Extends<Expr> {
  Expr* elt;
  vector<Comprehension*> generators;
};

// Every binary level from `or` down to `**`: the operator's text, so a
// consumer does not have to know the set. Chained comparisons are NOT here
// (see CompareExpr) -- a left-associative tree would change their meaning.
struct BinOp : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `not x`, `-x`, `+x`, `~x`.
struct UnaryOp : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `a < b <= c` is one node, not a binary tree: Python defines it as every
// pair sharing its neighbour, which a left-associative BinOp chain would not
// mean the same thing as.
struct CompareExpr : cursiv::Extends<Expr> {
  Expr* left;
  vector<string_view> ops;
  vector<Expr*> comparators;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr* test;
  Expr* body;
  Expr* orelse;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* annotation;
  Expr* defaultValue;
};

struct Params : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> posonly;
  vector<Param*> args;
  Param* vararg;
  vector<Param*> kwonly;
  Param* kwarg;
};

struct Lambda : cursiv::Extends<Expr> {
  Params* params;  // empty when the lambda takes none
  Expr* body;
};

// `name := value`; the target is always a plain name.
struct NamedExpr : cursiv::Extends<Expr> {
  Name* target;
  Expr* value;
};

struct YieldExpr : cursiv::Extends<Expr> {
  boolean isFrom;
  Expr* value;
};

struct AwaitExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct Attribute : cursiv::Extends<Expr> {
  Expr* value;
  string_view attr;
};

// `a[b:c:d]`; absent bounds are null. Extends Expr (as CPython's own ast.Slice
// does) so it shares a base with the other slice-item shapes (StarredExpr,
// a plain expression) inside a Subscript.
struct SliceExpr : cursiv::Extends<Expr> {
  Expr* lower;
  Expr* upper;
  Expr* step;
};

struct Subscript : cursiv::Extends<Expr> {
  Expr* value;
  Expr* slice;  // an Expr, or a SliceExpr, or a TupleExpr of either
};

struct Keyword : cursiv::Extends<cursiv::AstNode> {
  string_view arg;  // empty means `**value`
  Expr* value;
};

// `f(a, b, x=1)`. A bare `f(x for x in y)` is a call with one generator
// argument, matching CPython's own reading of that shape.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* func;
  vector<Expr*> args;
  vector<Keyword*> keywords;
};

// MATCH PATTERNS
// ==============

struct Pattern : cursiv::Extends<cursiv::AstNode> {};

// A literal-equality pattern (`1`, `"x"`, `None`) or a dotted-attribute
// value pattern (`Color.RED`): `value` holds the literal or the attribute
// chain to compare against.
struct MatchValue : cursiv::Extends<Pattern> {
  Expr* value;
};

// Binds the whole matched subject to `name`; an empty name is the bare `_`
// wildcard, matching anything without binding.
struct MatchCapture : cursiv::Extends<Pattern> {
  string_view name;
};

struct MatchOr : cursiv::Extends<Pattern> {
  vector<Pattern*> patterns;
};

// `pattern as name`.
struct MatchAs : cursiv::Extends<Pattern> {
  Pattern* pattern;
  string_view name;
};

// `*name` inside a sequence pattern; an empty name is a bare `*_`.
struct MatchStar : cursiv::Extends<Pattern> {
  string_view name;
};

struct MatchSequence : cursiv::Extends<Pattern> {
  vector<Pattern*> items;  // may hold a MatchStar* among plain patterns
};

struct MatchMappingItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Pattern* value;
};

struct MatchMapping : cursiv::Extends<Pattern> {
  vector<MatchMappingItem*> items;
  string_view rest;  // `**name`, empty if absent
};

struct MatchKeywordPattern : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Pattern* value;
};

struct MatchClass : cursiv::Extends<Pattern> {
  Expr* cls;
  vector<Pattern*> positional;
  vector<MatchKeywordPattern*> keywords;
};

struct MatchCase : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* guard;  // null when there is no `if`
  vector<Stmt*> body;
};

// STATEMENTS
// ==========

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

// `a = b = value`, `a, b = value`: each target is one Expr, a TupleExpr when
// its own group had more than one element or a trailing comma.
struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
  Expr* value;
};

struct AugAssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  string_view op;
  Expr* value;
};

// `target: annotation` and `target: annotation = value`.
struct AnnAssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* annotation;
  Expr* value;  // null when the annotation carries no value
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isVarTuple;   // `*Ts`
  boolean isParamSpec;  // `**P`
  Expr* bound;
  Expr* defaultValue;
};

struct TypeAliasStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<TypeParam*> typeParams;
  Expr* value;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ImportAlias : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view asName;  // empty when there is no `as`
};

struct ImportStmt : cursiv::Extends<Stmt> {
  vector<ImportAlias*> names;
};

struct ImportFromStmt : cursiv::Extends<Stmt> {
  string_view dots;    // leading '.'/'...' run, verbatim; empty if none
  string_view module;  // empty when the import has no module name
  vector<ImportAlias*> names;
  boolean isStar;
};

struct RaiseStmt : cursiv::Extends<Stmt> {
  Expr* exc;
  Expr* cause;
};

struct PassStmt : cursiv::Extends<Stmt> {};
struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};

struct DelStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* test;
  Expr* msg;
};

struct GlobalStmt : cursiv::Extends<Stmt> {
  vector<string_view> names;
};

struct NonlocalStmt : cursiv::Extends<Stmt> {
  vector<string_view> names;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* test;
  vector<Stmt*> body;
  vector<Stmt*> orelse;  // empty, an `else` body, or one nested IfStmt (elif)
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* test;
  vector<Stmt*> body;
  vector<Stmt*> orelse;
};

struct ForStmt : cursiv::Extends<Stmt> {
  boolean isAsync;
  Expr* target;
  Expr* iter;
  vector<Stmt*> body;
  vector<Stmt*> orelse;
};

struct WithItem : cursiv::Extends<cursiv::AstNode> {
  Expr* contextExpr;
  Expr* optionalVars;
};

struct WithStmt : cursiv::Extends<Stmt> {
  boolean isAsync;
  vector<WithItem*> items;
  vector<Stmt*> body;
};

struct ExceptHandler : cursiv::Extends<cursiv::AstNode> {
  Expr* type;  // a TupleExpr when written as an unparenthesized list (PEP 758)
  string_view name;  // empty when there is no `as`
  vector<Stmt*> body;
  boolean isStar;  // `except*`
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  vector<ExceptHandler*> handlers;  // `except*` is uniform across one try:
                                    // read from handlers[0]->isStar
  vector<Stmt*> orelse;
  vector<Stmt*> finalBody;
};

struct FunctionDef : cursiv::Extends<Stmt> {
  vector<Expr*> decorators;
  boolean isAsync;
  string_view name;
  vector<TypeParam*> typeParams;
  Params* params;  // empty when the def takes none
  Expr* returns;
  vector<Stmt*> body;
};

struct ClassDef : cursiv::Extends<Stmt> {
  vector<Expr*> decorators;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Expr*> bases;
  vector<Keyword*> keywords;
  vector<Stmt*> body;
};

struct MatchStmt : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<MatchCase*> cases;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

}  // namespace python::ast
