

#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pcre::ast {

enum class GroupKind : uint8_t {
  capturing,
  named_angle,
  named_quote,
  named_python,
  non_capturing,
  alt_group,
  atomic,
  lookahead,
  neg_lookahead,
  lookbehind,
  neg_lookbehind,
  non_atomic_lookahead,
  non_atomic_lookbehind,
};

enum class QuantifierKind : uint8_t { optional, star, plus, range };
enum class QuantifierMode : uint8_t { greedy, possessive, lazy };

struct Node : cursiv::Extends<cursiv::AstNode> {};

struct Sequence : cursiv::Extends<cursiv::AstNode> {
  vector<Node *> items;
};

struct Alternation : cursiv::Extends<cursiv::AstNode> {
  vector<Sequence *> branches;
};

struct Literal : cursiv::Extends<Node> {
  string_view text;
};

struct NumericEscape : cursiv::Extends<Node> {
  string_view text;
};
struct QuoteBoundary : cursiv::Extends<Node> {
  string_view text;
};

struct CharacterType : cursiv::Extends<Node> {
  string_view text;
};

struct CharacterClassRange : cursiv::Extends<Node> {
  Literal *from;
  Literal *to;
};

struct CharacterClass : cursiv::Extends<Node> {
  boolean negate;
  vector<Node *> atoms;
};

struct PosixCharacterClass : cursiv::Extends<Node> {
  boolean negate;
  string_view name;
};

struct Anchor : cursiv::Extends<Node> {
  string_view text;
};

struct MatchPointReset : cursiv::Extends<Node> {};

struct Quantified : cursiv::Extends<Node> {
  Node *operand;
  vector<Node *> annotations;
  QuantifierKind kind;
  int64 from;
  optional<int64> to;
  boolean hasComma;
  QuantifierMode mode;
};

struct Group : cursiv::Extends<Node> {
  GroupKind kind;
  boolean resetFlags;
  string_view name;
  string_view onFlags;
  string_view offFlags;
  Alternation *body;
};

struct Backreference : cursiv::Extends<Node> {
  string_view name;
  optional<int64> number;
  string_view sign;
};

struct SubroutineReference : cursiv::Extends<Node> {
  boolean wholePattern;
  string_view name;
  optional<int64> number;
  string_view sign;
};

struct ConditionalPattern : cursiv::Extends<Node> {
  string_view conditionText;
  Node *conditionNode;
  Sequence *thenBranch;
  Sequence *elseBranch;
};

struct OptionSetting : cursiv::Extends<Node> {
  string_view text;
};

struct BacktrackingControl : cursiv::Extends<Node> {
  string_view verb;
  string_view name;
  boolean hasName;
};

struct Callout : cursiv::Extends<Node> {
  optional<int64> number;
  string_view text;
  boolean stringArgument;
};

struct Comment : cursiv::Extends<Node> {
  string_view text;
};

struct Pcre : cursiv::Extends<cursiv::AstNode> {
  Alternation *pattern;
};

} // namespace pcre::ast
