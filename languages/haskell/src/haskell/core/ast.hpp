// The Haskell language: its AST. Expressions double as patterns (a pattern
// is a syntactic subset of an expression, told apart after parsing), so
// pattern positions hold an Expr*.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace haskell::ast {

struct Decl : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};

// NAMES
// =====
// A qualifying "A.B." prefix is its own token: it always
// reads as one contiguous run, tighter than a separate module-name-then-dot
// pair would be.

struct QName : cursiv::Extends<cursiv::AstNode> {
  string_view qualifier; // empty when unqualified
  string_view name;
};

// EXPRESSIONS
// ===========

struct Var : cursiv::Extends<Expr> {
  string_view qualifier;
  string_view name;
};

struct Con : cursiv::Extends<Expr> {
  string_view qualifier;
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

// Ordinary application is left associative: (f x) y has two App nodes.
struct App : cursiv::Extends<Expr> {
  Expr *func = nullptr;
  Expr *arg = nullptr;
};

// Haskell itself defers fixity resolution to a later pass (user-declared
// `infixl`/`infixr` precedence cannot be a static grammar table), so this
// stays the flat chain of operators and operands, unassociated.
struct OpChain : cursiv::Extends<Expr> {
  Expr *first = nullptr;
  vector<string_view> ops;
  vector<Expr *> operands;
};

struct NegApp : cursiv::Extends<Expr> {
  Expr *operand = nullptr;
};

struct TypeSig : cursiv::Extends<Expr> {
  Expr *value = nullptr;
  Type *type = nullptr;
};

// `-<`, `>-`, `-<<`, `>>-`: arrow-notation commands.
struct ArrowCmd : cursiv::Extends<Expr> {
  Expr *left = nullptr;
  string_view op;
  Expr *right = nullptr;
};

struct AsPat : cursiv::Extends<Expr> {
  Expr *binder = nullptr; // a Var
  Expr *pattern = nullptr;
};
struct LazyPat : cursiv::Extends<Expr> {
  Expr *pattern = nullptr;
};
struct BangPat : cursiv::Extends<Expr> {
  Expr *pattern = nullptr;
};

struct Lambda : cursiv::Extends<Expr> {
  vector<Expr *> params;
  Expr *body = nullptr;
};

struct LetExpr : cursiv::Extends<Expr> {
  vector<Decl *> decls;
  Expr *body = nullptr;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr *cond = nullptr;
  Expr *thenBranch = nullptr;
  Expr *elseBranch = nullptr;
};

struct CompQual;

struct GuardedExpr : cursiv::Extends<cursiv::AstNode> {
  vector<CompQual *> guards; // pattern-guards and boolean guards alike
  Expr *value = nullptr;
};

// A multi-way if: `if` followed directly by one or more `| guards -> exp`
// arms, no condition of its own.
struct MultiIfExpr : cursiv::Extends<Expr> {
  vector<GuardedExpr *> arms;
};

struct CaseAlt : cursiv::Extends<cursiv::AstNode> {
  Expr *pattern = nullptr;
  optional<Expr *> body;        // the `-> exp` form
  vector<GuardedExpr *> guards; // the `| guards -> exp` form
  vector<Decl *> whereDecls;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr *scrutinee = nullptr;
  vector<CaseAlt *> alts;
};

// `\case`.
struct LambdaCaseExpr : cursiv::Extends<Expr> {
  vector<CaseAlt *> alts;
};

struct DoStmt : cursiv::Extends<cursiv::AstNode> {
  optional<Expr *> bindPattern; // `pat <- exp`
  optional<Expr *> value;       // `exp`, or the `let`'s own decls below
  vector<Decl *> letDecls;      // `let decls` (no `in`)
  vector<DoStmt *> recStmts;    // `rec { stmts }`
};

struct DoExpr : cursiv::Extends<Expr> {
  bool isMdo = false;
  vector<DoStmt *> stmts;
};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr *> items;
};
struct UnboxedTupleExpr : cursiv::Extends<Expr> {
  vector<Expr *> items;
};

// A parenthesised expression that is neither a tuple nor a section adopts
// its content bare (no node of its own -- like examples/c's
// PrimaryExpression).

// `(op e)` / `(e op)`: a left or right operator section.
struct LeftSection : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand = nullptr;
};
struct RightSection : cursiv::Extends<Expr> {
  Expr *operand = nullptr;
  string_view op;
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr *> items;
};

// `[e1, e2 .. ]`, `[e1 .. ]`, `[e1 .. e2]`, `[e1, e2 .. e3]`.
struct ArithSeqExpr : cursiv::Extends<Expr> {
  Expr *from = nullptr;
  optional<Expr *> thenExpr;
  optional<Expr *> to;
};

// A qualifier: shared by list comprehensions and by guards -- both are a
// `pat <- exp | let decls | exp` shape. `then`/`group` fields are only ever
// set by a comprehension.
struct CompQual : cursiv::Extends<cursiv::AstNode> {
  optional<Expr *> bindPattern; // `pat <- exp`
  optional<Expr *> value;       // a boolean guard, or the qualifier's exp
  vector<Decl *> letDecls;
  // `then f`, `then f by e`, `then group using f`, `then group by e using f`.
  optional<Expr *> transformFn;
  optional<Expr *> byExpr;
  optional<Expr *> usingExpr;
};

struct ListCompExpr : cursiv::Extends<Expr> {
  Expr *body = nullptr;
  vector<CompQual *> quals;
};

struct FieldBind : cursiv::Extends<cursiv::AstNode> {
  string_view qualifier;
  string_view name;
  optional<Expr *> value;  // absent: `NamedFieldPuns`
  bool isWildcard = false; // `..`
};

// Record construction/update: `Con { fields }` / `e { fields }`.
struct RecordExpr : cursiv::Extends<Expr> {
  Expr *base = nullptr; // the constructor or the updated expression
  vector<FieldBind *> fields;
};

struct WildcardPat : cursiv::Extends<Expr> {};

// A visible type application, `e \@ty`.
struct TypeApp : cursiv::Extends<Expr> {
  Expr *func = nullptr;
  Type *arg = nullptr;
};

// Template Haskell: `$e`, `$$e`, quotes and quasiquotes. Ported as syntax,
// without a semantic model.
struct Splice : cursiv::Extends<Expr> {
  bool isTyped = false;
  Expr *value = nullptr;
};
struct Quote : cursiv::Extends<Expr> {
  bool isTyped = false; // `[||e||]` rather than `[|e|]`
  bool isPat = false;   // `[p|pat|]` rather than an expression quote
  optional<Expr *> exprBody;
  optional<Type *> typeBody;
  vector<Decl *> declBody;
};
struct QuasiQuote : cursiv::Extends<Expr> {
  string_view quoter;
  string_view body;
};

// EllipsisLit-style bare literals used outside expressions (wildcard `_`
// already covered by WildcardPat).

// TYPES
// =====

struct TyVar : cursiv::Extends<Type> {
  string_view name;
};
struct TyCon : cursiv::Extends<Type> {
  string_view qualifier;
  string_view name;
};
struct TyVisibleArg : cursiv::Extends<Type> {
  Type *type = nullptr; // the explicit @ marker at the type/kind level
};
struct TyApp : cursiv::Extends<Type> {
  vector<Type *> atoms;
};
struct TyFun : cursiv::Extends<Type> {
  Type *from = nullptr;
  Type *to = nullptr;
};
struct TyContext : cursiv::Extends<Type> {
  Type *context = nullptr;
  Type *body = nullptr;
};
struct TyForall : cursiv::Extends<Type> {
  vector<Type *> vars;
  Type *body = nullptr;
};
struct TyTuple : cursiv::Extends<Type> {
  vector<Type *> items;
};
struct TyUnboxedTuple : cursiv::Extends<Type> {
  vector<Type *> items;
};
struct TyList : cursiv::Extends<Type> {
  Type *item = nullptr;
};
struct TyKindSig : cursiv::Extends<Type> {
  Type *type = nullptr;
  Type *kind = nullptr;
};
struct TyBndr : cursiv::Extends<Type> {
  string_view name;
  optional<Type *> kind;
};
struct TyPromotedList : cursiv::Extends<Type> {
  vector<Type *> items;
};
struct TyPromotedTuple : cursiv::Extends<Type> {
  vector<Type *> items;
};
struct TyLit : cursiv::Extends<Type> {
  string_view text; // an integer or string kind literal
};
struct TyWildcard : cursiv::Extends<Type> {};
struct TyBang : cursiv::Extends<Type> { // strictness/unpack: `!ty`, `~ty`
  string_view marker;
  Type *type = nullptr;
};

// DECLARATIONS
// ============

struct GuardedRhs : cursiv::Extends<cursiv::AstNode> {
  vector<CompQual *> guards; // empty: the plain `= exp` form
  Expr *value = nullptr;
};

// A function/pattern binding, or a bare expression used as a top-level
// declaration (Template Haskell's `$(...)` folds into one, an expression
// with no operators of its own).
struct ValueDecl : cursiv::Extends<Decl> {
  Expr *lhs = nullptr;
  optional<Type *> resultSig;
  vector<GuardedRhs *> rhs;
  vector<Decl *> whereDecls;
};

struct TypeSigDecl : cursiv::Extends<Decl> {
  vector<Expr *> names;
  Type *type = nullptr;
};

struct FixityDecl : cursiv::Extends<Decl> {
  string_view assoc;
  optional<string_view> precedence;
  vector<string_view> ops;
};

struct FieldDecl : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type *type = nullptr;
};

struct ConstructorDecl : cursiv::Extends<cursiv::AstNode> {
  vector<TyBndr *> forallVars;
  optional<Type *> context;
  vector<string_view> gadtNames; // GADT form: `C1, C2 :: sig`
  optional<Type *> gadtType;
  optional<Type *> type; // ordinary form: the constructor applied to fields
  vector<FieldDecl *> fields;
};

struct DerivingClause : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> strategy;
  vector<Type *> types;
  optional<Type *> viaType;
};

struct DataDecl : cursiv::Extends<Decl> {
  bool isNewtype = false;
  bool isFamily = false;
  bool isInstance = false;
  bool isGadt = false;
  optional<string_view> ctype; // raw CTYPE pragma
  optional<Type *> context;
  Type *header = nullptr;
  optional<Type *> kindSig;
  vector<ConstructorDecl *> constructors;
  vector<DerivingClause *> derivings;
};

struct TypeDecl : cursiv::Extends<Decl> {
  bool isFamily = false;
  bool isInstance = false;
  Type *header = nullptr;
  optional<Type *> kindSig;
  optional<Type *> rhs;
  vector<Decl *> familyEqns; // closed type families: nested TypeDecl eqns
};

struct StandaloneKindSigDecl : cursiv::Extends<Decl> {
  vector<Expr *> names;
  Type *kind = nullptr;
};

struct FunDep : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> from;
  vector<string_view> to;
};

struct ClassDecl : cursiv::Extends<Decl> {
  optional<Type *> context;
  Type *header = nullptr;
  vector<FunDep *> dependencies;
  vector<Decl *> body;
};

struct InstanceDecl : cursiv::Extends<Decl> {
  optional<string_view> overlapPragma;
  optional<Type *> context;
  Type *head = nullptr;
  vector<Decl *> body;
};

struct StandaloneDerivingDecl : cursiv::Extends<Decl> {
  optional<string_view> strategy;
  optional<Type *> context;
  Type *type = nullptr;
};

struct RoleDecl : cursiv::Extends<Decl> {
  Type *header = nullptr;
  vector<string_view> roles;
};

struct DefaultDecl : cursiv::Extends<Decl> {
  vector<Type *> types;
};

struct ForeignDecl : cursiv::Extends<Decl> {
  bool isImport = false;
  string_view callconv;
  optional<string_view> safety;
  optional<string_view> externName;
  string_view varName;
  Type *type = nullptr;
};

// A pattern synonym: `pattern P vars = pat` or `pattern P vars <- pat
// where decls`.
struct PatternSynonymDecl : cursiv::Extends<Decl> {
  Expr *lhs = nullptr;
  bool isExplicitBidirectional = false;
  optional<Expr *> pattern;
  vector<Decl *> whereDecls;
};

// A catch-all for a pragma decl whose content this port parses but does not
// model further: `{-# DEPRECATED/WARNING/RULES/ANN/INLINE/NOINLINE/
// SPECIALISE/SPECIALISE_INLINE/MINIMAL/COMPLETE ... #-}`.
struct PragmaDecl : cursiv::Extends<Decl> {
  string_view kind;
  string_view body; // raw text following the keyword, including the closing #-}
};

struct LanguagePragmaDecl : cursiv::Extends<Decl> {
  vector<string_view> extensions;
};

// IMPORTS AND MODULE
// ==================

struct ImportItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool wildcard = false;
  vector<string_view> subNames;
};

struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  bool qualified = false;
  bool source = false;
  bool safe = false;
  optional<string_view> packageLiteral;
  string_view moduleName;
  optional<string_view> asName;
  bool hiding = false;
  bool hasSpec = false;
  vector<ImportItem *> items;
};

struct ExportItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool isModule = false;
  bool wildcard = false;
  vector<string_view> subNames;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  bool hasExports = false;
  vector<ExportItem *> exports;
  vector<Decl *> pragmas;
  vector<ImportDecl *> imports;
  vector<Decl *> decls;
};

} // namespace haskell::ast
