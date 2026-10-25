// The thrift language: its AST. One node per grammar shape; struct/union/
// exception share a node since they differ only in their opening keyword.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace thrift::ast {

struct Header : cursiv::Extends<cursiv::AstNode> {};

struct Include : cursiv::Extends<Header> {
  string_view path;
};

struct CppInclude : cursiv::Extends<Header> {
  string_view path;
};

struct TypeAnnotation;

struct NamespaceDecl : cursiv::Extends<Header> {
  // `*`, a scope identifier, or -- for the two legacy forms -- the
  // pseudo-keyword text itself ("cpp_namespace"/"php_namespace").
  string_view scope;
  string_view value;
  vector<TypeAnnotation*> annotations;
};

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct FieldType : cursiv::Extends<cursiv::AstNode> {};

struct BaseType : cursiv::Extends<FieldType> {
  string_view keyword;  // bool, byte, i16, i32, i64, double, string, binary
  vector<TypeAnnotation*> annotations;
};

struct NamedType : cursiv::Extends<FieldType> {
  string_view name;
};

struct MapType : cursiv::Extends<FieldType> {
  optional<string_view> cppType;
  FieldType* key;
  FieldType* value;
  vector<TypeAnnotation*> annotations;
};

struct SetType : cursiv::Extends<FieldType> {
  optional<string_view> cppType;
  FieldType* element;
  vector<TypeAnnotation*> annotations;
};

struct ListType : cursiv::Extends<FieldType> {
  FieldType* element;
  vector<string_view> cppTypes;
  vector<TypeAnnotation*> annotations;
};

struct VoidType : cursiv::Extends<FieldType> {};

struct ConstRef;
struct ConstList;
struct ConstMap;

// A constant value: a scalar inline, or a node for the recursive shapes
// and for a bare identifier reference (kept as a node, not text, so it
// does not collide with the LITERAL slot of the same variant).
using ConstValue = std::variant<std::monostate, std::int64_t, double,
                                std::string_view, ConstRef*, ConstList*,
                                ConstMap*>;

struct ConstRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // unresolved: may name a const, an enum value, or none
};

struct ConstList : cursiv::Extends<cursiv::AstNode> {
  vector<ConstValue> items;
};

struct ConstMapEntry : cursiv::Extends<cursiv::AstNode> {
  ConstValue key;
  ConstValue value;
};

struct ConstMap : cursiv::Extends<cursiv::AstNode> {
  vector<ConstMapEntry*> entries;
};

using AnnotationValue = std::variant<std::monostate, std::int64_t, std::string_view>;

struct TypeAnnotation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  AnnotationValue value;
};

enum class FieldReq : std::uint8_t { required_, optional_ };

struct Field : cursiv::Extends<cursiv::AstNode> {
  boolean isReference;
  boolean xsdOptional;
  boolean xsdNillable;
  vector<Field*> xsdAttrs;
  optional<int64> id;
  optional<FieldReq> req;
  FieldType* type;
  string_view name;
  ConstValue value;
  vector<TypeAnnotation*> annotations;
};

struct ConstDef : cursiv::Extends<Definition> {
  FieldType* type;
  string_view name;
  ConstValue value;
};

struct TypedefDef : cursiv::Extends<Definition> {
  FieldType* type;
  string_view name;
  vector<TypeAnnotation*> annotations;
};

struct EnumField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<int64> value;
  vector<TypeAnnotation*> annotations;
};

struct EnumDef : cursiv::Extends<Definition> {
  string_view name;
  vector<EnumField*> fields;
  vector<TypeAnnotation*> annotations;
};

struct SenumDef : cursiv::Extends<Definition> {
  string_view name;
  vector<string_view> values;
  vector<TypeAnnotation*> annotations;
};

enum class StructKind : std::uint8_t { struct_, union_, exception };

struct StructDef : cursiv::Extends<Definition> {
  boolean xsdAll;
  StructKind kind;
  string_view name;
  vector<Field*> fields;
  vector<TypeAnnotation*> annotations;
};

struct Function : cursiv::Extends<cursiv::AstNode> {
  boolean oneway;  // `oneway` or its legacy alias `async`
  FieldType* returnType;
  string_view name;
  vector<Field*> params;
  vector<Field*> throws;
  vector<TypeAnnotation*> annotations;
};

struct ServiceDef : cursiv::Extends<Definition> {
  string_view name;
  optional<string_view> extends_;
  vector<Function*> functions;
  vector<TypeAnnotation*> annotations;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Header*> headers;
  vector<Definition*> definitions;
};

}  // namespace thrift::ast
