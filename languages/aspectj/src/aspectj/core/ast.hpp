// The aspectj language: its AST. Java plus the aspect constructs, in one
// namespace: pointcuts, advice, inter-type declarations, declare forms and
// the annotation-style (@Aspect) forms.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace aspectj::ast {

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
struct PointcutExpr;
struct TypePattern;
struct MethodPattern;
struct ConstructorPattern;
struct FieldPattern;
struct FormalsItem;
struct AnnotationPatternClause;
struct ClassPatternRef;
struct PerClause;

// -- Annotations ----
// `Annotation` is a base: an ordinary `@Name(...)` use, or one of the
// annotation-style aspect forms (`@Aspect`, `@Before("pc()")`...) that an
// ordinary annotation position may also carry.

struct AnnotationValue : cursiv::Extends<cursiv::AstNode> {};

struct AnnotationExprValue : cursiv::Extends<AnnotationValue> {
  Expr* value;
};

struct AnnotationArrayValue : cursiv::Extends<AnnotationValue> {
  vector<AnnotationValue*> items;
};

struct AnnotationArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // empty for a single positional value
  AnnotationValue* value;
};

// An annotation can itself be the value of another annotation's argument
// (`@Foo(nested = @Bar)`), so it derives `AnnotationValue` too.
struct Annotation : cursiv::Extends<AnnotationValue> {};

struct PlainAnnotation : cursiv::Extends<Annotation> {
  string_view name;
  vector<AnnotationArg*> args;
};

// @After/@AfterThrowing/@Around/@Before/@DeclareError/@DeclareWarning: one
// quoted pointcut expression, told apart by `kind`.
struct AdvicePointcutAnnotation : cursiv::Extends<Annotation> {
  string_view kind;
  PointcutExpr* pointcut;
};

// @AfterReturning("pc()")  or  @AfterReturning(pointcut="pc()", returning="x")
struct AfterReturningAnnotation : cursiv::Extends<Annotation> {
  PointcutExpr* pointcut;
  string_view returning;
};

// @Aspect  or  @Aspect("perClause")
struct AspectAnnotation : cursiv::Extends<Annotation> {
  PerClause* perClause;
};

// @DeclareMixin(value="typePattern", interfaces={A.class, B.class})
struct DeclareMixinAnnotation : cursiv::Extends<Annotation> {
  TypePattern* valuePattern;
  vector<ClassPatternRef*> interfaces;
};

// @DeclareParents("typePattern")  or
// @DeclareParents(value="typePattern", defaultImpl=X.class)
struct DeclareParentsAnnotation : cursiv::Extends<Annotation> {
  TypePattern* pattern;
  ClassPatternRef* defaultImpl;
};

// @DeclarePrecedence("A, B, C")
struct DeclarePrecedenceAnnotation : cursiv::Extends<Annotation> {
  vector<TypePattern*> order;
};

// @Pointcut("pc()"), or a bare @Pointcut("") -- the expression is optional.
struct PointcutAnnotation : cursiv::Extends<Annotation> {
  PointcutExpr* pointcut;
};

// -- Types (usage) ----

struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

struct PrimitiveTypeRef : cursiv::Extends<TypeRef> {
  string_view keyword;
};

struct VoidTypeRef : cursiv::Extends<TypeRef> {};

struct TypeArg : cursiv::Extends<cursiv::AstNode> {};

struct TypeArgType : cursiv::Extends<TypeArg> {
  TypeRef* type;
};

struct WildcardTypeArg : cursiv::Extends<TypeArg> {
  vector<Annotation*> annotations;
  boolean superBound;  // true: `? super T`; false: `? extends T`
  TypeRef* bound;       // null: unbounded `?`
};

// One dotted segment of a class/interface type, nesting outward:
// `Outer<T>.Inner<U>` is a ClassTypeRef "Inner" whose qualifier is "Outer".
struct ClassTypeRef : cursiv::Extends<TypeRef> {
  ClassTypeRef* qualifier;
  vector<Annotation*> annotations;
  string_view name;
  vector<TypeArg*> typeArguments;
};

// One array dimension wrapping its element type; `int[][]` nests two.
struct ArrayTypeRef : cursiv::Extends<TypeRef> {
  TypeRef* element;
  vector<Annotation*> annotations;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  vector<TypeRef*> bounds;
};

// -- Patterns (instanceof, switch) ----

struct Pattern : cursiv::Extends<cursiv::AstNode> {};

// Real Java binds exactly one, uninitialized, name; `declarators` stays a
// list because the multi-declarator production is reused here.
struct InstanceOfPattern : cursiv::Extends<Pattern> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
};

struct RecordPattern : cursiv::Extends<Pattern> {
  TypeRef* type;
  vector<Pattern*> components;
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
  TypeRef* type;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<TypeArg*> typeArguments;
  vector<Expr*> args;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  vector<TypeArg*> typeArguments;  // explicit generic invocation: `obj.<T>method(...)`
  string_view field;
};

struct MethodRefExpr : cursiv::Extends<Expr> {
  Expr* target;         // set for an ordinary expression target
  TypeRef* typeTarget; // set instead for a bare type: `int[]::new`,
                       // `List<String>::new`
  vector<TypeArg*> typeArguments;
  string_view name;  // "new" for a constructor reference
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  vector<Annotation*> annotations;
  TypeRef* type;
  vector<TypeRef*> intersectionTypes;  // `(A & B) x`
  Expr* operand;
};

struct Initializer : cursiv::Extends<cursiv::AstNode> {};
struct ExprInit : cursiv::Extends<Initializer> {
  Expr* value;
};
struct ArrayInit : cursiv::Extends<Initializer> {
  vector<Initializer*> items;
};

struct NewObjectExpr : cursiv::Extends<Expr> {
  Expr* outer;  // qualified: `outer.new Inner()`
  vector<TypeArg*> typeArguments;
  TypeRef* type;
  vector<Expr*> args;
  boolean anonymousBody;
  vector<Item*> classBody;
};

struct ArrayDimension : cursiv::Extends<cursiv::AstNode> {
  Expr *value;
};

struct NewArrayExpr : cursiv::Extends<Expr> {
  TypeRef* elementType;
  vector<ArrayDimension *> dimensions;
  ArrayInit* init;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr* value;
  TypeRef* type;     // set for a plain type test
  Pattern* pattern;   // set instead for a pattern test
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

struct LambdaParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  TypeRef* type;  // null: inferred (bare name, or explicit `var`)
  boolean isVar;
  string_view name;
};

struct LambdaExpr : cursiv::Extends<Expr> {
  vector<LambdaParam*> params;
  Expr* bodyExpr;
  Block* bodyBlock;
};

// `<T>this(...)`, `<T>super(...)`, `<T>name(...)`, and the qualified
// `<T>super.<U>name(...)` (`qualifiedMember` set; `target` is then "super").
struct ExplicitGenericInvokeExpr : cursiv::Extends<Expr> {
  vector<TypeArg*> typeArguments;
  string_view target;  // "this", "super", or a method name
  vector<Expr*> args;
  string_view qualifiedMember;
  vector<TypeArg*> memberTypeArguments;
  boolean invoked;  // an argument list followed (vs. a bare qualified read)
};

// -- switch (as an expression) ----

struct CaseRule : cursiv::Extends<cursiv::AstNode> {};

// `values` holds one `NullLit` for a bare `case null ->`; `alsoDefault` is
// only meaningful then (`case null, default ->`).
struct ExprCaseRule : cursiv::Extends<CaseRule> {
  vector<Expr*> values;
  boolean alsoDefault;
  boolean arrow;
  Block* body;
};

struct PatternCaseRule : cursiv::Extends<CaseRule> {
  vector<Pattern*> patterns;
  Expr* guard;  // `when` clause, or null
  boolean arrow;
  Block* body;
};

struct DefaultCaseRule : cursiv::Extends<CaseRule> {
  boolean arrow;
  Block* body;
};

struct SwitchExpr : cursiv::Extends<Expr> {
  Expr* value;
  vector<CaseRule*> rules;
};

// -- Declarations shared shapes ----

struct VarDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  uint8 extraDims;  // legacy trailing `[]` after the name: `int a, b[];`
  Initializer* init;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  boolean varargs;
  boolean isReceiver;  // explicit `this` parameter, e.g. `Outer.this`
  string_view name;    // parameter name, or the receiver's qualifier
  uint8 extraDims;      // legacy trailing `[]` after the name: `int f(int a[])`
};

// A class/interface/record/aspect body member, a block item, or a
// compilation unit's type: one shared base for every list that mixes
// declarations and statements the way Java's own grammar does.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct FieldDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
};

struct MethodDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<TypeParam*> typeParams;
  TypeRef* returnType;
  uint8 returnExtraDims;  // legacy trailing `[]` after the params: `int f()[]`
  string_view name;
  vector<Param*> params;
  vector<TypeRef*> throwsList;
  Block* body;  // null: abstract/interface/native (`;` body)
};

struct ConstructorDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<TypeParam*> typeParams;
  string_view name;
  vector<Param*> params;
  vector<TypeRef*> throwsList;
  Block* body;
};

struct CompactConstructorDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  Block* body;
};

struct InitBlock : cursiv::Extends<Item> {
  boolean isStatic;
  Block* body;
};

struct TypeDecl : cursiv::Extends<Item> {};

struct ClassDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  TypeRef* superclass;
  vector<TypeRef*> interfaces;
  vector<TypeRef*> permits;
  vector<Item*> members;
};

struct InterfaceDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<TypeRef*> extendsList;
  vector<TypeRef*> permits;
  vector<Item*> members;
};

struct EnumConstant : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  vector<Expr*> args;
  boolean hasBody;
  vector<Item*> classBody;
};

struct EnumDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeRef*> interfaces;
  vector<EnumConstant*> constants;
  vector<Item*> members;
};

// Only the last component of a record header may vary its arity
// (`String... rest`): checked by the validator, not the grammar.
struct RecordComponent : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  TypeRef* type;
  boolean varargs;
  string_view name;
};

struct RecordDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<RecordComponent*> components;
  vector<TypeRef*> interfaces;
  vector<Item*> members;
};

struct AnnotationTypeElement : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  string_view name;
  AnnotationValue* defaultValue;
};

struct AnnotationTypeDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<Item*> members;
};

// -- aspects ----

struct PerClause : cursiv::Extends<cursiv::AstNode> {};
struct PerTargetClause : cursiv::Extends<PerClause> {
  PointcutExpr* pointcut;
};
struct PerThisClause : cursiv::Extends<PerClause> {
  PointcutExpr* pointcut;
};
struct PerCflowClause : cursiv::Extends<PerClause> {
  PointcutExpr* pointcut;
};
struct PerCflowBelowClause : cursiv::Extends<PerClause> {
  PointcutExpr* pointcut;
};
struct PerTypeWithinClause : cursiv::Extends<PerClause> {
  TypePattern* pattern;
};
struct IssingletonClause : cursiv::Extends<PerClause> {};

struct AspectDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean privileged;
  string_view name;
  TypeRef* superclass;
  vector<TypeRef*> interfaces;
  PerClause* perClause;
  vector<Item*> members;
};

// `before`/`after`/`around`, told apart by `kind`; `returning`/`throwing`
// mark which of the two `after` forms this is (mutually exclusive).
struct Advice : cursiv::Extends<Item> {
  boolean strictfp_;
  string_view kind;  // "before" | "after" | "around"
  boolean returning;
  boolean throwing;
  TypeRef* returnType;  // around's return type; null means `void`
  vector<Param*> params;
  Param* boundParam;  // after-returning/after-throwing's optional `(Type x)`
  vector<TypeRef*> throwsList;
  PointcutExpr* pointcut;
  Block* body;
};

struct PointcutDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view name;
  vector<Param*> params;
  PointcutExpr* body;  // null: abstract (`;` body, matching MethodDecl)
};

// -- inter-type declarations ----

struct InterTypeMethodDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  TypeRef* returnType;  // null means `void`
  TypeRef* targetType;
  string_view name;
  vector<Param*> params;
  vector<TypeRef*> throwsList;
  Block* body;  // null: abstract (`;` body, matching MethodDecl)
};

struct InterTypeConstructorDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  TypeRef* targetType;
  vector<Param*> params;
  vector<TypeRef*> throwsList;
  Block* body;
};

struct InterTypeFieldDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  TypeRef* fieldType;  // null means `void` (degenerate, matches the grammar)
  TypeRef* targetType;
  string_view name;
  Expr* init;
};

// -- declare forms ----

struct DeclareParents : cursiv::Extends<Item> {
  TypePattern* pattern;
  boolean isImplements;  // false: extends
  TypeRef* extendsType;
  vector<TypeRef*> implementsTypes;
};

struct DeclareMessage : cursiv::Extends<Item> {
  boolean isError;  // false: warning
  PointcutExpr* pointcut;
  string_view message;
};

struct DeclareSoft : cursiv::Extends<Item> {
  TypeRef* exceptionType;
  PointcutExpr* pointcut;
};

struct DeclarePrecedence : cursiv::Extends<Item> {
  vector<TypePattern*> order;
};

struct DeclareAnnotation : cursiv::Extends<Item> {
  string_view kind;  // "type" | "method" | "constructor" | "field"
  TypePattern* typePattern;
  MethodPattern* methodPattern;
  ConstructorPattern* constructorPattern;
  FieldPattern* fieldPattern;
  Annotation* annotation;
};

// -- type patterns (call/execution/within signatures, declare parents...) ----
// Unrelated to `Pattern` above (instanceof/switch): AspectJ's own term for a
// wildcarded type matcher, e.g. `com.foo..*` or `!(Foo || Bar)`.

struct TypePattern : cursiv::Extends<cursiv::AstNode> {};

struct NotTypePattern : cursiv::Extends<TypePattern> {
  TypePattern* operand;
};

struct BinaryTypePattern : cursiv::Extends<TypePattern> {
  TypePattern* left;
  string_view op;  // "&&" | "||"
  TypePattern* right;
};

// `(annotationPattern? typePattern)`, kept only when an annotation
// qualifies it -- bare grouping parentheses build no node.
struct ParenTypePattern : cursiv::Extends<TypePattern> {
  vector<AnnotationPatternClause*> annotations;
  TypePattern* pattern;
};

struct SimpleTypePattern : cursiv::Extends<TypePattern> {
  string_view name;  // the dotted/wildcarded name, e.g. "com.foo..*"
  boolean plus;        // trailing `+` (the type and its subtypes)
  uint8 arrayDims;
};

struct AnnotationPatternClause : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  string_view type;    // a dotted annotation type name
  TypePattern* typeExpr;  // set instead for the `(typePattern)` form
};

struct ModifierPatternClause : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  string_view modifier;
};

struct FieldPattern : cursiv::Extends<cursiv::AstNode> {
  vector<AnnotationPatternClause*> annotations;
  vector<ModifierPatternClause*> modifiers;
  TypePattern* type;
  TypePattern* declaringType;
  string_view dotOrDotDot;  // "." | ".." | "", set only with declaringType
  string_view name;          // the wildcarded field name pattern
};

struct FormalsItem : cursiv::Extends<cursiv::AstNode> {
  boolean dotdot;  // a bare `..` standing for any remaining arguments
  vector<AnnotationPatternClause*> annotations;
  TypePattern* type;  // null when dotdot
  boolean varargs;      // trailing `...` on the last parameter
};

struct MethodPattern : cursiv::Extends<cursiv::AstNode> {
  vector<AnnotationPatternClause*> annotations;
  vector<ModifierPatternClause*> modifiers;
  TypePattern* returnType;
  TypePattern* declaringType;
  string_view dotOrDotDot;
  string_view name;
  vector<FormalsItem*> params;
  vector<TypePattern*> throwsList;
};

struct ConstructorPattern : cursiv::Extends<cursiv::AstNode> {
  vector<AnnotationPatternClause*> annotations;
  vector<ModifierPatternClause*> modifiers;
  TypePattern* declaringType;
  string_view dotOrDotDot;
  vector<FormalsItem*> params;
  vector<TypePattern*> throwsList;
};

// `id.id....class`, as used by `@DeclareParents`/`@DeclareMixin`.
struct ClassPatternRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

// -- pointcut expressions ----

struct PointcutExpr : cursiv::Extends<cursiv::AstNode> {};

struct NotPointcut : cursiv::Extends<PointcutExpr> {
  PointcutExpr* operand;
};

struct BinaryPointcut : cursiv::Extends<PointcutExpr> {
  PointcutExpr* left;
  string_view op;  // "&&" | "||"
  PointcutExpr* right;
};

// call/execution/withincode (a method-or-constructor signature) and
// initialization/preinitialization (a constructor signature only), told
// apart by `kind`; exactly one of `method`/`ctor` is set.
struct SignaturePointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  MethodPattern* method;
  ConstructorPattern* ctor;
};

// staticinitialization/handler/within: one optionally-annotated type
// pattern.
struct TypePatternPointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  vector<AnnotationPatternClause*> annotations;
  TypePattern* pattern;
};

// get/set.
struct FieldPointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  FieldPattern* pattern;
};

// cflow/cflowbelow.
struct NestedPointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  PointcutExpr* pointcut;
};

struct IfPointcut : cursiv::Extends<PointcutExpr> {
  Expr* condition;  // null for a bare `if()`
};

struct AdviceExecutionPointcut : cursiv::Extends<PointcutExpr> {};

// this/target: a bare name spells the same token whether it names a type
// or binds a variable, so it is always read as a type -- telling the two
// apart needs the advice's own parameter list, outside grammar scope.
struct BindingPointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  TypeRef* type;
};

struct ArgPattern : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;          // set unless wildcard is
  string_view wildcard;  // "*" | ".." | "", set instead of type
};

struct ArgsPointcut : cursiv::Extends<PointcutExpr> {
  vector<ArgPattern*> args;
};

// @this/@target/@within/@withincode/@annotation.
struct AnnotationBindingPointcut : cursiv::Extends<PointcutExpr> {
  string_view kind;
  string_view annotationType;
};

struct AnnotationArgPattern : cursiv::Extends<cursiv::AstNode> {
  string_view name;      // empty when wildcard is set
  string_view wildcard;  // "*" | ".." | ""
};

struct AnnotationArgsPointcut : cursiv::Extends<PointcutExpr> {
  vector<AnnotationArgPattern*> args;
};

// A named pointcut's use, optionally qualified by the aspect declaring it:
// `MyAspect.myPointcut(x)`.
struct ReferencePointcut : cursiv::Extends<PointcutExpr> {
  TypePattern* qualifier;
  string_view name;
  vector<FormalsItem*> params;
};

// -- Statements ----

struct Stmt : cursiv::Extends<Item> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Item*> items;
};

struct LocalVarDecl : cursiv::Extends<Stmt> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;  // null when `var`
  boolean isVar;
  vector<VarDeclarator*> declarators;
};

// `expr;` and the empty statement `;`.
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Expr* message;
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

struct ForStmt : cursiv::Extends<Stmt> {
  LocalVarDecl* initDecl;
  vector<Expr*> initExprs;
  Expr* cond;
  vector<Expr*> update;
  Stmt* body;
};

struct ForEachStmt : cursiv::Extends<Stmt> {
  vector<string_view> modifiers;
  TypeRef* type;  // null when `var`
  boolean isVar;
  string_view name;
  uint8 extraDims;
  Expr* iterable;
  Stmt* body;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<TypeRef*> types;  // multi-catch: `catch (A | B e)`
  string_view name;
  Block* body;
};

// A declared resource, or (when `init` is null) a bare name referring to an
// existing effectively-final variable.
struct Resource : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  boolean isVar;
  string_view name;
  uint8 extraDims;
  Expr* init;
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<Resource*> resources;
  Block* body;
  vector<CatchClause*> catches;
  Block* finallyBlock;
};

struct SyncStmt : cursiv::Extends<Stmt> {
  Expr* lock;
  Block* body;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct YieldStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

struct SwitchLabel : cursiv::Extends<cursiv::AstNode> {};

struct CaseLabel : cursiv::Extends<SwitchLabel> {
  Expr* constant;         // a constant-expression case
  TypeRef* patternType;    // an old-style type-pattern case: `case String s:`
  string_view patternName;
};

struct DefaultLabel : cursiv::Extends<SwitchLabel> {};

// One or more labels followed by the statements they guard; also used for a
// trailing run of empty labels, whose `statements` is then empty.
struct SwitchGroup : cursiv::Extends<cursiv::AstNode> {
  vector<SwitchLabel*> labels;
  vector<Item*> statements;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<SwitchGroup*> groups;
};

// -- Top level ----

struct PackageDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
};

struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  string_view qualifiedName;
  boolean onDemand;  // trailing `.*`
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
  vector<Annotation*> annotations;
  boolean open;
  string_view name;
  vector<ModuleDirective*> directives;
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  PackageDecl* package_;
  vector<ImportDecl*> imports;
  vector<TypeDecl*> types;
  ModuleDecl* module_;  // set instead of package_/types for a module-info file
};

}  // namespace aspectj::ast
