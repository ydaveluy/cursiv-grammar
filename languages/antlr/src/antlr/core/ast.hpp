// The antlr language: its AST for one grammar file (lexer, parser or
// combined). Plain structs -- the grammar names their members, and the
// generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace antlr::ast {

enum class EbnfKind : std::uint8_t { none, question, star, plus };

// Shared by every element an alternative can hold: what comes after it
// (`?`, `*`, `+`, and their non-greedy twin) is one shape regardless of what
// the element itself is.
struct Element : cursiv::Extends<cursiv::AstNode> {
  EbnfKind suffix = EbnfKind::none;
  boolean nonGreedy;
};

// A call to a parser rule: `expr`, `expr[3]`, `id<assoc=right>`.
struct RuleRef : cursiv::Extends<Element> {
  string_view name;
  string_view args; // the `[...]` text, unquoted of its brackets; empty
  vector<string_view> options;
};

// A reference to a token by name: `ID`, `EOF`, a token declared only in
// `tokens{}` or brought in by an imported lexer -- never resolved, since a
// multi-file grammar set is outside one document.
struct TokenRef : cursiv::Extends<Element> {
  string_view name;
  vector<string_view> options;
};

// A quoted literal used as a token: `'if'`, `'a'`. Kept quoted, like every
// other raw-text member in this AST.
struct StringLit : cursiv::Extends<Element> {
  string_view text;
  vector<string_view> options;
};

// `'a'..'z'`: both endpoints quoted, as written.
struct CharRange : cursiv::Extends<Element> {
  string_view from;
  string_view to;
};

struct Wildcard : cursiv::Extends<Element> {
  vector<string_view> options;
};

// `[a-zA-Z_]`, `[\r\n]`: the bracket set of a lexer rule, brackets stripped.
struct CharSet : cursiv::Extends<Element> {
  string_view body;
};

// `~X`, `~(X | Y | Z)`: one or more of TokenRef/StringLit/CharRange/CharSet.
struct NotSet : cursiv::Extends<Element> {
  vector<Element *> items;
};

struct Option;
struct NamedAction;
struct Alt;

// A parenthesised group: `(a | b)`, also a lexer alternative block. `options`
// and `actions` hold the rare `(options {...} : a | b)` prelude.
struct Block : cursiv::Extends<Element> {
  vector<Option *> options;
  vector<NamedAction *> actions;
  vector<Alt *> alternatives;
};

// `x=atom`, `xs+=block`: a labeled element. `value` is a bare atom or block,
// its own suffix (if any) belongs to this node, not to `value`.
struct Labeled : cursiv::Extends<Element> {
  string_view name;
  boolean isList; // `+=` rather than `=`
  Element *value;
};

// An inline action or semantic predicate: `{ ... }`, `{ ... }?`. `options`
// holds the rare `<...>` after a predicate.
struct Action : cursiv::Extends<Element> {
  string_view text;
  boolean isPredicate;
  vector<string_view> options;
};

// `-> skip`, `-> channel(HIDDEN)`, `-> type(NAME), mode(X)`: one command.
// `name` is not resolved against a fixed vocabulary -- the set of lexer
// commands is target-language-defined, so this stays text.
struct LexerCommand : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view arg; // empty when the command takes none
};

// One alternative: a rule's top-level arm (`label` may be set, no
// `commands`), or a nested block's arm (`label` empty), or a lexer rule's
// arm (`commands` may be set, `label` empty).
struct Alt : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> options;
  vector<Element *> elements;
  string_view label; // the `#Name` after a top-level rule alternative
  vector<LexerCommand *> commands;
};

// `@parser::members {...}`, a rule's `@init {...}`: `scope` is only ever set
// on the grammar-level form.
struct NamedAction : cursiv::Extends<cursiv::AstNode> {
  string_view scope;
  string_view name;
  string_view text;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  string_view argType; // the `[...]` of `catch [...] {...}`
  string_view body;
};

// `name = value;` inside `options { ... }`. `value` is kept as its whole
// source span -- a dotted name, a quoted string, an action or an int -- since
// nothing here interprets an option's meaning.
struct Option : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

// `import Grammar;` or `import Label = Grammar;`. Imported grammars are
// named here; loading and resolving a multi-file set is a workspace concern.
struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view label; // empty unless the `label = Grammar` form is used
};

struct RuleDecl : cursiv::Extends<cursiv::AstNode> {};

struct ParserRule : cursiv::Extends<RuleDecl> {
  string_view name;
  string_view args;        // `ruleref`-style `[...]`, this rule's own params
  string_view returnsText; // `returns [...]`
  vector<string_view> throwsTypes;
  string_view locals; // `locals [...]`
  vector<Option *> ruleOptions;
  vector<NamedAction *> ruleActions;
  vector<Alt *> alternatives;
  vector<CatchClause *> catches;
  string_view finallyAction; // empty when there is no `finally` clause
};

struct LexerRule : cursiv::Extends<RuleDecl> {
  boolean fragment;
  string_view name;
  vector<Option *> ruleOptions;
  vector<Alt *> alternatives;
};

struct Mode : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<LexerRule *> rules;
};

// The document root: one grammar file. `lexerOnly`/`parserOnly` are both
// false for a combined grammar (`grammar X;`).
struct Grammar : cursiv::Extends<cursiv::AstNode> {
  boolean lexerOnly;
  boolean parserOnly;
  string_view name;
  vector<Option *> options;
  vector<Import *> imports;
  vector<string_view> tokens;
  vector<string_view> channels;
  vector<NamedAction *> actions;
  vector<RuleDecl *> rules;
  vector<Mode *> modes;
};

} // namespace antlr::ast
