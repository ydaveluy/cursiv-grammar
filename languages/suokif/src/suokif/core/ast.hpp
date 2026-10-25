#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace suokif::ast {
struct Term : cursiv::Extends<cursiv::AstNode> {};
struct Word : cursiv::Extends<Term> { string_view name; };
struct Variable : cursiv::Extends<Term> { string_view name; };
struct Str : cursiv::Extends<Term> { string_view literal; };
struct Number : cursiv::Extends<Term> { string_view value; };
struct List : cursiv::Extends<Term> { vector<Term*> items; };
struct Document : cursiv::Extends<cursiv::AstNode> { vector<Term*> sentences; };
}  // namespace suokif::ast
