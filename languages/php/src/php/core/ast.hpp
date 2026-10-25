// PHP syntax tree; inline HTML remains text.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace php::ast {

// A block item: a statement or a declaration (function/class/namespace/...).
struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Item> {};
struct Decl : cursiv::Extends<Item> {};

// An expression -- empty, so any alternative of `expression` fits any operand
// slot; what an expression is, is its subclass (as in cursiv's `c` example).
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// `Foo`, `\Foo\Bar`, `namespace\Bar`.
struct Name : cursiv::Extends<cursiv::AstNode> {
  boolean rooted;          // leading '\'
  boolean nsRelative;      // leading 'namespace\'
  vector<string_view> parts;
};

// A type hint as written.
struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

struct NamedTypeRef : cursiv::Extends<TypeRef> {
  Name* name;
  vector<TypeRef*> genericArgs;  // '<:' T, U ':>' -- not standard PHP, rare
};

struct PrimitiveTypeRef : cursiv::Extends<TypeRef> {
  string_view keyword;  // int, string, array, callable, object...
};

struct StaticTypeRef : cursiv::Extends<TypeRef> {};

struct NullableTypeRef : cursiv::Extends<TypeRef> {
  TypeRef* type;
};

struct IntersectionTypeRef : cursiv::Extends<TypeRef> {
  vector<TypeRef*> options;
};

struct UnionTypeRef : cursiv::Extends<TypeRef> {
  vector<TypeRef*> options;
};

// One call argument: `name:`? `...`? expr, or `&chain`.
struct Arg : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean spread;
  boolean byRef;
  Expr* value;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  Name* name;
  vector<Arg*> args;
};

struct AttributeGroup : cursiv::Extends<cursiv::AstNode> {
  string_view target;  // an optional `identifier ':'` before the first attribute
  vector<Attribute*> items;
};

// A name used where an expression is expected: a bare constant, a class or
// function reference, the scope of `::`.
struct NameExpr : cursiv::Extends<Expr> {
  Name* name;
  boolean isStatic;  // bare `static` used as a name
};

// A bare word used as a `->`/`::` member name.
struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// `$x`, `$$x`, `${expr}`, and PHP's variable-variable indirection.
struct Var : cursiv::Extends<Expr> {
  boolean byRef;
  string_view name;   // set when the base is a plain VarName
  Expr* nameExpr;      // dynamic name or another variable
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NullLit : cursiv::Extends<Expr> {};

// Quoted source text, or the body of a heredoc/nowdoc.
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

// A raw run of text inside an interpolated string.
struct StrText : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// Double-quoted text and interpolated expressions.
struct InterpString : cursiv::Extends<Expr> {
  vector<cursiv::AstNode*> pieces;  // StrText* or Expr*
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* base;
  Expr* index;  // null for the append form, `$a[] = ...`
};

// `scope::member` -- a class constant, a static property, `Class::CONST`.
struct StaticAccessExpr : cursiv::Extends<Expr> {
  Expr* scope;
  Expr* member;  // NameExpr (a constant name) or Var (a static property)
};

// An object property access; calls and indexes form separate postfix nodes.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean nullsafe;
  Expr* member;  // NameExpr, Var, Ident, or an expression in `{}`/braces
};

// `callee(args)` -- the callee is any expression: a name, a `->`/`::`
// access, a parenthesised expression, or another call's result.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<TypeRef*> genericArgs;
  vector<Arg*> args;
};

struct NewExpr : cursiv::Extends<Expr> {
  Expr* target;      // NameExpr or an indirect chain (MemberExpr/IndexExpr/Var)
  boolean isStatic;   // `new static(...)`
  Decl* anonClass;    // `new class(...) { ... }`; null otherwise
  vector<Arg*> args;
};

struct CloneExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct PrintExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // ~ @ ! + -
  Expr* operand;
};

struct IncDecExpr : cursiv::Extends<Expr> {
  string_view op;  // ++ or --
  boolean prefix;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  string_view type;
  Expr* operand;
};

// Every binary operator level between `**` and `||`, one class -- `op` holds
// the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct InstanceofExpr : cursiv::Extends<Expr> {
  Expr* value;
  Expr* type;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;  // null for the `cond ?: else` short form
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;  // "=" or a compound assignment
  boolean byRef;    // `$x = &$y`
  Expr* value;
};

struct IncludeExpr : cursiv::Extends<Expr> {
  string_view keyword;  // include, include_once, require, require_once
  Expr* operand;
};

struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct YieldExpr : cursiv::Extends<Expr> {
  boolean from;
  Expr* key;
  Expr* value;
};

// `value` is null for a slot skipped in a destructuring list, `[, $b] = $c`.
struct ArrayItem : cursiv::Extends<cursiv::AstNode> {
  boolean spread;
  Expr* key;
  boolean byRef;
  Expr* value;
};

// An array literal, or the list a destructuring assignment or a `foreach`
// stores into: `[$a, $b] = $c`, `list($a, $b) = $c`.
struct ArrayExpr : cursiv::Extends<Expr> {
  vector<ArrayItem*> items;
};

struct IssetExpr : cursiv::Extends<Expr> {
  vector<Expr*> args;
};

struct EmptyExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct EvalExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct ExitExpr : cursiv::Extends<Expr> {
  vector<Arg*> args;
};

struct Param;
struct PropertyHook;
struct Stmt;

struct Closure : cursiv::Extends<Expr> {
  vector<AttributeGroup*> attributes;
  boolean isStatic;
  boolean byRef;
  vector<Param*> params;
  vector<Var*> uses;
  TypeRef* returnType;
  Stmt* body;          // block form
  Expr* arrowBody;      // `fn(...) => expr` form
};

struct MatchArm : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> conditions;  // empty = `default`
  Expr* value;
};

struct MatchExpr : cursiv::Extends<Expr> {
  Expr* subject;
  vector<MatchArm*> arms;
};

// Declarations ---------------------------------------------------------

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<PropertyHook*> hooks;
  vector<AttributeGroup*> attributes;
  vector<string_view> modifiers;  // promoted-property modifiers: public, readonly...
  TypeRef* type;
  boolean byRef;
  boolean variadic;
  Var* name;
  Expr* defaultValue;
};

struct FunctionDecl : cursiv::Extends<Decl> {
  vector<AttributeGroup*> attributes;
  boolean byRef;
  string_view name;
  vector<Param*> params;
  TypeRef* returnType;
  Stmt* body;
};

struct ClassConst : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct PropertyHook : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeGroup*> attributes;
  vector<string_view> modifiers;
  boolean byRef;
  string_view name;
  vector<Param*> params;
  Stmt* body;
  Expr* expression;
};

struct PropertyDecl : cursiv::Extends<cursiv::AstNode> {
  vector<PropertyHook*> hooks;
  Var* name;
  Expr* defaultValue;
};

struct ClassMember : cursiv::Extends<cursiv::AstNode> {};

struct ConstMember : cursiv::Extends<ClassMember> {
  vector<AttributeGroup*> attributes;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<ClassConst*> consts;
};

struct PropertyMember : cursiv::Extends<ClassMember> {
  vector<AttributeGroup*> attributes;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<PropertyDecl*> props;
};

struct MethodMember : cursiv::Extends<ClassMember> {
  vector<AttributeGroup*> attributes;
  vector<string_view> modifiers;
  FunctionDecl* function;
};

struct TraitAlias : cursiv::Extends<cursiv::AstNode> {
  Name* traitName;
  string_view methodName;
  string_view asModifier;
  string_view asName;
};

struct TraitPrecedence : cursiv::Extends<cursiv::AstNode> {
  Name* traitName;
  string_view methodName;
  vector<Name*> insteadOf;
};

struct UseTraitMember : cursiv::Extends<ClassMember> {
  vector<Name*> traits;
  vector<TraitPrecedence*> precedences;
  vector<TraitAlias*> aliases;
};

struct ClassDecl : cursiv::Extends<Decl> {
  vector<AttributeGroup*> attributes;
  boolean isPrivate;
  vector<string_view> modifiers;
  boolean isPartial;
  string_view kind;       // class, interface, trait
  string_view name;       // empty for an anonymous class
  Name* extendsName;      // a class's single base
  vector<Name*> extendsList;  // an interface's several bases
  vector<Name*> implementsList;
  vector<ClassMember*> members;
};

struct EnumCase : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeGroup*> attributes;
  string_view name;
  Expr* value;
};

struct EnumDecl : cursiv::Extends<Decl> {
  vector<AttributeGroup*> attributes;
  string_view name;
  string_view backingType;
  vector<Name*> implementsList;
  vector<EnumCase*> cases;
  vector<ClassMember*> members;
};

struct NamespaceDecl : cursiv::Extends<Decl> {
  Name* name;
  vector<Item*> items;  // empty and `braced=false` for the semicolon form
  boolean braced;
};

struct UseItem : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Name* name;
  string_view alias;
  vector<UseItem*> group;  // `A\{B, C as D}` -- one level of group-use
};

struct UseDecl : cursiv::Extends<Decl> {
  string_view kind;  // function, const, empty
  vector<UseItem*> items;
};

struct ImportDecl : cursiv::Extends<Decl> {
  Name* name;
};

struct GlobalConstDecl : cursiv::Extends<Decl> {
  vector<AttributeGroup*> attributes;
  vector<ClassConst*> consts;
};

// Statements -------------------------------------------------------------

struct LabelStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Item*> items;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  vector<Expr*> init;
  vector<Expr*> cond;
  vector<Expr*> update;
  Stmt* body;
};

struct CaseArm : cursiv::Extends<cursiv::AstNode> {
  Expr* value;  // null for `default`
  vector<Item*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<CaseArm*> cases;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  Expr* level;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  Expr* level;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct GlobalStmt : cursiv::Extends<Stmt> {
  vector<Expr*> vars;
};

struct StaticVarStmt : cursiv::Extends<Stmt> {
  vector<PropertyDecl*> vars;
};

struct EchoStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct UnsetStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct ForeachStmt : cursiv::Extends<Stmt> {
  Expr* source;
  boolean keyByRef;    // unused; kept false (key is never by-ref in PHP)
  Expr* key;
  boolean valueByRef;
  Expr* value;
  Stmt* body;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<Name*> types;
  Var* var;
  Stmt* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  vector<CatchClause*> catches;
  Stmt* finallyBody;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view target;
};

struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct DeclareStmt : cursiv::Extends<Stmt> {
  vector<Directive*> directives;
  Stmt* body;
};

struct InlineHtmlStmt : cursiv::Extends<Stmt> {
  string_view text;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

// The document -------------------------------------------------------------

// A shebang line, then any mix of raw markup and PHP blocks in source order.
struct PhpBlock : cursiv::Extends<Item> {
  vector<Item*> items;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  string_view shebang;
  vector<Item*> items;  // InlineHtmlStmt* or PhpBlock*
};

}  // namespace php::ast
