// The vaxscan language: its AST. VAX SCAN mixes a conventional imperative
// core (procedures, statements, expressions) with three small pattern
// sub-languages evaluated at scan time: TOKEN patterns, SET/GROUP boolean
// algebras of characters, and MACRO pictures. Each gets its own flat node
// family; the imperative core shares one Stmt family across module level
// and procedure/macro bodies, since the grammar alone restricts what each
// context can append.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace vaxscan::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// ---- Types --------------------------------------------------------------

struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

// INTEGER, BOOLEAN, FILE, or a bare name/keyword standing for a
// previously-declared TYPE.
struct SimpleType : cursiv::Extends<TypeRef> {
  string_view keyword;
};

struct StringType : cursiv::Extends<TypeRef> {
  boolean varying;
  boolean fixed;
  boolean dynamic;
  Expr* length;
};

struct FillType : cursiv::Extends<TypeRef> {
  Expr* length;
};

struct PointerType : cursiv::Extends<TypeRef> {
  TypeRef* target;
};

struct TreePtrType : cursiv::Extends<TypeRef> {
  string_view level;
  TypeRef* target;
};

struct TreeType : cursiv::Extends<TypeRef> {
  vector<string_view> levels;
  TypeRef* of;
};

struct RecordComponent : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeRef* type;
};

struct RecordType : cursiv::Extends<TypeRef> {
  boolean overlay;
  vector<RecordComponent*> components;
};

// ---- Expressions ----------------------------------------------------------

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct FieldExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct DerefExpr : cursiv::Extends<Expr> {
  Expr* object;
};

struct OmittedArg : cursiv::Extends<Expr> {};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct StrLit : cursiv::Extends<Expr> {
  string_view value;
};

struct NilLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// A part reference base: `x[i]` (IndexPart) or `x[i..]`/`x[i..j]`
// (RangePart, `end` null for the open form).
struct PartRef : cursiv::Extends<cursiv::AstNode> {};

struct IndexPart : cursiv::Extends<PartRef> {
  Expr* index;
};

struct RangePart : cursiv::Extends<PartRef> {
  Expr* start;
  Expr* end;
};

struct SubscriptExpr : cursiv::Extends<Expr> {
  Expr* base;
  PartRef* part;
};

// NOT, unary +/-: the operator text plus its operand.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// Every binary level: relational chain, `&` concat, +-, */, AND, OR/XOR.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// ---- Statements -----------------------------------------------------------

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct LabelStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* inner;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBody;
  boolean hasElse;
  vector<Stmt*> elseBody;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view var;
  Expr* from;
  Expr* to;
  Expr* step;
  vector<Stmt*> body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

struct CallStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr*> args;
};

struct OpenStmt : cursiv::Extends<Stmt> {
  Expr* fileRef;
  Expr* asExpr;
  boolean forInput;
};

struct CloseStmt : cursiv::Extends<Stmt> {
  Expr* fileRef;
};

struct ReadStmt : cursiv::Extends<Stmt> {
  Expr* fileRef;
  Expr* prompt;
  Expr* target;
};

struct WriteStmt : cursiv::Extends<Stmt> {
  Expr* fileRef;
  vector<Expr*> values;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct CaseExprBase : cursiv::Extends<cursiv::AstNode> {};
struct CaseInRange : cursiv::Extends<CaseExprBase> {};
struct CaseOutRange : cursiv::Extends<CaseExprBase> {};
struct CaseValue : cursiv::Extends<CaseExprBase> {
  Expr* value;
};
struct CaseRange : cursiv::Extends<CaseExprBase> {
  Expr* low;
  Expr* high;
};

struct CaseItem : cursiv::Extends<cursiv::AstNode> {
  vector<CaseExprBase*> tests;
  vector<Stmt*> body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* selector;
  Expr* from;
  Expr* to;
  vector<CaseItem*> items;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AnswerStmt : cursiv::Extends<Stmt> {
  boolean trigger;
  vector<Expr*> values;
};

struct FailStmt : cursiv::Extends<Stmt> {};

struct ScanAttr : cursiv::Extends<cursiv::AstNode> {
  string_view direction;  // INPUT, OUTPUT, DATA
  string_view kind;       // FILE, STRING, PROCEDURE, WIDTH, STACK
  Expr* value;
  string_view procName;
};

struct StartScanStmt : cursiv::Extends<Stmt> {
  vector<ScanAttr*> attrs;
};

struct StopScanStmt : cursiv::Extends<Stmt> {};

struct PruneStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct AllocateStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct FreeStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

// ---- Declarations (module level and, for the form kinds, inside a body) --

struct ConstantDecl : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value;           // '=' exp, or GLOBAL '=' exp
  TypeRef* externalType;  // set for `EXTERNAL` with a type
  boolean isGlobal;
};

struct VariableDecl : cursiv::Extends<Stmt> {
  vector<string_view> names;  // pic_id list; "*" marks a placeholder
  string_view storageAttr;
  TypeRef* type;
};

struct TypeDecl : cursiv::Extends<Stmt> {
  string_view name;
  TypeRef* type;
};

struct Formal : cursiv::Extends<cursiv::AstNode> {
  string_view passingMech;
  TypeRef* type;
};

struct ProcedureFormDecl : cursiv::Extends<Stmt> {
  boolean isForward;  // false: EXTERNAL PROCEDURE
  string_view name;
  vector<Formal*> formals;
  TypeRef* resultType;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view passingMech;
  TypeRef* type;
};

struct ProcedureDecl : cursiv::Extends<Stmt> {
  string_view name;
  boolean isMain;
  vector<Param*> params;
  TypeRef* resultType;
  vector<Stmt*> body;
};

// ---- Pattern sub-language: SET / GROUP boolean algebras -------------------

struct BoolExpr : cursiv::Extends<cursiv::AstNode> {};
struct BoolRef : cursiv::Extends<BoolExpr> {
  string_view name;
};
struct BoolLiteral : cursiv::Extends<BoolExpr> {
  string_view text;
};
struct BoolRange : cursiv::Extends<BoolExpr> {
  string_view low;
  string_view high;
};
struct BoolNot : cursiv::Extends<BoolExpr> {
  BoolExpr* operand;
};
struct BoolBinary : cursiv::Extends<BoolExpr> {
  BoolExpr* left;
  string_view op;  // AND, OR
  BoolExpr* right;
};

struct SetDecl : cursiv::Extends<Stmt> {
  string_view name;
  BoolExpr* expr;
};

struct GroupDecl : cursiv::Extends<Stmt> {
  string_view name;
  BoolExpr* expr;
};

// ---- Pattern sub-language: TOKEN patterns and COLLATE USER sequences ------

struct PatternExpr : cursiv::Extends<cursiv::AstNode> {};
struct PatternLiteral : cursiv::Extends<PatternExpr> {
  string_view text;
};
struct PatternRef : cursiv::Extends<PatternExpr> {
  string_view name;
};
struct PatternRange : cursiv::Extends<PatternExpr> {
  string_view low;
  string_view high;
};
struct PatternOptional : cursiv::Extends<PatternExpr> {
  PatternExpr* operand;
};
struct PatternRepeat : cursiv::Extends<PatternExpr> {
  PatternExpr* operand;
};
struct PatternSeq : cursiv::Extends<PatternExpr> {
  PatternExpr* left;
  PatternExpr* right;
};
struct PatternAlt : cursiv::Extends<PatternExpr> {
  PatternExpr* left;
  string_view op;  // | or /
  PatternExpr* right;
};

struct TokenAttr : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // CASELESS, IGNORE, ALIAS
  string_view alias;
};

struct TokenDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<TokenAttr*> attrs;
  PatternExpr* pattern;
};

// ---- Pattern sub-language: MACRO pictures ----------------------------------

struct Picture : cursiv::Extends<cursiv::AstNode> {};
struct PicName : cursiv::Extends<Picture> {
  string_view name;
};
struct PicCall : cursiv::Extends<Picture> {
  string_view name;
  vector<Expr*> args;
};
struct PicAlias : cursiv::Extends<Picture> {
  string_view text;
};
struct PicOptional : cursiv::Extends<Picture> {
  Picture* operand;
};
struct PicRepeat : cursiv::Extends<Picture> {
  Picture* operand;
};
struct PicSeq : cursiv::Extends<Picture> {
  Picture* left;
  Picture* right;
};
struct PicSplit : cursiv::Extends<Picture> {
  Picture* left;
  Picture* right;
};
struct PicAlt : cursiv::Extends<Picture> {
  Picture* left;
  string_view op;
  Picture* right;
};
struct PicBinding : cursiv::Extends<Picture> {
  string_view name;
  boolean wildcard;
  vector<string_view> names;
  Picture* item;
};

struct MacroDecl : cursiv::Extends<Stmt> {
  string_view name;
  string_view kind;  // TRIGGER, SYNTAX
  boolean expose;
  Picture* picture;
  vector<Stmt*> body;
};

// ---- Directives -------------------------------------------------------------

struct IncludeDirective : cursiv::Extends<Stmt> {
  Expr* file;
};

struct ListDirective : cursiv::Extends<Stmt> {
  string_view mode;  // ON, OFF, PAGE, TITLE
  Expr* title;
};

struct CollateDirective : cursiv::Extends<Stmt> {
  string_view seq;  // NATIVE, ASCII, EBCDIC, DEC_MULTI, USER
  PatternExpr* userSeq;
};

struct RedefineDirective : cursiv::Extends<Stmt> {
  string_view charLiteral;
  string_view ctrlChar;
};

// ---- Module -----------------------------------------------------------------

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view ident;  // IDENT clause, empty when absent
  vector<Stmt*> statements;
};

}  // namespace vaxscan::ast
