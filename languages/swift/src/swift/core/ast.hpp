// The swift language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace swift::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Attribute;
struct Modifier;
struct Decl : cursiv::Extends<Stmt> {
  vector<Attribute*> attributes;
  vector<Modifier*> modifiers;
};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Pattern : cursiv::Extends<cursiv::AstNode> {};
struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

// Balanced attribute/key-path tokens: leaves retain their value, groups their
// opening delimiter, and string literals their structured interpolation.
struct Opaque : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  Expr* literal;
  vector<Opaque*> children;
};

// -- Attributes, generics -------------------------------------------------

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> nameParts;
  boolean hasArguments;
  Opaque* rawArgs;
};

// A declaration or access-level modifier, with the parenthesized qualifier
// that changes its meaning kept: `private` vs `private(set)`, `unowned` vs
// `unowned(safe)`.
struct Modifier : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  optional<string_view> qualifier;
};

struct GenericParam : cursiv::Extends<cursiv::AstNode> {
  boolean isPack;
  string_view name;
  TypeRef* constraint;
};
struct GenericParams : cursiv::Extends<cursiv::AstNode> {
  vector<GenericParam*> params;
};

struct Requirement : cursiv::Extends<cursiv::AstNode> {};
struct ConformanceReq : cursiv::Extends<Requirement> {
  TypeRef* type;
  TypeRef* constraint;
};
struct SameTypeReq : cursiv::Extends<Requirement> {
  TypeRef* left;
  TypeRef* right;
};

// -- Types ------------------------------------------------------------

struct NamedType : cursiv::Extends<TypeRef> {
  NamedType* qualifier;
  string_view name;
  vector<TypeRef*> genericArgs;
};
struct TupleTypeElement : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> label;
  TypeRef* type;
  Expr* defaultValue;
};
struct TupleType : cursiv::Extends<TypeRef> {
  vector<TupleTypeElement*> elements;
};
struct ArrayType : cursiv::Extends<TypeRef> {
  TypeRef* element;
};
struct DictionaryType : cursiv::Extends<TypeRef> {
  TypeRef* key;
  TypeRef* value;
};
struct FunctionTypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  boolean inoutFlag;
  vector<string_view> specifiers;
  optional<string_view> label;
  TypeRef* type;
};
struct FunctionType : cursiv::Extends<TypeRef> {
  vector<Attribute*> attributes;
  vector<FunctionTypeParam*> params;
  boolean variadic;
  string_view asyncKind;
  string_view throwsKind;
  TypeRef* thrownType;
  TypeRef* result;
};
struct ProtocolCompositionType : cursiv::Extends<TypeRef> {
  vector<TypeRef*> members;
};
struct OptionalType : cursiv::Extends<TypeRef> {
  TypeRef* wrapped;
  boolean implicitlyUnwrapped;
};
struct MetatypeType : cursiv::Extends<TypeRef> {
  TypeRef* base;
  boolean isProtocol;
};
struct OpaqueType : cursiv::Extends<TypeRef> {
  TypeRef* constraint;
};
struct ExistentialType : cursiv::Extends<TypeRef> {
  TypeRef* constraint;
};
struct PackElementType : cursiv::Extends<TypeRef> {
  TypeRef* pack;
};
struct PackExpansionType : cursiv::Extends<TypeRef> {
  TypeRef* pattern;
};
struct AnyType : cursiv::Extends<TypeRef> {};
struct SelfType : cursiv::Extends<TypeRef> {};

// -- Patterns ---------------------------------------------------------

struct WildcardPattern : cursiv::Extends<Pattern> {
  TypeRef* type;
};
struct IdentifierPattern : cursiv::Extends<Pattern> {
  string_view name;
  TypeRef* type;
};
struct TuplePatternElement : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> label;
  Pattern* pattern;
};
struct TuplePattern : cursiv::Extends<Pattern> {
  vector<TuplePatternElement*> elements;
  TypeRef* type;
};
struct BindingPattern : cursiv::Extends<Pattern> {
  boolean isVar;
  Pattern* sub;
};
struct EnumCasePattern : cursiv::Extends<Pattern> {
  TypeRef* typeIdentifier;
  string_view caseName;
  TuplePattern* args;
};
struct OptionalPattern : cursiv::Extends<Pattern> {
  string_view name;
};
struct IsTypePattern : cursiv::Extends<Pattern> {
  TypeRef* type;
};
struct AsTypePattern : cursiv::Extends<Pattern> {
  Pattern* sub;
  TypeRef* type;
};
struct ExprPattern : cursiv::Extends<Pattern> {
  Expr* expr;
};

// -- Expressions --------------------------------------------------------

struct Ident : cursiv::Extends<Expr> {
  string_view name;
  vector<string_view> argumentLabels;
  vector<TypeRef*> genericArgs;
};
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NilLit : cursiv::Extends<Expr> {};

// A run of literal text, or one interpolated expression: `"a\(x)b"` is
// [text "a", expr x, text "b"].
struct Argument;
struct StringPart : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  boolean interpolation;
  vector<Argument*> arguments;
};
struct StringLit : cursiv::Extends<Expr> {
  vector<StringPart*> parts;
  boolean isMultiline;
};
struct RawStringLit : cursiv::Extends<Expr> {
  string_view text;
};
struct RegexLit : cursiv::Extends<Expr> {
  string_view text;
};
struct ControlExpr : cursiv::Extends<Expr> {
  Stmt* statement;
};

struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};
struct DictEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};
struct DictLit : cursiv::Extends<Expr> {
  vector<DictEntry*> entries;
};

struct SelfExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};

struct ClosureParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeRef* type;
  boolean variadic;
};
struct CaptureItem : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> specifier;
  optional<string_view> name;
  Expr* value;
};
struct ClosureExpr : cursiv::Extends<Expr> {
  vector<CaptureItem*> captures;
  boolean hasSignature;
  vector<ClosureParam*> params;
  string_view asyncKind;
  string_view throwsKind;
  TypeRef* thrownType;
  TypeRef* resultType;
  vector<Stmt*> body;
};

struct OperatorRefExpr : cursiv::Extends<Expr> {
  string_view op;
};
struct TupleElement : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> label;
  Expr* value;
};
struct TupleExpr : cursiv::Extends<Expr> {
  vector<TupleElement*> elements;
};
struct WildcardExpr : cursiv::Extends<Expr> {};

struct KeyPathComponent : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  vector<string_view> postfixes;
  vector<Opaque*> subscriptArgs;
};
struct KeyPathExpr : cursiv::Extends<Expr> {
  TypeRef* root;
  vector<KeyPathComponent*> components;
};
struct SelectorExpr : cursiv::Extends<Expr> {
  optional<string_view> kind;
  Expr* value;
};
struct KeyPathStringExpr : cursiv::Extends<Expr> {
  Expr* value;
};
struct MacroLiteralExpr : cursiv::Extends<Expr> {
  string_view kind;
};
struct PlaygroundColorLit : cursiv::Extends<Expr> {
  Expr* red;
  Expr* green;
  Expr* blue;
  Expr* alpha;
};
struct PlaygroundResourceLit : cursiv::Extends<Expr> {
  boolean isImage;
  Expr* resourceName;
};

// Postfix chain, one node per suffix -- built by nesting around the operand
// that precedes it, so `a.b(c)!` reads as the chain it was written in.
struct Argument : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> label;
  Expr* value;
};
struct LabeledClosure : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  ClosureExpr* closure;
};
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Argument*> args;
  ClosureExpr* trailingClosure;
  vector<LabeledClosure*> labeledTrailingClosures;
};
struct InitExpr : cursiv::Extends<Expr> {
  Expr* base;
  vector<string_view> argumentLabels;
};
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* base;
  string_view name;
  vector<TypeRef*> genericArgs;
  vector<string_view> argumentLabels;
};
struct SelfPostfixExpr : cursiv::Extends<Expr> {
  Expr* base;
};
struct SubscriptExpr : cursiv::Extends<Expr> {
  Expr* base;
  vector<Argument*> args;
};
struct ForceUnwrapExpr : cursiv::Extends<Expr> {
  Expr* base;
};
struct OptionalChainExpr : cursiv::Extends<Expr> {
  Expr* base;
};
struct PostfixOpExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};
struct TryExpr : cursiv::Extends<Expr> {
  string_view kind;  // try, try?, try!
  Expr* operand;
};
struct AwaitExpr : cursiv::Extends<Expr> {
  Expr* operand;
};
struct OwnershipExpr : cursiv::Extends<Expr> {
  string_view kind;
  Expr* operand;
};
struct ImplicitMemberExpr : cursiv::Extends<Expr> {
  string_view name;
};

// A flat chain: Swift's real operator precedence is a declaration-defined table (user
// `precedencegroup`s), not something a static grammar can climb, so binary
// expressions stay unassociated here.
struct BinaryOpItem : cursiv::Extends<cursiv::AstNode> {};
struct BinaryOp : cursiv::Extends<BinaryOpItem> {
  string_view op;
  Expr* operand;
};
struct AssignOp : cursiv::Extends<BinaryOpItem> {
  Expr* operand;
};
struct TernaryOp : cursiv::Extends<BinaryOpItem> {
  Expr* thenExpr;
  Expr* operand;
};
struct IsCastOp : cursiv::Extends<BinaryOpItem> {
  TypeRef* type;
};
struct AsCastOp : cursiv::Extends<BinaryOpItem> {
  optional<string_view> marker;  // ? or !
  TypeRef* type;
};
struct BinaryChainExpr : cursiv::Extends<Expr> {
  Expr* first;
  vector<BinaryOpItem*> items;
};

// -- Conditions (if/while/guard/for-case) --------------------------------

struct AvailabilityArg : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> platform;
  optional<string_view> version;
  boolean wildcard;
};
struct Condition : cursiv::Extends<cursiv::AstNode> {
  string_view bindingKeyword;  // "let" or "var" for an optional-binding condition, else empty
  boolean isCase;              // case pattern = expr
  boolean isAvailability;
  boolean isUnavailable;
  Expr* expr;
  Pattern* pattern;
  vector<AvailabilityArg*> availability;
};

// -- Statements -----------------------------------------------------------

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct ForInStmt : cursiv::Extends<Stmt> {
  boolean tryFlag;
  boolean awaitFlag;
  boolean isCase;
  Pattern* pattern;
  Expr* collection;
  Expr* whereClause;
  Block* body;
};
struct WhileStmt : cursiv::Extends<Stmt> {
  vector<Condition*> conditions;
  Block* body;
};
struct RepeatWhileStmt : cursiv::Extends<Stmt> {
  Block* body;
  Expr* cond;
};
struct IfStmt : cursiv::Extends<Stmt> {
  vector<Condition*> conditions;
  Block* thenBranch;
  Stmt* elseBranch;  // Block* or IfStmt*
};
struct GuardStmt : cursiv::Extends<Stmt> {
  vector<Condition*> conditions;
  Block* elseBranch;
};
struct CaseItem : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* whereClause;
};
struct SwitchCase : cursiv::Extends<Stmt> {
  vector<Attribute*> attributes;
  boolean isDefault;
  vector<CaseItem*> items;
  vector<Stmt*> body;
};
struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<Stmt*> cases;  // SwitchCase* or IfConfigStmt*
};
struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};
struct BreakStmt : cursiv::Extends<Stmt> {
  optional<string_view> label;
};
struct ContinueStmt : cursiv::Extends<Stmt> {
  optional<string_view> label;
};
struct FallthroughStmt : cursiv::Extends<Stmt> {};
struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct DeferStmt : cursiv::Extends<Stmt> {
  Block* body;
};
struct CatchPattern : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* whereClause;
};
struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<CatchPattern*> patterns;
  Block* body;
};
struct DoStmt : cursiv::Extends<Stmt> {
  Block* body;
  vector<CatchClause*> catches;
};

// `os(...)`, `arch(...)`, `swift(>=5)`, `compiler(>=5)`, `canImport(...)`,
// `targetEnvironment(...)` -- the platform-condition forms of `#if`.
struct PlatformCondition : cursiv::Extends<Expr> {
  string_view kind;
  string_view argument;
};
struct IfConfigBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* condition;  // null for #else
  vector<Stmt*> body;
};
struct IfConfigStmt : cursiv::Extends<Stmt> {
  vector<IfConfigBranch*> branches;
};
struct LineControlStmt : cursiv::Extends<Stmt> {
  optional<string_view> fileName;
  optional<string_view> line;
};
struct DiagnosticStmt : cursiv::Extends<Stmt> {
  boolean isError;
  Expr* message;
};

// -- Declarations ---------------------------------------------------------

struct ImportDecl : cursiv::Extends<Decl> {
  optional<string_view> kind;
  vector<string_view> path;
};

struct PatternBinding : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* initializer;
};
struct Accessor : cursiv::Extends<cursiv::AstNode> {
  string_view mutability;
  vector<Attribute*> attributes;
  string_view kind;  // get, set, willSet, didSet
  optional<string_view> parameterName;
  string_view asyncKind;
  string_view throwsKind;
  TypeRef* thrownType;
  vector<Stmt*> body;
  boolean hasBody;
};
struct VariableDecl : cursiv::Extends<Decl> {
  boolean isConstant;
  vector<PatternBinding*> bindings;
  vector<Accessor*> accessors;
};

struct TypealiasDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  TypeRef* target;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean inoutFlag;
  vector<string_view> specifiers;
  vector<Attribute*> attributes;
  optional<string_view> externalName;
  string_view localName;
  TypeRef* type;
  Expr* defaultValue;
  boolean variadic;
};
struct FunctionDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  vector<Param*> params;
  string_view asyncKind;
  string_view throwsKind;
  TypeRef* thrownType;
  TypeRef* resultType;
  vector<Requirement*> whereClause;
  vector<Stmt*> body;
  boolean hasBody;
};

struct EnumCaseElement : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeRef* associatedValue;
  Expr* rawValue;
  TypeRef* opaqueBase;  // Legacy slot; associated values belong to associatedValue.
};
struct EnumCaseDecl : cursiv::Extends<Decl> {
  vector<EnumCaseElement*> cases;
};
struct EnumDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};

struct StructDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};
struct ClassDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};
struct ActorDecl : cursiv::Extends<Decl> {
  string_view name;
  GenericParams* generics;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};
struct ProtocolDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};
struct ExtensionDecl : cursiv::Extends<Decl> {
  TypeRef* type;
  vector<TypeRef*> inheritance;
  vector<Requirement*> whereClause;
  vector<Stmt*> members;
};
struct AssociatedTypeDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<TypeRef*> inheritance;
  TypeRef* defaultType;
  vector<Requirement*> whereClause;
};

struct InitializerDecl : cursiv::Extends<Decl> {
  optional<string_view> failable;  // ? or !
  GenericParams* generics;
  vector<Param*> params;
  string_view asyncKind;
  string_view throwsKind;
  TypeRef* thrownType;
  vector<Requirement*> whereClause;
  vector<Stmt*> body;
  boolean hasBody;
};
struct DeinitializerDecl : cursiv::Extends<Decl> {
  vector<Stmt*> body;
};
struct SubscriptDecl : cursiv::Extends<Decl> {
  GenericParams* generics;
  vector<Param*> params;
  TypeRef* resultType;
  vector<Requirement*> whereClause;
  vector<Accessor*> accessors;
};

struct OperatorDecl : cursiv::Extends<Decl> {
  string_view kind;  // prefix, postfix, infix
  string_view op;
  optional<string_view> precedenceGroup;
};
struct PrecedenceGroupDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<string_view> higherThan;
  vector<string_view> lowerThan;
  boolean isAssignment;
  optional<string_view> associativity;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> hashbang;
  vector<Stmt*> statements;
};

}  // namespace swift::ast
