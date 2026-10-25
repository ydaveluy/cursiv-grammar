// The abnf language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace abnf::ast {

// One element of a rule's right-hand side: a rule-name reference, a
// parenthesised group, a bracketed option, a quoted string, a numeric
// value, or a prose description -- each its own class so a reference (a
// name to look up elsewhere) is not confused with a literal string or
// number spelled at this same syntactic position.
struct Element : cursiv::Extends<cursiv::AstNode> {};

// A rulename used as an element: resolved by name, not `reference<>` --
// rfc5322.abnf, one of the shipped examples, names rules an included
// fragment never defines.
struct RuleRef : cursiv::Extends<Element> {
  string_view name;
};

// A quoted character-value string, optionally prefixed `%s`/`%i` for
// case sensitivity (RFC 7405); kept raw, prefix and quotes included --
// unquoting and the case-sensitivity flag are for a reader downstream,
// not this tree.
struct CharVal : cursiv::Extends<Element> {
  string_view text;
};

// A numeric value, `%b`/`%d`/`%x` followed by binary/decimal/hex digits,
// kept raw.
struct NumVal : cursiv::Extends<Element> {
  string_view text;
};

// A prose description, `<...>`, kept raw.
struct ProseVal : cursiv::Extends<Element> {
  string_view text;
};

struct Alternation;

struct Group : cursiv::Extends<Element> {
  Alternation* alt;
};

struct OptionElem : cursiv::Extends<Element> {
  Alternation* alt;
};

// A repetition count: a bare INT (`exact`), or `INT? '*' INT?` (`min`/`max`,
// either side optional); `hasStar` tells the two apart when both are unset
// ("*" alone).
struct Repetition : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> exact;
  optional<string_view> min;
  optional<string_view> max;
  boolean hasStar;
  Element* element;
};

struct Concatenation : cursiv::Extends<cursiv::AstNode> {
  vector<Repetition*> repetitions;
};

struct Alternation : cursiv::Extends<cursiv::AstNode> {
  vector<Concatenation*> concatenations;
};

// A rule definition, `name = elements` or incremental `name =/ elements`
// (RFC 7405): the latter adds alternatives to a rule defined earlier, not
// a redefinition.
struct Rule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean incremental;
  Alternation* elements;
};

struct Rulelist : cursiv::Extends<cursiv::AstNode> {
  vector<Rule*> rules;
};

}  // namespace abnf::ast
