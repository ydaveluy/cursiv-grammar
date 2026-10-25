#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace objc::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Stmt : cursiv::Extends<Item> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct TypeName;
struct UnresolvedName : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};

struct GenericAssociation : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  Expr* value;
};
struct GenericExpr : cursiv::Extends<Expr> {
  cursiv::AstNode* control;
  vector<GenericAssociation*> associations;
};

struct KeywordLit : cursiv::Extends<Expr> {
  string_view keyword;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct LabelAddrExpr : cursiv::Extends<Expr> {
  string_view label;
};

struct Specifiers;
struct Declarator;

struct TypeName : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;
};

struct SizeofTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct SizeofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  TypeName* type;
  Expr* operand;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct ArgumentTypeExpr : cursiv::Extends<Expr> {
  struct TypeSpec* type;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  string_view field;
};

struct Initializer;

struct CompoundLiteralExpr : cursiv::Extends<Expr> {
  TypeName* type;
  Initializer* init;
};

struct StmtExpr : cursiv::Extends<Expr> {
  struct CompoundStmt* body;
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

struct DictPair : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};

struct DictExpr : cursiv::Extends<Expr> {
  vector<DictPair*> pairs;
};

struct ArrayExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct BoxedExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct Param;

struct BlockExpr : cursiv::Extends<Expr> {
  Specifiers* returnType;
  Declarator* returnDeclarator;
  string_view nullability;
  vector<Param*> params;
  struct CompoundStmt* body;
};

struct MessageArg : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  Expr* value;
};

struct MessageExpr : cursiv::Extends<Expr> {
  Expr* receiver;
  TypeName* receiverType;
  string_view unarySelector;
  vector<MessageArg*> args;
  vector<Expr*> variadicArgs;
};

struct SelectorPart : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct SelectorExpr : cursiv::Extends<Expr> {
  string_view simple;
  vector<SelectorPart*> parts;
};

struct ProtocolExpr : cursiv::Extends<Expr> {
  string_view variance;
  string_view name;
};

struct EncodeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

struct BuiltinType : cursiv::Extends<TypeSpec> {
  string_view keyword;
};

struct TypeofType : cursiv::Extends<TypeSpec> {
  cursiv::AstNode* operand;
  vector<string_view> qualifiers;
};

struct FieldDeclaration;

struct TagType : cursiv::Extends<TypeSpec> {
  string_view keyword;
  string_view name;
  boolean defined;
  vector<Item*> members;
};

struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct EnumType : cursiv::Extends<TypeSpec> {
  string_view nsMacro;
  TypeName* underlying;
  string_view name;
  boolean defined;
  vector<Enumerator*> enumerators;
};

struct ProtocolRef : cursiv::Extends<cursiv::AstNode> {
  string_view variance;
  string_view name;
};

struct TypeArgument : cursiv::Extends<cursiv::AstNode> {
  string_view variance;
  TypeName* type;
  TypeName* bound;
};

struct AngleClause : cursiv::Extends<cursiv::AstNode> {
  vector<TypeArgument*> args;
};

struct IdType : cursiv::Extends<TypeSpec> {
  vector<AngleClause*> clauses;
  vector<string_view> qualifiers;
};

struct ClassRef : cursiv::Extends<TypeSpec> {
  string_view name;
  vector<AngleClause*> clauses;
  vector<string_view> qualifiers;
};

struct AtomicType : cursiv::Extends<TypeSpec> {
  TypeName* type;
};
struct BitIntType : cursiv::Extends<TypeSpec> {
  Expr* width;
};
struct Alignment : cursiv::Extends<cursiv::AstNode> {
  cursiv::AstNode* operand;
};

struct Attribute;

struct Specifiers : cursiv::Extends<cursiv::AstNode> {
  vector<Alignment*> alignments;
  vector<string_view> qualifiers;
  vector<Attribute*> attributes;
  boolean ibOutlet;
  boolean ibOutletCollection;
  vector<string_view> ibOutletCollectionTypes;
  vector<TypeSpec*> types;
};

struct AttributeParam : cursiv::Extends<cursiv::AstNode> {
  Attribute* nested;
  Expr* value;
  string_view assignName;
  string_view assignText;
  Expr* assignValue;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<AttributeParam*> params;
};

struct Pointer : cursiv::Extends<cursiv::AstNode> {
  boolean isBlock;
  vector<string_view> qualifiers;
};
struct Suffix : cursiv::Extends<cursiv::AstNode> {};
struct FunctionSuffix : cursiv::Extends<Suffix> {
  vector<Param*> params;
  boolean variadic;
};

struct ArraySuffix : cursiv::Extends<Suffix> {
  Expr* size;
  vector<string_view> qualifiers;
  boolean isStatic;
};

struct Param;

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  vector<Pointer*> pointers;
  string_view name;
  Declarator* nested;
  vector<Suffix*> suffixes;
  vector<Attribute*> attributes;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;
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
  string_view name;
};
struct IndexDesignator : cursiv::Extends<Designator> {
  Expr* index;
  Expr* rangeEnd;
};
struct DesignatedInit : cursiv::Extends<Initializer> {
  vector<Designator*> designators;
  Initializer* value;
};

struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  Initializer* init;
};

struct FieldDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  Expr* bitWidth;
};

struct FieldDeclaration : cursiv::Extends<Item> {
  Specifiers* specifiers;
  vector<FieldDeclarator*> declarators;
  struct Macro* trailingMacro;
};

struct VisibilitySection : cursiv::Extends<cursiv::AstNode> {
  string_view access;
  vector<FieldDeclaration*> fields;
};

struct InstanceVariables : cursiv::Extends<cursiv::AstNode> {
  vector<VisibilitySection*> sections;
};

struct Macro : cursiv::Extends<cursiv::AstNode> {};

struct CallMacro : cursiv::Extends<Macro> {
  string_view name;
  vector<Expr*> args;
};

struct SwiftNameMacro : cursiv::Extends<Macro> {
  vector<string_view> aliasPath;
  string_view selectorName;
  vector<string_view> selectorParts;
};

struct ApiVersion : cursiv::Extends<cursiv::AstNode> {
  string_view platform;
  string_view version;
};

struct ApiAvailableMacro : cursiv::Extends<Macro> {
  vector<ApiVersion*> versions;
};

struct ApiUnavailableMacro : cursiv::Extends<Macro> {
  vector<string_view> platforms;
};

struct SwiftUnavailableMacro : cursiv::Extends<Macro> {
  Expr* message;
};

struct ClangAttrArg : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  Expr* value;
  string_view assignName;
  string_view assignVersion;
  Expr* assignValue;
};

struct ClangAttr : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<ClangAttrArg*> args;
};

struct AttributeMacro : cursiv::Extends<Macro> {
  vector<ClangAttr*> attrs;
};

struct CompoundStmt;

struct Declaration;

struct FunctionDef : cursiv::Extends<Item> {
  Specifiers* specifiers;
  Declarator* declarator;
  vector<Declaration*> paramDeclarations;
  CompoundStmt* body;
};

struct UnresolvedCallOrDeclaration : cursiv::Extends<Item> {
  vector<Attribute*> attrsBefore;
  vector<Attribute*> attrsAfter;
  string_view name;
  Declarator* declarator;
  Macro* trailingMacro;
};

struct Declaration : cursiv::Extends<Item> {
  Specifiers* specifiers;
  vector<InitDeclarator*> declarators;
  Macro* trailingMacro;
};

struct MethodParam : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  TypeName* type;
  string_view arc;
  string_view name;
  vector<Attribute*> attributes;
};

struct MethodDecl : cursiv::Extends<Item> {
  boolean isClassMethod;
  TypeName* returnType;
  string_view unarySelector;
  vector<MethodParam*> params;
  boolean variadic;
  vector<Param*> extraParams;
  vector<Macro*> macros;
};

struct MethodDef : cursiv::Extends<Item> {
  boolean isClassMethod;
  TypeName* returnType;
  string_view unarySelector;
  vector<MethodParam*> params;
  boolean variadic;
  vector<Param*> extraParams;
  vector<Macro*> macros;
  CompoundStmt* body;
};

struct PropertyAttr : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  string_view getterName;
  string_view setterName;
};

struct PropertyDecl : cursiv::Extends<Item> {
  vector<PropertyAttr*> attrs;
  boolean ibOutlet;
  boolean ibOutletCollection;
  string_view ibOutletCollectionType;
  boolean ibInspectable;
  FieldDeclaration* field;
};

struct SynthesizeItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view backing;
};

struct PropertyImpl : cursiv::Extends<Item> {
  boolean dynamic;
  vector<SynthesizeItem*> items;
};

struct ImportDecl : cursiv::Extends<Item> {
  vector<string_view> path;
};

struct ClassInterface : cursiv::Extends<Item> {
  vector<Attribute*> attributes;
  boolean ibDesignable;
  ClassRef* className;
  string_view superclassName;
  vector<AngleClause*> clauses;
  InstanceVariables* ivars;
  vector<Item*> members;
};

struct CategoryInterface : cursiv::Extends<Item> {
  vector<Attribute*> attributes;
  ClassRef* className;
  string_view categoryName;
  AngleClause* clause;
  InstanceVariables* ivars;
  vector<Item*> members;
};

struct ClassImplementation : cursiv::Extends<Item> {
  vector<Attribute*> attributes;
  ClassRef* className;
  string_view superclassName;
  InstanceVariables* ivars;
  vector<Item*> members;
};

struct CategoryImplementation : cursiv::Extends<Item> {
  vector<Attribute*> attributes;
  ClassRef* className;
  string_view categoryName;
  vector<Item*> members;
};

struct ProtocolSection : cursiv::Extends<cursiv::AstNode> {
  string_view modifier;
  vector<Item*> members;
};

struct ProtocolDecl : cursiv::Extends<Item> {
  vector<Attribute*> attributes;
  Macro* macro;
  string_view variance;
  string_view name;
  AngleClause* clause;
  vector<ProtocolSection*> sections;
};

struct ProtocolForwardDecl : cursiv::Extends<Item> {
  vector<ProtocolRef*> protocols;
};

struct ClassForwardDecl : cursiv::Extends<Item> {
  vector<ClassRef*> classes;
};

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

struct CaseStmt : cursiv::Extends<Stmt> {
  boolean isDefault;
  Expr* value;
  Expr* rangeEnd;
  Item* body;
};
struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForInit : cursiv::Extends<Item> {
  Specifiers* specifiers;
  vector<InitDeclarator*> declarators;
  Expr* expr;
};

struct ForStmt : cursiv::Extends<Stmt> {
  ForInit* init;
  Expr* cond;
  Expr* step;
  Stmt* body;
};

struct ForInStmt : cursiv::Extends<Stmt> {
  ForInit* init;
  Expr* collection;
  Stmt* body;
};

struct Label : cursiv::Extends<Stmt> {
  string_view name;
  Item* body;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  reference<Label> target;
};

struct BreakStmt : cursiv::Extends<Stmt> {};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;
  boolean isEllipsis;
  CompoundStmt* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  CompoundStmt* tryBlock;
  vector<CatchClause*> catches;
  CompoundStmt* finallyBlock;
};

struct SynchronizedStmt : cursiv::Extends<Stmt> {
  Expr* lock;
  CompoundStmt* body;
};

struct AutoreleaseStmt : cursiv::Extends<Stmt> {
  CompoundStmt* body;
};

struct AvailabilityClause : cursiv::Extends<cursiv::AstNode> {
  string_view platform;
  string_view version;
  boolean wildcard;
};
struct AvailableExpr : cursiv::Extends<Expr> {
  vector<AvailabilityClause*> clauses;
};
struct AnnotationMarker : cursiv::Extends<Item> {
  string_view text;
};

struct StaticAssert : cursiv::Extends<Item> {
  Expr* condition;
  StringLit* message;
};
struct CompatibilityAlias : cursiv::Extends<Item> {
  string_view name;
  string_view original;
};

struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace objc::ast
