// The peoplecode language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace peoplecode::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

// import A:B:C -- a plain class import; import A:B:* -- wildcard, `target`
// then holds "A:B" and `wildcard` is set.
struct ImportStmt : cursiv::Extends<Stmt> {
  string_view target;
  boolean wildcard;
};

// Declare Function f PeopleCode REC.FIELD FieldChange;
struct DeclareFuncStmt : cursiv::Extends<Stmt> {
  string_view name;
  string_view recDefnPath;
  string_view event;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view varType;  // empty when no "As" clause
};

struct ClassMember : cursiv::Extends<cursiv::AstNode> {};

struct MethodDecl : cursiv::Extends<ClassMember> {
  string_view name;
  vector<Param*> params;
  string_view returnType;
};

struct ConstantDecl : cursiv::Extends<ClassMember> {
  string_view name;
  Expr* value;
};

struct PropertyDecl : cursiv::Extends<ClassMember> {
  string_view varType;
  string_view name;
  boolean hasGet;
  boolean hasSet;
  boolean readonly;
};

struct InstanceDecl : cursiv::Extends<ClassMember> {
  string_view varType;
  vector<string_view> names;
};

struct ClassBlock : cursiv::Extends<cursiv::AstNode> {
  boolean isPrivate;
  vector<ClassMember*> members;
};

struct ClassDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<ClassBlock*> blocks;
};

struct MethodImplStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Stmt*> body;
};

struct GetImplStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Stmt*> body;
};

struct SetImplStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Stmt*> body;
};

struct FuncImplStmt : cursiv::Extends<Stmt> {
  string_view name;
  vector<Param*> params;
  string_view returnType;
  vector<Stmt*> body;
};

struct VarDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* init;
};

struct VarDeclStmt : cursiv::Extends<Stmt> {
  string_view scope;  // "Local", "Global", "Component", ... -- a plain name
  string_view varType;
  vector<VarDeclarator*> declarators;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBranch;
  vector<Stmt*> elseBranch;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view var;
  Expr* from;
  Expr* to;
  Expr* step;  // null when the Step clause is absent
  vector<Stmt*> body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

// The comparison the branch tests against the Evaluate value: "=" (also the
// default, when the branch writes no operator), or another comparison.
struct WhenBranch : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  Expr* value;
  vector<Stmt*> body;
};

struct EvaluateStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<WhenBranch*> branches;
  boolean hasOtherBranch;
  vector<Stmt*> otherBranch;
};

struct TryCatchStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> tryBody;
  string_view exceptionVar;
  vector<Stmt*> catchBody;
};

struct CreateExpr : cursiv::Extends<Expr> {
  string_view className;
  vector<Expr*> args;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct DecimalLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct IdExpr : cursiv::Extends<Expr> {
  string_view name;  // as written, sigil included: "&x", "%This", "Field"
};

struct DynamicRefExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct DotExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  vector<Expr*> indices;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct NegateExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

enum class BinOp : std::uint8_t {
  multiply,
  divide,
  add,
  subtract,
  less_equal,
  greater_equal,
  less,
  greater,
  equal,
  not_equal,
  logical_and,
  logical_or,
  concat,
  power,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

struct ExitStmt : cursiv::Extends<Stmt> {};
struct BreakStmt : cursiv::Extends<Stmt> {};

struct ErrorStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct WarningStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;  // null when bare "Return"
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

}  // namespace peoplecode::ast
