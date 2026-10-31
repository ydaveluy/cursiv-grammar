// The modelica language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace modelica::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct BreakExpr : cursiv::Extends<Expr> {};
struct Equation : cursiv::Extends<cursiv::AstNode> {};
struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct ClassSpec : cursiv::Extends<cursiv::AstNode> {};
struct Element : cursiv::Extends<cursiv::AstNode> {};
struct Argument : cursiv::Extends<cursiv::AstNode> {};
struct Annotation;

// -- names ----

struct Name : cursiv::Extends<cursiv::AstNode> {
  boolean global;
  vector<string_view> parts;
};

struct RefPart : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> subscripts;
};

struct ComponentRef : cursiv::Extends<Expr> {
  boolean global;
  vector<RefPart*> parts;
};

// -- expressions ----

struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct EndExpr : cursiv::Extends<Expr> {};

// A bare `:` array subscript: every dimension at that position.
struct ColonExpr : cursiv::Extends<Expr> {};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// step is null when the range is `start:stop`, not `start:step:stop`.
struct RangeExpr : cursiv::Extends<Expr> {
  Expr* start;
  Expr* step;
  Expr* stop;
};

struct IfExpr : cursiv::Extends<Expr> {
  vector<Expr*> conditions;  // if, then each elseif
  vector<Expr*> branches;    // one per condition, same order
  Expr* elseBranch;
};

struct ForIndex : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* range;  // null: no `in expression`
};

struct NamedArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

// A function partial application, `function name(named...)`, as a
// function_argument -- distinct from a plain expression argument.
struct FunctionRefArg : cursiv::Extends<Expr> {
  Name* func;
  vector<NamedArg*> named;
};

// function_arguments / function_call_args: a positional list optionally
// closed by a `for` comprehension from its first expression, or followed
// by named arguments. Array arguments populate only positional/forIndices.
struct CallArgs : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> positional;
  vector<ForIndex*> forIndices;
  vector<NamedArg*> named;
};

struct CallExpr : cursiv::Extends<Expr> {
  ComponentRef* callee;  // indexed/qualified reference, or der/initial/pure
  CallArgs* args;
};

// `(' output_expression_list ')` grouping and multi-output targets: a slot
// left empty in the source is represented by OmittedOutput, preserving
// every position in the items/targets vectors.
struct OmittedOutput : cursiv::Extends<Expr> {};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ExprRow : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> items;
};

struct MatrixLit : cursiv::Extends<Expr> {
  vector<ExprRow*> rows;
};

struct ArrayLit : cursiv::Extends<Expr> {
  CallArgs* args;
};

// -- equations ----

struct EqBlock : cursiv::Extends<cursiv::AstNode> {
  vector<Equation*> equations;
};

struct EqEquality : cursiv::Extends<Equation> {
  Expr* lhs;
  Expr* rhs;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct CallEquation : cursiv::Extends<Equation> {
  ComponentRef* callee;
  CallArgs* args;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct IfEquation : cursiv::Extends<Equation> {
  vector<Expr*> conditions;
  vector<EqBlock*> branches;
  EqBlock* elseBranch;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ForEquation : cursiv::Extends<Equation> {
  vector<ForIndex*> indices;
  vector<Equation*> body;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct WhenEquation : cursiv::Extends<Equation> {
  vector<Expr*> conditions;
  vector<EqBlock*> branches;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ConnectEquation : cursiv::Extends<Equation> {
  ComponentRef* a;
  ComponentRef* b;
  vector<string_view> docStrings;
  Annotation* annotation;
};

// -- statements ----

struct StmtBlock : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

struct AssignStmt : cursiv::Extends<Statement> {
  ComponentRef* target;
  Expr* value;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct CallStmt : cursiv::Extends<Statement> {
  ComponentRef* callee;
  CallArgs* args;
  vector<string_view> docStrings;
  Annotation* annotation;
};

// `(' output_expression_list ')` := ref(args)`, the multi-output call.
struct MultiAssignStmt : cursiv::Extends<Statement> {
  vector<Expr*> targets;
  ComponentRef* callee;
  CallArgs* args;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct BreakStmt : cursiv::Extends<Statement> {
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ReturnStmt : cursiv::Extends<Statement> {
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct IfStmt : cursiv::Extends<Statement> {
  vector<Expr*> conditions;
  vector<StmtBlock*> branches;
  StmtBlock* elseBranch;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ForStmt : cursiv::Extends<Statement> {
  vector<ForIndex*> indices;
  vector<Statement*> body;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct WhileStmt : cursiv::Extends<Statement> {
  Expr* cond;
  vector<Statement*> body;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct WhenStmt : cursiv::Extends<Statement> {
  vector<Expr*> conditions;
  vector<StmtBlock*> branches;
  vector<string_view> docStrings;
  Annotation* annotation;
};

// -- modification system ----

struct InheritanceBreak : cursiv::Extends<Argument> {
  string_view name;
  ConnectEquation* connection;
};

struct ClassMod : cursiv::Extends<cursiv::AstNode> {
  vector<Argument*> args;
};

struct Modification : cursiv::Extends<cursiv::AstNode> {
  ClassMod* mod;       // null: no `(...)` part
  string_view assignOp;  // "", "=", ":="
  Expr* value;          // null: no assigned value
};

struct ElementMod : cursiv::Extends<Argument> {
  boolean each_;
  boolean final_;
  Name* name;
  Modification* mod;  // null: bare name
  vector<string_view> docStrings;
};

struct ComponentClause;
struct ClassDef;

struct ElementReplaceableArg : cursiv::Extends<Argument> {
  boolean each_;
  boolean final_;
  ClassDef* classDef;         // one of these two is set
  ComponentClause* component;
  Name* constrainingClause;    // null: no `constrainedby`
  ClassMod* constrainingMod;
};

struct ElementRedeclare : cursiv::Extends<Argument> {
  boolean each_;
  boolean final_;
  ClassDef* classDef;
  ComponentClause* component;
  ElementReplaceableArg* replaceable_;  // set instead of classDef/component
};

// -- declarations ----

struct Annotation : cursiv::Extends<cursiv::AstNode> {
  ClassMod* mod;
};

struct ImportClause : cursiv::Extends<Element> {
  string_view alias;              // IDENT '=' form; empty otherwise
  Name* name;
  boolean wildcard;                // `name.*`
  vector<string_view> selective;   // `name.{a, b}`
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ExtendsClause : cursiv::Extends<Element> {
  Name* name;
  ClassMod* mod;
  Annotation* annotation;
};

struct ComponentDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> subscripts;
  Modification* mod;
  Expr* condition;  // null: no `if` attribute (component_declaration only)
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ComponentClause : cursiv::Extends<cursiv::AstNode> {
  string_view flowStream;   // "", "flow", "stream"
  string_view variability;  // "", "discrete", "parameter", "constant"
  string_view causality;    // "", "input", "output"
  Name* type;
  vector<Expr*> subscripts;  // component_clause only, empty for component_clause1
  vector<ComponentDecl*> components;
};

struct ElementDecl : cursiv::Extends<Element> {
  boolean redeclare_;
  boolean final_;
  boolean inner_;
  boolean outer_;
  boolean replaceable_;
  ClassDef* classDef;
  ComponentClause* component;
  Name* constrainingClause;  // set only when replaceable_ and present
  ClassMod* constrainingMod;
  vector<string_view> docStrings;  // trailing comment of the constraining clause
  Annotation* annotation;
};

struct CompositionSection : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // "", "public", "protected", "equation", "algorithm"
  boolean initial_;   // equation/algorithm sections only
  vector<Element*> elements;
  vector<Equation*> equations;
  vector<Statement*> statements;
};

struct ExternalCall : cursiv::Extends<cursiv::AstNode> {
  ComponentRef* target;  // null: no `ref =` part
  string_view func;
  vector<Expr*> args;
};

struct Composition : cursiv::Extends<cursiv::AstNode> {
  vector<CompositionSection*> sections;
  boolean hasExternal;
  string_view externalLang;  // language_specification, unquoted; empty if absent
  ExternalCall* externalCall;
  Annotation* externalAnnotation;
  Annotation* finalAnnotation;
};

struct LongClassSpec : cursiv::Extends<ClassSpec> {
  boolean isExtends;
  string_view name;
  ClassMod* extendsMod;  // isExtends only
  vector<string_view> docStrings;
  Composition* composition;
  string_view endName;
};

struct EnumLiteral : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ShortClassSpec : cursiv::Extends<ClassSpec> {
  string_view name;
  boolean isEnumeration;
  string_view flowStream;
  string_view variability;
  string_view causality;
  Name* baseType;             // isEnumeration == false
  vector<Expr*> subscripts;
  ClassMod* mod;
  boolean enumUnspecified;     // `enumeration(:)`
  vector<EnumLiteral*> literals;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct DerClassSpec : cursiv::Extends<ClassSpec> {
  string_view name;
  Name* baseType;
  vector<string_view> args;
  vector<string_view> docStrings;
  Annotation* annotation;
};

struct ClassDef : cursiv::Extends<cursiv::AstNode> {
  boolean encapsulated_;
  boolean partial_;
  string_view kind;  // class, model, record, block, connector, type, package,
                      // function, operator
  boolean isOperator;
  boolean isExpandable;
  string_view purity;  // "", "pure", "impure"
  ClassSpec* specifier;
};

struct TopClassDef : cursiv::Extends<cursiv::AstNode> {
  boolean final_;
  ClassDef* def;
};

struct WithinClause : cursiv::Extends<cursiv::AstNode> {
  Name* name;  // null: `within ;`
};

struct StoredDefinition : cursiv::Extends<cursiv::AstNode> {
  vector<WithinClause*> withins;
  vector<TopClassDef*> classes;
};

}  // namespace modelica::ast
