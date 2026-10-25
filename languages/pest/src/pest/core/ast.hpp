#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pest::ast {

enum class Modifier : std::uint8_t {
  none,
  silent,
  atomic,
  compound_atomic,
  non_atomic,
};

enum class PrefixOp : std::uint8_t { positive_predicate, negative_predicate };

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Term;

struct Sequence : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> terms;
};

struct Expression : cursiv::Extends<Item> {
  vector<Sequence*> alternatives;  // `|`-separated
};

struct RuleRef : cursiv::Extends<Item> {
  string_view name;
};

struct StringLit : cursiv::Extends<Item> {
  string_view text;  // quotes included, escapes not decoded
};

struct InsensitiveStringLit : cursiv::Extends<Item> {
  string_view text;  // quotes included
};

struct CharRange : cursiv::Extends<Item> {
  string_view start;
  string_view end;
};

struct Push : cursiv::Extends<Item> {
  Expression* expr;
};

struct PushLiteral : cursiv::Extends<Item> {
  string_view text;
};

struct PeekSlice : cursiv::Extends<Item> {
  optional<string_view> start;
  optional<string_view> end;
};

struct Optional_ : cursiv::Extends<cursiv::AstNode> {};
struct ZeroOrMore : cursiv::Extends<cursiv::AstNode> {};
struct OneOrMore : cursiv::Extends<cursiv::AstNode> {};

struct RepeatExact : cursiv::Extends<cursiv::AstNode> {
  string_view count;
};
struct RepeatMin : cursiv::Extends<cursiv::AstNode> {
  string_view min;
};
struct RepeatMax : cursiv::Extends<cursiv::AstNode> {
  string_view max;
};
struct RepeatMinMax : cursiv::Extends<cursiv::AstNode> {
  string_view min;
  string_view max;
};

using Postfix = std::variant<Optional_*, ZeroOrMore*, OneOrMore*, RepeatExact*,
                              RepeatMin*, RepeatMax*, RepeatMinMax*>;

struct Term : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> tag;
  vector<PrefixOp> prefixes;
  Item* item;
  vector<Postfix> postfixes;
};

struct RuleDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Modifier modifier;
  Expression* expression;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<RuleDecl*> rules;
};

}  // namespace pest::ast
