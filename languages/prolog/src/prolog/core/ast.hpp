#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace prolog::ast {
struct Term : cursiv::Extends<cursiv::AstNode> {};
struct VariableTerm : cursiv::Extends<Term> {
  string_view name;
};
struct NilTerm : cursiv::Extends<Term> {};
struct AtomTerm : cursiv::Extends<Term> {
  string_view value;
};
struct StringTerm : cursiv::Extends<Term> {
  string_view value;
};
struct CodeListTerm : cursiv::Extends<Term> {
  string_view value;
};
struct IntegerTerm : cursiv::Extends<Term> {
  string_view value;
};
struct FloatTerm : cursiv::Extends<Term> {
  float64 value;
};
struct RationalTerm : cursiv::Extends<Term> {
  string_view text;
};
struct BracedTerm : cursiv::Extends<Term> {
  Term *inner;
};
struct CompoundTerm : cursiv::Extends<Term> {
  boolean nilFunctor;
  string_view functor;
  vector<Term *> args;
};
struct BinaryTerm : cursiv::Extends<Term> {
  Term *left;
  string_view op;
  Term *right;
};
struct UnaryTerm : cursiv::Extends<Term> {
  string_view op;
  Term *operand;
};
struct ListTerm : cursiv::Extends<Term> {
  vector<Term *> items;
  Term *tail;
};
struct CurlyTerm : cursiv::Extends<Term> {
  Term *inner;
};
struct DictEntry : cursiv::Extends<cursiv::AstNode> {
  Term *key;
  Term *value;
};
struct DictTerm : cursiv::Extends<Term> {
  Term *tag;
  vector<DictEntry *> entries;
};
struct QuasiTerm : cursiv::Extends<Term> {
  Term *syntax;
  string_view content;
};
struct Item : cursiv::Extends<cursiv::AstNode> {
  Term *term;
};
struct PText : cursiv::Extends<cursiv::AstNode> {
  vector<Item *> items;
};
} // namespace prolog::ast
