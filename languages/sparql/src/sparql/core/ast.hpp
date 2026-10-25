// The SPARQL 1.0 language: its AST. RDF terms and expression nodes share one
// base (`Term`) so a triple's subject/predicate/object, a FILTER constraint
// and a solution-modifier ordering key all read as `Term*`.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sparql::ast {

enum class Operator : std::uint8_t {
  logical_or,
  logical_and,
  equal,
  not_equal,
  less,
  greater,
  less_equal,
  greater_equal,
  add,
  subtract,
  multiply,
  divide,
};

enum class BuiltIn : std::uint8_t {
  str,
  lang,
  lang_matches,
  datatype,
  bound,
  same_term,
  is_iri,
  is_blank,
  is_literal,
  regex,
};

struct Term : cursiv::Extends<cursiv::AstNode> {};

struct IriRef : cursiv::Extends<Term> {
  string_view value;
};

struct PrefixedTerm : cursiv::Extends<Term> {
  string_view value; // `prefix:local` as one view, split on the first ':'
};

struct BlankNode : cursiv::Extends<Term> {
  string_view label; // empty for the anonymous `[]` spelling
};

// The `a` shorthand for rdf:type. No payload: the node's type is the fact.
struct RdfType : cursiv::Extends<Term> {};

// `()`, the empty collection (rdf:nil). `collection` (`(` graphNode+ `)`)
// always holds at least one item, so an empty list needs its own class.
struct NilTerm : cursiv::Extends<Term> {};

// `?x` and `$x` are the same variable spelled two ways: the sigil itself
// carries no meaning SPARQL distinguishes on, so it is not recorded.
struct Var : cursiv::Extends<Term> {
  string_view name;
};

struct Projection : cursiv::Extends<Term> {
  Term *expression{};
  Var *variable{};
};

struct NumberLit : cursiv::Extends<Term> {
  string_view text; // integer/decimal/double, signed forms keep their sign
};

struct BoolLit : cursiv::Extends<Term> {
  boolean value;
};

struct StringLit : cursiv::Extends<Term> {
  string_view value;
  string_view lang; // set for a `@lang` tag, else empty
  Term *datatype{}; // set for a `^^iri` suffix, else null
};

struct Collection : cursiv::Extends<Term> {
  vector<Term *> items;
};

struct PredicateObject : cursiv::Extends<cursiv::AstNode> {
  Term *predicate{};
  vector<Term *> objects;
};

// `propertyListNotEmpty`'s own node: shared by a Triple (nullable -- a
// triplesNode subject may carry none) and a BlankNodePropertyList
// (mandatory), so the shape is written once.
struct PredicateObjectList : cursiv::Extends<cursiv::AstNode> {
  vector<PredicateObject *> items;
};

struct BlankNodePropertyList : cursiv::Extends<Term> {
  PredicateObjectList *properties{};
};

// `!x`, `+x`, `-x`: the operator text plus its operand.
struct UnaryExpr : cursiv::Extends<Term> {
  string_view op;
  Term *operand{};
};

struct BinaryExpr : cursiv::Extends<Term> {
  Term *left{};
  Operator op;
  Term *right{};
};

// `iri(args)`, and a plain `iri` read through `iriRefOrFunction` never
// reaches this class -- it stays the bare IriRef/PrefixedTerm.
struct FuncCall : cursiv::Extends<Term> {
  Term *target{};
  vector<Term *> args;
};

// STR, LANG, BOUND, sameTerm, isIRI/isBLANK/isLITERAL and REGEX: one class,
// the function told apart by `fn`, since each just carries 1-3 operands.
struct BuiltInCall : cursiv::Extends<Term> {
  BuiltIn fn;
  vector<Term *> args;
};

struct Triple : cursiv::Extends<cursiv::AstNode> {
  Term *subject{};
  PredicateObjectList *properties{}; // null for a bare `[ ... ] .`
};

struct Filter : cursiv::Extends<cursiv::AstNode> {
  Term *constraint{};
};

struct GroupGraphPattern;

struct OptionalGraphPattern : cursiv::Extends<cursiv::AstNode> {
  GroupGraphPattern *pattern{};
};

struct GraphGraphPattern : cursiv::Extends<cursiv::AstNode> {
  Term *graph{};
  GroupGraphPattern *pattern{};
};

struct GroupOrUnionGraphPattern : cursiv::Extends<cursiv::AstNode> {
  vector<GroupGraphPattern *> alternatives;
};

// The body of `{ ... }`: triples and the four other pattern shapes, in the
// order they were written -- a heterogeneous list, since FILTER's scope and
// a triple's position relative to OPTIONAL both read from that order.
struct GroupGraphPattern : cursiv::Extends<cursiv::AstNode> {
  vector<
      std::variant<Triple *, OptionalGraphPattern *, GroupOrUnionGraphPattern *,
                   GraphGraphPattern *, Filter *>>
      elements;
};

struct DatasetClause : cursiv::Extends<cursiv::AstNode> {
  boolean named; // FROM NAMED, rather than plain FROM
  Term *graph{};
};

struct OrderCondition : cursiv::Extends<cursiv::AstNode> {
  Term *value{};
  boolean descending; // DESC; ASC and the bare-expression form both sort up
};

struct SolutionModifier : cursiv::Extends<cursiv::AstNode> {
  vector<OrderCondition *> orderBy;
  optional<string_view> limit;
  optional<string_view> offset;
};

struct PrefixDecl : cursiv::Extends<cursiv::AstNode> {
  string_view prefix; // PNAME_NS text, ':' dropped
  string_view iri;
};

struct Prologue : cursiv::Extends<cursiv::AstNode> {
  IriRef *base{}; // null when no BASE was declared
  vector<PrefixDecl *> prefixes;
};

struct SelectQuery : cursiv::Extends<cursiv::AstNode> {
  boolean distinct;
  boolean reduced;
  boolean star; // SELECT * -- `vars` is empty
  vector<Term *> vars;
  vector<DatasetClause *> datasets;
  GroupGraphPattern *where{};
  SolutionModifier *modifier{};
};

struct ConstructQuery : cursiv::Extends<cursiv::AstNode> {
  vector<Triple *> templateTriples;
  vector<DatasetClause *> datasets;
  GroupGraphPattern *where{};
  SolutionModifier *modifier{};
};

struct DescribeQuery : cursiv::Extends<cursiv::AstNode> {
  boolean star;
  vector<Term *> vars; // DESCRIBE ?x <iri> -- varOrIRIref+, mixed
  vector<DatasetClause *> datasets;
  GroupGraphPattern *where{}; // null: whereClause is optional here
  SolutionModifier *modifier{};
};

struct AskQuery : cursiv::Extends<cursiv::AstNode> {
  vector<DatasetClause *> datasets;
  GroupGraphPattern *where{};
};

struct Query : cursiv::Extends<cursiv::AstNode> {
  Prologue *prologue{};
  std::variant<SelectQuery *, ConstructQuery *, DescribeQuery *, AskQuery *>
      form;
};

} // namespace sparql::ast
