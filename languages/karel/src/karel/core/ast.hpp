// The karel language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace karel::ast {

// The five fixed robot verbs; a call to a DEFINEd name is the other reading
// of `instruction` and carries no enum value.
enum class Instr : std::uint8_t {
  move,
  turn_left,
  pick_beeper,
  put_beeper,
  turn_off,
};

enum class Cond : std::uint8_t {
  front_clear,
  front_blocked,
  left_clear,
  left_blocked,
  right_clear,
  right_blocked,
  back_clear,
  back_blocked,
  next_to_beeper,
  not_next_to_beeper,
  any_beepers_in_bag,
  no_beepers_in_bag,
  facing_north,
  not_facing_north,
  facing_south,
  not_facing_south,
  facing_east,
  not_facing_east,
  facing_west,
  not_facing_west,
};

struct Block;
struct Iteration;
struct Loop;
struct Conditional;
struct Instruction;

// Namespace scope, so the AstNode vocabulary is out of reach here and the
// std spelling is the only one.
using Statement =
    std::variant<Block*, Iteration*, Loop*, Conditional*, Instruction*>;

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Statement> statements;
};

struct Iteration : cursiv::Extends<cursiv::AstNode> {
  int64 count;
  Statement body;
};

struct Loop : cursiv::Extends<cursiv::AstNode> {
  Cond condition;
  Statement body;
};

struct Conditional : cursiv::Extends<cursiv::AstNode> {
  Cond condition;
  Statement thenBranch;
  optional<Statement> elseBranch;
};

// A fixed verb, or a call: the name of some DEFINEd statement. There is no
// symbol table here, so the name is text, not a reference.
struct Instruction : cursiv::Extends<cursiv::AstNode> {
  variant<Instr, string_view> op;
};

struct Definition : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Statement body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Definition*> definitions;
  vector<Statement> statements;
};

}  // namespace karel::ast
