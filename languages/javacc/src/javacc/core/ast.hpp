// The javacc language: its AST. A .jj file names a parser (options, a
// PARSER_BEGIN/PARSER_END Java compilation unit, then productions), and its
// productions embed a real declaration/statement/expression grammar for the
// generated parser's target language (Java, with the C++ extensions
// CPPCODE/template/const/pointer declarators add) -- Decl is the shared root
// for every declaration and statement so a block's items need one list type.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace javacc::ast {

struct Decl : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Decl> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};

struct StringList : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> items;
};

struct OptionBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  variant<int64, boolean, string_view, StringList*> value;
};

// -- types --------------------------------------------------------------

struct PrimitiveType : cursiv::Extends<Type> {
  string_view keyword;
};

struct ArrayType : cursiv::Extends<Type> {
  Type* element;
  uint8 dims;
};

struct Annotation;
struct Modifiers;

struct TypeArgument : cursiv::Extends<cursiv::AstNode> {};

struct ConcreteTypeArgument : cursiv::Extends<TypeArgument> {
  Type* type;
  boolean variadic;
};

struct WildcardTypeArgument : cursiv::Extends<TypeArgument> {
  string_view boundKind;  // "extends", "super", or empty
  Type* bound;
};

struct TemplateBase : cursiv::Extends<cursiv::AstNode> {
  string_view pack;  // "class" or "typename"
  boolean variadic;
  string_view name;
};

// The C++-generation extension's `template<class T, ...>` prefix on a type.
struct Template : cursiv::Extends<cursiv::AstNode> {
  vector<TemplateBase*> bases;
};

struct ClassTypePart : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeArgument*> typeArgs;
};

struct ClassType : cursiv::Extends<Type> {
  boolean globalScope;  // a leading `::`
  Template* tmpl;
  vector<ClassTypePart*> parts;
};

struct TypeParameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Type*> bounds;
};

// `void`, or `const`? type `*`/`&`? -- the C++ extension's pointer/reference
// result markers sit beside the plain Java form.
struct ResultType : cursiv::Extends<cursiv::AstNode> {
  boolean isVoid;
  boolean isConst;
  Type* type;  // null when isVoid
  boolean pointer;
  boolean reference;
};

// -- names, parameters ----------------------------------------------------

struct VariableDeclaratorId : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  uint8 extraDims;
};

struct FormalParameter : cursiv::Extends<cursiv::AstNode> {
  Modifiers* mods;
  Type* type;
  string_view marker;  // "&", "*", "...", or empty
  VariableDeclaratorId* id;
};

struct FormalParameters : cursiv::Extends<cursiv::AstNode> {
  vector<FormalParameter*> params;
};

// -- annotations, member values -------------------------------------------

struct Annotation : cursiv::Extends<cursiv::AstNode> {};

struct MarkerAnnotation : cursiv::Extends<Annotation> {
  string_view name;
};

struct AnnotationArrayInit;

struct SingleMemberAnnotation : cursiv::Extends<Annotation> {
  string_view name;
  variant<Annotation*, AnnotationArrayInit*, Expr*> value;
};

struct MemberValuePair : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  variant<Annotation*, AnnotationArrayInit*, Expr*> value;
};

struct NormalAnnotation : cursiv::Extends<Annotation> {
  string_view name;
  vector<MemberValuePair*> pairs;
};

struct AnnotationArrayInit : cursiv::Extends<cursiv::AstNode> {
  vector<variant<Annotation*, AnnotationArrayInit*, Expr*>> items;
};

struct Modifiers : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> keywords;
  vector<Annotation*> annotations;
};

// -- expressions ------------------------------------------------------------

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;  // dotted, as written
};

struct ThisExpr : cursiv::Extends<Expr> {};

struct SuperFieldExpr : cursiv::Extends<Expr> {
  string_view field;
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

struct ClassLiteralExpr : cursiv::Extends<Expr> {
  ResultType* type;
};

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
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NullLit : cursiv::Extends<Expr> {};

struct ClassBody;

struct NewObjectExpr : cursiv::Extends<Expr> {
  ClassType* type;
  vector<TypeArgument*> typeArgs;
  vector<Expr*> args;
  ClassBody* body;  // an anonymous class body, or null
};

struct ArrayInit;

struct NewArrayExpr : cursiv::Extends<Expr> {
  Type* elementType;
  vector<Expr*> dims;  // `[expr]` dimensions
  uint8 extraDims;      // trailing `[]`
  ArrayInit* init;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct FieldAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct ThisAccessExpr : cursiv::Extends<Expr> {  // `outer.this`
  Expr* object;
};

struct AllocAccessExpr : cursiv::Extends<Expr> {  // `outer.new Inner()`
  Expr* object;
  Expr* alloc;  // a NewObjectExpr*, or a NewArrayExpr* the grammar over-accepts
};

struct GenericMemberExpr : cursiv::Extends<Expr> {  // `x.<T>method`, T dropped
  Expr* object;
  string_view field;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* operand;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Type* type;
};

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

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// -- statements, declarations ----------------------------------------------

struct Block : cursiv::Extends<Stmt> {
  vector<Decl*> items;
};

struct ArrayInit : cursiv::Extends<cursiv::AstNode> {
  vector<variant<ArrayInit*, Expr*>> items;
};

struct VariableDeclarator : cursiv::Extends<cursiv::AstNode> {
  VariableDeclaratorId* id;
  variant<std::monostate, ArrayInit*, Expr*> init;
};

// A field or a local variable: the two grammar rules are structurally
// identical, so one class serves both.
struct VarDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  Type* type;
  vector<VariableDeclarator*> decls;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;  // null for the empty statement `;`
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

struct ExprList : cursiv::Extends<Decl> {
  vector<Expr*> exprs;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Decl* init;  // a VarDecl*, an ExprList*, or null
  Expr* cond;
  ExprList* update;
  Stmt* body;
};

struct ForEachStmt : cursiv::Extends<Stmt> {
  Modifiers* mods;
  Type* type;
  string_view name;
  Expr* iterable;
  Stmt* body;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};
struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};
struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Expr* message;
};

struct SwitchLabelStmt : cursiv::Extends<Stmt> {
  boolean isDefault;
  Expr* value;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<Decl*> items;  // SwitchLabelStmt and block items, in source order
};

struct SynchronizedStmt : cursiv::Extends<Stmt> {
  Expr* lock;
  Block* body;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

struct ResourceDecl : cursiv::Extends<Decl> {
  Type* type;
  VariableDeclaratorId* id;
  Expr* init;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Modifiers* mods;
  string_view marker;  // "&", "*", "...", or empty
  vector<Type*> types;  // multi-catch: more than one on `|`
  VariableDeclaratorId* id;
  Block* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<ResourceDecl*> resources;
  Block* body;
  vector<CatchClause*> catches;
  Block* finallyBlock;
};

struct ExplicitCtorInvocation : cursiv::Extends<cursiv::AstNode> {
  boolean isSuper;
  Expr* qualifier;  // the optional `expr.super(...)` qualifier
  vector<Expr*> args;
};

struct ConstructorDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  vector<TypeParameter*> typeParams;
  string_view name;
  FormalParameters* params;
  vector<string_view> throwsList;
  ExplicitCtorInvocation* eci;
  vector<Decl*> body;
};

struct MethodDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  vector<TypeParameter*> typeParams;
  ResultType* resultType;
  string_view name;
  FormalParameters* params;
  uint8 extraDims;
  vector<string_view> throwsList;
  Block* body;  // null when the declaration ends in `;`
};

struct InitializerBlock : cursiv::Extends<Decl> {
  boolean isStatic;
  Block* body;
};

struct EmptyDecl : cursiv::Extends<Decl> {};

struct ClassBody : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> members;
};

struct ClassOrInterfaceDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  boolean isInterface;
  string_view name;
  vector<TypeParameter*> typeParams;
  vector<Type*> extendsList;
  vector<Type*> implementsList;
  ClassBody* body;
};

struct EnumConstant : cursiv::Extends<cursiv::AstNode> {
  Modifiers* mods;
  string_view name;
  vector<Expr*> args;
  ClassBody* body;
};

struct EnumDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  string_view name;
  vector<Type*> implementsList;
  vector<EnumConstant*> constants;
  vector<Decl*> bodyDecls;
};

struct AnnotationElement : cursiv::Extends<Decl> {
  Modifiers* mods;
  Type* type;
  string_view name;
  variant<std::monostate, Annotation*, AnnotationArrayInit*, Expr*> defaultVal;
};

struct AnnotationTypeDecl : cursiv::Extends<Decl> {
  Modifiers* mods;
  string_view name;
  vector<Decl*> members;
};

// -- compilation unit -------------------------------------------------------

struct PackageDecl : cursiv::Extends<cursiv::AstNode> {
  Modifiers* mods;
  string_view name;
};

struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  string_view name;
  boolean onDemand;  // a trailing `.*`
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  PackageDecl* pkg;
  vector<ImportDecl*> imports;
  vector<Decl*> types;
};

// -- javacc: node descriptors, regular expressions -------------------------

struct NodeDescriptor : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // an identifier, or "void"
  string_view args;  // the raw `(...)` customization, balanced, or empty
};

struct RegularExpression : cursiv::Extends<cursiv::AstNode> {};

struct StringLiteralRegex : cursiv::Extends<RegularExpression> {
  string_view text;
};

struct TokenRefRegex : cursiv::Extends<RegularExpression> {
  string_view name;
};

struct EofRegex : cursiv::Extends<RegularExpression> {};

struct CharDescriptor : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  string_view to;  // empty when the descriptor is a single character
};

struct RegexUnit : cursiv::Extends<cursiv::AstNode> {};

struct RegexStringUnit : cursiv::Extends<RegexUnit> {
  string_view text;
};

struct RegexRefUnit : cursiv::Extends<RegexUnit> {
  string_view name;
};

struct CharListUnit : cursiv::Extends<RegexUnit> {
  boolean negated;
  vector<CharDescriptor*> items;
};

struct ComplexRegexAlt : cursiv::Extends<cursiv::AstNode> {
  vector<RegexUnit*> units;
};

struct RegexGroupUnit : cursiv::Extends<RegexUnit> {
  vector<ComplexRegexAlt*> choices;
  string_view simpleSuffix;  // "+", "*", "?", or empty
  boolean hasCountSuffix;    // the `{m,n}` form
  optional<int64> repeatMin;
  boolean repeatHasComma;
  optional<int64> repeatMax;
};

struct ComplexRegex : cursiv::Extends<RegularExpression> {
  boolean isPrivate;
  string_view label;  // the optional `#label:`, empty when absent
  vector<ComplexRegexAlt*> choices;
};

struct RegexprSpec : cursiv::Extends<cursiv::AstNode> {
  RegularExpression* regex;
  Block* action;
  string_view lexicalStateAfter;  // the trailing `: STATE`, empty when absent
};

// -- javacc: expansions -------------------------------------------------

struct ExpansionChoices : cursiv::Extends<cursiv::AstNode> {
  vector<struct Expansion*> alts;
};

struct LocalLookahead : cursiv::Extends<cursiv::AstNode> {
  optional<int64> amount;
  ExpansionChoices* choices;
  Expr* semanticPredicate;
};

struct ExpansionUnit : cursiv::Extends<cursiv::AstNode> {};

struct LookaheadUnit : cursiv::Extends<ExpansionUnit> {
  LocalLookahead* lookahead;
};

struct ActionUnit : cursiv::Extends<ExpansionUnit> {
  Block* action;
};

struct OptionalUnit : cursiv::Extends<ExpansionUnit> {
  ExpansionChoices* choices;
};

// The expansion-level try/catch is its own, simpler shape: `catch (name ID)?
// block` -- not a real Java try statement's full typed catch parameter.
struct ExpansionCatchClause : cursiv::Extends<cursiv::AstNode> {
  string_view typeName;  // empty when the parameter itself was omitted
  string_view varName;
  Block* body;
};

struct TryExpansionUnit : cursiv::Extends<ExpansionUnit> {
  ExpansionChoices* body;
  vector<ExpansionCatchClause*> catches;
  Block* finallyBlock;
};

struct GroupUnit : cursiv::Extends<ExpansionUnit> {
  ExpansionChoices* choices;
  string_view repeat;  // "+", "*", "?", or empty
};

struct ExpansionCallUnit : cursiv::Extends<ExpansionUnit> {
  Expr* assignTarget;
  string_view name;
  vector<TypeArgument*> typeArgs;
  vector<Expr*> args;
};

struct ExpansionRegexUnit : cursiv::Extends<ExpansionUnit> {
  Expr* assignTarget;
  RegularExpression* regex;
  string_view lexicalStateAfter;  // the trailing `.IDENTIFIER`
};

struct ExpansionItem : cursiv::Extends<cursiv::AstNode> {
  ExpansionUnit* unit;
  NodeDescriptor* nodeDesc;
};

struct Expansion : cursiv::Extends<cursiv::AstNode> {
  LocalLookahead* lookahead;
  vector<ExpansionItem*> items;
};

// -- javacc: productions -----------------------------------------------

struct Production : cursiv::Extends<cursiv::AstNode> {};

struct JavaCodeProduction : cursiv::Extends<Production> {
  ResultType* resultType;
  string_view name;
  FormalParameters* params;
  vector<string_view> throwsList;
  NodeDescriptor* nodeDesc;
  Block* body;
};

struct CppCodeProduction : cursiv::Extends<Production> {
  string_view accessModifier;  // "public"/"protected"/"private", or empty
  ResultType* resultType;
  string_view name;
  FormalParameters* params;
  vector<string_view> throwsList;
  Block* body;
};

struct RegularExprProduction : cursiv::Extends<Production> {
  boolean allStates;      // `< * >`
  vector<string_view> states;
  string_view kind;       // TOKEN, SPECIAL_TOKEN, SKIP, MORE
  boolean ignoreCase;
  vector<RegexprSpec*> specs;
};

struct TokenManagerDecls : cursiv::Extends<Production> {
  ClassBody* body;
};

struct BnfProduction : cursiv::Extends<Production> {
  ResultType* resultType;
  string_view name;
  FormalParameters* params;
  vector<string_view> throwsList;
  NodeDescriptor* nodeDesc;
  Block* localDecls;
  ExpansionChoices* body;
};

struct JavaccInput : cursiv::Extends<cursiv::AstNode> {
  vector<OptionBinding*> options;
  string_view parserBeginName;
  CompilationUnit* unit;
  string_view parserEndName;
  vector<Production*> productions;
};

}  // namespace javacc::ast
