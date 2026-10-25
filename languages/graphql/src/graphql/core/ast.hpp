// The graphql language: its AST. Covers both GraphQL sublanguages -- the
// executable query language (operations, fragments) and the type-system
// definition language (schema, types, directives, extensions).
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace graphql::ast {

struct ListValue;
struct ObjectValue;
struct Variable;
struct EnumLit;
struct IntLit;
struct FloatLit;

// GraphQL's syntax preserves numeric spelling and has no machine-number bound.
// Schema coercion, including the GraphQL Int range, belongs to later validation.
using Val =
    std::variant<std::monostate, bool, IntLit *, FloatLit *, std::string_view,
                 Variable *, EnumLit *, ListValue *, ObjectValue *>;

struct IntLit : cursiv::Extends<cursiv::AstNode> {
  string_view value;
};
struct FloatLit : cursiv::Extends<cursiv::AstNode> {
  string_view value;
};

struct Variable : cursiv::Extends<cursiv::AstNode> {
  string_view name; // without the leading $
};

struct EnumLit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct ObjectField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Val value;
};

struct ListValue : cursiv::Extends<cursiv::AstNode> {
  vector<Val> items;
};

struct ObjectValue : cursiv::Extends<cursiv::AstNode> {
  vector<ObjectField *> fields;
};

struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Val value;
};

struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Argument *> arguments;
};

// A type reference: a name, or a list of one, each optionally non-null.
struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

struct NamedTypeRef : cursiv::Extends<TypeRef> {
  string_view name;
  boolean nonNull;
};

struct ListTypeRef : cursiv::Extends<TypeRef> {
  TypeRef *itemType;
  boolean nonNull;
};

struct VariableDefinition : cursiv::Extends<cursiv::AstNode> {
  vector<struct Directive *> directives;
  string_view name;
  TypeRef *type;
  optional<Val> defaultValue;
};

struct Selection : cursiv::Extends<cursiv::AstNode> {};

struct SelectionSet : cursiv::Extends<cursiv::AstNode> {
  vector<Selection *> selections;
};

struct Field : cursiv::Extends<Selection> {
  string_view alias; // empty when absent
  string_view name;
  vector<Argument *> arguments;
  vector<Directive *> directives;
  SelectionSet *selectionSet; // null when absent
};

struct FragmentSpread : cursiv::Extends<Selection> {
  string_view name;
  vector<Directive *> directives;
};

struct InlineFragment : cursiv::Extends<Selection> {
  string_view typeCondition; // empty when absent
  vector<Directive *> directives;
  SelectionSet *selectionSet;
};

enum class OperationKind : std::uint8_t { Query, Mutation, Subscription };

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct OperationDefinition : cursiv::Extends<Definition> {
  OperationKind operation =
      OperationKind::Query; // shorthand form: bare selectionSet
  string_view name;
  vector<VariableDefinition *> variableDefinitions;
  vector<Directive *> directives;
  SelectionSet *selectionSet;
};

struct FragmentDefinition : cursiv::Extends<Definition> {
  string_view name;
  string_view typeCondition;
  vector<Directive *> directives;
  SelectionSet *selectionSet;
};

struct RootOperationTypeDefinition : cursiv::Extends<cursiv::AstNode> {
  OperationKind operation;
  string_view type;
};

struct SchemaDefinition : cursiv::Extends<Definition> {
  string_view description;
  vector<Directive *> directives;
  vector<RootOperationTypeDefinition *> operationTypes;
};

struct SchemaExtension : cursiv::Extends<Definition> {
  vector<Directive *> directives;
  vector<RootOperationTypeDefinition *> operationTypes;
};

struct InputValueDefinition : cursiv::Extends<cursiv::AstNode> {
  string_view description;
  string_view name;
  TypeRef *type;
  optional<Val> defaultValue;
  vector<Directive *> directives;
};

struct FieldDefinition : cursiv::Extends<cursiv::AstNode> {
  string_view description;
  string_view name;
  vector<InputValueDefinition *> arguments;
  TypeRef *type;
  vector<Directive *> directives;
};

// Shared shape of scalar/object/interface/union/enum/input-object
// definitions: a description, a name, directives.
struct TypeDefinition : cursiv::Extends<Definition> {
  string_view description;
  string_view name;
  vector<Directive *> directives;
};

struct ScalarTypeDefinition : cursiv::Extends<TypeDefinition> {};

struct ObjectTypeDefinition : cursiv::Extends<TypeDefinition> {
  vector<string_view> interfaces;
  vector<FieldDefinition *> fields;
};

struct InterfaceTypeDefinition : cursiv::Extends<TypeDefinition> {
  vector<string_view> interfaces;
  vector<FieldDefinition *> fields;
};

struct UnionTypeDefinition : cursiv::Extends<TypeDefinition> {
  vector<string_view> members;
};

struct EnumValueDefinition : cursiv::Extends<cursiv::AstNode> {
  string_view description;
  string_view name;
  vector<Directive *> directives;
};

struct EnumTypeDefinition : cursiv::Extends<TypeDefinition> {
  vector<EnumValueDefinition *> values;
};

struct InputObjectTypeDefinition : cursiv::Extends<TypeDefinition> {
  vector<InputValueDefinition *> fields;
};

// Shared shape of the six extension kinds: no description (the spec gives
// extensions none), a name, directives.
struct TypeExtension : cursiv::Extends<Definition> {
  string_view name;
  vector<Directive *> directives;
};

struct ScalarTypeExtension : cursiv::Extends<TypeExtension> {};

struct ObjectTypeExtension : cursiv::Extends<TypeExtension> {
  vector<string_view> interfaces;
  vector<FieldDefinition *> fields;
};

struct InterfaceTypeExtension : cursiv::Extends<TypeExtension> {
  vector<string_view> interfaces;
  vector<FieldDefinition *> fields;
};

struct UnionTypeExtension : cursiv::Extends<TypeExtension> {
  vector<string_view> members;
};

struct EnumTypeExtension : cursiv::Extends<TypeExtension> {
  vector<EnumValueDefinition *> values;
};

struct InputObjectTypeExtension : cursiv::Extends<TypeExtension> {
  vector<InputValueDefinition *> fields;
};

enum class DirectiveLocationKind : std::uint8_t {
  Query,
  Mutation,
  Subscription,
  Field,
  FragmentDefinition,
  FragmentSpread,
  InlineFragment,
  VariableDefinition,
  Schema,
  Scalar,
  Object,
  FieldDefinition,
  ArgumentDefinition,
  Interface,
  Union,
  Enum,
  EnumValue,
  InputObject,
  InputFieldDefinition,
};

struct DirectiveDefinition : cursiv::Extends<Definition> {
  string_view description;
  string_view name;
  vector<InputValueDefinition *> arguments;
  boolean repeatable;
  vector<DirectiveLocationKind> locations;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Definition *> definitions;
};

} // namespace graphql::ast
