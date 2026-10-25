// The odin language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace odin::ast {

// Anything that can stand in a block or at file scope: a declaration or a
// statement. Odin's declarations and statements share most shapes (a
// named/var declaration, a foreign block, a `when`), so one family covers
// both instead of two parallel hierarchies.
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Type : cursiv::Extends<cursiv::AstNode> {};

struct ImportItem : cursiv::Extends<cursiv::AstNode> {
  string_view alias;  // empty when the import has none
  string_view path;
};

struct Unit : cursiv::Extends<cursiv::AstNode> {
  string_view packageName;
  vector<ImportItem*> imports;
  vector<Stmt*> items;
};

struct AttributeElem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  vector<AttributeElem*> elems;
};

struct ForeignImportDecl : cursiv::Extends<Stmt> {
  string_view name;
  string_view path;
};

// A parametric argument list, `Table(string, int)`: each slot is a type or a
// (constant) expression, and the two families share no base a plain vector
// could hold, so the element is the variant itself.
struct TypeArgs : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Type*, Expr*>> args;
};

struct BasicType : cursiv::Extends<Type> {
  string_view keyword;
};

struct NamedType : cursiv::Extends<Type> {
  string_view pkg;  // empty when unqualified
  string_view name;
  TypeArgs* typeArgs;
};

// `$T` or `$T/Specialization`, standalone in type position.
struct PolyType : cursiv::Extends<Type> {
  string_view name;
  Type* specialization;
};

struct DistinctType : cursiv::Extends<Type> {
  Type* base;
};

struct PointerType : cursiv::Extends<Type> {
  Type* base;
};

struct MultiPointerType : cursiv::Extends<Type> {
  Type* base;
};

struct ArrayType : cursiv::Extends<Type> {
  Expr* size;         // the fixed-length form
  string_view polyDim;  // `[$N]T`; empty when not polymorphic
  boolean inferred;     // `[?]T`
  Type* elem;
};

struct SliceType : cursiv::Extends<Type> {
  Type* elem;
};

struct DynamicArrayType : cursiv::Extends<Type> {
  Type* elem;
  Expr* capacity;  // `[dynamic; expr]`; null without a fixed backing capacity
};

struct MapType : cursiv::Extends<Type> {
  Type* key;
  Type* value;
};

struct ReturnParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // empty for an unnamed return
  Type* type;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // empty for an unnamed parameter
  boolean isPoly;              // `$name` rather than a plain name
  boolean usingParam;
  boolean variadic;            // `..type`
  Type* type;
  Type* specialization;        // a poly parameter's `/type`
  Expr* defaultValue;
  boolean inferredDefault;     // `name := expr`, type omitted
};

struct ProcType : cursiv::Extends<Type> {
  string_view callingConvention;  // unquoted STRING; empty when none was given
  vector<Param*> params;
  vector<ReturnParam*> results;
};

struct WhereCondition : cursiv::Extends<cursiv::AstNode> {
  Expr* expr;
  string_view op;  // "==", "!=", or empty for a plain expression
  Type* type;
};

struct PolyParamGroup : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // `$Key, $Value: typeid`
  Type* type;
  Type* specialization;
};

struct StructField : cursiv::Extends<cursiv::AstNode> {
  boolean usingField;
  vector<string_view> names;
  Type* type;
  string_view tag;  // a trailing string tag, quotes/backticks included; empty
                    // when absent
};

struct StructType : cursiv::Extends<Type> {
  vector<string_view> tags;
  vector<PolyParamGroup*> polyParams;
  vector<Expr*> align;  // All #align(expr) occurrences, checked runtime.
  vector<Expr*> minFieldAlign;  // All #min_field_align(expr) occurrences.
  boolean packed;
  boolean rawUnion;
  boolean simple;
  boolean allOrNone;
  vector<WhereCondition*> whereConditions;
  vector<StructField*> fields;
};

struct EnumField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct EnumType : cursiv::Extends<Type> {
  Type* underlying;
  vector<EnumField*> fields;
};

struct UnionType : cursiv::Extends<Type> {
  vector<PolyParamGroup*> polyParams;
  boolean noNil;
  boolean sharedNil;
  boolean maybe;
  vector<Type*> variants;
};

// The shape shared by a set element and a switch case value: a range, or a
// plain value. `high`/`inclusive` are set only for the range form.
struct RangeExpr : cursiv::Extends<Expr> {
  Expr* low;
  Expr* high;
  boolean inclusive;
};

struct BitSetType : cursiv::Extends<Type> {
  std::variant<Expr*, Type*> elem;  // a RangeExpr (as Expr) or an enum type
  Type* word;                        // optional `; type` backing word
};

struct BitFieldEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
  Expr* width;
};

struct BitFieldType : cursiv::Extends<Type> {
  Type* backing;
  vector<BitFieldEntry*> entries;
};

struct MatrixType : cursiv::Extends<Type> {
  boolean rowMajor;
  Expr* rows;
  Expr* cols;
  Type* elem;
};

struct SoaArrayType : cursiv::Extends<Type> {
  Expr* size;       // null for the dynamic/slice forms
  boolean inferred;  // `#soa[?]T`
  boolean dynamic;   // `#soa[dynamic]T`
  Type* elem;
};

struct SparseArrayType : cursiv::Extends<Type> {
  Type* key;
  Type* elem;
};

struct ProcExpr : cursiv::Extends<Expr> {
  string_view callingConvention;
  vector<Param*> params;
  vector<ReturnParam*> results;
  vector<WhereCondition*> whereConditions;
  Stmt* body;  // a Block
};

// `proc{a, b, c}`: a procedure group. Members are kept as text, not
// `reference<>`: they may name overloads declared later in the same block,
// and this port does not model two-pass scoping.
struct ProcGroup : cursiv::Extends<Expr> {
  vector<string_view> members;
};

// `x :: rhs`, `x : T : expr`. Exactly one of `value`,
// `typeDecl` is set for a `::` declaration: `value` for a proc/proc group/
// expression, `typeDecl` for a struct/enum/union/bit_set/bit_field/matrix/
// distinct type. The two typed forms (`x : T : ...`) always set `type` and
// `value`.
struct NamedDecl : cursiv::Extends<Stmt> {
  vector<Attribute*> attributes;
  string_view name;
  Type* type;
  Expr* value;
  Type* typeDecl;
};

struct VarDecl : cursiv::Extends<Stmt> {
  vector<Attribute*> attributes;
  vector<string_view> names;
  Type* type;  // null for the `:=` inferred form
  vector<Expr*> values;
};

struct ForeignEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  string_view name;
  string_view callingConvention;
  vector<Param*> params;
  vector<ReturnParam*> results;
};

struct ForeignBlock : cursiv::Extends<Stmt> {
  vector<Attribute*> attributes;
  string_view name;
  vector<ForeignEntry*> entries;
};

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> items;
};

struct LabelStmt : cursiv::Extends<Stmt> {
  string_view name;
  Stmt* body;  // a ForStmt, SwitchStmt, TypeSwitchStmt or Block
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
  string_view op;
  vector<Expr*> values;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Stmt* init;
  Expr* cond;
  Block* thenBlock;
  Stmt* elseBranch;  // an IfStmt, a Block, or null
};

// A range form (`value[, index] in expr`, or a name list `in` a range) is
// the one that leaves `names` non-empty; the other three forms never touch
// it.
struct ForStmt : cursiv::Extends<Stmt> {
  string_view directive;  // "#reverse", "#unroll", or empty
  Stmt* init;              // the C-style clause's init
  Expr* cond;               // the C-style or while-condition form
  Stmt* post;               // the C-style clause's post
  boolean byRef;             // a leading `&` on the range form's value name
  vector<string_view> names;  // range-loop names, value[, index]
  Expr* rangeValue;           // the `in` operand -- a RangeExpr or a plain expr
  Block* body;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Expr*, Type*>> values;  // empty means the default case
  vector<Stmt*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  boolean partial;
  Stmt* init;
  Expr* tag;
  vector<SwitchCase*> cases;
};

// `switch name in value`: the cases are types, `nil` among them. `name` is
// empty for `switch in value`.
struct TypeSwitchStmt : cursiv::Extends<Stmt> {
  boolean partial;
  string_view name;
  Expr* value;
  vector<SwitchCase*> cases;
};

struct WhenStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
  WhenStmt* elseWhen;       // `else when ...`
  vector<Stmt*> elseBody;    // a plain `else { ... }`; empty when absent
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct DeferStmt : cursiv::Extends<Stmt> {
  Stmt* body;
};

struct UsingStmt : cursiv::Extends<Stmt> {
  Expr* target;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct FallthroughStmt : cursiv::Extends<Stmt> {};

struct AssertStmt : cursiv::Extends<Stmt> {
  WhereCondition* cond;
};

struct Ident : cursiv::Extends<Expr> {
  string_view pkg;  // empty when unqualified
  string_view name;
  TypeArgs* typeArgs;
};

struct BasicTypeExpr : cursiv::Extends<Expr> {
  string_view keyword;
};

// `.A`: an implicit enum/selector expression.
struct ImplicitSelectorExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ImagLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;  // quotes/backticks included, escapes raw
  boolean raw;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NilLit : cursiv::Extends<Expr> {};
struct UninitLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct LiteralElem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;  // null for a positional element
  Expr* value;
};

// An array literal (`[3]int{1,2,3}`), a nested struct literal, a bare
// `{...}`, and every other typed/untyped composite literal. At most one of
// `typeExpr` (a named type reached as an expression, `Vector2{1,2}`) and
// `typeSpec` (an inline type shape, `[]int{1,2}`) is set; neither is set
// for a bare literal.
struct CompositeLit : cursiv::Extends<Expr> {
  Expr* typeExpr;
  Type* typeSpec;
  vector<LiteralElem*> elems;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

// `.( type )` (union type assertion) and `.?` (optional unwrap) share one
// shape: `type` is null for the `.?` form.
struct UnwrapExpr : cursiv::Extends<Expr> {
  Expr* object;
  Type* type;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
  Expr* index2;  // set for the matrix two-index form `m[i, j]`
};

struct SliceExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* low;
  Expr* high;
  Expr* cap;
};

struct CallArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // set for a named argument
  Expr* value;
  boolean spread;     // `..expr`
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<CallArg*> args;
};

struct DerefExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct AssertExpr : cursiv::Extends<Expr> {
  WhereCondition* cond;
};

struct DefinedExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct ProcedureExpr : cursiv::Extends<Expr> {};

struct LoadExpr : cursiv::Extends<Expr> {
  Expr* path;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  string_view kind;  // "cast" or "transmute"
  Type* type;
  Expr* operand;
};

struct AutoCastExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `then if cond else else_`, and the compile-time `when` form (`compileTime`
// set).
struct CondExpr : cursiv::Extends<Expr> {
  Expr* thenValue;
  Expr* cond;
  Expr* elseValue;
  boolean compileTime;
};

// The C-style `cond ? then : else_`.
struct TernaryExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct OrElseExpr : cursiv::Extends<Expr> {
  Expr* value;
  Expr* fallback;
};

// `expr or_return`, `expr or_continue label?`, `expr or_break label?`: one
// shape, `kind` naming which.
struct OrJumpExpr : cursiv::Extends<Expr> {
  Expr* value;
  string_view kind;  // "or_return", "or_continue" or "or_break"
  string_view label;
};

}  // namespace odin::ast
