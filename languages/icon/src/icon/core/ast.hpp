// The Icon language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace icon::ast {

struct Declaration : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct InvocableItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view arity;
};
struct InvocableDeclaration : cursiv::Extends<Declaration> {
  vector<InvocableItem*> items;
};
struct NullExpr : cursiv::Extends<Expression> {};

struct LinkDeclaration : cursiv::Extends<Declaration> {
  vector<string_view> files;
};

struct GlobalDeclaration : cursiv::Extends<Declaration> {
  vector<string_view> names;
};

struct RecordDeclaration : cursiv::Extends<Declaration> {
  string_view name;
  vector<string_view> fields;
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean vararg;
};

struct LocalDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  vector<string_view> names;
};

struct Procedure : cursiv::Extends<Declaration> {
  string_view name;
  vector<Parameter*> params;
  vector<LocalDecl*> locals;
  Expression* initial;
  vector<Expression*> body;
};

struct BreakExpr : cursiv::Extends<Expression> {
  Expression* value;
};

struct CreateExpr : cursiv::Extends<Expression> {
  Expression* value;
};

struct ReturnExpr : cursiv::Extends<Expression> {
  Expression* value;
};

struct SuspendExpr : cursiv::Extends<Expression> {
  Expression* value;
  Expression* doClause;
};

struct FailExpr : cursiv::Extends<Expression> {};
struct NextExpr : cursiv::Extends<Expression> {};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Expression* test;
  boolean isDefault;
  Expression* value;
};

struct CaseExpr : cursiv::Extends<Expression> {
  Expression* selector;
  vector<CaseClause*> clauses;
};

struct IfExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* thenValue;
  Expression* elseValue;
};

struct RepeatExpr : cursiv::Extends<Expression> {
  Expression* body;
};

struct WhileExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* doClause;
};

struct UntilExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* doClause;
};

struct EveryExpr : cursiv::Extends<Expression> {
  Expression* generator;
  Expression* doClause;
};

// Every binary level of expr1..expr3 and expr5..expr11: op holds the
// operator's text.
struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  string_view op;
  Expression* right;
};

// expr4: `a to b` and `a to b by c`. Ternary, so not a level of a ladder.
struct ToRangeExpr : cursiv::Extends<Expression> {
  Expression* from;
  Expression* to;
  Expression* by;
};

// expr12, the prefix chain: `not x`, `-x`, `\x`, `.x`, ...
struct PrefixExpr : cursiv::Extends<Expression> {
  string_view op;
  Expression* operand;
};

struct ExprList : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

struct Block : cursiv::Extends<Expression> {
  vector<Expression*> body;
};

struct ListLit : cursiv::Extends<Expression> {
  vector<Expression*> items;
};

struct NameRef : cursiv::Extends<Expression> {
  string_view name;
};

// `&null`, `&fail`, `&pos`, ... -- a keyword variable.
struct KeywordRef : cursiv::Extends<Expression> {
  string_view name;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view text;
};

struct CsetLit : cursiv::Extends<Expression> {
  string_view text;
};

struct IntLit : cursiv::Extends<Expression> {
  string_view text;
};

struct RealLit : cursiv::Extends<Expression> {
  string_view text;
};

struct FieldAccess : cursiv::Extends<Expression> {
  Expression* object;
  string_view field;
};

struct CallExpr : cursiv::Extends<Expression> {
  Expression* callee;
  vector<Expression*> args;
};

// `expr13 '{' expression_list '}'` -- a second call form the grammar keeps
// distinct from `(...)`.
struct BraceCallExpr : cursiv::Extends<Expression> {
  Expression* callee;
  vector<Expression*> args;
};

// A plain index when op/extent are empty; `a:b`, `a+:b`, `a-:b` otherwise.
struct Subscript : cursiv::Extends<cursiv::AstNode> {
  Expression* index;
  string_view op;
  Expression* extent;
};

struct SubscriptExpr : cursiv::Extends<Expression> {
  Expression* object;
  vector<Subscript*> subscripts;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration*> declarations;
};

}  // namespace icon::ast
