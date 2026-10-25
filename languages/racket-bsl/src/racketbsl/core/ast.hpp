#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace racketbsl::ast {
struct Discard;
struct Datum : cursiv::Extends<cursiv::AstNode> {
  vector<Discard *> discards;
};
struct Atom : cursiv::Extends<Datum> {
  string_view text;
};
struct String : cursiv::Extends<Datum> {
  string_view text;
};
struct Character : cursiv::Extends<Datum> {
  string_view text;
};
struct Boolean : cursiv::Extends<Datum> {
  string_view text;
};
struct Keyword : cursiv::Extends<Datum> {
  string_view text;
};
struct Bytes : cursiv::Extends<Datum> {
  string_view text;
};
struct Regex : cursiv::Extends<Datum> {
  string_view text;
};
struct GraphRef : cursiv::Extends<Datum> {
  string_view text;
};
struct List : cursiv::Extends<Datum> {
  string_view opener;
  vector<Datum *> items;
  Datum *tail;
  string_view closer;
};
struct Prefix : cursiv::Extends<Datum> {
  string_view marker;
  Datum *value;
};
struct Discard : cursiv::Extends<Datum> {
  Datum *value;
};
struct Directive : cursiv::Extends<Datum> {
  string_view text;
};
struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Datum *> forms;
};
} // namespace racketbsl::ast
