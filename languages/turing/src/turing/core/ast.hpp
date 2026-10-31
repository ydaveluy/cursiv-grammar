// The turing language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace turing::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<Item> {};
struct Stmt : cursiv::Extends<Item> {};
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Selector : cursiv::Extends<cursiv::AstNode> {};

struct InitList;

struct TypeDecl : cursiv::Extends<Decl> {
  string_view name;
  TypeSpec* typeSpec;
};

struct FieldSelector : cursiv::Extends<Selector> {
  string_view field;
};
struct IndexSelector : cursiv::Extends<Selector> {
  vector<Expr*> args;
};
// A substring bound: a value, or `*` counted from the end, optionally offset
// (`* - n`).
struct Position : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean fromEnd;
  Expr* offset;
};
struct RangeSelector : cursiv::Extends<Selector> {
  Position* from;
  Position* to;  // null: a single position, no range
};

// A name plus the chain of selectors applied to it -- indexing, field
// access, substring -- also stands for a bare procedure call, the same
// shape with zero selectors.
struct Reference : cursiv::Extends<Expr> {
  string_view name;
  vector<Selector*> selectors;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};
struct RealLit : cursiv::Extends<Expr> {
  float64 value;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

enum class BinOp : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  intDivide,
  modulo,
  exponent,
  less,
  greater,
  equal,
  lessEqual,
  greaterEqual,
  notEqual,
  logicalAnd,
  logicalOr,
  member,
  notMember,
  implies,
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

enum class UnOp : std::uint8_t { plus, minus, logicalNot };
struct UnaryExpr : cursiv::Extends<Expr> {
  UnOp op;
  Expr* operand;
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// `init(...)`, nested: an array or record initializer. A plain value needs
// no node of its own -- it lives inline in the variant it fills.
struct InitList : cursiv::Extends<cursiv::AstNode> {
  vector<variant<Expr*, InitList*>> items;
};

struct StandardType : cursiv::Extends<TypeSpec> {
  enum class Kind : std::uint8_t { int_, real, boolean, string };
  Kind kind;
  Expr* length;  // string(N); null otherwise
};
struct SubrangeType : cursiv::Extends<TypeSpec> {
  Expr* low;
  Expr* high;
};
struct EnumType : cursiv::Extends<TypeSpec> {
  vector<string_view> names;
};
struct SetType : cursiv::Extends<TypeSpec> {
  TypeSpec* base;
};
struct PointerType : cursiv::Extends<TypeSpec> {
  string_view collection;
};
struct RecordField : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeSpec* type;
};
struct RecordType : cursiv::Extends<TypeSpec> {
  vector<RecordField*> fields;
};
struct ArrayType : cursiv::Extends<TypeSpec> {
  vector<TypeSpec*> indices;
  TypeSpec* element;
};
// Text, not a reference: the corpus names standard types this grammar's
// standardType never lists (`nat`), so an unresolved lookup is not an error.
struct NamedType : cursiv::Extends<TypeSpec> {
  string_view name;
};
// `string(*)`: an open bound, legal only in a parameter type.
struct StringStarType : cursiv::Extends<TypeSpec> {};

struct ParamType : cursiv::Extends<cursiv::AstNode> {};
struct SimpleParamType : cursiv::Extends<ParamType> {
  TypeSpec* type;
};
struct OpenArrayParamType : cursiv::Extends<ParamType> {
  vector<Expr*> bounds;
  TypeSpec* element;
};
struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean isVar;
  vector<string_view> names;
  ParamType* type;
};

struct SubprogramDecl : cursiv::Extends<Item> {
  boolean isFunction;
  string_view name;
  vector<Param*> params;
  TypeSpec* returnType;  // function only
  vector<Item*> body;
  string_view endName;
};

struct ConstDecl : cursiv::Extends<Decl> {
  string_view name;
  TypeSpec* typeSpec;
  variant<Expr*, InitList*> value;
};
struct VarDecl : cursiv::Extends<Decl> {
  vector<string_view> names;
  TypeSpec* typeSpec;
  optional<variant<Expr*, InitList*>> value;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};
struct CallStmt : cursiv::Extends<Stmt> {
  Expr* call;
};
struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* cond;
};
struct ReturnStmt : cursiv::Extends<Stmt> {};
struct BeginStmt : cursiv::Extends<Stmt> {
  vector<Item*> body;
};

struct ResultStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ElsifClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Item*> body;
};
struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Item*> thenBody;
  vector<ElsifClause*> elsifs;
  vector<Item*> elseBody;
};
struct LoopStmt : cursiv::Extends<Stmt> {
  vector<Item*> body;
};
struct ExitStmt : cursiv::Extends<Stmt> {
  Expr* cond;  // null: unconditional
};

struct CaseLabel : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> values;  // empty: the trailing default label
  vector<Item*> body;
};
struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<CaseLabel*> labels;
};

struct ForStmt : cursiv::Extends<Stmt> {
  boolean decreasing;
  string_view varName;
  Expr* from;
  Expr* to;
  Expr* step;  // null: no 'by'
  vector<Item*> body;
};

struct PutItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;  // null: 'skip'
  Expr* width;
  Expr* fraction;
  Expr* exponent;
};
struct PutStmt : cursiv::Extends<Stmt> {
  Expr* stream;
  vector<PutItem*> items;
  boolean noNewline;
};

struct GetItem : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  Expr* width;
  boolean skipToEnd;
};
struct GetStmt : cursiv::Extends<Stmt> {
  Expr* stream;
  vector<GetItem*> items;
};

enum class Capability : std::uint8_t { get_, put_ };
struct OpenStmt : cursiv::Extends<Stmt> {
  Expr* fileNumber;
  string_view path;
  vector<Capability> capabilities;
};
struct CloseStmt : cursiv::Extends<Stmt> {
  Expr* fileNumber;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace turing::ast
