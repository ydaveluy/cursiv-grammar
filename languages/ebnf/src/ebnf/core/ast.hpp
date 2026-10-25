// The ebnf language: its AST -- a document of BNF rule definitions, each a
// choice of alternatives built from nested brackets and literal words.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ebnf::ast {

struct Element : cursiv::Extends<cursiv::AstNode> {};

// `<name>`: a nonterminal, as a definition's left side or a reference to one.
struct Id : cursiv::Extends<Element> {
  string_view name;
};

// A bare word: a terminal symbol spelled literally.
struct Text : cursiv::Extends<Element> {
  string_view value;
};

struct Alternative : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> elements;
};

struct Optional : cursiv::Extends<Element> {
  vector<Alternative*> alternatives;
};

struct ZeroOrMore : cursiv::Extends<Element> {
  vector<Alternative*> alternatives;
};

struct OneOrMore : cursiv::Extends<Element> {
  vector<Alternative*> alternatives;
};

struct Rule : cursiv::Extends<cursiv::AstNode> {
  Id* lhs;
  vector<Alternative*> alternatives;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Rule*> rules;
};

}  // namespace ebnf::ast
