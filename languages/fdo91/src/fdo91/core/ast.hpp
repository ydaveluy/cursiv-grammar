// The fdo91 language: its AST. An atom is a command name and its ordered
// arguments -- a scalar, or a group of nested atoms sharing one argument
// slot (`atom+` with no separator, inside `<...>`).
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fdo91::ast {

struct Atom;
struct AtomGroup;
struct Piped;

// A source name; atom-reference binding is deferred.
struct Ident : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// A global id, kept as written.
struct Gid : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Number : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
struct Hex : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

// Source arguments.
using ArgValue =
    std::variant<Ident*, Number*, std::string_view, Gid*, AtomGroup*, Hex*, Piped*>;

struct AtomGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Atom*> atoms;
};
struct Piped : cursiv::Extends<cursiv::AstNode> {
  vector<ArgValue> parts;
};

struct Atom : cursiv::Extends<cursiv::AstNode> {
  boolean bracketed;
  boolean atomPrefix;  // written with a leading `atom$`
  string_view command;
  vector<ArgValue> args;  // literal_args (bare) or arglist (`<...>`)
};

struct Doc : cursiv::Extends<cursiv::AstNode> {
  vector<Atom*> atoms;
};

}  // namespace fdo91::ast
