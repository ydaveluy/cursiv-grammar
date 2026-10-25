#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rcs::ast {

struct Symbol : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view revision;
};

struct NewPhrase : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> words;
};

struct Admin : cursiv::Extends<cursiv::AstNode> {
  string_view head;
  optional<string_view> branch;
  vector<string_view> access;
  vector<Symbol *> symbols;
  vector<Symbol *> locks;
  bool strict = false;
  optional<string_view> integrity;
  optional<string_view> comment;
  optional<string_view> expand;
  vector<NewPhrase *> newphrases;
};

struct Delta : cursiv::Extends<cursiv::AstNode> {
  string_view revision;
  string_view date;
  string_view author;
  string_view state;
  vector<string_view> branches;
  optional<string_view> next;
  optional<string_view> commitid;
  vector<NewPhrase *> newphrases;
};

struct DeltaText : cursiv::Extends<cursiv::AstNode> {
  string_view revision;
  string_view log;
  vector<NewPhrase *> newphrases;
  string_view text;
  bool hasText = false;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  Admin *admin;
  vector<Delta *> deltas;
  string_view desc;
  vector<DeltaText *> deltatexts;
};

} // namespace rcs::ast
