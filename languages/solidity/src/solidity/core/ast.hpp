// The solidity language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace solidity::ast {

// Base of every source-level and contract-body declaration: pragmas,
// imports, contracts, functions, state variables... the list a SourceUnit
// or a contract body holds.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct TypeName : cursiv::Extends<cursiv::AstNode> {};

// A Yul statement. YulExpr extends it: a bare Yul call is both a statement
// and an expression, and the grammar admits it in either position.
struct YulStmt : cursiv::Extends<cursiv::AstNode> {};

struct YulExpr : cursiv::Extends<YulStmt> {};

struct SourceUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

struct PragmaDirective : cursiv::Extends<Item> {
  string_view text;  // everything between `pragma` and `;`, raw
};

struct ImportAlias : cursiv::Extends<cursiv::AstNode> {
  string_view symbol;
  string_view alias;
};

struct ImportDirective : cursiv::Extends<Item> {
  string_view path;       // the quoted literal, as written
  string_view unitAlias;  // `as name`, single- or star-import form
  boolean isStar;         // `import * as name from "path"`
  vector<ImportAlias*> symbols;  // `import {a, b as c} from "path"`
};

struct UsingAlias : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  string_view op;  // the overridden operator's text, empty when absent
};

struct UsingDirective : cursiv::Extends<Item> {
  string_view libraryPath;    // `using A for ...`
  vector<UsingAlias*> aliases;  // `using {a, b as +} for ...`
  TypeName* forType;           // null when `forAny`
  boolean forAny;              // `using ... for *`
  boolean isGlobal;
};

struct CallArguments;

struct InheritanceSpecifier : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  CallArguments* arguments;  // null tells `Base` apart from `Base()`
};

struct StorageLayoutSpecifier : cursiv::Extends<cursiv::AstNode> {
  Expr* baseSlot;
};

struct ContractDefinition : cursiv::Extends<Item> {
  boolean isAbstract;
  string_view name;
  vector<InheritanceSpecifier*> baseContracts;
  vector<StorageLayoutSpecifier*> storageLayouts;
  vector<Item*> body;
};

struct InterfaceDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<InheritanceSpecifier*> baseContracts;
  vector<Item*> body;
};

struct LibraryDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> body;
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  string_view location;  // memory/storage/calldata, empty when absent
  string_view name;      // empty for an unnamed parameter
};

struct ModifierInvocation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  CallArguments* arguments;  // null tells `mod` apart from `mod()`
};

struct OverrideSpecifier : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> bases;  // empty: bare `override`
};

// One item of a function-like header's modifier loop: a keyword
// (visibility, state mutability, `virtual`), a modifier call, or an
// override specifier -- the source order carries no meaning, only the set.
using FunctionModifier =
    std::variant<std::string_view, ModifierInvocation*, OverrideSpecifier*>;

struct FunctionDefinition : cursiv::Extends<Item> {
  string_view name;  // identifier, or `fallback`/`receive` used as a name
  vector<Parameter*> parameters;
  vector<FunctionModifier> modifiers;
  vector<Parameter*> returnParameters;
  Stmt* body;  // null when the declaration ends in `;`
};

struct ConstructorDefinition : cursiv::Extends<Item> {
  vector<Parameter*> parameters;
  vector<FunctionModifier> modifiers;
  Stmt* body;
};

struct ModifierDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<Parameter*> parameters;
  boolean hasParameters;  // tells `mod` apart from `mod()`
  vector<FunctionModifier> modifiers;  // virtual | override
  Stmt* body;
};

struct FallbackFunctionDefinition : cursiv::Extends<Item> {
  vector<Parameter*> parameters;
  vector<FunctionModifier> modifiers;
  vector<Parameter*> returnParameters;
  Stmt* body;
};

struct ReceiveFunctionDefinition : cursiv::Extends<Item> {
  vector<FunctionModifier> modifiers;
  Stmt* body;
};

struct StructMember : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  string_view name;
};

struct StructDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<StructMember*> members;
};

struct EnumDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> values;
};

struct UserDefinedValueTypeDefinition : cursiv::Extends<Item> {
  string_view name;
  TypeName* underlying;
};

// A state variable's header loop: visibility/mutability keywords mixed
// with an override specifier, the same shape as a function's.
using StateVarModifier = std::variant<std::string_view, OverrideSpecifier*>;

struct StateVariableDeclaration : cursiv::Extends<Item> {
  TypeName* type;
  vector<StateVarModifier> modifiers;
  string_view name;
  Expr* initialValue;
};

struct ConstantVariableDeclaration : cursiv::Extends<Item> {
  TypeName* type;
  string_view name;
  Expr* initialValue;
};

struct EventParameter : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  boolean indexed;
  string_view name;
};

struct EventDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<EventParameter*> parameters;
  boolean isAnonymous;
};

struct ErrorParameter : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  string_view name;
};

struct ErrorDefinition : cursiv::Extends<Item> {
  string_view name;
  vector<ErrorParameter*> parameters;
};

// Types.

struct ElementaryType : cursiv::Extends<TypeName> {
  string_view text;  // address, bool, string, bytesN, (u)intN, (u)fixedNxM...
  string_view mutability;
};

struct FunctionType : cursiv::Extends<TypeName> {
  vector<Parameter*> parameters;
  vector<string_view> visibility;
  vector<string_view> stateMutability;
  vector<Parameter*> returnParameters;
};

struct MappingType : cursiv::Extends<TypeName> {
  TypeName* keyType;
  string_view keyName;
  TypeName* valueType;
  string_view valueName;
};

struct NamedType : cursiv::Extends<TypeName> {
  string_view path;  // a contract, struct, enum or user-defined-value-type name
};

struct ArrayType : cursiv::Extends<TypeName> {
  TypeName* base;
  Expr* size;  // null for a dynamic array
};

// Expressions.

// An elided element of a tuple or inline array (`(a, , b)`), or of a
// `[]`/`[:]` index whose bound was left out.
struct EmptyExpr : cursiv::Extends<Expr> {};

struct IdentifierExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct ElementaryTypeExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;  // decimal or hex, as written
  string_view unit;  // wei/gwei/ether/seconds/.../years, empty when absent
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;  // adjacent literals stay separate, quotes kept
};

struct HexStringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};

struct UnicodeStringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};

struct NamedArgument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

// `(...)` after a callee: a plain list, or one of `{name: value, ...}`.
struct CallArguments : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> positional;
  vector<NamedArgument*> named;
};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> elements;  // an elided slot is an EmptyExpr
};

struct InlineArrayExpr : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};

struct NewExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct PayableConversionExpr : cursiv::Extends<Expr> {
  CallArguments* args;
};

struct MetaTypeExpr : cursiv::Extends<Expr> {
  TypeName* type;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;  // EmptyExpr when `arr[]`
};

struct IndexRangeExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* start;  // EmptyExpr when the bound was left out
  Expr* end;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct FunctionCallOptionsExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<NamedArgument*> options;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  CallArguments* args;
};

// `x++`, `x--`. The text tells the two apart.
struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

// `++x`, `--x`, `!x`, `~x`, `delete x`, `-x`.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// Every binary level of the ladder: `**`, `*` `/` `%`, `+` `-`, shifts,
// `&` `^` `|`, comparisons, `==` `!=`, `&&`, `||`. `op` holds the text.
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

// Statements.

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct UncheckedBlock : cursiv::Extends<Stmt> {
  Block* body;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Stmt* init;  // null for `for (;;)`
  Expr* cond;
  Expr* post;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct BreakStmt : cursiv::Extends<Stmt> {};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Parameter*> parameters;
  boolean hasParameters;
  Block* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<Parameter*> returnParameters;
  Block* body;
  vector<CatchClause*> catchClauses;
};

struct ThrowStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct EmitStmt : cursiv::Extends<Stmt> {
  Expr* event;
  CallArguments* args;
};

struct RevertStmt : cursiv::Extends<Stmt> {
  Expr* error;
  CallArguments* args;
};

struct AssemblyStmt : cursiv::Extends<Stmt> {
  string_view dialect;  // `"evmasm"`, empty when absent
  vector<string_view> flags;
  vector<YulStmt*> body;
};

// A single variable declaration -- also used for one written slot of a
// tuple destructuring.
struct VariableDeclaration : cursiv::Extends<cursiv::AstNode> {
  TypeName* type;
  string_view location;
  string_view name;
};

struct VariableDeclarationStatement : cursiv::Extends<Stmt> {
  VariableDeclaration* single;  // the non-tuple form
  // Every tuple slot has one element; an elision has null type and empty name.
  vector<VariableDeclaration*> tuple;
  boolean isTuple;
  Expr* initialValue;
};

struct ExpressionStatement : cursiv::Extends<Stmt> {
  Expr* value;
};

// Yul.

struct YulBlock : cursiv::Extends<YulStmt> {
  vector<YulStmt*> statements;
};

struct YulVariableDeclaration : cursiv::Extends<YulStmt> {
  vector<string_view> variables;
  YulExpr* value;
};

struct YulAssignment : cursiv::Extends<YulStmt> {
  vector<string_view> targets;
  YulExpr* value;
};

struct YulIfStmt : cursiv::Extends<YulStmt> {
  YulExpr* cond;
  YulBlock* body;
};

struct YulForStmt : cursiv::Extends<YulStmt> {
  YulBlock* init;
  YulExpr* cond;
  YulBlock* post;
  YulBlock* body;
};

struct YulSwitchCase : cursiv::Extends<cursiv::AstNode> {
  YulExpr* value;
  YulBlock* body;
};

struct YulSwitchStmt : cursiv::Extends<YulStmt> {
  YulExpr* value;
  vector<YulSwitchCase*> cases;
  YulBlock* defaultBody;  // null when there is no default case
};

// `break`, `continue`, `leave` -- three keywords, one shape.
struct YulKeywordStmt : cursiv::Extends<YulStmt> {
  string_view keyword;
};

struct YulFunctionDefinition : cursiv::Extends<YulStmt> {
  string_view name;
  vector<string_view> parameters;
  vector<string_view> returnParameters;
  YulBlock* body;
};

struct YulPathExpr : cursiv::Extends<YulExpr> {
  string_view path;
};

struct YulFunctionCall : cursiv::Extends<YulExpr> {
  string_view name;
  vector<YulExpr*> args;
};

struct YulLiteral : cursiv::Extends<YulExpr> {
  string_view text;  // decimal, hex, string or hex-string, as written
};

}  // namespace solidity::ast
