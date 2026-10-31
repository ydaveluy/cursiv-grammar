// The moo language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace moo::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A property reference: `base` then a chain of `.name` or `[index]` steps.
// Namespace scope, so the AstNode vocabulary (`vector`, `reference`) is out
// of reach for the variant and `std::string_view`/`std::variant` are spelled.
using PropAccess = std::variant<std::string_view, Expr*>;

struct Property : cursiv::Extends<Expr> {
  string_view base;
  boolean stringBase;
  boolean objectBase;
  vector<PropAccess> path;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value = 0;
};

struct RealLit : cursiv::Extends<Expr> {
  float64 value = 0;
};

struct ErrorLit : cursiv::Extends<Expr> {
  string_view name;
};

struct ObjRef : cursiv::Extends<Expr> {
  string_view text;  // `#123`, kept raw: the id is rarely computed on
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};

// One enum for both arithmetic and relational levels: an operator token
// hands over what it means, not what it spelled.
enum class Operator : std::uint8_t {
  power,
  multiply,
  divide,
  modulo,
  add,
  subtract,
  equal,
  not_equal,
  greater,
  greater_equal,
  less,
  less_equal,
  logical_and,
  logical_or,
  in_,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  Operator op{};
  Expr* right = nullptr;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* condition;
  Expr* consequent;
  Expr* alternate;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

struct VerbCall : cursiv::Extends<Expr> {
  Property* target = nullptr;
  string_view verb;
  vector<Expr*> args;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Statement*> body;
};
struct IfStmt : cursiv::Extends<Statement> {
  Expr* cond = nullptr;
  vector<Statement*> thenBody;
  vector<ElseIfClause*> elseIfs;
  boolean hasElse;
  vector<Statement*> elseBody;
};

struct WhileStmt : cursiv::Extends<Statement> {
  string_view name;
  Expr* cond = nullptr;
  vector<Statement*> body;
};

struct DoStmt : cursiv::Extends<Statement> {
  vector<Statement*> body;
  Expr* cond = nullptr;
};

struct ForkStmt : cursiv::Extends<Statement> {
  string_view name;
  Expr* delay;
  vector<Statement*> body;
};

struct ForStmt : cursiv::Extends<Statement> {
  string_view var;
  Expr* iterable = nullptr;
  Expr* from;
  Expr* to;
  vector<Statement*> body;
};

struct ExceptClause : cursiv::Extends<cursiv::AstNode> {
  string_view variable;
  boolean any;
  vector<Expr*> codes;
  vector<Statement*> body;
};
struct LoopControlStmt : cursiv::Extends<Statement> {
  boolean continue_;
  string_view name;
};
struct TryStmt : cursiv::Extends<Statement> {
  vector<Statement*> body;
  Property* except = nullptr;
  vector<Statement*> handler;
  vector<ExceptClause*> clauses;
  boolean hasFinally;
  vector<Statement*> cleanup;
};

struct AssignStmt : cursiv::Extends<Statement> {
  Property* target = nullptr;
  Expr* value = nullptr;
};

struct ReturnCommand : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
};

using Command = std::variant<VerbCall*, ReturnCommand*, FuncCall*>;

struct CommandStmt : cursiv::Extends<Statement> {
  Command command;
};

struct Declaration : cursiv::Extends<cursiv::AstNode> {};

struct ProgramDecl : cursiv::Extends<Declaration> {
  string_view object;
  string_view verb;
  vector<Statement*> body;
};

struct VerbDecl : cursiv::Extends<Declaration> {
  string_view object;
  string_view verb;
  vector<string_view> args;
  string_view permissions;
};

struct PropertyDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
  Expr* value = nullptr;
  string_view permissions;
};

struct RmPropertyDecl : cursiv::Extends<Declaration> {
  string_view name;
};

struct SetPropertyDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
  Expr* value = nullptr;
};

struct DisplayPropertyDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
};

struct KidsDecl : cursiv::Extends<Declaration> {
  string_view name;
};

struct ParentDecl : cursiv::Extends<Declaration> {
  string_view name;
};

struct DescribeDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
  Expr* value = nullptr;
};

struct ContentsDecl : cursiv::Extends<Declaration> {
  string_view name;
};

struct NoteditDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
};

struct CreateDecl : cursiv::Extends<Declaration> {
  string_view parent;
  vector<Expr*> args;
};

struct EditDecl : cursiv::Extends<Declaration> {
  Property* target = nullptr;
};

struct AddAliasDecl : cursiv::Extends<Declaration> {
  vector<string_view> names;
  Expr* target = nullptr;
};

struct Prog : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration*> declarations;
};

}  // namespace moo::ast
