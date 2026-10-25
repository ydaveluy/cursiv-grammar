#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rego::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct NumberLiteral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
using Val =
    std::variant<std::monostate, bool, std::string_view, NumberLiteral*>;
using RefPart = std::variant<std::string_view, Expr*>;

struct Ref : cursiv::Extends<Expr> {
  string_view name;
  vector<RefPart> parts;
};
struct AccessExpr : cursiv::Extends<Expr> {
  Expr* target;
  vector<RefPart> parts;
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};
struct MembershipExpr : cursiv::Extends<Expr> {
  Expr* key;
  Expr* value;
  Expr* collection;
};
struct ObjectItem : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};
struct ObjectLit : cursiv::Extends<Expr> {
  vector<ObjectItem*> items;
};
struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct SetLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct Query;
struct ArrayCompr : cursiv::Extends<Expr> {
  Expr* item;
  Query* body;
};
struct SetCompr : cursiv::Extends<Expr> {
  Expr* item;
  Query* body;
};
struct ObjectCompr : cursiv::Extends<Expr> {
  Expr* key;
  Expr* value;
  Query* body;
};
struct ScalarTerm : cursiv::Extends<Expr> {
  string_view sign;
  Val value;
};
struct Call : cursiv::Extends<Expr> {
  Expr* target;
  vector<Expr*> args;
};
struct EqTerm : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  Expr* value;
};
struct LiteralExpr : cursiv::Extends<cursiv::AstNode> {
  Expr* first;
  vector<EqTerm*> rest;
};
struct WithMod : cursiv::Extends<cursiv::AstNode> {
  Expr* target;
  Expr* value;
};
struct QueryItem : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  vector<WithMod*> withs;
};
struct Literal : cursiv::Extends<QueryItem> {
  LiteralExpr* expr;
};
struct SomeDecl : cursiv::Extends<QueryItem> {
  vector<Expr*> symbols;
  Expr* collection;
};
struct Every : cursiv::Extends<QueryItem> {
  vector<Expr*> symbols;
  Expr* collection;
  Query* body;
};
struct Query : cursiv::Extends<cursiv::AstNode> {
  vector<QueryItem*> literals;
};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct PackageDecl : cursiv::Extends<Stmt> {
  Ref* path;
};
struct ImportDecl : cursiv::Extends<Stmt> {
  Ref* path;
  Ref* alias;
};
struct RuleBody : cursiv::Extends<cursiv::AstNode> {
  boolean isElse;
  boolean hasIf;
  string_view op;
  Expr* elseValue;
  Query* body;
};
struct Rule : cursiv::Extends<Stmt> {
  string_view name;
  vector<RefPart> parts;
  vector<Expr*> params;
  boolean contains;
  Expr* key;
  string_view op;
  Expr* value;
  vector<RuleBody*> bodies;
};
struct DefaultRule : cursiv::Extends<Rule> {};
struct QueryStmt : cursiv::Extends<Stmt> {
  Query* body;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

}  // namespace rego::ast
