// The sapf language: its AST. sapf is concatenative (RPN): a document is a
// flat sequence of items an interpreter folds against a stack, so the tree
// mirrors that sequence rather than an operator-precedence expression tree.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sapf::ast {

// Anything that stands in a sequence of items: a line, a group, a list.
struct Item : cursiv::Extends<cursiv::AstNode> {};

// An operand: a number, a word, a group, a list, a form, a function.
struct Expression : cursiv::Extends<Item> {};

enum class OpKind : std::uint8_t { add, subtract, multiply, divide };

// A bare '+' '-' '*' '/': whether it applies as a binary fold or a leading
// sign is a runtime-stack question, not a parse-time one.
struct Operator : cursiv::Extends<Item> {
  OpKind kind;
};

enum class Suffix : std::uint8_t { none, pi, M, k, h, c, m, u };

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
  Suffix suffix = Suffix::none;
};

// The body spelling is retained, as for function documentation strings.
struct StringLiteral : cursiv::Extends<Expression> {
  string_view value;
};

enum class WordPrefix : std::uint8_t { none, backtick, quote, comma, dot };

struct Word : cursiv::Extends<Expression> {
  WordPrefix prefix = WordPrefix::none;
  string_view name;
};

struct Group : cursiv::Extends<Expression> {
  vector<Item*> items;
};

struct ListLiteral : cursiv::Extends<Expression> {
  boolean hashPrefix = false;
  vector<Item*> items;
};

struct Key : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct FormLiteral : cursiv::Extends<Expression> {
  vector<std::variant<Key*, Item*>> items;
  Word* name = nullptr;
};

struct FunctionLiteral : cursiv::Extends<Expression> {
  vector<string_view> params;
  string_view doc;  // empty when the function carries no doc string
  ListLiteral* body;
  Word* name = nullptr;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace sapf::ast
