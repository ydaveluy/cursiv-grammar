// The trapc language: its AST. TrapC is C without typedef feedback: this
// parser tries `declaration` before `expression-statement` and leaves "is T a
// type" to a later semantic pass, so type-position nodes are syntactic, not
// resolved -- see CastExpr, NamedType and Ident.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace trapc::ast {

// Base for anything a list of items may hold: a statement, a declaration, or
// a function definition. `TranslationUnit::items`, `CompoundStmt::items` and
// `TagType::members` share it.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Stmt : cursiv::Extends<Item> {};

// An expression, empty by design: every alternative of the primary choice
// fits any operand slot, since the builder's family check is against this
// base.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Declarator;

// Kept as text rather than `reference<Declarator>`: C's ordinary namespace
// spans two unrelated node types (`Declarator`, `Enumerator`) that share no
// base a reference could name, and `int x = x;` declares before use.
struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// The spelling, not the value: `0x1fUL`, `07` and `1e9f` are held as written
// because their value depends on the target's type widths, which a parser
// doesn't know.
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

// Adjacent literals are one operand: `"a" "b"` is the string `ab` in C, but
// the pieces stay separate because concatenation is a translation phase this
// grammar doesn't run, and a formatter needs them back verbatim.
struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};

// C23 `true`/`false`. The `false` branch leaves `value` at its default.
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NullptrLit : cursiv::Extends<Expr> {};

// `__func__`, `__FUNCTION__`, `__PRETTY_FUNCTION__`: not a declared name, so
// not an `Ident` -- nothing to resolve, the text names which form was used.
struct PredefinedExpr : cursiv::Extends<Expr> {
  string_view name;
};

// `-x`, `!x`, `*p`, `&x`, `~m`, `++i`, `--i`.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `i++`, `i--`. A separate class from the prefix form: the two compute
// different values, and the tag's family test is how a consumer asks.
struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct Declarator;
struct Specifiers;

// A type as written in an expression: the operand of a cast, `sizeof(T)`,
// `alignof(T)`, `typeof(T)` and a compound literal. The declarator is the
// abstract one -- `int *`, `char[10]` -- the same `Declarator` class with no
// name in it.
struct TypeName : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;
};

// `(T)x`. The ambiguity this grammar exists to carry: `(a) + b` parses as a
// cast of `+b` to type `a` because ordered choice tries the cast first; a
// semantic pass rewrites it to BinaryExpr once it knows `a` is not a type.
struct CastExpr : cursiv::Extends<Expr> {
  TypeName* type;
  Expr* operand;
};

struct SizeofTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct SizeofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct AlignofTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct AlignofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// GCC `_Countof`, the parenthesised-type and bare-operand forms, mirroring
// sizeof/alignof exactly.
struct CountofTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct CountofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// GCC `_Maxof(T)`/`_Minof(T)`: one class for both since each has only the
// parenthesised-type form; `op` tells them apart.
struct MinMaxOfExpr : cursiv::Extends<Expr> {
  string_view op;
  TypeName* type;
};

// `&&label`: GCC's address of a label, valid wherever `unaryExpression` is
// even though TrapC has no `goto` to consume the value.
struct AddrOfLabelExpr : cursiv::Extends<Expr> {
  string_view label;
};

// The GNU `__builtin_*` pseudo-functions (`__builtin_va_arg`,
// `__builtin_offsetof`, `__builtin_choose_expr`,
// `__builtin_types_compatible_p`, `__builtin_tgmath`, `__builtin_complex`):
// one generic node, since each mixes type and expression arguments
// differently and no tool needs to tell them apart structurally -- `name`
// says which, and `types`/`args` hold its arguments left to right within
// their own kind.
struct BuiltinExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<TypeName*> types;
  vector<Expr*> args;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

// `s.f` and `p->f`. One class, one flag: the two differ by an implicit
// dereference on the left and by nothing else.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  string_view field;  // resolved against the type of `object`: a semantic pass
};

// Every binary level of the operator ladder and the comma operator, which is
// one more left-associative level over the assignment rule.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

// Right-associative; its left operand is a unary expression, which is what
// the C grammar says and what keeps `a + b = c` out of the language.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

struct CompoundStmt;

// GNU statement expression: `({ stmts; value; })`.
struct StmtExpr : cursiv::Extends<Expr> {
  CompoundStmt* body;
};

struct Initializer;

// `(T){ ... }`: a postfix primary, so it may be indexed, called or accessed
// like any other expression of its type.
struct CompoundLiteral : cursiv::Extends<Expr> {
  TypeName* type;
  Initializer* init;
};

// `_Generic(e, T: a, default: b)`. `type == nullptr` marks the `default`
// association.
struct GenericAssoc : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  Expr* value;
};

struct GenericExpr : cursiv::Extends<Expr> {
  Expr* control;
  vector<GenericAssoc*> assocs;
};

// A type specifier: the one core of a declaration's specifier list.
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

// `unsigned long int`, `void`, `char`, `bool`. A list, because C spells one
// type with several keywords and their order carries nothing: the set is the
// type.
struct BuiltinType : cursiv::Extends<TypeSpec> {
  vector<string_view> keywords;
};

// A typedef-name: a bare identifier in type position, produced whenever
// nothing else matched. Whether it really is one is left to a semantic pass.
struct NamedType : cursiv::Extends<TypeSpec> {
  string_view name;
};

// `_Atomic(T)`, the specifier form. The bare qualifier form (`_Atomic int`)
// is text in `Specifiers::qualifiers`, same as `const`.
struct AtomicType : cursiv::Extends<TypeSpec> {
  TypeName* type;
};

// `typeof(e)`/`typeof(T)`/`typeof_unqual(...)`. Exactly one of `type`,
// `expr` is set; ordered choice tries the type reading first, same as a cast.
struct TypeofType : cursiv::Extends<TypeSpec> {
  TypeName* type;
  Expr* expr;
  boolean unqual;
};

struct Declaration;

// `struct S {...}` is the supported tag kind; no rule builds a union.
struct TagType : cursiv::Extends<TypeSpec> {
  vector<string_view> attributes;
  string_view name;  // empty for an anonymous struct
  boolean defined;   // a braced member list was present
  vector<Item*> members;  // Declaration* or StaticAssertItem*
};

struct Enumerator;

// `enum Color : int { ... }`: an optional fixed underlying type.
struct EnumType : cursiv::Extends<TypeSpec> {
  vector<string_view> attributes;
  string_view name;
  Specifiers* underlying;  // absent when no fixed underlying type is given
  boolean defined;
  vector<Enumerator*> enumerators;
};

struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> attributes;
  string_view name;
  Expr* value;
};

// `alignas(T)`/`alignas(expr)`, `_Alignas(...)`: exactly one of `type`,
// `expr` is set, the type reading tried first like every other cast-shaped
// ambiguity here.
struct AlignSpec : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  Expr* expr;
};

// Specifier list of a declaration. Qualifiers are a list -- they may repeat
// on either side of the type -- but `type` is single.
struct Specifiers : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> attributes;
  vector<string_view> qualifiers;  // static, const, _Atomic, constexpr...
  vector<AlignSpec*> aligns;
  TypeSpec* type;
};

// One `*` or (Blocks) `^` of a declarator, with the qualifiers that apply to
// that pointer.
struct Pointer : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> qualifiers;
  boolean isBlock;  // `^` (Apple Blocks) rather than `*`
};

// What follows a declarator's core, in source order -- and the order is the
// type: `int *a[10]` is an array of pointers, `int (*a)[10]` a pointer to an
// array.
struct Suffix : cursiv::Extends<cursiv::AstNode> {};

struct ArraySuffix : cursiv::Extends<Suffix> {
  Expr* size;  // absent in `int a[]`
};

struct FunctionSuffix : cursiv::Extends<Suffix> {
  vector<TypeName*> params;  // `(void)` is one parameter of type void
  boolean variadic;          // a trailing `, ...`
};

// Concrete and abstract in one class: `int *x[3]` and `int *[3]` differ only
// by whether `name` was written, so an empty (not optional) `name` marks the
// abstract form.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> attributes;
  string_view callingConvention;  // `__cdecl`, `__stdcall`... empty if none
  vector<Pointer*> pointers;
  string_view name;
  Declarator* nested;
  vector<Suffix*> suffixes;
};

struct Initializer : cursiv::Extends<cursiv::AstNode> {};

struct ExprInit : cursiv::Extends<Initializer> {
  Expr* value;
};

struct ListInit : cursiv::Extends<Initializer> {
  vector<Initializer*> items;
};

struct Designator : cursiv::Extends<cursiv::AstNode> {};

struct FieldDesignator : cursiv::Extends<Designator> {
  string_view name;  // resolved against the aggregate's type: a semantic pass
};

struct IndexDesignator : cursiv::Extends<Designator> {
  Expr* index;
};

struct DesignatedInit : cursiv::Extends<Initializer> {
  vector<Designator*> designators;
  Initializer* value;
};

// One declarator with what may follow it: two grammar rules share this
// class, `InitDeclarator` setting declarator+init and `MemberDeclarator`
// declarator+bitWidth.
struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  Initializer* init;
  Expr* bitWidth;
};

struct Declaration : cursiv::Extends<Item> {
  vector<string_view> attributes;
  Specifiers* specifiers;
  vector<InitDeclarator*> declarators;
};

struct StaticAssertItem : cursiv::Extends<Item> {
  Expr* cond;
  StringLit* message;  // absent: the plain `_Static_assert(cond);` form
};

// One expression group of an inline asm statement: everything up to the next
// `:` or the closing `)`.
struct AsmOperandGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> exprs;
};

// `__asm`/`__asm__ volatile (...)`, simpler than GNU's asm statement: no
// constraint strings, no clobber list.
struct AsmStmt : cursiv::Extends<Stmt> {
  vector<AsmOperandGroup*> groups;
};

// One operand of a file-scope asm: `"constraint"(expr)`, optionally named
// `[symbolicName] "constraint"(expr)`.
struct AsmOperand : cursiv::Extends<cursiv::AstNode> {
  string_view symbolicName;  // empty when unnamed
  StringLit* constraint;
  Expr* expr;
};

// File-scope `asm("...")`, with the output/input operand groups a statement
// asm (AsmStmt) does not have -- there is no register-allocation context to
// simplify away at file scope, so the constraints stay.
struct AsmDecl : cursiv::Extends<Item> {
  StringLit* code;
  vector<AsmOperand*> outputs;
  vector<AsmOperand*> inputs;
};

struct TrapStmt : cursiv::Extends<Stmt> {
  CompoundStmt* body;
};

// GNU `__label__ name;`: declares a block-scoped label name. TrapC has no
// `goto`, so as with a plain label, nothing ever reads `name` back.
struct LocalLabelDecl : cursiv::Extends<Stmt> {
  string_view name;
};

// A standalone attribute declaration retains the matched attribute list.
struct AttributeDecl : cursiv::Extends<Item> {
  vector<string_view> attributes;
};

// `expr;` and the empty statement `;` -- which is why the expression is
// optional rather than a null pointer with a comment.
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CompoundStmt : cursiv::Extends<Stmt> {
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

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

// `for (init; cond; step) body`. The init clause is an `Item`: a declaration
// or an expression statement, exactly the `Item` family, so the clause needs
// no union of its own.
struct ForStmt : cursiv::Extends<Stmt> {
  Item* init;
  Expr* cond;
  Expr* step;
  Stmt* body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

// A plain `name:` label. TrapC has no `goto`, so nothing ever reads `name`
// back as a jump target -- the grammar still accepts the label, unreferenced.
struct Label : cursiv::Extends<Stmt> {
  string_view name;
  Stmt* body;
};

struct CaseLabel : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct DefaultLabel : cursiv::Extends<Stmt> {
  Stmt* body;
};

struct BreakStmt : cursiv::Extends<Stmt> {};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct FunctionDef : cursiv::Extends<Item> {
  Specifiers* specifiers;
  Declarator* declarator;
  CompoundStmt* body;
};

// A translation unit: declarations, function definitions and file-scope asm
// in source order, which is why they share one list and one element type.
struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace trapc::ast
