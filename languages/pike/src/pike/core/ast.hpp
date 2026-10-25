// The pike language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pike::ast {

// `'static'|'private'|'nomask'|'public'|'protected'|'inline'`: at most one
// per declaration.
enum class Modifier : std::uint8_t {
  Static,
  Private,
  Nomask,
  Public,
  Protected,
  Inline,
};

// `modifier` sits on the base: every definition kind reads `modifiers?` at
// the same spot, and only ever one of them at a time.
struct Definition : cursiv::Extends<cursiv::AstNode> {
  optional<Modifier> modifier;
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Extends `Expr`, not just `AstNode`: `constant_identifier` is also one of
// `expression6`'s primary alternatives (a bare name), so the whole
// `expression4`..`expression6` chain shares one common ancestor for the
// `infix<>` table to bind against.
struct ConstantIdentifier : cursiv::Extends<Expr> {
  vector<string_view> parts;
};

struct Type : cursiv::Extends<cursiv::AstNode> {
  boolean pointer; // trailing `'*'*` of `type_`, collapsed to a fact
};

enum class SimpleTypeKind : std::uint8_t { Int, Float, String, Program };

struct SimpleType : cursiv::Extends<Type> {
  SimpleTypeKind kind;
};

// `object` or `object(Some.Program)`.
struct ObjectType : cursiv::Extends<Type> {
  ConstantIdentifier *program; // null when bare `object`
};

// `mapping` or `mapping(K:V)`.
struct MappingType : cursiv::Extends<Type> {
  Type *key;
  Type *value;
};

// `array` or `array(T)`.
struct ArrayType : cursiv::Extends<Type> {
  Type *element;
};

// `multiset` or `multiset(T)`.
struct MultisetType : cursiv::Extends<Type> {
  Type *element;
};

// `function` or `function(T, U, ...)`.
struct FunctionType : cursiv::Extends<Type> {
  vector<Type *> params;
  boolean variadic;
};

struct Argument : cursiv::Extends<cursiv::AstNode> {
  Type *type;
  boolean variadic; // a trailing `...` before the name
  string_view name;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct Block : cursiv::Extends<Statement> {
  vector<Statement *> statements;
};

struct EmptyStatement : cursiv::Extends<Statement> {};

struct ExprStatement : cursiv::Extends<Statement> {
  Expr *expr;
};

struct If : cursiv::Extends<Statement> {
  Expr *cond;
  Statement *then;
  Statement *else_; // null without `else`
};

struct While : cursiv::Extends<Statement> {
  Expr *cond;
  Statement *body;
};

struct DoWhile : cursiv::Extends<Statement> {
  Statement *body;
  Expr *cond;
};

struct For : cursiv::Extends<Statement> {
  Expr *init; // null when omitted
  Expr *cond;
  Expr *step;
  Statement *body;
};

struct Switch : cursiv::Extends<Statement> {
  Expr *value;
  Block *body;
};

struct Case : cursiv::Extends<Statement> {
  Expr *value;
  Expr *rangeEnd; // set by `.. expression`
};

struct Default : cursiv::Extends<Statement> {};

struct Foreach : cursiv::Extends<Statement> {
  Expr *collection;
  Expr *variable;
  Statement *body;
};

struct Break : cursiv::Extends<Statement> {};
struct Continue : cursiv::Extends<Statement> {};

// No trailing `;` of its own: the statement after it, a bare `';'`, is
// what consumes the source's own terminator (`example1.pike`'s `return 0;`).
struct Return : cursiv::Extends<Statement> {
  Expr *value;
};

struct LValue : cursiv::Extends<cursiv::AstNode> {};

// `'lambda' expression6` as an lvalue.
struct LambdaLValue : cursiv::Extends<LValue> {
  Expr *target;
};

// `type_ identifier`.
struct DeclLValue : cursiv::Extends<LValue> {
  Type *type;
  string_view name;
};

// A name, index or member already in scope: `c`, `a[0]`, `o->m`.
struct TargetLValue : cursiv::Extends<LValue> {
  Expr *target;
};

enum class AssignOp : std::uint8_t {
  Assign,
  Add,
  Subtract,
  Multiply,
  Divide,
  BitAnd,
  BitOr,
  BitXor,
  ShiftLeft,
  ShiftRight,
  Modulo,
};

struct Assignment : cursiv::Extends<Expr> {
  LValue *lvalue;
  AssignOp op;
  Expr *right;
};

struct Ternary : cursiv::Extends<Expr> {
  Expr *cond;
  Expr *then;
  Expr *else_;
};

enum class BinOp : std::uint8_t {
  Or,
  And,
  BitOr,
  BitXor,
  BitAnd,
  Equal,
  NotEqual,
  Greater,
  Less,
  GreaterEqual,
  LessEqual,
  ShiftLeft,
  ShiftRight,
  Add,
  Subtract,
  Multiply,
  Divide,
  Modulo,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  BinOp op;
  Expr *right;
};

struct CastExpr : cursiv::Extends<Expr> {
  Type *type;
  Expr *operand;
};

// `~x`, `-x`, `!x`, prefix/postfix `++`/`--`.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args; // ArgItem*, held as Expr* through its base
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  string_view field;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr *object;
  Expr *index;
  Expr *rangeEnd; // set by `[a..b]`
};

// `'@'? expression2` inside an `expression_list`: the splice marks an array
// argument to be spread rather than passed as one value.
struct ArgItem : cursiv::Extends<Expr> {
  boolean splice;
  Expr *value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

// `catch(expr)` or `catch { block }`.
struct CatchExpr : cursiv::Extends<Expr> {
  variant<Expr *, Block *> body;
};

// `gauge(expr)` or `gauge { block }`.
struct GaugeExpr : cursiv::Extends<Expr> {
  variant<Expr *, Block *> body;
};

struct SscanfExpr : cursiv::Extends<Expr> {
  Expr *input;
  Expr *format;
  vector<LValue *> targets;
};

struct LambdaExpr : cursiv::Extends<Expr> {
  vector<Argument *> args;
  Block *body;
};

struct Program;

// `'class' identifier? '{' program '}'` as a value -- an anonymous or named
// class body used where an expression is expected.
struct ClassExpr : cursiv::Extends<Expr> {
  string_view name; // empty when anonymous
  Program *body;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr *key;
  Expr *value;
};

struct MappingLit : cursiv::Extends<Expr> {
  vector<MapEntry *> entries;
};

struct MultisetLit : cursiv::Extends<Expr> {
  vector<Expr *> items; // ArgItem*
};

struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr *> items; // ArgItem*
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr *inner;
};

// Always a CommaExpr, even for one item, so every caller reads one uniform
// class -- a call's argument list included.
struct CommaExpr : cursiv::Extends<Expr> {
  vector<Expr *> items;
};

struct Import : cursiv::Extends<Definition> {
  ConstantIdentifier *path;
};

struct Inheritance : cursiv::Extends<Definition> {
  ConstantIdentifier *program;
  string_view alias; // set by `: identifier`
};

struct FunctionDeclaration : cursiv::Extends<Definition> {
  Type *returnType;
  string_view name;
  vector<Argument *> args;
};

struct FunctionDefinition : cursiv::Extends<Definition> {
  Type *returnType;
  string_view name;
  vector<Argument *> args;
  Block *body;
};

struct VariableDecl : cursiv::Extends<cursiv::AstNode> {
  boolean pointer; // leading `'*'*` of `variable_name`
  string_view name;
  Expr *init; // null without `= expression2`
};

struct Variables : cursiv::Extends<Definition> {
  Type *type;
  vector<VariableDecl *> names;
};

struct ConstantDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr *value;
};

struct Constant : cursiv::Extends<Definition> {
  vector<ConstantDecl *> names;
};

// No name or body: this shape only represents a bare `class` or `class;`.
struct ClassDef : cursiv::Extends<Definition> {};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Definition *> definitions;
};

} // namespace pike::ast
