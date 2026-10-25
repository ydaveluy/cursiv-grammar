#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace capnproto::ast {
struct Expression : cursiv::Extends<cursiv::AstNode> {};
struct NameExpr : cursiv::Extends<Expression> {
  string_view name;
  boolean absolute = false;
};
struct IntegerLiteral : cursiv::Extends<Expression> {
  string_view text;
};
struct FloatLiteral : cursiv::Extends<Expression> {
  string_view text;
};
struct NegativeExpr : cursiv::Extends<Expression> {
  Expression* operand;
};
struct TextLiteral : cursiv::Extends<Expression> {
  string_view value;
};
struct BytesLiteral : cursiv::Extends<Expression> {
  string_view value;
};
struct ImportExpr : cursiv::Extends<Expression> {
  string_view path;
};
struct EmbedExpr : cursiv::Extends<Expression> {
  string_view path;
};
struct ConstList : cursiv::Extends<Expression> {
  vector<Expression*> items;
};
struct ConstFieldAssign : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expression* value;
};
struct ConstStruct : cursiv::Extends<Expression> {
  vector<ConstFieldAssign*> fields;
};
struct MemberExpr : cursiv::Extends<Expression> {
  Expression* parentExpr;
  string_view name;
};
struct ApplicationExpr : cursiv::Extends<Expression> {
  Expression* function;
  vector<ConstFieldAssign*> params;
};
struct AnnotationReference : cursiv::Extends<cursiv::AstNode> {
  Expression* expression;
};
struct Id : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
struct Declaration : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Id* id;
  vector<AnnotationReference*> annotations;
  vector<Declaration*> content;
};
struct FileId : cursiv::Extends<Declaration> {
  Id* value;
};
struct AnnotationStmt : cursiv::Extends<Declaration> {
  AnnotationReference* annotation;
};
struct UsingAlias : cursiv::Extends<Declaration> {
  Expression* target;
};
struct StructDef : cursiv::Extends<Declaration> {
  vector<string_view> typeParams;
};
struct InterfaceDef : cursiv::Extends<Declaration> {
  vector<string_view> typeParams;
  vector<Expression*> extends;
};
struct EnumDef : cursiv::Extends<Declaration> {};
struct EnumContent : cursiv::Extends<Declaration> {};
struct FieldDef : cursiv::Extends<Declaration> {
  Expression* type;
  Expression* defaultValue;
};
struct GroupDef : cursiv::Extends<Declaration> {};
struct UnionDef : cursiv::Extends<Declaration> {
  boolean legacy = false;
};
struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expression* type;
  Expression* defaultValue;
  vector<AnnotationReference*> annotations;
};
struct ParamList : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  Expression* type;
  boolean stream = false;
};
struct FunctionDef : cursiv::Extends<Declaration> {
  vector<string_view> typeParams;
  ParamList* params;
  ParamList* results;
};
struct AnnotationDef : cursiv::Extends<Declaration> {
  vector<string_view> targets;
  Expression* valueType;
};
struct ConstDef : cursiv::Extends<Declaration> {
  Expression* type;
  Expression* value;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Declaration*> content;
};
}  // namespace capnproto::ast
