// The refal language: its AST. A program is definitions (functions and
// external declarations); a function is sentences, each a pattern, guard
// conditions, and either a result expression or a nested block.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace refal::ast {

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct ExternalDecl : cursiv::Extends<Definition> {
  string_view keyword;  // $EXTERNAL, $EXTERN or $EXTRN, as written
  vector<string_view> names;
};

// Expressions retain both structural parentheses and literal categories.
struct Term : cursiv::Extends<cursiv::AstNode> {};

struct Symbol : cursiv::Extends<Term> {
  string_view text;  // decoded compound-symbol content
};

struct NumberSymbol : cursiv::Extends<Symbol> {};
struct Characters : cursiv::Extends<Symbol> {};

struct Group : cursiv::Extends<Term> {
  vector<Term*> body;
};

enum class VarKind : std::uint8_t { s, t, e };

struct Variable : cursiv::Extends<Term> {
  VarKind kind;
  string_view index;
};

struct Call : cursiv::Extends<Term> {
  vector<Term*> body;
};

// One guard of a sentence: `, arg : pattern`.
struct Condition : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> arg;
  vector<Term*> pattern;
};

struct Sentence;

// The `, arg : { ... }` closing form: a nested block matched against arg
// instead of a result expression.
struct BlockEnding : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> arg;
  vector<Sentence*> sentences;
};

struct Sentence : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> pattern;  // the left side
  vector<Condition*> conditions;
  vector<Term*> result;  // set when the sentence closes with '='
  BlockEnding* ending;    // set when it closes with a nested block instead
};

struct FunctionDef : cursiv::Extends<Definition> {
  boolean entry;
  string_view name;
  vector<Sentence*> sentences;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Definition*> items;
};

}  // namespace refal::ast
