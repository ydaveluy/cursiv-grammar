#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace rcs::ast {

struct Symbol : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view revision;
};

struct NewPhrase : cursiv::Extends<cursiv::AstNode> {
  vector<string> words;
};

struct Admin : cursiv::Extends<cursiv::AstNode> {
  string_view head;
  optional<string_view> branch;
  vector<string_view> access;
  vector<Symbol *> symbols;
  vector<Symbol *> locks;
  bool strict = false;
  optional<string> integrity;
  optional<string> comment;
  optional<string> expand;
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
  string log;
  vector<NewPhrase *> newphrases;
  string text;
  bool hasText = false;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  Admin *admin;
  vector<Delta *> deltas;
  string desc;
  vector<DeltaText *> deltatexts;
};

} // namespace rcs::ast
