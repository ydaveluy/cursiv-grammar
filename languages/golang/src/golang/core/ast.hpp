// The golang language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace golang::ast {

// Shared by a top-level declaration and a statement: a function body and a
// source file both hold a sequence of items, and a const/type/var
// declaration is legal in either place.
struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<Item> {};
struct Stmt : cursiv::Extends<Item> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};

// -- Types --------------------------------------------------------------

struct NamedType : cursiv::Extends<Type> {
  string_view pkg;   // empty when unqualified
  string_view name;
  vector<Type*> typeArgs;
};

struct PointerType : cursiv::Extends<Type> {
  Type* base;
};

struct ArrayType : cursiv::Extends<Type> {
  Expr* length;       // null when `ellipsisLen`
  boolean ellipsisLen;  // `[...]T`, only legal inside a composite literal's type
  Type* elem;
};

struct SliceType : cursiv::Extends<Type> {
  Type* elem;
};

struct MapType : cursiv::Extends<Type> {
  Type* key;
  Type* elem;
};

struct ChannelType : cursiv::Extends<Type> {
  boolean sendOnly;  // `chan<-`
  boolean recvOnly;  // `<-chan`
  Type* elem;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // empty when unnamed
  boolean variadic;
  Type* type;
};

struct Signature : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  vector<Param*> results;
};

struct FuncType : cursiv::Extends<Type> {
  Signature* sig;
};

struct FieldDecl : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // empty for an embedded field
  boolean embedded;
  boolean embeddedPointer;  // `*T` embedded
  Type* type;
  string_view tag;  // raw string literal, including its quotes; empty if absent
};

struct StructType : cursiv::Extends<Type> {
  vector<FieldDecl*> fields;
};

struct MethodSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Signature* sig;
};

// One term of a type constraint / interface element: `~int`, `int`.
struct TypeTerm : cursiv::Extends<cursiv::AstNode> {
  boolean underlying;  // `~`
  Type* type;
};

// `~int | ~int32 | MyInterface`: a union of terms, OR-separated.
struct TypeElement : cursiv::Extends<cursiv::AstNode> {
  vector<TypeTerm*> terms;
};

struct InterfaceType : cursiv::Extends<Type> {
  vector<MethodSpec*> methods;
  vector<TypeElement*> embeds;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeElement* constraint;
};

// -- Expressions ----------------------------------------------------------

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// The spelling, not the value: a converter would need the target's width,
// which a parser does not know.
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ImaginaryLit : cursiv::Extends<Expr> {
  string_view text;
};

struct RuneLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;  // raw or interpreted, including its delimiters
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// A type supplied in a call or a generic argument list. Type names remain
// ordinary expressions until runtime name resolution distinguishes them.
struct TypeExpr : cursiv::Extends<Expr> {
  Type* type;
};

struct KeyedElement : cursiv::Extends<cursiv::AstNode> {
  Expr* key;    // null when the element carries no key
  Expr* value;  // an expression, or a nested LiteralValue (itself an Expr)
};

// `{a, b: c}`: a composite literal's value with its type elided -- legal only
// nested inside another composite literal, where the outer type supplies it.
struct LiteralValue : cursiv::Extends<Expr> {
  vector<KeyedElement*> elements;
};

struct CompositeLit : cursiv::Extends<Expr> {
  Type* type;
  LiteralValue* value;
};

struct Block;

struct FuncLit : cursiv::Extends<Expr> {
  Signature* sig;
  Block* body;
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

// `f(a, b...)`. The callee is an expression, not a name: `x[i](a)` and
// `(*fp)(a)` are calls too.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
  boolean ellipsis;  // a trailing `...` spreading the last argument
};

// `T(x)`: a conversion whose type is a type literal or a parenthesised type,
// the only shapes an ordinary call cannot already read.
struct ConversionExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* operand;
};

// `a[i]`, and `a[T1, T2, ...]` a generic instantiation with explicit type
// arguments: syntactically the same shape, told apart only by a symbol
// table real Go has and this parser does not.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<Expr*> indices;
};

struct SliceExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Expr* low;
  Expr* high;
  Expr* max;  // the three-index form `a[lo:hi:max]`
};

struct TypeAssertExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Type* type;
};

struct SelectorExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view field;
};

// A method expression with a literal type; named types use SelectorExpr.
struct MethodExpr : cursiv::Extends<Expr> {
  Type* type;
  string_view method;
};

// -- Statements -------------------------------------------------------------

struct Block : cursiv::Extends<Stmt> {
  vector<Item*> stmts;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Item* stmt;  // null: a label with nothing after it; a Stmt or a Decl
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct SendStmt : cursiv::Extends<Stmt> {
  Expr* channel;
  Expr* value;
};

struct IncDecStmt : cursiv::Extends<Stmt> {
  Expr* operand;
  string_view op;  // "++" or "--"
};

struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> lhs;
  string_view op;  // "=", "+=", ...
  vector<Expr*> rhs;
};

struct ShortVarDecl : cursiv::Extends<Stmt> {
  vector<string_view> names;
  vector<Expr*> values;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct FallthroughStmt : cursiv::Extends<Stmt> {};

struct GoStmt : cursiv::Extends<Stmt> {
  Expr* call;
};

struct DeferStmt : cursiv::Extends<Stmt> {
  Expr* call;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  Expr* cond;
  Block* then_;
  Stmt* else_;  // an IfStmt (else if) or a Block, or null
};

struct ForStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  Expr* cond;
  Stmt* post;
  Block* body;
};

struct RangeStmt : cursiv::Extends<Stmt> {
  vector<Expr*> lhs;              // assign-form targets
  vector<string_view> declNames;  // `:=`-form new names
  Expr* range;
  Block* body;
};

struct ExprCaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> values;
  boolean isDefault;
  vector<Item*> body;
};

struct ExprSwitchStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  Expr* tag;
  vector<ExprCaseClause*> cases;
};

struct TypeCaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<Type*> types;
  boolean isDefault;
  vector<Item*> body;
};

struct TypeSwitchStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  string_view bindName;  // `x := y.(type)`, empty when absent
  Expr* subject;         // `y` above
  vector<TypeCaseClause*> cases;
};

struct RecvStmt : cursiv::Extends<Stmt> {
  vector<Expr*> lhs;
  vector<string_view> declNames;
  Expr* value;
};

struct CommClause : cursiv::Extends<cursiv::AstNode> {
  Stmt* comm;  // a SendStmt or RecvStmt; null for `default`
  boolean isDefault;
  vector<Item*> body;
};

struct SelectStmt : cursiv::Extends<Stmt> {
  vector<CommClause*> cases;
};

// -- Declarations -----------------------------------------------------------

struct ImportSpec : cursiv::Extends<cursiv::AstNode> {
  string_view alias;  // empty when absent
  boolean dotImport;  // `import . "pkg"`
  string_view path;   // the string literal, including its quotes
};

struct ConstSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type;
  vector<Expr*> values;
};

struct ConstDecl : cursiv::Extends<Decl> {
  vector<ConstSpec*> specs;
};

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeParam*> typeParams;
  boolean isAlias;
  Type* type;
};

struct TypeDecl : cursiv::Extends<Decl> {
  vector<TypeSpec*> specs;
};

struct VarSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type;
  vector<Expr*> values;
};

struct VarDecl : cursiv::Extends<Decl> {
  vector<VarSpec*> specs;
};

struct FuncDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<TypeParam*> typeParams;
  Signature* sig;
  Block* body;  // null: a forward declaration with no body
};

struct MethodDecl : cursiv::Extends<Decl> {
  vector<Param*> receiver;
  string_view name;
  Signature* sig;
  Block* body;
};

struct SourceFile : cursiv::Extends<cursiv::AstNode> {
  string_view packageName;
  vector<ImportSpec*> imports;
  vector<Item*> decls;  // FuncDecl, MethodDecl, ConstDecl, TypeDecl, VarDecl
};

}  // namespace golang::ast
