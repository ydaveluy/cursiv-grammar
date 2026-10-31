// The bcpl language: its AST. BCPL has one flat per-block namespace and no
// type system a parser can see, so declared names are `string_view`/`Ident`,
// never `reference<>`.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bcpl::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
  boolean floating;
};

// Raw digit text, radix included (`#X1F`, `#B101`, `37`): the value's
// width depends on the target, which a parser doesn't know.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// `?`: the undefined value.
struct UndefLit : cursiv::Extends<Expr> {};

struct GroupExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// `SLCT e9 (: e9 (: e9)?)?`.
struct SelectExpr : cursiv::Extends<Expr> {
  Expr* a;
  Expr* b;
  Expr* c;
};

// `NEXT`/`EXIT`/`BREAK`/`LOOP`/`ENDCASE` read as a value, not a command.
struct KeywordExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct TableExpr : cursiv::Extends<Expr> {
  vector<Expr*> values;
};

struct ValofExpr : cursiv::Extends<Expr> {
  Stmt* body;
};

// `FLOAT`/`FIX`/`!`/`@`, numeric `+`/`-`/`ABS`/`#+`/`#-`/`#ABS`, `NOT`/`~`:
// one class, the operator's text says which.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
  boolean method;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

// `#( e0, ... )`: a vector literal.
struct VecAllocExpr : cursiv::Extends<Expr> {
  vector<Expr*> values;
};

// Every binary operator (`OF`/`::`/`!`/`%`, mult, add, relational, shift,
// `&`, `|`, `EQV`/`XOR`/`NEQV`) shares one class; `op` says which.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `cond -> then, else` (`->` or the float form `#->`).
struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  string_view op;
  Expr* thenValue;
  Expr* elseValue;
};

struct PatternList;

struct Pattern : cursiv::Extends<cursiv::AstNode> {};

// A bare pattern value, with an optional range end (`bpat (range bpat)?`).
struct ValuePattern : cursiv::Extends<Pattern> {
  string_view value;
  string_view rangeOp;
  string_view rangeValue;
};

// `relop (bpat | '(' e0 ')')`: `value` is set for the bpat form, `expr` for
// the parenthesized one.
struct RelPattern : cursiv::Extends<Pattern> {
  string_view op;
  string_view value;
  Expr* expr;
};

struct JcomPattern : cursiv::Extends<Pattern> {
  string_view op;
};

// `FLT identifier`.
struct FlnamePattern : cursiv::Extends<Pattern> {
  string_view name;
};

// `'(' p0 ')'` or `'[' p0 ']'`.
struct GroupPattern : cursiv::Extends<Pattern> {
  boolean bracket;
  PatternList* body;
};

// One element of a pattern list, and how it joins the one before it: `,`,
// `|`, or bare (empty `join`, always true of the first element). The
// reference nests these through mutually-restricted rules (p0/p1/p2/p3,
// sp0/sp1); flattened here to one list since the nesting changes which
// join may follow which, never what a whole pattern list accepts overall
// against a well-formed clause.
struct PatternTerm : cursiv::Extends<cursiv::AstNode> {
  string_view join;
  Pattern* term;
};

struct PatternList : cursiv::Extends<cursiv::AstNode> {
  vector<PatternTerm*> items;
};

struct MatchClause : cursiv::Extends<cursiv::AstNode> {
  PatternList* pattern;
  Expr* result;
  Stmt* action;
};

// `MATCH`/`EVERY` '(' e0, ... ')' mlist.
struct MatchExpr : cursiv::Extends<Expr> {
  string_view kind;
  vector<Expr*> scrutinees;
  vector<MatchClause*> clauses;
};

struct Prefix : cursiv::Extends<cursiv::AstNode> {};

struct LabelPrefix : cursiv::Extends<Prefix> {
  string_view name;
};

struct CasePrefix : cursiv::Extends<Prefix> {
  Expr* value;
};

struct DefaultPrefix : cursiv::Extends<Prefix> {};

// `prefix_* unlabelled_command`, or bare prefixes with no command (a label
// at the end of a block): `body` is null for the second form.
struct Command : cursiv::Extends<Stmt> {
  vector<Prefix*> prefixes;
  Stmt* body;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
  string_view op;
  vector<Expr*> values;
};

// `BREAK`/`LOOP`/`ENDCASE`/`RETURN`/`FINISH`.
struct SimpleStmt : cursiv::Extends<Stmt> {
  string_view kw;
};

// The target is a full expression, a computed jump included.
struct GotoStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct CallStmt : cursiv::Extends<Stmt> {
  Expr* callee;
  vector<Expr*> args;
  boolean method;
};

struct ResultisStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct SwitchonStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

// `REPEAT`/`REPEATUNTIL`/`REPEATWHILE` suffixing a command; `cond` is null
// for plain `REPEAT`.
struct RepeatStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  string_view kind;
  Expr* cond;
};

struct UntilStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Ident* var;
  Expr* from;
  Expr* to;
  Expr* by;
  Stmt* body;
};

struct TestStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
};

struct UnlessStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
};

struct Decl;

// `OB declaration_part semi command_list? CB`.
struct Block : cursiv::Extends<Stmt> {
  string_view openTag;
  string_view closeTag;  // empty for an untagged or implicit closer
  vector<Decl*> declarations;
  vector<Stmt*> commands;
};

// `OB command_list? CB`: no declarations, unlike `Block`.
struct CompoundStmt : cursiv::Extends<Stmt> {
  string_view openTag;
  string_view closeTag;
  vector<Stmt*> commands;
};

struct Decl : cursiv::Extends<cursiv::AstNode> {};

struct ManifestItem : cursiv::Extends<cursiv::AstNode> {
  Ident* name;
  Expr* value;
};

struct ManifestDecl : cursiv::Extends<Decl> {
  string_view openTag;
  string_view closeTag;
  vector<ManifestItem*> items;
};

struct StaticDecl : cursiv::Extends<Decl> {
  string_view openTag;
  string_view closeTag;
  vector<ManifestItem*> items;
};

struct GlobalItem : cursiv::Extends<cursiv::AstNode> {
  Ident* name;
  Expr* value;
};

struct GlobalDecl : cursiv::Extends<Decl> {
  string_view openTag;
  string_view closeTag;
  vector<GlobalItem*> items;
};

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct SimpleDef : cursiv::Extends<Definition> {
  vector<Ident*> names;
  vector<Expr*> values;
};

struct VectorDef : cursiv::Extends<Definition> {
  Ident* name;
  Expr* size;
};

struct FunctionDef : cursiv::Extends<Definition> {
  Ident* name;
  vector<Ident*> params;
  Expr* body;
};

struct RoutineDef : cursiv::Extends<Definition> {
  Ident* name;
  vector<Ident*> params;
  Stmt* body;
};

// `LET definition (AND definition)*`.
struct LetDecl : cursiv::Extends<Decl> {
  vector<Definition*> definitions;
};

struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view kw;
  string_view text;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Directive*> directives;
  vector<Decl*> declarations;
};

}  // namespace bcpl::ast
