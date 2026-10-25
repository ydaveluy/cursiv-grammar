// The protobuf (proto3) language: its AST. One node per grammar shape; a
// scalar field type and a named (message/enum) type share one text slot,
// since nothing short of a symbol table tells them apart.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace protobuf::ast {

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  boolean isPublic;
  boolean isWeak;
};

struct Package : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct MessageValue;
struct MessageListValue;
struct IdentRef;
struct NumberLiteral : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  boolean floating;
};
struct ExtendDef;

using ConstantValue =
    std::variant<std::monostate, NumberLiteral *, bool, std::string_view,
                 IdentRef *, MessageValue *, MessageListValue *>;

struct IdentRef : cursiv::Extends<cursiv::AstNode> {
  string_view name; // unresolved: names a const, an enum value, or an option
};

struct MessageFieldValue : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ConstantValue value;
};

struct MessageValue : cursiv::Extends<cursiv::AstNode> {
  vector<MessageFieldValue *> fields;
};

struct MessageListValue : cursiv::Extends<cursiv::AstNode> {
  vector<ConstantValue> items;
};

struct OptionName : cursiv::Extends<cursiv::AstNode> {
  string_view name; // fullIdent, or the '(' fullIdent ')' extension name
  boolean isExtension;
  optional<string_view> suffix; // trailing .fullIdent after the ')'
};

struct OptionDecl : cursiv::Extends<cursiv::AstNode> {
  OptionName *name;
  ConstantValue value;
};

struct FieldOption : cursiv::Extends<cursiv::AstNode> {
  OptionName *name;
  ConstantValue value;
};

enum class FieldLabel : std::uint8_t { optional_, repeated_ };

struct Field : cursiv::Extends<cursiv::AstNode> {
  optional<FieldLabel> label;
  string_view type; // a scalar keyword, or a (possibly dotted, possibly
                    // leading-dot) message/enum type name
  string_view name;
  string_view number;
  vector<FieldOption *> options;
};

struct OneofField : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
  string_view number;
  vector<FieldOption *> options;
};

struct Oneof : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OneofField *> fields;
  vector<OptionDecl *> options;
};

struct MapField : cursiv::Extends<cursiv::AstNode> {
  string_view keyType;
  string_view valueType;
  string_view name;
  string_view number;
  vector<FieldOption *> options;
};

struct ReservedRange : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  optional<string_view> to; // set for "N to M"
  boolean toMax;            // set for "N to max"
};

struct Reserved : cursiv::Extends<cursiv::AstNode> {
  vector<ReservedRange *> ranges; // "reserved 2, 9 to 11;"
  vector<string_view> fieldNames; // "reserved \"foo\", \"bar\";"
};

struct EnumField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
  vector<FieldOption *> options;
};

struct EnumDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<EnumField *> fields;
  vector<OptionDecl *> options;
  vector<Reserved *> reserved;
};

struct MessageDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Field *> fields;
  vector<EnumDef *> enums;
  vector<MessageDef *> messages;
  vector<ExtendDef *> extends_;
  vector<OptionDecl *> options;
  vector<Oneof *> oneofs;
  vector<MapField *> mapFields;
  vector<Reserved *> reserved;
};

struct ExtendDef : cursiv::Extends<cursiv::AstNode> {
  string_view target; // messageType, dotted, optional leading dot
  vector<Field *> fields;
};

struct Rpc : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean requestStream;
  string_view requestType;
  boolean responseStream;
  string_view responseType;
  vector<OptionDecl *> options;
};

struct ServiceDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Rpc *> rpcs;
  vector<OptionDecl *> options;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  string_view syntax;
  vector<Import *> imports;
  vector<Package *> packages; // the grammar allows more than one
  vector<OptionDecl *> options;
  vector<MessageDef *> messages;
  vector<EnumDef *> enums;
  vector<ExtendDef *> extends_;
  vector<ServiceDef *> services;
};

} // namespace protobuf::ast
