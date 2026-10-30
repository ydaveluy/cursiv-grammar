// The clif language: its AST -- Common Logic Interchange Format (ISO/IEC
// 24707), an s-expression syntax for first-order logic texts.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace clif::ast {

// A term: a name (bare, quoted, enclosed or numeral), an application of one
// term to a sequence of terms, a sequence marker, or a commented term.
struct Term : cursiv::Extends<cursiv::AstNode> {};

struct NameTerm : cursiv::Extends<Term> {
  string_view value;
};

struct NumeralTerm : cursiv::Extends<Term> {
  string_view value;  // kept as digits: a CL numeral denotes an individual, not a number to compute with
};

struct StringTerm : cursiv::Extends<Term> {
  string_view value;
};

struct SeqMarkTerm : cursiv::Extends<Term> {
  string_view value;  // "..." plus its trailing tag, e.g. "...rest"
};

struct Application : cursiv::Extends<Term> {
  Term* op;
  vector<Term*> args;
};

struct CommentedTerm : cursiv::Extends<Term> {
  string_view comment;
  Term* body;
};

// A phrase: a sentence, a module, an import, or a commented phrase.
struct Phrase : cursiv::Extends<cursiv::AstNode> {};

// A sentence: every phrase that asserts something.
struct Sentence : cursiv::Extends<Phrase> {};

struct Equation : cursiv::Extends<Sentence> {
  Term* left;
  Term* right;
};

struct Atom : cursiv::Extends<Sentence> {
  Term* predicate;
  vector<Term*> args;
};

struct RoleBinding : cursiv::Extends<cursiv::AstNode> {
  Term* role;
  Term* filler;
};

// `(term (roleset: (name term)...))`, including an empty role set.
struct RolesetAtom : cursiv::Extends<Sentence> {
  Term* subject;
  vector<RoleBinding*> roles;
};

enum class BoolOp : std::uint8_t { And, Or, If, Iff, Not };

struct BoolSentence : cursiv::Extends<Sentence> {
  BoolOp op;
  vector<Sentence*> operands;  // and/or: any count; if/iff: two; not: one
};

// One entry of a boundlist: a bound name (or a sequence marker), with an
// optional sort term from the parenthesized `(name term)` form.
struct BoundItem : cursiv::Extends<cursiv::AstNode> {
  Term* name;
  Term* sort;
};

struct BoundList : cursiv::Extends<cursiv::AstNode> {
  vector<BoundItem*> items;
};

struct QuantSentence : cursiv::Extends<Sentence> {
  boolean isExists;  // false: forall, true: exists
  Term* boundName;   // the optional interpretablename before the boundlist
  BoundList* bounds;
  Sentence* body;
};

struct CommentedSentence : cursiv::Extends<Sentence> {
  string_view comment;
  Sentence* body;
};

struct Module;
struct NamedText;

// A run of phrases parses two ways when grouped and split both derive the
// same span: a bare phrase stands for itself here, and a multi-phrase group
// survives only where it is read exactly once, as a named text's body.
// Module already fits the Phrase* arm (a variant member matches exactly one
// alternative), so it has no arm of its own.
using CltextItem = std::variant<NamedText*, Phrase*>;

struct Text : cursiv::Extends<cursiv::AstNode> {
  vector<Phrase*> phrases;
};

struct NamedText : cursiv::Extends<cursiv::AstNode> {
  Term* name;
  Text* body;  // the optional body of a named text: `(name)` or `(name text)`
};

struct Imports : cursiv::Extends<Phrase> {
  Term* name;
};

struct CommentedPhrase : cursiv::Extends<Phrase> {
  string_view comment;
  vector<CltextItem> items;  // the complete commented text, possibly empty
};

struct Module : cursiv::Extends<Phrase> {
  Term* name;
  vector<Term*> excludes;
  vector<CltextItem> body;
};

// The root: zero or more top-level named texts and phrases.
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<CltextItem> items;
};

}  // namespace clif::ast
