// The flatbuffers language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace flatbuffers::ast {

// The base of every top-level schema item, so `Schema::declarations` can
// hold them in the order they were written.
struct Decl : cursiv::Extends<cursiv::AstNode> {};

struct Include : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  boolean isNative;
};

struct Namespace : cursiv::Extends<Decl> {
  string_view path;
};

struct Attribute : cursiv::Extends<Decl> {
  string_view name;
};

struct Root : cursiv::Extends<Decl> {
  string_view name;
};

struct FileExtension : cursiv::Extends<Decl> {
  string_view ext;
};

struct FileIdentifier : cursiv::Extends<Decl> {
  string_view id;
};

// One `(name: value)` or bare `(name)` pair, attached to a type, a field, an
// enum, a union or an rpc method.
struct Metadata : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> value;
};

// A field's type: a base type or a qualified custom-type name in `name`, or
// `[element]` / `[element:length]` with `name` left empty.
struct Type : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* element;
  optional<string_view> length;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
  optional<string_view> defaultValue;
  vector<Metadata*> metadata;
};

struct TypeDecl : cursiv::Extends<Decl> {
  boolean isStruct;
  string_view name;
  vector<Metadata*> metadata;
  vector<Field*> fields;
};

struct EnumVal : cursiv::Extends<cursiv::AstNode> {
  vector<Metadata*> metadata;
  string_view name;
  optional<string_view> value;
};

struct EnumDecl : cursiv::Extends<Decl> {
  string_view name;
  Type* baseType;
  vector<Metadata*> metadata;
  vector<EnumVal*> values;
};

// `name` is the first identifier read; `type`, when a `:` follows, is the
// second -- which one is the alias is left to semantic analysis beyond
// this grammar, so the AST mirrors the position rather than guessing.
struct UnionVal : cursiv::Extends<cursiv::AstNode> {
  vector<Metadata*> metadata;
  string_view name;
  optional<string_view> type;
  optional<string_view> value;
};

struct UnionDecl : cursiv::Extends<Decl> {
  Type* baseType;
  string_view name;
  vector<Metadata*> metadata;
  vector<UnionVal*> values;
};

struct RpcMethod : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view argType;
  string_view returnType;
  vector<Metadata*> metadata;
};

struct RpcDecl : cursiv::Extends<Decl> {
  vector<Metadata*> metadata;
  string_view name;
  vector<RpcMethod*> methods;
};

// A schema-level default-data literal: itself a `Decl`, admitted wherever
// a declaration is, and also the shape of a field's or an array's nested
// value.
struct Value : cursiv::Extends<Decl> {};

struct KeyValue;

using Val = std::variant<std::string_view, Value*>;

struct Object : cursiv::Extends<Value> {
  vector<KeyValue*> members;
};

struct ArrayVal : cursiv::Extends<Value> {
  vector<Val> items;
};

struct KeyValue : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Val value;
};

struct Schema : cursiv::Extends<cursiv::AstNode> {
  vector<Include*> includes;
  vector<Decl*> declarations;
};

}  // namespace flatbuffers::ast
