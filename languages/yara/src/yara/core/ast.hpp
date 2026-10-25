#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include <cstdint>
#include <string_view>
#include <variant>

namespace yara::ast {
struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  string_view module;
};
struct IncludeDecl : cursiv::Extends<cursiv::AstNode> {
  string_view path;
};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Literal : cursiv::Extends<Expr> {
  string_view text;
  boolean quoted;
};
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};
struct IndexExpr : cursiv::Extends<Expr> {
  Expr *base;
  Expr *index;
};
struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  Expr *field;
};
struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args;
};
struct Range : cursiv::Extends<Expr> {
  Expr *lo;
  Expr *hi;
};
struct OfExpr : cursiv::Extends<Expr> {
  Expr *quantifier;
  boolean percentage;
  vector<string_view> set;
  boolean them;
  Range *range;
  Expr *offset;
};
struct InExpr : cursiv::Extends<Expr> {
  Expr *subject;
  Range *range;
};
struct AtExpr : cursiv::Extends<Expr> {
  Expr *subject;
  Expr *offset;
};
struct SetExpr : cursiv::Extends<Expr> {
  vector<Expr *> items;
};
struct ForExpr : cursiv::Extends<Expr> {
  Expr *quantifier;
  vector<string_view> vars;
  Expr *iterable;
  vector<string_view> set;
  boolean them;
  Expr *body;
};
struct Jump : cursiv::Extends<cursiv::AstNode> {
  string_view lo;
  boolean ranged;
  string_view hi;
};
struct Alt;
using HexItem = std::variant<std::string_view, Jump *, Alt *>;
struct AltBranch : cursiv::Extends<cursiv::AstNode> {
  vector<HexItem> items;
};
struct Alt : cursiv::Extends<cursiv::AstNode> {
  vector<AltBranch *> branches;
};
struct HexPattern : cursiv::Extends<cursiv::AstNode> {
  vector<HexItem> items;
};
struct RegexLit : cursiv::Extends<Expr> {
  string_view text;
};
using StringVal = std::variant<std::string_view, RegexLit *, HexPattern *>;
struct StringModifier : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  boolean hasArg;
  string_view arg;
  optional<string_view> lo;
  boolean ranged;
  optional<string_view> hi;
};
struct StringDef : cursiv::Extends<cursiv::AstNode> {
  string_view id;
  StringVal value;
  vector<StringModifier *> modifiers;
};
struct IntegerLiteral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
using MetaVal = std::variant<bool, IntegerLiteral *, double, std::string_view>;
struct MetaEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  MetaVal value;
};
struct RuleDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isPrivate;
  boolean isGlobal;
  string_view name;
  vector<string_view> tags;
  vector<MetaEntry *> meta;
  vector<StringDef *> strings;
  Expr *condition;
};
using Declaration = std::variant<ImportDecl *, IncludeDecl *, RuleDecl *>;
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration> declarations;
};
} // namespace yara::ast
