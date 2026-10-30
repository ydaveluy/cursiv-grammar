// The CLU language: its AST.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace clu::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};
struct Statement : cursiv::Extends<cursiv::AstNode> {};

// Namespace scope, so the AstNode vocabulary is out of reach: the std
// spelling is the only one.
using Constant = std::variant<Expression*, TypeSpec*>;

// ---- shared shapes ----

struct Decl : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeSpec* type = nullptr;
};

struct TypeSet;
struct OperDecl;

struct Equate : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  variant<Expression*, TypeSpec*, TypeSet*> value;
};

struct TypeSetItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OperDecl*> ops;
  vector<Equate*> equates;
};

struct TypeSet : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeSetItem*> items;
};

struct OpName : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Constant> args;
};

struct OperDecl : cursiv::Extends<cursiv::AstNode> {
  vector<OpName*> names;
  TypeSpec* type = nullptr;
};

struct Restriction : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OperDecl*> hasOps;
  TypeSet* inTypeSet = nullptr;
  string_view inName;
};

struct Exception : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeSpec*> types;
};

struct Body : cursiv::Extends<cursiv::AstNode> {
  vector<Equate*> equates;
  vector<Statement*> statements;
};

// ---- type specs ----

enum class Builtin : std::uint8_t {
  null_,
  bool_,
  int_,
  real_,
  char_,
  string_,
  any_,
  rep_,
  cvt_,
  type_
};

struct BuiltinTypeSpec : cursiv::Extends<TypeSpec> {
  Builtin kind;
};

struct ArrayTypeSpec : cursiv::Extends<TypeSpec> {
  TypeSpec* element = nullptr;
};

struct SequenceTypeSpec : cursiv::Extends<TypeSpec> {
  TypeSpec* element = nullptr;
};

struct FieldSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeSpec* type = nullptr;
};

enum class RecordKind : std::uint8_t { record_, struct_, oneof_, variant_ };

struct RecordTypeSpec : cursiv::Extends<TypeSpec> {
  RecordKind kind;
  vector<FieldSpec*> fields;
};

enum class RoutineTypeKind : std::uint8_t { proc_, iter_ };

struct RoutineTypeSpec : cursiv::Extends<TypeSpec> {
  RoutineTypeKind kind;
  vector<TypeSpec*> params;
  vector<TypeSpec*> results;
  vector<Exception*> signals;
};

// A user-defined type, possibly parameterized: `idn`, or `idn[c1, c2]`.
struct NamedTypeSpec : cursiv::Extends<TypeSpec> {
  string_view name;
  vector<Constant> args;
};

// ---- expressions ----

enum class BinOp : std::uint8_t {
  power,
  intdiv,
  div,
  mul,
  concat,
  add,
  sub,
  lt,
  le,
  eq,
  ge,
  gt,
  nlt,
  nle,
  neq,
  nge,
  ngt,
  and_,
  cand,
  or_,
  cor
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left = nullptr;
  BinOp op;
  Expression* right = nullptr;
};

enum class UnOp : std::uint8_t { not_, neg };

struct UnaryExpr : cursiv::Extends<Expression> {
  UnOp op;
  Expression* operand = nullptr;
};

struct NilLit : cursiv::Extends<Expression> {};

struct BoolLit : cursiv::Extends<Expression> {
  boolean value;
};

struct IntLit : cursiv::Extends<Expression> {
  std::int64_t value = 0;
};

struct RealLit : cursiv::Extends<Expression> {
  double value = 0.0;
};

struct CharLit : cursiv::Extends<Expression> {
  string_view value;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

// A bare name, optionally parameterized: `x`, or the constant-abbreviation
// `mytype[int, 5]`.
struct NameRef : cursiv::Extends<Expression> {
  string_view name;
  vector<Constant> args;
};

struct GroupedExpr : cursiv::Extends<Expression> {
  Expression* inner = nullptr;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Expression* value = nullptr;
};

// `type$ { field, ... }` -- a record/struct constructor.
struct TypeRecordLit : cursiv::Extends<Expression> {
  TypeSpec* type = nullptr;
  vector<Field*> fields;
};

// `type$[ [start :] item, ... ]` -- an array/sequence literal.
struct TypeSeqLit : cursiv::Extends<Expression> {
  TypeSpec* type = nullptr;
  Expression* start = nullptr;
  vector<Expression*> items;
};

// `type$name[c1, c2]` -- an operation of a type used as a value.
struct TypeOpRef : cursiv::Extends<Expression> {
  TypeSpec* type = nullptr;
  string_view name;
  vector<Constant> args;
};

struct ForceExpr : cursiv::Extends<Expression> {
  TypeSpec* type = nullptr;
};

enum class UpDown : std::uint8_t { up, down };

struct UpDownExpr : cursiv::Extends<Expression> {
  UpDown kind;
  Expression* operand = nullptr;
};

struct Select : cursiv::Extends<Expression> {
  Expression* object = nullptr;
  string_view name;
};

// `primary [ expression ]`. An unbracketed reading of this
// alternative is a primary immediately followed by another expression --
// the shape of two adjacent statements -- and is unboundedly ambiguous.
struct Index : cursiv::Extends<Expression> {
  Expression* object = nullptr;
  Expression* index = nullptr;
};

struct Call : cursiv::Extends<Expression> {
  Expression* callee = nullptr;
  vector<Expression*> args;
};

// ---- statements ----

struct DeclStmt : cursiv::Extends<Statement> {
  Decl* decl = nullptr;
};

struct VarInitStmt : cursiv::Extends<Statement> {
  string_view name;
  TypeSpec* type = nullptr;
  Expression* value = nullptr;
};

struct MultiInitStmt : cursiv::Extends<Statement> {
  vector<Decl*> decls;
  Expression* value = nullptr;  // a Call
};

struct AssignStmt : cursiv::Extends<Statement> {
  vector<string_view> names;
  vector<Expression*> values;
};

struct SelectAssignStmt : cursiv::Extends<Statement> {
  Expression* target = nullptr;
  string_view name;
  Expression* value = nullptr;
};

// `primary [ expression ] := expression`, the assignment counterpart of
// `Index`, added for the same reason.
struct IndexAssignStmt : cursiv::Extends<Statement> {
  Expression* target = nullptr;
  Expression* index = nullptr;
  Expression* value = nullptr;
};

struct ExprStmt : cursiv::Extends<Statement> {
  Expression* call = nullptr;
};

struct WhileStmt : cursiv::Extends<Statement> {
  Expression* cond = nullptr;
  Body* body = nullptr;
};

struct ForStmt : cursiv::Extends<Statement> {
  vector<Decl*> decls;
  vector<string_view> names;
  Expression* iterCall = nullptr;  // a Call
  Body* body = nullptr;
};

struct ElseIf : cursiv::Extends<cursiv::AstNode> {
  Expression* cond = nullptr;
  Body* body = nullptr;
};

struct IfStmt : cursiv::Extends<Statement> {
  Expression* cond = nullptr;
  Body* thenBody = nullptr;
  vector<ElseIf*> elseIfs;
  Body* elseBody = nullptr;
};

struct TagArm : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> tags;
  string_view bindName;
  TypeSpec* bindType = nullptr;
  Body* body = nullptr;
};

struct TagcaseStmt : cursiv::Extends<Statement> {
  Expression* expr = nullptr;
  vector<TagArm*> arms;
  boolean hasOthers;
  Body* othersBody = nullptr;
};

struct ReturnStmt : cursiv::Extends<Statement> {
  boolean isYield;
  vector<Expression*> values;
};

struct SignalStmt : cursiv::Extends<Statement> {
  string_view name;
  vector<Expression*> values;
};

struct ExitStmt : cursiv::Extends<Statement> {
  string_view name;
  vector<Expression*> values;
};

struct BreakStmt : cursiv::Extends<Statement> {};
struct ContinueStmt : cursiv::Extends<Statement> {};

struct BeginStmt : cursiv::Extends<Statement> {
  Body* body = nullptr;
};

struct WhenHandler : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  boolean star;
  vector<Decl*> decls;
  Body* body = nullptr;
};

struct OthersHandler : cursiv::Extends<cursiv::AstNode> {
  string_view bindName;
  TypeSpec* bindType = nullptr;
  Body* body = nullptr;
};

struct ExceptStmt : cursiv::Extends<Statement> {
  Statement* inner = nullptr;
  vector<WhenHandler*> whens;
  OthersHandler* others = nullptr;
};

struct ResignalStmt : cursiv::Extends<Statement> {
  Statement* inner = nullptr;
  vector<string_view> names;
};

// ---- routines ----

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  boolean isTypeParam;
  TypeSpec* type = nullptr;
};

struct OwnVar : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  Expression* init = nullptr;
};

struct RoutineDecl : cursiv::Extends<cursiv::AstNode> {};

struct Routine : cursiv::Extends<RoutineDecl> {
  boolean isIterator;
  string_view name;
  vector<Param*> parms;
  vector<Decl*> args;
  vector<TypeSpec*> results;  // returns, or yields
  vector<Exception*> signals;
  vector<Restriction*> where;
  vector<Equate*> bodyEquates;
  vector<OwnVar*> ownVars;
  vector<Statement*> statements;
  string_view endName;
};

struct Cluster : cursiv::Extends<RoutineDecl> {
  string_view name;
  vector<Param*> parms;
  vector<string_view> isNames;
  vector<Restriction*> where;
  vector<Equate*> preRepEquates;
  TypeSpec* rep = nullptr;
  vector<Equate*> postRepEquates;
  vector<OwnVar*> ownVars;
  vector<Routine*> routines;
  string_view endName;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  vector<Equate*> equates;
  RoutineDecl* routine = nullptr;
};

}  // namespace clu::ast
