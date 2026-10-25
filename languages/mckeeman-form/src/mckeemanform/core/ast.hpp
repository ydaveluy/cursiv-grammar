// The mckeemanform language: its AST. A document is named rules, each a
// list of alternatives; an item is a rule reference, a codepoint or closed
// codepoint range (`to` absent: one codepoint), or a quoted literal string.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace mckeemanform::ast {

struct Production;

struct RuleRef : cursiv::Extends<cursiv::AstNode> {
  reference<Production> target;
};

// A quoted codepoint, or a closed range of two, less what `excludes` names.
// An excluded entry never carries its own excludes.
struct Range : cursiv::Extends<cursiv::AstNode> {
  string_view from;
  optional<string_view> to;
  vector<Range*> excludes;
};

using Item = std::variant<RuleRef*, Range*, std::string_view>;

struct Alternative : cursiv::Extends<cursiv::AstNode> {
  vector<Item> items;
};

struct Production : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Alternative*> alternatives;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Production*> rules;
};

}  // namespace mckeemanform::ast
