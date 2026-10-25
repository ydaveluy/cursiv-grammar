// The lambda language: its AST. One class per term shape -- a name, an
// abstraction binding one variable, an application of two terms.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lambda::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct Variable : cursiv::Extends<Expression> {
  string_view name;
};

struct Function : cursiv::Extends<Expression> {
  string_view param;
  Expression* body;
};

struct Application : cursiv::Extends<Expression> {
  Expression* func;
  Expression* arg;
};

}  // namespace lambda::ast
