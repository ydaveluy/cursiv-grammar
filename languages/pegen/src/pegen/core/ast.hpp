// The pegen language: its AST. pegen is CPython's PEG metagrammar (the
// `.gram` format), not Python itself -- a Rule is one grammar rule, an Alt
// one of its ordered alternatives.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pegen::ast {

enum class Card : std::uint8_t { one, optional, star, plus };
enum class LookKind : std::uint8_t { positive, negative, cut };

// An atom: a parenthesized group, a rule/token name, or a string literal.
struct Part : cursiv::Extends<cursiv::AstNode> {};

struct NameRef : cursiv::Extends<Part> {
  string_view name;
};

struct StringLit : cursiv::Extends<Part> {
  string_view value;  // Includes quotes: single and double quotes have different meanings.
};

struct Alts;

struct Group : cursiv::Extends<Part> {
  Alts* body = nullptr;
};

// An atom with its cardinality; `sep` is set only for the `s.e+` form, where
// it holds the separator and `value` the repeated element.
struct Item : cursiv::Extends<cursiv::AstNode> {
  Part* value = nullptr;
  Part* sep = nullptr;
  Card card = Card::one;
};

struct NamedItem : cursiv::Extends<cursiv::AstNode> {};

struct Named : cursiv::Extends<NamedItem> {
  string_view attrName;
  string_view attrType;
  Item* value = nullptr;
};

struct Forced : cursiv::Extends<NamedItem> {
  Part* value = nullptr;
};

struct Lookahead : cursiv::Extends<NamedItem> {
  LookKind kind = LookKind::positive;
  Part* value = nullptr;  // null for `~`
};

struct Alt : cursiv::Extends<cursiv::AstNode> {
  vector<NamedItem*> items;
  boolean cutMark = false;
  string_view action;  // the action's text, braces off; empty when absent
};

struct Alts : cursiv::Extends<cursiv::AstNode> {
  vector<Alt*> alts;
};

struct Meta : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view valueName;
  string_view valueString;
};

struct Rule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view returnType;
  boolean memo = false;
  Alts* body = nullptr;
};

struct Grammar : cursiv::Extends<cursiv::AstNode> {
  vector<Meta*> metas;
  vector<Rule*> rules;
};

}  // namespace pegen::ast
