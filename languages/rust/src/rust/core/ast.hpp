// The rust language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rust::ast {

// Paths and generics -- shared by expressions, types and patterns.

struct GenericArgs;
struct GenericParams;
struct Ty;
struct Expr;

struct PathSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  GenericArgs* args;
  vector<Ty*> fnSugarParams;  // `Fn(A, B) -> C` path-fn sugar; empty if unused
  Ty* fnSugarReturn;
};

// Covers a plain path and a qualified path (`<T as Trait>::rest`) alike:
// qualifierType set means the path opened on `<`.
struct Path : cursiv::Extends<cursiv::AstNode> {
  boolean leadingSep;
  Ty* qualifierType;
  Path* qualifierTrait;
  vector<PathSegment*> segments;
};

struct TokenTree : cursiv::Extends<cursiv::AstNode> {
  string_view token;
  string_view open;
  vector<TokenTree*> children;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  TokenTree* tokens;
  Path* path;
  string_view
      literalArg;  // `#[path = "literal"]`; delimited arguments are in tokens.
};

struct TraitBound : cursiv::Extends<cursiv::AstNode> {
  boolean maybe;  // `?Sized`
  GenericParams* forLifetimes;
  Path* path;
};

// A lifetime name, or a trait bound -- the two shapes a type-param bound
// or a trait-object/impl-trait bound spells. Namespace scope, so the
// AstNode vocabulary is out of reach and the std spelling is the only one.
using Bound = std::variant<std::string_view, TraitBound*>;

struct GenericArg : cursiv::Extends<cursiv::AstNode> {};
struct LifetimeArg : cursiv::Extends<GenericArg> {
  string_view name;
};
struct TypeArg : cursiv::Extends<GenericArg> {
  Ty* type;
};
struct ConstArg : cursiv::Extends<GenericArg> {
  Expr* value;
};
struct BindingArg : cursiv::Extends<GenericArg> {
  string_view name;
  Ty* type;
};
struct GenericArgs : cursiv::Extends<cursiv::AstNode> {
  vector<GenericArg*> args;
};

struct GenericParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
};
struct LifetimeParam : cursiv::Extends<GenericParam> {
  string_view name;
  vector<string_view> bounds;
};
struct TypeParam : cursiv::Extends<GenericParam> {
  string_view name;
  vector<Bound> bounds;
  Ty* defaultType;
};
struct ConstParam : cursiv::Extends<GenericParam> {
  string_view name;
  Ty* type;
  Expr* defaultValue;
};
struct GenericParams : cursiv::Extends<cursiv::AstNode> {
  vector<GenericParam*> params;
};

struct WhereClauseItem : cursiv::Extends<cursiv::AstNode> {};
struct LifetimeWhereItem : cursiv::Extends<WhereClauseItem> {
  string_view lifetime;
  vector<string_view> bounds;
};
struct TypeBoundWhereItem : cursiv::Extends<WhereClauseItem> {
  GenericParams* forLifetimes;
  Ty* type;
  vector<Bound> bounds;
};
struct WhereClause : cursiv::Extends<cursiv::AstNode> {
  vector<WhereClauseItem*> items;
};

// Attributes and visibility.

struct Visibility : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // "crate", "self", "super", "in", or empty for bare `pub`
  Path* inPath;
};

// Types.

struct Ty : cursiv::Extends<cursiv::AstNode> {};
struct PathTy : cursiv::Extends<Ty> {
  Path* path;
};
struct TupleTy : cursiv::Extends<Ty> {
  vector<Ty*> elements;
};
struct NeverTy : cursiv::Extends<Ty> {};
struct RawPtrTy : cursiv::Extends<Ty> {
  boolean isMut;
  Ty* inner;
};
struct RefTy : cursiv::Extends<Ty> {
  string_view lifetime;
  boolean isMut;
  Ty* inner;
};
struct ArrayTy : cursiv::Extends<Ty> {
  Ty* element;
  Expr* size;
};
struct SliceTy : cursiv::Extends<Ty> {
  Ty* element;
};
struct InferredTy : cursiv::Extends<Ty> {};
struct BareFnParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  optional<string_view> name;
  Ty* type;
};
struct BareFnTy : cursiv::Extends<Ty> {
  GenericParams* forLifetimes;
  boolean isUnsafe;
  boolean isExtern;
  string_view abi;
  vector<BareFnParam*> params;  // empty entry where a parameter is unnamed
  boolean variadic;
  Ty* returnType;
};
struct ImplTraitTy : cursiv::Extends<Ty> {
  vector<Bound> bounds;
};
struct TraitObjectTy : cursiv::Extends<Ty> {
  boolean isDyn;
  vector<Bound> bounds;
};
struct ParenTy : cursiv::Extends<Ty> {
  Ty* inner;
};
struct MacroTy : cursiv::Extends<Ty> {
  TokenTree* tokens;
  Path* path;
};

// Patterns.

struct Pat : cursiv::Extends<cursiv::AstNode> {};
struct OrPat : cursiv::Extends<Pat> {
  vector<Pat*> alternatives;
};
struct LiteralPat : cursiv::Extends<Pat> {
  boolean negative;
  string_view text;
};
struct IdentPat : cursiv::Extends<Pat> {
  boolean isRef;
  boolean isMut;
  string_view name;
  Pat* subPattern;
};
struct WildcardPat : cursiv::Extends<Pat> {};
struct RestPat : cursiv::Extends<Pat> {};
struct RefPat : cursiv::Extends<Pat> {
  boolean doubleRef;
  boolean isMut;
  Pat* inner;
};
struct StructPatField : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  string_view name;  // identifier or tuple-index text
  Pat* pattern;       // null for the `ref? mut? identifier` shorthand
  boolean isRef;
  boolean isMut;
};
struct StructPat : cursiv::Extends<Pat> {
  Path* path;
  vector<StructPatField*> fields;
  boolean hasRest;
};
struct TupleStructPat : cursiv::Extends<Pat> {
  Path* path;
  vector<Pat*> elements;
};
struct TuplePat : cursiv::Extends<Pat> {
  vector<Pat*> elements;
};
struct GroupedPat : cursiv::Extends<Pat> {
  Pat* inner;
};
struct SlicePat : cursiv::Extends<Pat> {
  vector<Pat*> elements;
};
struct PathPat : cursiv::Extends<Pat> {
  Path* path;
};
struct RangePatBound : cursiv::Extends<cursiv::AstNode> {
  boolean negative;
  string_view text;
  Path* path;
};
struct RangePat : cursiv::Extends<Pat> {
  boolean inclusive;
  boolean obsolete;  // `...`, superseded by `..=`
  RangePatBound* start;
  RangePatBound* end;
};
struct MacroPat : cursiv::Extends<Pat> {
  TokenTree* tokens;
  Path* path;
};

// Expressions.

struct Stmt;

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct LiteralExpr : cursiv::Extends<Expr> {
  string_view text;
};
struct PathExpr : cursiv::Extends<Expr> {
  Path* path;
};
struct MethodCallExpr : cursiv::Extends<Expr> {
  Expr* receiver;
  string_view method;
  GenericArgs* genericArgs;
  vector<Expr*> args;
};
struct FieldExpr : cursiv::Extends<Expr> {
  Expr* receiver;
  string_view field;
};
struct TupleIndexExpr : cursiv::Extends<Expr> {
  Expr* receiver;
  string_view index;
};
struct AwaitExpr : cursiv::Extends<Expr> {
  Expr* receiver;
};
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* receiver;
  Expr* index;
};
struct TryExpr : cursiv::Extends<Expr> {
  Expr* receiver;
};
struct BorrowExpr : cursiv::Extends<Expr> {
  boolean doubleAmp;
  boolean isMut;
  Expr* operand;
  boolean isRaw; // &raw const / &raw mut, distinct from a reference borrow
};
struct DerefExpr : cursiv::Extends<Expr> {
  Expr* operand;
};
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "-" or "!"
  Expr* operand;
};
struct CastExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Ty* type;
};

// The operation a binary or assignment level applies, not its text: one
// enum and one node serve every precedence tier from `*` to `>>=`.
enum class Operator : std::uint8_t {
  multiply, divide, remainder,
  add, subtract,
  shl, shr,
  bit_and, bit_xor, bit_or,
  eq, ne, lt, gt, le, ge,
  and_and, or_or,
  assign, add_assign, sub_assign, mul_assign, div_assign, rem_assign,
  bitand_assign, bitor_assign, bitxor_assign, shl_assign, shr_assign,
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

struct RangeExpr : cursiv::Extends<Expr> {
  Expr* start;
  Expr* end;
  boolean inclusive;
};

struct ContinueExpr : cursiv::Extends<Expr> {
  string_view label;
  Expr* value; // legacy field, always null: continue has no value operand
};
struct BreakExpr : cursiv::Extends<Expr> {
  string_view label;
  Expr* value;
};
struct ReturnExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct GroupedExpr : cursiv::Extends<Expr> {
  vector<Attribute*> innerAttrs;
  Expr* inner;
};
struct ArrayExpr : cursiv::Extends<Expr> {
  vector<Attribute*> innerAttrs;
  vector<Expr*> elements;
  Expr* repeatValue;
  Expr* repeatCount;
};
struct TupleExpr : cursiv::Extends<Expr> {
  vector<Attribute*> innerAttrs;
  vector<Expr*> elements;
};

struct StructExprField : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  string_view name;
  Expr* value;  // null for the `identifier` shorthand
};
// Serves both a struct literal and an enum-variant literal: the two are
// syntactically identical, resolved against the same path either way.
// Brace constructors; tuple constructors and function calls use CallExpr.
struct StructExpr : cursiv::Extends<Expr> {
  Path* path;
  vector<Attribute*> innerAttrs;
  vector<StructExprField*> fields;
  Expr* base;  // `..expr`
  vector<Expr*> tupleArgs;
  boolean isTuple;
  boolean isUnit;
};

struct ClosureParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  Pat* pattern;
  Ty* type;
};
struct ClosureExpr : cursiv::Extends<Expr> {
  boolean isAsync;
  boolean isMove;
  vector<ClosureParam*> params;
  Ty* returnType;
  Expr* body;
};

struct BlockExpr : cursiv::Extends<Expr> {
  vector<Attribute*> innerAttrs;
  vector<Stmt*> statements;
  Expr* tailExpr;
};
struct AsyncBlockExpr : cursiv::Extends<Expr> {
  boolean isMove;
  BlockExpr* body;
};
struct UnsafeBlockExpr : cursiv::Extends<Expr> {
  BlockExpr* body;
};

struct LoopExpr : cursiv::Extends<Expr> {
  string_view label;
};
struct InfiniteLoopExpr : cursiv::Extends<LoopExpr> {
  BlockExpr* body;
};
struct WhileLoopExpr : cursiv::Extends<LoopExpr> {
  Expr* cond;
  BlockExpr* body;
};
struct WhileLetLoopExpr : cursiv::Extends<LoopExpr> {
  Pat* pattern;
  Expr* value;
  BlockExpr* body;
};
struct ForLoopExpr : cursiv::Extends<LoopExpr> {
  Pat* pattern;
  Expr* iter;
  BlockExpr* body;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  BlockExpr* thenBranch;
  Expr* elseBranch;  // BlockExpr*, IfExpr* or IfLetExpr*
};
struct IfLetExpr : cursiv::Extends<Expr> {
  Pat* pattern;
  Expr* value;
  BlockExpr* thenBranch;
  Expr* elseBranch;
};

struct MatchArm : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  Pat* pattern;
  Expr* guard;
  Expr* body;
};
struct MatchExpr : cursiv::Extends<Expr> {
  Expr* subject;
  vector<Attribute*> innerAttrs;
  vector<MatchArm*> arms;
};

struct AttrExpr : cursiv::Extends<Expr> {
  vector<Attribute*> attrs;
  Expr* value;
};

struct MacroExpr : cursiv::Extends<Expr> {
  TokenTree* tokens;
  Path* path;
};

// Statements and items.

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

// Items double as statements, matching Rust: a `fn`/`struct`/... nested in
// a block is both an Item and a Stmt.
struct Item : cursiv::Extends<Stmt> {
  vector<Attribute*> attrs;
};

struct LetStmt : cursiv::Extends<Stmt> {
  vector<Attribute*> attrs;
  Pat* pattern;
  Ty* type;
  Expr* value;
  BlockExpr* elseBranch;
};
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct MacroStmt : cursiv::Extends<Item> {
  TokenTree* tokens;
  Path* path;
};

struct FnParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  boolean isSelf;
  boolean selfByRef;
  string_view selfLifetime;
  boolean isMut;
  Ty* selfType;
  Pat* pattern;
  Ty* type;
  boolean variadic;
};
struct Function : cursiv::Extends<Item> {
  Visibility* vis;
  boolean isConst;
  boolean isAsync;
  boolean isUnsafe;
  boolean isExtern;
  string_view abi;
  string_view name;
  GenericParams* generics;
  vector<FnParam*> params;
  Ty* returnType;
  WhereClause* whereClause;
  BlockExpr* body;  // null when the declaration ends in `;`
};

struct TypeAlias : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;
  GenericParams* generics;
  WhereClause* whereClause;
  Ty* type;
};

struct StructField : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  Visibility* vis;
  string_view name;
  Ty* type;
};
struct TupleField : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  Visibility* vis;
  Ty* type;
};
struct Struct : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;
  GenericParams* generics;
  WhereClause* whereClause;
  vector<StructField*> fields;
  vector<TupleField*> tupleFields;
  boolean isTuple;
};

struct EnumVariant : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attrs;
  Visibility* vis;
  string_view name;
  vector<TupleField*> tupleFields;
  vector<StructField*> structFields;
  Expr* discriminant;
  boolean isTuple;
  boolean isStruct;
};
struct Enum : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;
  GenericParams* generics;
  WhereClause* whereClause;
  vector<EnumVariant*> variants;
};

struct Union : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;
  GenericParams* generics;
  WhereClause* whereClause;
  vector<StructField*> fields;
};

struct ConstItem : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;  // empty for `const _`
  Ty* type;
  Expr* value;
};
struct StaticItem : cursiv::Extends<Item> {
  Visibility* vis;
  boolean isMut;
  string_view name;
  Ty* type;
  Expr* value;
};

struct Trait : cursiv::Extends<Item> {
  Visibility* vis;
  boolean isUnsafe;
  string_view name;
  GenericParams* generics;
  vector<Bound> supertraits;
  WhereClause* whereClause;
  vector<Attribute*> innerAttrs;
  vector<Item*> items;
};

struct Impl : cursiv::Extends<Item> {
  boolean isUnsafe;
  GenericParams* generics;
  boolean isNegative;
  Path* traitRef;  // null for an inherent impl
  Ty* type;        // the Self type
  WhereClause* whereClause;
  vector<Attribute*> innerAttrs;
  vector<Item*> items;
};

struct ExternBlock : cursiv::Extends<Item> {
  Visibility* vis;
  boolean isUnsafe;
  string_view abi;
  vector<Attribute*> innerAttrs;
  vector<Item*> items;
};

struct Module : cursiv::Extends<Item> {
  Visibility* vis;
  boolean isUnsafe;
  string_view name;
  boolean isInline;  // has a `{ ... }` body, vs `mod name;`
  vector<Attribute*> innerAttrs;
  vector<Item*> items;
};

struct ExternCrate : cursiv::Extends<Item> {
  Visibility* vis;
  string_view name;
  string_view asName;
};

struct UseTree : cursiv::Extends<cursiv::AstNode> {
  Path* path;
  boolean leadingSep; // absolute root on pathless ::* / ::{...} imports
  boolean isGlob;
  vector<UseTree*> nested;
  string_view asName;
};
struct UseDecl : cursiv::Extends<Item> {
  Visibility* vis;
  UseTree* tree;
};

struct MacroRulesDef : cursiv::Extends<Item> {
  TokenTree* tokens;
  string_view name;
};

struct Crate : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> innerAttrs;
  vector<Item*> items;
};

}  // namespace rust::ast
