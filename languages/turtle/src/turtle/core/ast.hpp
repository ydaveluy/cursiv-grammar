// The turtle language: its AST. RDF terms share one base so subject,
// predicate and object positions all read as `Term*`; prefixed names keep
// their `prefix:local` spelling as one view rather than splitting it.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace turtle::ast {

struct Term : cursiv::Extends<cursiv::AstNode> {};

struct IriRef : cursiv::Extends<Term> {
  string_view value;
};

struct PrefixedTerm : cursiv::Extends<Term> {
  string_view value;  // `ex:Type`, `:x`, `ex:` -- split on the first ':'
};

struct BlankNode : cursiv::Extends<Term> {
  string_view label;  // empty for the anonymous `[]` spelling
};

struct Collection : cursiv::Extends<Term> {
  vector<Term*> items;
};

// The `a` shorthand for rdf:type. No payload: the node's type is the fact.
struct RdfType : cursiv::Extends<Term> {};

struct NumberLit : cursiv::Extends<Term> {
  string_view text;  // integer/decimal/double are shapes of one lexeme
};

struct BoolLit : cursiv::Extends<Term> {
  boolean value;
};

struct RdfLiteral : cursiv::Extends<Term> {
  string_view value;
  string_view lang;   // set for a `@lang` tag, else empty
  Term* datatype{};    // set for a `^^iri` suffix, else null
};

struct PredicateObject : cursiv::Extends<cursiv::AstNode> {
  Term* predicate{};
  vector<Term*> objects;
};

struct PredicateObjectList : cursiv::Extends<cursiv::AstNode> {
  vector<PredicateObject*> items;
};

struct BlankNodePropertyList : cursiv::Extends<Term> {
  PredicateObjectList* properties{};
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct Triples : cursiv::Extends<Statement> {
  Term* subject{};
  PredicateObjectList* properties{};  // null for a bare `[ ... ] .`
};

struct PrefixDecl : cursiv::Extends<Statement> {
  string_view prefix;  // PNAME_NS text, ':' dropped
  string_view iri;
};

struct BaseDecl : cursiv::Extends<Statement> {
  string_view iri;
};

struct SparqlPrefixDecl : cursiv::Extends<Statement> {
  string_view prefix;
  string_view iri;
};

struct SparqlBaseDecl : cursiv::Extends<Statement> {
  string_view iri;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace turtle::ast
