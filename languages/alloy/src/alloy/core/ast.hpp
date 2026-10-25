// The Alloy language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace alloy::ast {

// A relation multiplicity, or the coercion 'set': the same six spellings
// cover sig declarations, quantifiers, unary tests and the arrow operator,
// each rule offering only the subset it accepts.
enum class Mult : std::uint8_t { lone, some, one, set };

enum class Quantifier : std::uint8_t { all, no, sum, lone, some, one };

// The value an operator token denotes, not the text that spelled it -- two
// spellings of one meaning (`||`/`or`) share one enumerator.
enum class UnaryOp : std::uint8_t {
  not_,
  no,
  lone,
  some,
  one,
  set,
  card,
  transpose,
  closure,
  rclosure,
  integer,
  sequence,
};

enum class BinOp : std::uint8_t {
  or_,
  and_,
  iff,
  implies,
  intersect,
  add,
  subtract,
  override_,
  domain_restrict,
  range_restrict,
  join,
  integer_add,
  integer_subtract,
  multiply,
  divide,
  remainder,
  shift_left,
  shift_right,
  shift_arithmetic,
};

// `not_eq` is a reserved alternative token (== "!="): the enumerator is
// spelled `neq`.
enum class CompareOp : std::uint8_t {
  in,
  not_in,
  eq,
  neq,
  lt,
  not_lt,
  gt,
  not_gt,
  le,
  not_le,
  ge,
  not_ge
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Paragraph : cursiv::Extends<cursiv::AstNode> {};

struct Block : cursiv::Extends<Expr> {
  vector<Expr *> exprs;
};

struct Decl : cursiv::Extends<cursiv::AstNode> {
  bool private_;
  bool defined;
  bool namesDisjoint;
  vector<string_view> names;
  bool typeDisjoint;
  Expr *type;
};

struct LetDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value;
};

// const_: '-'? number | 'none' | 'univ' | 'iden'
struct NumberExpr : cursiv::Extends<Expr> {
  bool negative;
  int64 value;
};
struct NoneExpr : cursiv::Extends<Expr> {};
struct UnivExpr : cursiv::Extends<Expr> {};
struct IdenExpr : cursiv::Extends<Expr> {};

struct StringExpr : cursiv::Extends<Expr> {
  string_view raw;
};

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};
// '@' name: the literal (unjoined) field or variable.
struct FieldExpr : cursiv::Extends<Expr> {
  string_view name;
};
struct ThisExpr : cursiv::Extends<Expr> {};

// expr '[' expr (',' expr)* ']': box join, read left to right so
// `r[a][b]` nests as ((r[a])[b]).
struct BoxJoinExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  UnaryOp op;
  Expr *operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  BinOp op;
  Expr *right;
};

// (mult|'set')? '->' (mult|'set')?: multiplicities decorate the arrow
// itself, not either operand, so they sit beside left/right rather than
// folded into an operator value.
struct ArrowExpr : cursiv::Extends<Expr> {
  Expr *left;
  optional<Mult> leftMult;
  optional<Mult> rightMult;
  Expr *right;
};

struct CompareExpr : cursiv::Extends<Expr> {
  Expr *left;
  CompareOp op;
  Expr *right;
};

// An implication, with an optional else branch.
struct IfExpr : cursiv::Extends<Expr> {
  Expr *cond;
  Expr *then;
  Expr *else_;
};

struct LetExpr : cursiv::Extends<Expr> {
  vector<LetDecl *> decls;
  Block *body;
};

struct QuantExpr : cursiv::Extends<Expr> {
  Quantifier quant;
  vector<Decl *> decls;
  Block *body;
};

// '{' decl (',' decl)* blockOrBar '}': a relation comprehension.
struct SetComprExpr : cursiv::Extends<Expr> {
  vector<Decl *> decls;
  Block *body;
};

struct ModuleParam : cursiv::Extends<cursiv::AstNode> {
  bool exactly_;
  string_view name;
};

struct ModuleDecl : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  vector<ModuleParam *> names;
};

struct Import : cursiv::Extends<cursiv::AstNode> {
  bool private_;
  vector<string_view> args;
  vector<string_view> paths;
  optional<string_view> alias;
};

struct SigExt : cursiv::Extends<cursiv::AstNode> {};
struct SigExtends : cursiv::Extends<SigExt> {
  string_view target;
};
struct SigIn : cursiv::Extends<SigExt> {
  bool exact;
  vector<string_view> targets;
};

enum class SigQualifier : std::uint8_t { abstract_, lone, one, some, private_ };

struct SigDecl : cursiv::Extends<Paragraph> {
  vector<SigQualifier> qualifiers;
  vector<string_view> names;
  SigExt *ext;
  vector<Decl *> decls;
  Block *facts;
};

struct FactDecl : cursiv::Extends<Paragraph> {
  optional<string_view> name;
  Block *body;
};

struct PredDecl : cursiv::Extends<Paragraph> {
  bool private_;
  optional<string_view> qualifier;
  string_view name;
  vector<Decl *> params;
  Block *body;
};

struct FunDecl : cursiv::Extends<Paragraph> {
  bool private_;
  optional<string_view> qualifier;
  string_view name;
  vector<Decl *> params;
  Expr *returnType;
  Block *body;
};

struct AssertDecl : cursiv::Extends<Paragraph> {
  optional<string_view> name;
  Block *body;
};

struct EnumDecl : cursiv::Extends<Paragraph> {
  bool private_;
  string_view name;
  vector<string_view> members;
};

struct MacroDecl : cursiv::Extends<Paragraph> {
  bool private_;
  string_view name;
  vector<string_view> params;
  Expr *body;
};

struct TypeScope : cursiv::Extends<cursiv::AstNode> {
  bool exactly_;
  int64 count;
  optional<int64> end;
  optional<int64> step;
  string_view type;
};

struct Scope : cursiv::Extends<cursiv::AstNode> {
  optional<int64> overall;
  vector<TypeScope *> typescopes;
};

enum class CmdKind : std::uint8_t { run, check };

struct CmdDecl : cursiv::Extends<Paragraph> {
  optional<string_view> name;
  optional<CmdKind> kind;
  variant<string_view, Block *> target;
  Scope *scope;
  optional<int64> expects;
  CmdDecl *next;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  ModuleDecl *moduleDecl;
  vector<Import *> imports;
  vector<Paragraph *> paragraphs;
};

} // namespace alloy::ast
