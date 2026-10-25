#pragma once

#include <optional>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace langium::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
using Condition = Expr;
using ValueLiteral = Expr;

struct ParameterReference : cursiv::Extends<Expr> {
  string_view parameter;
};
struct BooleanLiteral : cursiv::Extends<Expr> {
  boolean value;
};
struct Negation : cursiv::Extends<Expr> {
  Condition* value = nullptr;
};
struct Conjunction : cursiv::Extends<Expr> {
  Condition* left = nullptr;
  Condition* right = nullptr;
};
struct Disjunction : cursiv::Extends<Expr> {
  Condition* left = nullptr;
  Condition* right = nullptr;
};
struct StringLiteral : cursiv::Extends<Expr> {
  string_view value;
};
struct NumberLiteral : cursiv::Extends<Expr> {
  float64 value;
};
struct ArrayLiteral : cursiv::Extends<Expr> {
  vector<ValueLiteral*> elements;
};

struct AbstractElement : cursiv::Extends<cursiv::AstNode> {
  string_view cardinality;
  string_view predicate;
};
struct Keyword : cursiv::Extends<AbstractElement> {
  string_view value;
};
struct NamedArgument : cursiv::Extends<cursiv::AstNode> {
  string_view parameter;
  boolean calledByName;
  Condition* value = nullptr;
};
struct RuleCall : cursiv::Extends<AbstractElement> {
  string_view rule;
  vector<NamedArgument*> arguments;
};
struct CrossReference : cursiv::Extends<AbstractElement> {
  string_view type;
  boolean isMulti;
  boolean deprecatedSyntax;
  AbstractElement* terminal = nullptr;
};
struct Alternatives : cursiv::Extends<AbstractElement> {
  vector<AbstractElement*> elements;
};
struct Group : cursiv::Extends<AbstractElement> {
  Condition* guardCondition = nullptr;
  vector<AbstractElement*> elements;
};
struct UnorderedGroup : cursiv::Extends<AbstractElement> {
  vector<AbstractElement*> elements;
};
struct InferredType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct Action : cursiv::Extends<AbstractElement> {
  string_view type;
  InferredType* inferredType = nullptr;
  string_view feature;
  string_view op;
};
struct Assignment : cursiv::Extends<AbstractElement> {
  string_view feature;
  string_view op;
  AbstractElement* terminal = nullptr;
};
struct EndOfFile : cursiv::Extends<AbstractElement> {};

struct TerminalElement : cursiv::Extends<AbstractElement> {
  string_view lookahead;
  boolean parenthesized;
};
struct TerminalAlternatives : cursiv::Extends<TerminalElement> {
  vector<AbstractElement*> elements;
};
struct TerminalGroup : cursiv::Extends<TerminalElement> {
  vector<AbstractElement*> elements;
};
struct CharacterRange : cursiv::Extends<TerminalElement> {
  Keyword* left = nullptr;
  Keyword* right = nullptr;
};
struct TerminalRuleCall : cursiv::Extends<TerminalElement> {
  string_view rule;
};
struct NegatedToken : cursiv::Extends<TerminalElement> {
  AbstractElement* terminal = nullptr;
};
struct UntilToken : cursiv::Extends<TerminalElement> {
  AbstractElement* terminal = nullptr;
};
struct RegexToken : cursiv::Extends<TerminalElement> {
  string_view regex;
};
struct Wildcard : cursiv::Extends<TerminalElement> {};
struct ReturnType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct TypeDefinition : cursiv::Extends<cursiv::AstNode> {};
struct UnionType : cursiv::Extends<TypeDefinition> {
  vector<TypeDefinition*> types;
};
struct ArrayType : cursiv::Extends<TypeDefinition> {
  TypeDefinition* elementType = nullptr;
};
struct ReferenceType : cursiv::Extends<TypeDefinition> {
  TypeDefinition* referenceType = nullptr;
  boolean isMulti;
};
struct SimpleType : cursiv::Extends<TypeDefinition> {
  string_view typeRef;
  string_view primitiveType;
  std::optional<string_view> stringType;
};
struct TypeAttribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isOptional;
  TypeDefinition* type = nullptr;
  ValueLiteral* defaultValue = nullptr;
};
struct Interface : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> superTypes;
  vector<TypeAttribute*> attributes;
};
struct Type : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TypeDefinition* type = nullptr;
};
struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct AbstractRule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct AbstractParserRule : cursiv::Extends<AbstractRule> {
  vector<Parameter*> parameters;
  string_view returnType;
  string_view dataType;
  InferredType* inferredType = nullptr;
};
struct TerminalRule : cursiv::Extends<AbstractRule> {
  boolean hidden;
  boolean fragment;
  ReturnType* type = nullptr;
  TerminalElement* definition = nullptr;
};
struct ParserRule : cursiv::Extends<AbstractParserRule> {
  boolean entry;
  boolean fragment;
  boolean wildcard;
  boolean definesHiddenTokens;
  vector<string_view> hiddenTokens;
  AbstractElement* definition = nullptr;
};
struct InfixRuleOperatorList : cursiv::Extends<cursiv::AstNode> {
  string_view associativity;
  vector<Keyword*> operators;
};
struct InfixRuleOperators : cursiv::Extends<cursiv::AstNode> {
  vector<InfixRuleOperatorList*> precedences;
};
struct InfixRule : cursiv::Extends<AbstractParserRule> {
  RuleCall* call = nullptr;
  InfixRuleOperators* operators = nullptr;
};
struct GrammarImport : cursiv::Extends<cursiv::AstNode> {
  string_view path;
};
struct Grammar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isDeclared;
  vector<string_view> usedGrammars;
  boolean definesHiddenTokens;
  vector<string_view> hiddenTokens;
  vector<GrammarImport*> imports;
  vector<AbstractRule*> rules;
  vector<Interface*> interfaces;
  vector<Type*> types;
};

}  // namespace langium::ast
