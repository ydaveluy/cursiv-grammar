// The C AST retains unresolved declaration/expression shapes for runtime scope
// analysis.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace c::ast {

// Balanced argument tokens keep their order and grouping. Trivia stays in
// the CST; no converter rescans or repeatedly copies nested arguments.
struct AttributeToken : cursiv::Extends<cursiv::AstNode> {};
struct AttributeAtom : cursiv::Extends<AttributeToken> {
  string_view text;
};
struct AttributeGroup : cursiv::Extends<AttributeToken> {
  string_view opening;
  string_view closing;
  vector<AttributeToken*> tokens;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  AttributeGroup* arguments;
};

enum class AttributeKind { Standard, Gnu, Declspec };

struct AttributeSpecifier : cursiv::Extends<cursiv::AstNode> {
  AttributeKind kind = AttributeKind::Standard;
  vector<Attribute*> attributes;
  AttributeGroup* arguments;
};

// Base for anything a list of items may hold: a statement, a declaration, or a
// function definition. `TranslationUnit::items` and `CompoundStmt::items` share
// it -- the grammar, not this type, keeps a function definition out of a block.
struct Item : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
};

// A statement, empty by design: what a statement is, is its subclass.
struct Stmt : cursiv::Extends<Item> {};

// An expression. Empty for the same reason, and it is the family every operand
// member points at -- the builder's family check is against this base, so any
// alternative of the primary choice fits any operand slot.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Initializer;
struct CompoundStmt;

// The declared name of the language, and so the target of every identifier
// used as a value. Defined with the declarators, below.
struct Declarator;

// Kept as text rather than `reference<Declarator>`: C's ordinary namespace
// spans two unrelated node types (`Declarator`, `Enumerator`) that share no
// base a reference could name, and `int x = x;` declares before use.
struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// The spelling, not the value: `0x1fUL`, `07` and `1e9f` are held as written
// because their value depends on the target's type widths, which a parser
// doesn't know. One class per lexical kind, so a type test replaces re-lexing.
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

// `-x`, `!x`, `*p`, `&x`, `~m`, `++i`, `--i`. The operator's text, so a
// consumer does not have to know the set.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `i++`, `i--`. A separate class from the prefix form and not a flag on it:
// the two compute different values, and the tag's family test is how a consumer
// asks.
struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct Declarator;
struct Specifiers;

// A type as written in an expression: the operand of a cast, `sizeof(T)`,
// `alignof(T)`, `typeof(T)` and a compound literal. The declarator is the
// abstract one -- `int *`, `char[10]`, `int (*)(void)` -- which is the same
// `Declarator` class with no name in it.
struct TypeName : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
  Specifiers* specifiers;
  Declarator* declarator;
};

// `(T)x`. The ambiguity this grammar exists for: `(a) + b` parses as a cast of
// `+b` to type `a` because ordered choice tries the cast first; a semantic
// pass rewrites it to BinaryExpr once it knows `a` is not a type.
struct CastExpr : cursiv::Extends<Expr> {
  TypeName* type;
  Expr* operand;
};

// `sizeof(T)` -- the parenthesised type form, which has to be tried before the
// expression form or `sizeof (int)` would look for an expression `int`.
struct SizeofTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

// `sizeof x`, and `sizeof (x)` when x is not a type name.
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

struct Label;

// `&&label`: GCC's address of a label, valid as a unary expression and read
// back by a computed `goto *e`.
struct AddrOfLabelExpr : cursiv::Extends<Expr> {
  reference<Label> label;
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

// `f(a, b)`. The callee is an expression, not a name: `(*fp)(x)` and
// `table[i](x)` are calls too.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

// `s.f` and `p->f`. One class, one flag: the two differ by an implicit
// dereference on the left and by nothing else, and `arrow` is written by the
// fact of the `->` token.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  string_view field;  // resolved against the type of `object`: a semantic pass
};

// Every binary level of the operator ladder and the comma operator, which is
// one more left-associative level over the assignment rule. `op` holds the
// operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `c ? a : b`. Not a level of the ladder: it is ternary, and a precedence
// table holds binary operators.
struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

// `x = v`, `x += v`, ... Right-associative and its left operand is a unary
// expression, which is what the C grammar says and what keeps `a + b = c` out
// of the language.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// GNU statement expression: `({ int x = f(); x + 1; })` -- a block used as a
// value, taking the last expression statement's value (a semantic fact this
// parser does not compute).
struct StmtExpr : cursiv::Extends<Expr> {
  CompoundStmt* body;
};

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
  Expr* bitWidth;
};

// A typedef-name: a bare identifier in type position, produced whenever
// nothing else matched. Whether it really is one is left to a semantic pass,
// which resolves it or reports the phrase as an expression instead.
struct NamedType : cursiv::Extends<TypeSpec> {
  string_view name;
};

// `_Atomic(T)`, the specifier form. The bare qualifier form (`_Atomic int`)
// is text in `Specifiers::qualifiers`, same as `const`.
struct AtomicType : cursiv::Extends<TypeSpec> {
  TypeName* type;
};

// `typeof(e)`/`typeof(T)`/`typeof_unqual(...)`. Exactly one of `type`, `expr`
// is set; ordered choice tries the type reading first, same as a cast.
struct TypeofType : cursiv::Extends<TypeSpec> {
  TypeName* type;
  Expr* expr;
  boolean unqual;
};

struct Declaration;

// `struct S {...}`, `union U` -- one class for both keywords: they differ in
// layout, not syntax. `defined` tells an empty definition (`struct S {}`) apart
// from a bare reference to a tag declared elsewhere -- empty `members` can't.
struct TagType : cursiv::Extends<TypeSpec> {
  vector<AttributeSpecifier*> attributes;
  string_view keyword;  // "struct" or "union", as written
  string_view name;     // empty for an anonymous aggregate
  boolean defined;       // a braced member list was present
  vector<Item*> members;  // Declaration* or StaticAssertDecl*
};

struct Enumerator;

// Fixed underlying enum type and explicit C23 colon. A missing colon can be
// diagnosed by semantic validation while retaining the intended type.
struct EnumType : cursiv::Extends<TypeSpec> {
  vector<AttributeSpecifier*> attributes;
  boolean hasUnderlyingColon;
  string_view name;
  Specifiers* underlying;  // absent when no fixed underlying type is given
  boolean defined;
  vector<Enumerator*> enumerators;
};

struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
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

struct Specifiers : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
  boolean inferred;  // C23 auto without an explicit type specifier
  vector<string_view> qualifiers;  // static, extern, const, volatile, inline...
  vector<AlignSpec*> aligns;
  vector<TypeSpec*> types;
};

// One `*` or (Apple Blocks) `^` of a declarator, with the qualifiers that
// apply to that pointer: `char * const * p` has two, and only the inner one
// is const.
struct Pointer : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
  vector<string_view> qualifiers;
  boolean isBlock;  // `^` rather than `*`
};

// What follows a declarator's core, in source order -- and the order is the
// type: `int *a[10]` is an array of pointers, `int (*a)[10]` a pointer to an
// array, the difference entirely in where parentheses place the suffixes.
struct Suffix : cursiv::Extends<cursiv::AstNode> {};

struct ArraySuffix : cursiv::Extends<Suffix> {
  vector<AttributeSpecifier*> attributes;
  vector<string_view> qualifiers;  // `int a[const 5]`, a parameter's own array
  boolean isStatic;                // `int a[static 5]`: at least this many
  boolean isStar;                  // `int a[*]`: complete variable-size type
  Expr* size;                      // absent in `int a[]`
};

struct FunctionSuffix : cursiv::Extends<Suffix> {
  vector<AttributeSpecifier*> attributes;
  vector<TypeName*> params;  // `(void)` is one parameter of type void
  boolean variadic;          // a trailing `, ...`, with or without named ones
};

// Concrete and abstract in one class: `int *x[3]` and `int *[3]` differ only
// by whether `name` was written, so an empty (not optional) `name` marks the
// abstract form. `nested`, the parenthesised form, is exclusive with `name`.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
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

// `.field = v`
struct FieldDesignator : cursiv::Extends<Designator> {
  string_view name;  // resolved against the aggregate's type: a semantic pass
};

// `[i] = v`, or the GNU range form `[lo ... hi] = v`
struct IndexDesignator : cursiv::Extends<Designator> {
  Expr* index;
  Expr* rangeEnd;  // null for an ordinary, single-element designator
};

// `.a[2].b = v` -- several designators, one initialiser.
struct DesignatedInit : cursiv::Extends<Initializer> {
  boolean obsoleteColon;
  boolean omittedEquals;
  vector<Designator*> designators;
  Initializer* value;
};

// One declarator with what may follow it: two grammar rules share this class,
// `InitDeclarator` setting declarator+init and `MemberDeclarator`
// declarator+bitWidth -- the latter alone for an anonymous bit field.
struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeSpecifier*> attributes;
  vector<string_view> assemblerNameParts;  // GNU asm label/register string pieces
  Declarator* declarator;
  Initializer* init;
  Expr* bitWidth;
};

// A declaration is an item and not a statement: C's grammar says a block item
// is a declaration or a statement, and this AST says the same. It is also what
// a struct's member list holds, and what a `for` clause may start with.
struct Declaration : cursiv::Extends<Item> {
  Specifiers* specifiers;
  vector<InitDeclarator*> declarators;
};

// `_Static_assert(cond, "msg")` / `static_assert(cond)`. File- and
// block-scope only: unlike a call, it has no operand form to fall back on, so
// it needs its own node to parse at file scope at all.
struct StaticAssertDecl : cursiv::Extends<Item> {
  Expr* cond;
  string_view message;  // empty when the message was omitted (C23)
};

// A standalone `[[...]];` or `__attribute__((...));`: the attribute list
// itself is matched and discarded (see AttributeSpec in the grammar, same as
// everywhere else an attribute may appear), so this marks only that one stood
// alone as a declaration.
struct AttributeDecl : cursiv::Extends<Item> {};

struct FunctionDef : cursiv::Extends<Item> {
  Specifiers* specifiers;
  Declarator* declarator;
  vector<Declaration*> paramDeclarations;  // K&R's separate `int a; char *b;`
  CompoundStmt* body;
};

// Both declaration and expression readings require the surrounding typedef
// scope; preserve the unresolved shape until that scope is available.
struct AmbiguousStmt : cursiv::Extends<Stmt> {
  string_view leftName;
  string_view form;  // ( or *
  string_view rightName;
};

// `expr;` and the empty statement `;`.
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

// `for (init; cond; step) body`. The init clause is an `Item`: C99 allows a
// declaration there and the older form is an expression statement, which is
// exactly the `Item` family, so the clause needs no union of its own.
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

// A label: the one resolvable name in this grammar, since `goto` and its
// target share one flat, typeless namespace per function body. Every other
// meaning a C identifier can have needs a symbol table this parser lacks.
struct Label : cursiv::Extends<Stmt> {
  string_view name;
  Stmt* body;
};

struct CaseLabel : cursiv::Extends<Stmt> {
  Expr* value;
  Expr* rangeEnd;  // GNU `case lo ... hi:`; null for an ordinary case
  Stmt* body;
};

struct DefaultLabel : cursiv::Extends<Stmt> {
  Stmt* body;
};

// GNU `__label__ name;`: declares a block-scoped label name distinct from an
// ordinary (function-scoped) one; nothing resolves against it.
struct LocalLabelDecl : cursiv::Extends<Stmt> {
  string_view name;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  reference<Label> target;
};

// GCC computed goto: `goto *expr;`, jumping to a value read from
// `&&label`/a jump table -- `target` is arbitrary, unlike `GotoStmt`'s.
struct ComputedGotoStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct BreakStmt : cursiv::Extends<Stmt> {};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

// One operand of an asm statement or file-scope asm: `"constraint"(expr)`,
// optionally named `[symbolicName] "constraint"(expr)`.
struct AsmOperand : cursiv::Extends<cursiv::AstNode> {
  string_view symbolicName;  // empty when unnamed
  StringLit* constraint;
  Expr* expr;
};

// `asm volatile ("code" : outputs : inputs : clobbers);` -- a statement.
// `clobbers` also absorbs any further `:`-separated group (GCC's "asm goto"
// jump-label list), which shares its comma-of-string-or-identifier shape.
struct AsmStmt : cursiv::Extends<Stmt> {
  vector<string_view> qualifiers;  // volatile, inline, goto, as written
  StringLit* code;
  vector<AsmOperand*> outputs;
  vector<AsmOperand*> inputs;
  vector<string_view> clobbers;
};

// File-scope `asm("..." : outputs : inputs);`.
struct AsmDecl : cursiv::Extends<Item> {
  StringLit* code;
  vector<AsmOperand*> outputs;
  vector<AsmOperand*> inputs;
};

// A translation unit: declarations, function definitions and file-scope asm
// in source order, which is why they share one list and one element type.
struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace c::ast
