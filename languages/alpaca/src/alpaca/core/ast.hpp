// The alpaca language: its AST. A document declares states, classes and
// neighbourhoods; each carries membership facts and transition rules whose
// guards are boolean expressions over adjacency and state-equality tests.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace alpaca::ast {

struct StateDefn;
struct ClassDefn;
struct NbhdDefn;

// The common base of everything a rule's guard or target may name: a state
// (by name, by direction, or `me`) or a class (`is X`).
struct Referent : cursiv::Extends<cursiv::AstNode> {};

struct StateRef : cursiv::Extends<Referent> {};

struct NamedStateRef : cursiv::Extends<StateRef> {
  reference<StateDefn> state;
};

// A direction chain (`>`, `^v`, ...) naming the state of the neighbour it
// points to, rather than a state by name.
struct ArrowStateRef : cursiv::Extends<StateRef> {
  string_view chain;
};

// `me`: the cell's own current state.
struct MeStateRef : cursiv::Extends<StateRef> {};

struct ClassRef : cursiv::Extends<Referent> {
  reference<ClassDefn> cls;
};

// One direction chain of a neighbourhood: `>`, `^^`, `<v`, one arrow class
// character each.
struct Neighbourhood : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> chains;
};

// The boolean expression family: guards of a transition rule.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

enum class LogicOp : std::uint8_t { logical_and, logical_or, logical_xor };

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  LogicOp op;
  Expr* right;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

enum class BoolKind : std::uint8_t { literal_true, literal_false, guess };

struct BoolLit : cursiv::Extends<Expr> {
  BoolKind kind;
};

// `a`, `a = b`, `a is C`: an equality between two state references, or a
// class membership test. The `=` is optional and carries no meaning of its
// own -- `a b` and `a = b` are the same test -- so it is not recorded.
struct RelationalPred : cursiv::Extends<Expr> {
  StateRef* left;
  Referent* right;
};

// `N [in nbhd] target`: how many neighbours matching `target` sit in a
// neighbourhood -- named, inline, or the rule's default.
struct AdjacencyPred : cursiv::Extends<Expr> {
  uint32 count;
  Neighbourhood* inlineNbhd;    // '(' ... ')' written in place, else null
  reference<NbhdDefn> nbhdRef;  // a named neighbourhood, else unresolved
  Referent* target;
};

// `to target [when guard]`.
struct TransitionRule : cursiv::Extends<cursiv::AstNode> {
  StateRef* target;
  Expr* guard;
};

struct StateDefn : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> display;  // the quoted glyph, absent when unwritten
  vector<ClassRef*> isA;
  vector<TransitionRule*> rules;
};

struct ClassDefn : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<ClassRef*> isA;
  vector<TransitionRule*> rules;
};

struct NbhdDefn : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Neighbourhood* body;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  vector<StateDefn*> states;
  vector<ClassDefn*> classes;
  vector<NbhdDefn*> nbhds;
  boolean beginTerminated;  // the document closed on `begin`, not `.`
};

}  // namespace alpaca::ast
