// The sici language: its AST. A SICI names an issue (ISSN, chronology,
// enumeration), a contribution inside it, and zero or more control segments.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sici::ast {

struct Enumeration : cursiv::Extends<cursiv::AstNode> {
  string_view volume;
  string_view issue;
  vector<string_view> additional;
  string_view supplement;
};

struct Item : cursiv::Extends<cursiv::AstNode> {
  string_view issn;
  string_view chronology;  // the parens' content, e.g. "199502/03"
  Enumeration* enumeration;
};

struct Contribution : cursiv::Extends<cursiv::AstNode> {
  string_view location;
  string_view title;
  string_view localNumber;
};

struct Control : cursiv::Extends<cursiv::AstNode> {
  string_view csi;
  string_view dpi;
  string_view mfi;
  string_view version;
  string_view check;
};

struct Sici : cursiv::Extends<cursiv::AstNode> {
  Item* item;
  Contribution* contribution;
  vector<Control*> controls;
};

}  // namespace sici::ast
