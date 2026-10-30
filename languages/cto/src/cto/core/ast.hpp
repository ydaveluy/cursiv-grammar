// The cto language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cto::ast {

struct DecoratorIdentifier : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isArray;
};

// Namespace scope, so the AstNode vocabulary is out of reach here and the
// std spelling is the only one.
using Val = std::variant<std::monostate, bool, std::int64_t, double,
                         std::string_view, DecoratorIdentifier*>;

struct Decorator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Val> arguments;
};

// A field of a class body -- the six kinds the language has (string,
// boolean, numeric, date, identifier-typed, relationship) unified: `typeName`
// plus `isReference` tell them apart, `regex`/`rangeMin`/`rangeMax` apply
// only to the string/numeric kinds the grammar allows them on.
struct Property : cursiv::Extends<cursiv::AstNode> {
  vector<Decorator*> decorators;
  string_view typeName;
  boolean isReference;
  boolean isArray;
  boolean isOptional;
  string_view name;
  Val defaultValue;
  string_view regex;
  string_view regexFlags;
  optional<float64> rangeMin;
  optional<float64> rangeMax;
};

struct EnumConstant : cursiv::Extends<cursiv::AstNode> {
  vector<Decorator*> decorators;
  string_view name;
  boolean isOptional;
};

struct Declaration : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct EnumDeclaration : cursiv::Extends<Declaration> {
  vector<Decorator*> decorators;
  vector<EnumConstant*> constants;
};

// The five class kinds share decorators, abstractness, heritage and fields.
// Concepts leave identifiedBy empty; the grammar controls each kind.
struct ClassDeclaration : cursiv::Extends<Declaration> {
  vector<Decorator*> decorators;
  boolean isAbstract;
  string_view extends;
  string_view identifiedBy;
  vector<Property*> properties;
};

struct AssetDeclaration : cursiv::Extends<ClassDeclaration> {};
struct ConceptDeclaration : cursiv::Extends<ClassDeclaration> {};
struct ParticipantDeclaration : cursiv::Extends<ClassDeclaration> {};
struct TransactionDeclaration : cursiv::Extends<ClassDeclaration> {};
struct EventDeclaration : cursiv::Extends<ClassDeclaration> {};

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  boolean wildcard;
  optional<string_view> uri;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  string_view namespaceName;
  vector<Import*> imports;
  vector<Declaration*> declarations;
};

}  // namespace cto::ast
