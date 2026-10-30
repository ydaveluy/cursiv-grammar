// The infosapient language: its AST. A fuzzy business rule -- one named
// premise/consequent pair over attributes qualified by linguistic hedges.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace infosapient::ast {

// A linguistic qualifier read before an attribute's value ('very', 'about',
// 'must', ...): the word itself, not its text, so no reader downstream
// compares spellings.
enum class Hedge : std::uint8_t {
  about,
  above,
  after,
  around,
  before,
  below,
  close_to,
  definitely,
  extremely,
  generally,
  mostly,
  must,
  near,
  negative,
  negatively,
  not_,
  positive,
  positively,
  roughly,
  should,
  slightly,
  somewhat,
  very,
  in_vicinity_of,
};

// The two directional restrictions a clause may name instead of a value.
enum class Restriction : std::uint8_t {
  increased,
  decreased,
};

// The connective joining two operands of a clause -- a fuzzy operator, not
// necessarily boolean.
enum class LogicalOperator : std::uint8_t {
  and_,
  bounded_and,
  cosine_not,
  mean_and,
  mean_or,
  or_,
  product_and,
  product_or,
  sugeno_not,
  threshold_not,
  yager_and,
  yager_not,
  yager_or,
};

// The operand family: an attribute clause, or two such clauses joined by an
// operator. A grouping's own parentheses build no node of their own.
struct Operand : cursiv::Extends<cursiv::AstNode> {};

// `subject 'is'|'are' [hedges] value` -- an attribute compared, directly or
// through hedges, to a number, another attribute's name, or a restriction.
struct AttribClause : cursiv::Extends<Operand> {
  string_view subject;
  boolean plural;  // true for 'are', false for 'is' (or legacy 's')
  vector<Hedge> hedges;
  variant<float64, string_view, Restriction> value;
};

struct LogicalExpr : cursiv::Extends<Operand> {
  Operand* left;
  LogicalOperator op;
  Operand* right;
};

// THEN's clause is mandatory; ELSE's is null when absent.
struct Consequent : cursiv::Extends<cursiv::AstNode> {
  AttribClause* thenClause;
  AttribClause* elseClause;
};

struct ConditionalRule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Operand* premise;
  Consequent* consequent;
};

}  // namespace infosapient::ast
