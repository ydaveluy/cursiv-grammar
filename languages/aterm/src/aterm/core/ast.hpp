#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace aterm::ast {
struct Term : cursiv::Extends<cursiv::AstNode> {
  vector<Term *> annotations;
  Term *typeAnnotation = nullptr;
  bool resultAnnotation = false;
};
struct Function : cursiv::Extends<Term> {
  string_view afun;
  vector<Term *> args;
};
struct Number : cursiv::Extends<Term> {
  string_view raw;
};
struct Placeholder : cursiv::Extends<Term> {
  Term *inner = nullptr;
};
struct List : cursiv::Extends<Term> {
  vector<Term *> items;
};
struct Doc : cursiv::Extends<cursiv::AstNode> {
  Term *term = nullptr;
};
} // namespace aterm::ast
