// The provn language: its AST. PROV-N is a flat log of typed statements
// (entity, activity, wasGeneratedBy, ...) plus namespace prefixes and
// bundles; Expression and Literal are empty bases so each kind's vector
// stays homogeneous through a base pointer.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace provn::ast {

struct PrefixDecl : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  string_view iri;
};

struct Literal : cursiv::Extends<cursiv::AstNode> {};

// A quoted value tagged with its own type name after '%%'.
struct TypedLiteral : cursiv::Extends<Literal> {
  string_view text;
  string_view datatype;
};

struct StringLiteral : cursiv::Extends<Literal> {
  string_view text;
  optional<string_view> lang;
};

struct IntLiteral : cursiv::Extends<Literal> {
  string_view value;  // exact unbounded integer spelling
};

// 'prefix:local' -- the convenience form of a qualified name as a value.
struct QualifiedNameLiteral : cursiv::Extends<Literal> {
  string_view value;
};

struct AttributeValuePair : cursiv::Extends<cursiv::AstNode> {
  string_view attribute;
  Literal* value = nullptr;
};

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct EntityExpr : cursiv::Extends<Expression> {
  string_view entity;
  vector<AttributeValuePair*> attributes;
};

struct ActivityExpr : cursiv::Extends<Expression> {
  string_view activity;
  optional<string_view> startTime;
  optional<string_view> endTime;
  vector<AttributeValuePair*> attributes;
};

struct GenerationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view entity;
  optional<string_view> activity;
  optional<string_view> time;
  vector<AttributeValuePair*> attributes;
};

struct UsageExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view activity;
  optional<string_view> entity;
  optional<string_view> time;
  vector<AttributeValuePair*> attributes;
};

struct StartExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view activity;
  optional<string_view> trigger;
  optional<string_view> starter;
  optional<string_view> time;
  vector<AttributeValuePair*> attributes;
};

struct EndExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view activity;
  optional<string_view> trigger;
  optional<string_view> ender;
  optional<string_view> time;
  vector<AttributeValuePair*> attributes;
};

struct InvalidationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view entity;
  optional<string_view> activity;
  optional<string_view> time;
  vector<AttributeValuePair*> attributes;
};

struct CommunicationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view informed;
  string_view informant;
  vector<AttributeValuePair*> attributes;
};

struct AgentExpr : cursiv::Extends<Expression> {
  string_view agent;
  vector<AttributeValuePair*> attributes;
};

struct AssociationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view activity;
  optional<string_view> agent;
  optional<string_view> plan;
  vector<AttributeValuePair*> attributes;
};

struct AttributionExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view entity;
  string_view agent;
  vector<AttributeValuePair*> attributes;
};

struct DelegationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view delegate;
  string_view responsible;
  optional<string_view> activity;
  vector<AttributeValuePair*> attributes;
};

struct DerivationExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view generatedEntity;
  string_view usedEntity;
  optional<string_view> activity;
  optional<string_view> generation;
  optional<string_view> usage;
  vector<AttributeValuePair*> attributes;
};

struct InfluenceExpr : cursiv::Extends<Expression> {
  optional<string_view> id;
  string_view influencee;
  string_view influencer;
  vector<AttributeValuePair*> attributes;
};

struct AlternateExpr : cursiv::Extends<Expression> {
  string_view entity1;
  string_view entity2;
};

struct SpecializationExpr : cursiv::Extends<Expression> {
  string_view specificEntity;
  string_view generalEntity;
};

struct MembershipExpr : cursiv::Extends<Expression> {
  string_view collection;
  string_view member;
};

struct ExtArg : cursiv::Extends<cursiv::AstNode> {};

struct ExtArgIdentifier : cursiv::Extends<ExtArg> {
  string_view value;  // empty when written as the '-' marker
};

struct ExtArgLiteral : cursiv::Extends<ExtArg> {
  Literal* value = nullptr;
};

struct ExtArgTime : cursiv::Extends<ExtArg> {
  string_view value;
};

struct ExtArgExpression : cursiv::Extends<ExtArg> {
  Expression* value = nullptr;
};

// '{' args '}' or '(' args ')' -- the two spellings the grammar allows,
// told apart by `braces`.
struct ExtArgTuple : cursiv::Extends<ExtArg> {
  vector<ExtArg*> items;
  boolean braces;
};

// A relation named by a qualified name instead of a fixed keyword -- PROV-N's
// escape hatch for a domain's own relation kinds.
struct ExtensibilityExpr : cursiv::Extends<Expression> {
  string_view name;
  optional<string_view> id;
  vector<ExtArg*> args;
  vector<AttributeValuePair*> attributes;
};

struct Bundle : cursiv::Extends<cursiv::AstNode> {
  string_view identifier;
  optional<string_view> defaultNamespace;
  vector<PrefixDecl*> prefixes;
  vector<Expression*> expressions;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> defaultNamespace;
  vector<PrefixDecl*> prefixes;
  vector<Expression*> expressions;
  vector<Bundle*> bundles;
};

}  // namespace provn::ast
