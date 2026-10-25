#pragma once
#include <cstdint>
#include <string>

#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace terraform::ast {
struct Expression : cursiv::Extends<cursiv::AstNode> {};
struct Identifier : cursiv::Extends<Expression> {
  string_view name;
};
struct ParenthesizedExpr : cursiv::Extends<Expression> {
  Expression* value;
};
struct MemberExpr : cursiv::Extends<Expression> {
  Expression* object;
  string_view name;
};
struct IndexExpr : cursiv::Extends<Expression> {
  Expression* object;
  Expression* index;
};
struct AnonSymbolExpr : cursiv::Extends<Expression> {};
struct SplatExpr : cursiv::Extends<Expression> {
  Expression* source;
  Expression* each;
  boolean legacy = false;
};
enum class Operator : std::uint8_t {
  multiply,
  divide,
  modulo,
  add,
  subtract,
  greater,
  greater_equal,
  less,
  less_equal,
  equal,
  not_equal,
  logical_and,
  logical_or
};
struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};
enum class UnaryOp : std::uint8_t { negate, logical_not, positive };
struct UnaryExpr : cursiv::Extends<Expression> {
  UnaryOp op;
  Expression* operand;
};
struct TernaryExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* thenValue;
  Expression* elseValue;
};
struct ForLoop : cursiv::Extends<Expression> {
  vector<string_view> variables;
  Expression* collection;
  Expression* key;
  Expression* result;
  Expression* condition;
  boolean grouping = false;
  boolean object = false;
};
struct NullLit : cursiv::Extends<Expression> {};
struct BoolLit : cursiv::Extends<Expression> {
  boolean value = false;
};
struct NumberLit : cursiv::Extends<Expression> {
  string_view text;
};
struct TemplatePart : cursiv::Extends<cursiv::AstNode> {};
struct TemplateText : cursiv::Extends<TemplatePart> {
  string_view raw;
};
struct Interpolation : cursiv::Extends<TemplatePart> {
  Expression* value;
  boolean trimLeft = false;
  boolean trimRight = false;
};
struct TemplateDirective : cursiv::Extends<TemplatePart> {
  string_view name;
  vector<string_view> variables;
  Expression* value;
  boolean trimLeft = false;
  boolean trimRight = false;
};
struct StringLit : cursiv::Extends<Expression> {
  vector<TemplatePart*> parts;
};
struct HeredocLit : cursiv::Extends<Expression> {
  string_view text;
};
struct List : cursiv::Extends<Expression> {
  vector<Expression*> items;
};
struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expression* key;
  Expression* value;
};
struct MapObj : cursiv::Extends<Expression> {
  vector<MapEntry*> entries;
};
struct FuncCall : cursiv::Extends<Expression> {
  vector<string_view> name;
  vector<Expression*> args;
  boolean expandFinal = false;
};
struct BodyItem : cursiv::Extends<cursiv::AstNode> {};
struct Argument : cursiv::Extends<BodyItem> {
  string_view name;
  Expression* value;
};
struct Block : cursiv::Extends<BodyItem> {
  string_view blockType;
  vector<Expression*> labels;
  vector<BodyItem*> body;
};
struct File : cursiv::Extends<cursiv::AstNode> {
  vector<BodyItem*> items;
};
}  // namespace terraform::ast
