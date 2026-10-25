// The java language: its AST.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace java::ast {

struct Annotation;
struct AnnotationValue;
struct TypeRef;
struct TypeArg;
struct TypeParam;
struct Item;
struct Stmt;
struct Expr;
struct Block;
struct Pattern;
struct VarDeclarator;

// -- Annotations ----

struct AnnotationValue : cursiv::Extends<cursiv::AstNode> {};

struct AnnotationExprValue : cursiv::Extends<AnnotationValue> {
  Expr *value;
};

struct AnnotationArrayValue : cursiv::Extends<AnnotationValue> {
  vector<AnnotationValue *> items;
};

struct AnnotationArg : cursiv::Extends<cursiv::AstNode> {
  string_view name; // empty for a single positional value
  AnnotationValue *value;
};

struct Annotation : cursiv::Extends<AnnotationValue> {
  string_view name;
  vector<AnnotationArg *> args;
};

// -- Types (usage) ----

struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

struct PrimitiveTypeRef : cursiv::Extends<TypeRef> {
  vector<Annotation *> annotations;
  string_view keyword;
};

struct VoidTypeRef : cursiv::Extends<TypeRef> {};

struct TypeArg : cursiv::Extends<cursiv::AstNode> {};

struct TypeArgType : cursiv::Extends<TypeArg> {
  TypeRef *type;
};

struct WildcardTypeArg : cursiv::Extends<TypeArg> {
  vector<Annotation *> annotations;
  boolean superBound; // true: `? super T`; false: `? extends T`
  TypeRef *bound;     // null: unbounded `?`
};

// One dotted segment of a class/interface type, nesting outward:
// `Outer<T>.Inner<U>` is a ClassTypeRef "Inner" whose qualifier is "Outer".
struct ClassTypeRef : cursiv::Extends<TypeRef> {
  ClassTypeRef *qualifier;
  vector<Annotation *> annotations;
  string_view name;
  vector<TypeArg *> typeArguments;
};

// One array dimension wrapping its element type; `int[][]` nests two.
struct ArrayTypeRef : cursiv::Extends<TypeRef> {
  TypeRef *element;
  vector<Annotation *> annotations;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  string_view name;
  vector<TypeRef *> bounds;
};

// -- Patterns (instanceof, switch) ----

struct Pattern : cursiv::Extends<cursiv::AstNode> {};

// Real Java binds exactly one, uninitialized, name; `declarators` stays a
// list because the multi-declarator production is reused here.
struct TypePattern : cursiv::Extends<Pattern> {
  vector<string_view> modifiers;
  vector<Annotation *> annotations;
  TypeRef *type;
  vector<VarDeclarator *> declarators;
};

struct RecordPattern : cursiv::Extends<Pattern> {
  TypeRef *type;
  vector<Pattern *> components;
};

// -- Expressions ----

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};

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
struct TextBlockLit : cursiv::Extends<Expr> {
  string_view text;
};
struct BoolLit : cursiv::Extends<Expr> {
  string_view text;
};
struct NullLit : cursiv::Extends<Expr> {};

struct ClassLit : cursiv::Extends<Expr> {
  TypeRef *type;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr *array;
  Expr *index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<TypeArg *> typeArguments;
  vector<Expr *> args;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  vector<TypeArg *>
      typeArguments; // explicit generic invocation: `obj.<T>method(...)`
  string_view field;
};

struct MethodRefExpr : cursiv::Extends<Expr> {
  Expr *target;        // set for an ordinary expression target
  TypeRef *typeTarget; // set instead for a bare type: `int[]::new`,
                       // `List<String>::new`
  vector<TypeArg *> typeArguments;
  string_view name; // "new" for a constructor reference
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  TypeRef *type;
  vector<TypeRef *> intersectionTypes; // `(A & B) x`
  Expr *operand;
};

struct Initializer : cursiv::Extends<cursiv::AstNode> {};
struct ExprInit : cursiv::Extends<Initializer> {
  Expr *value;
};
struct ArrayInit : cursiv::Extends<Initializer> {
  vector<Initializer *> items;
};

struct NewObjectExpr : cursiv::Extends<Expr> {
  Expr *outer; // qualified: `outer.new Inner()`
  vector<TypeArg *> typeArguments;
  TypeRef *type;
  vector<Expr *> args;
  boolean anonymousBody;
  vector<Item *> classBody;
};

struct ArrayDimension : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  Expr *size; // null denotes an unsized dimension
};

struct NewArrayExpr : cursiv::Extends<Expr> {
  TypeRef *elementType;
  vector<ArrayDimension *> dimensions;
  ArrayInit *init;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr *value;
  TypeRef *type;    // set for a plain type test
  Pattern *pattern; // set instead for a pattern test
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr *cond;
  Expr *thenValue;
  Expr *elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr *target;
  string_view op;
  Expr *value;
};

struct LambdaParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  TypeRef *type; // null: inferred (bare name, or explicit `var`)
  boolean isVar;
  string_view name;
};

struct LambdaExpr : cursiv::Extends<Expr> {
  vector<LambdaParam *> params;
  Expr *bodyExpr;
  Block *bodyBlock;
};

// `<T>this(...)`, `<T>super(...)`, `<T>name(...)`, and the qualified
// `<T>super.<U>name(...)` (`qualifiedMember` set; `target` is then "super").
struct ExplicitGenericInvokeExpr : cursiv::Extends<Expr> {
  vector<TypeArg *> typeArguments;
  string_view target; // "this", "super", or a method name
  vector<Expr *> args;
  string_view qualifiedMember;
  vector<TypeArg *> memberTypeArguments;
  boolean invoked; // an argument list followed (vs. a bare qualified read)
};

// -- switch (as an expression) ----

struct CaseRule : cursiv::Extends<cursiv::AstNode> {};

// `values` holds one `NullLit` for a bare `case null ->`; `alsoDefault` is
// only meaningful then (`case null, default ->`).
struct ExprCaseRule : cursiv::Extends<CaseRule> {
  vector<Expr *> values;
  boolean alsoDefault;
  boolean arrow;
  Block *body;
};

struct PatternCaseRule : cursiv::Extends<CaseRule> {
  vector<Pattern *> patterns;
  Expr *guard; // `when` clause, or null
  boolean arrow;
  Block *body;
};

struct DefaultCaseRule : cursiv::Extends<CaseRule> {
  boolean arrow;
  Block *body;
};

struct SwitchExpr : cursiv::Extends<Expr> {
  Expr *value;
  vector<CaseRule *> rules;
};

// -- Declarations shared shapes ----

struct VarDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<ArrayDimension *> extraDims;
  Initializer *init;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  TypeRef *type;
  boolean varargs;
  vector<Annotation *> varargsAnnotations;
  boolean isReceiver; // explicit `this` parameter, e.g. `Outer.this`
  string_view name;   // parameter name, or the receiver's qualifier
  vector<ArrayDimension *> extraDims;
};

// A class/interface/record body member, a block item, or a compilation
// unit's type: one shared base for every list that mixes declarations and
// statements the way Java's own grammar does.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct FieldDecl : cursiv::Extends<Item> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  TypeRef *type;
  vector<VarDeclarator *> declarators;
};

struct MethodDecl : cursiv::Extends<Item> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  vector<TypeParam *> typeParams;
  TypeRef *returnType;
  vector<ArrayDimension *> returnExtraDims;
  string_view name;
  vector<Param *> params;
  vector<TypeRef *> throwsList;
  Block *body; // null: abstract/interface/native (`;` body)
};

struct ConstructorDecl : cursiv::Extends<Item> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  vector<TypeParam *> typeParams;
  string_view name;
  vector<Param *> params;
  vector<TypeRef *> throwsList;
  Block *body;
};

struct CompactConstructorDecl : cursiv::Extends<Item> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  Block *body;
};

struct InitBlock : cursiv::Extends<Item> {
  boolean isStatic;
  Block *body;
};

struct TypeDecl : cursiv::Extends<Item> {};

struct ClassDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam *> typeParams;
  TypeRef *superclass;
  vector<TypeRef *> interfaces;
  vector<TypeRef *> permits;
  vector<Item *> members;
};

struct InterfaceDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam *> typeParams;
  vector<TypeRef *> extendsList;
  vector<TypeRef *> permits;
  vector<Item *> members;
};

struct EnumConstant : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  string_view name;
  vector<Expr *> args;
  boolean hasBody;
  vector<Item *> classBody;
};

struct EnumDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeRef *> interfaces;
  vector<EnumConstant *> constants;
  vector<Item *> members;
};

// Only the last component of a record header may vary its arity
// (`String... rest`): checked by the validator, not the grammar.
struct RecordComponent : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  TypeRef *type;
  boolean varargs;
  vector<Annotation *> varargsAnnotations;
  string_view name;
};

struct RecordDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam *> typeParams;
  vector<RecordComponent *> components;
  vector<TypeRef *> interfaces;
  vector<Item *> members;
};

struct AnnotationTypeElement : cursiv::Extends<Item> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  TypeRef *type;
  string_view name;
  AnnotationValue *defaultValue;
};

struct AnnotationTypeDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<Item *> members;
};

// -- Statements ----

struct Stmt : cursiv::Extends<Item> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Item *> items;
};

struct LocalVarDecl : cursiv::Extends<Stmt> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  TypeRef *type; // null when `var`
  boolean isVar;
  vector<VarDeclarator *> declarators;
};

// `expr;` and the empty statement `;`.
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct AssertStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Expr *message;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Stmt *thenBranch;
  Stmt *elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Stmt *body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt *body;
  Expr *cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  LocalVarDecl *initDecl;
  vector<Expr *> initExprs;
  Expr *cond;
  vector<Expr *> update;
  Stmt *body;
};

struct ForEachStmt : cursiv::Extends<Stmt> {
  vector<string_view> modifiers;
  TypeRef *type; // null when `var`
  boolean isVar;
  string_view name;
  vector<ArrayDimension *> extraDims;
  Expr *iterable;
  Stmt *body;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  vector<TypeRef *> types; // multi-catch: `catch (A | B e)`
  string_view name;
  Block *body;
};

// A declared resource, or (when `init` is null) a bare name referring to an
// existing effectively-final variable.
struct Resource : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  vector<string_view> modifiers;
  TypeRef *type;
  boolean isVar;
  string_view name;
  vector<ArrayDimension *> extraDims;
  Expr *init;
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<Resource *> resources;
  Block *body;
  vector<CatchClause *> catches;
  Block *finallyBlock;
};

struct SyncStmt : cursiv::Extends<Stmt> {
  Expr *lock;
  Block *body;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct YieldStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt *body;
};

struct SwitchLabel : cursiv::Extends<cursiv::AstNode> {};

struct CaseLabel : cursiv::Extends<SwitchLabel> {
  Expr *constant;       // a constant-expression case
  TypeRef *patternType; // an old-style type-pattern case: `case String s:`
  string_view patternName;
};

struct DefaultLabel : cursiv::Extends<SwitchLabel> {};

// One or more labels followed by the statements they guard; also used for a
// trailing run of empty labels, whose `statements` is then empty.
struct SwitchGroup : cursiv::Extends<cursiv::AstNode> {
  vector<SwitchLabel *> labels;
  vector<Item *> statements;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr *value;
  vector<SwitchGroup *> groups;
};

// -- Top level ----

struct PackageDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  string_view name;
};

struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  string_view qualifiedName;
  boolean onDemand; // trailing `.*`
};

struct ModuleDirective : cursiv::Extends<cursiv::AstNode> {};

struct RequiresDirective : cursiv::Extends<ModuleDirective> {
  boolean isTransitive;
  boolean isStatic;
  string_view module_;
};

struct ExportsDirective : cursiv::Extends<ModuleDirective> {
  string_view package_;
  vector<string_view> to;
};

struct OpensDirective : cursiv::Extends<ModuleDirective> {
  string_view package_;
  vector<string_view> to;
};

struct UsesDirective : cursiv::Extends<ModuleDirective> {
  string_view service;
};

struct ProvidesDirective : cursiv::Extends<ModuleDirective> {
  string_view service;
  vector<string_view> with;
};

struct ModuleDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation *> annotations;
  boolean open;
  string_view name;
  vector<ModuleDirective *> directives;
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  PackageDecl *package_;
  vector<ImportDecl *> imports;
  vector<TypeDecl *> types;
  ModuleDecl *module_; // set instead of package_/types for a module-info file
};

} // namespace java::ast
