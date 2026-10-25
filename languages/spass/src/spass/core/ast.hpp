// The spass language: its AST. A SPASS problem file is a fixed sequence of
// named list-of-... sections around a first-order term language.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace spass::ast {

enum class LogState : std::uint8_t { satisfiable, unsatisfiable, unknown };
enum class Origin : std::uint8_t { axioms, conjectures };

struct Description : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  optional<string_view> author;
  optional<string_view> version;
  optional<string_view> logic;
  LogState status = LogState::unknown;
  optional<string_view> text;
  optional<string_view> date;
};

// A first-order term: a symbol applied to arguments, or a quantifier
// binding a list of variable terms over a body.
struct Term : cursiv::Extends<cursiv::AstNode> {};

struct AppTerm : cursiv::Extends<Term> {
  string_view symbol;
  vector<Term*> args;
};

struct TermList : cursiv::Extends<cursiv::AstNode> {
  vector<Term*> items;
};

struct QuantTerm : cursiv::Extends<Term> {
  string_view quantifier;
  TermList* vars = nullptr;
  Term* body = nullptr;
};

struct SymSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<std::int64_t> arity;
};

struct SymbolList : cursiv::Extends<cursiv::AstNode> {
  vector<SymSpec*> functions;
  vector<SymSpec*> predicates;
  vector<string_view> sorts;
};

struct SubsortDecl : cursiv::Extends<cursiv::AstNode> {
  string_view first;
  string_view second;
};

// The 'forall(vars, term).' shape reaches this through `term` itself (a
// QuantTerm), so this only stores the resulting term.
struct TermDecl : cursiv::Extends<cursiv::AstNode> {
  Term* term = nullptr;
};

struct PredDecl : cursiv::Extends<cursiv::AstNode> {
  string_view pred;
  vector<string_view> sorts;
};

struct GenDecl : cursiv::Extends<cursiv::AstNode> {
  string_view sort;
  boolean freely;
  vector<string_view> functions;
};

struct DeclarationList : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<SubsortDecl*, TermDecl*, PredDecl*, GenDecl*>>
      declarations;
};

using Label = std::variant<std::string_view, std::int64_t>;

struct Formula : cursiv::Extends<cursiv::AstNode> {
  optional<Term*> term;
  optional<Label> label;
};

struct FormulaList : cursiv::Extends<cursiv::AstNode> {
  Origin origin = Origin::axioms;
  vector<Formula*> formulas;
};

struct Clause : cursiv::Extends<cursiv::AstNode> {
  boolean isDnf;
  TermList* vars = nullptr;
  vector<Term*> literals;
};

struct ClauseEntry : cursiv::Extends<cursiv::AstNode> {
  optional<Clause*> clause;
  optional<Label> label;
};

struct ClauseList : cursiv::Extends<cursiv::AstNode> {
  Origin origin = Origin::axioms;
  boolean isDnf;
  vector<ClauseEntry*> clauses;
};

using RefVal = std::variant<Term*, std::int64_t>;
using ResultVal = std::variant<Term*, Clause*>;
using RuleApplVal = std::variant<Term*, std::string_view>;
using KeyVal = std::variant<Term*, std::string_view>;
using ValueVal = std::variant<Term*, std::int64_t>;

struct ParentList : cursiv::Extends<cursiv::AstNode> {
  vector<RefVal> items;
};

struct AssocEntry : cursiv::Extends<cursiv::AstNode> {
  KeyVal key;
  ValueVal value;
};

struct AssocList : cursiv::Extends<cursiv::AstNode> {
  vector<AssocEntry*> entries;
};

struct Step : cursiv::Extends<cursiv::AstNode> {
  RefVal reference;
  ResultVal result;
  RuleApplVal ruleAppl;
  ParentList* parents = nullptr;
  AssocList* assoc = nullptr;
};

struct ProofList : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> proofType;
  AssocList* assoc = nullptr;
  vector<Step*> steps;
};

struct SettingEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Label> labels;
};

struct GeneralSettings : cursiv::Extends<cursiv::AstNode> {
  vector<SettingEntry*> entries;
};

struct NamedSettings : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  optional<string_view> text;
};

struct LogicalPart : cursiv::Extends<cursiv::AstNode> {
  optional<SymbolList*> symbols;
  optional<DeclarationList*> declarations;
  vector<FormulaList*> formulaLists;
  vector<ClauseList*> clauseLists;
  vector<ProofList*> proofLists;
};

struct Problem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Description* description = nullptr;
  LogicalPart* logicalPart = nullptr;
  vector<std::variant<GeneralSettings*, NamedSettings*>> settings;
};

}  // namespace spass::ast
