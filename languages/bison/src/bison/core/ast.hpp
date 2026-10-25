// The bison language: its AST. A grammar file is a prologue section, a
// grammar section (declarations and productions), and a raw epilogue.
#pragma once

#include <optional>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bison::ast {

// An embedded action.
struct Action : cursiv::Extends<cursiv::AstNode> {
  string_view tag;       // the `<type>` ahead of an action in a rhs, if any
  string_view code;      // the text between the delimiters
  string_view namedRef;  // a trailing `[name]`, if any
};

// A grammar symbol used inside a rule body, with its optional named
// reference (`exp[e1]`).
struct SymbolRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view namedRef;
};

// A directive appearing inside a rhs that is not itself a symbol or an
// action: `%prec`, `%dprec`, `%merge`, `%expect`, `%expect-rr`, `%empty`.
struct RhsMisc : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  string_view text;
};

// A GLR semantic predicate, `%?{ ... }`.
struct Predicate : cursiv::Extends<cursiv::AstNode> {
  string_view code;
};

using RhsElement = std::variant<SymbolRef*, Action*, RhsMisc*, Predicate*>;

struct Alternative : cursiv::Extends<cursiv::AstNode> {
  vector<RhsElement> elements;
};

// A production with optional named references and alternative bodies.
struct Production : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view namedRef;
  vector<Alternative*> alternatives;
};

// One symbol of a `%token`/`%nterm`/`%type`/`%left`/`%right`/`%nonassoc`/
// `%binary`/`%precedence` list.
struct TokenSymbol : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view number;   // %token's optional INT
  string_view refType;  // the Ruby extension's `id ( id )`, raw, if any
  string_view alias;    // a trailing quoted alias
};

// A `<tag>`-delimited run of a `%token`/`%nterm`/`%type`/precedence-declarator
// list: the tag governs every symbol up to the next one, or none.
struct TokenGroup : cursiv::Extends<cursiv::AstNode> {
  string_view tag;
  vector<TokenSymbol*> symbols;
};

struct SymbolDecl : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  vector<TokenGroup*> groups;
};

// `%destructor`/`%printer`: an action over a list of symbols and tags.
struct CodePropsDecl : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Action* action = nullptr;
  vector<string_view> items;
};

// Every other declaration, prologue-only or grammar-section: `%debug`,
// `%define`, `%code [ID] { ... }`, `%start`, `%union { ... }`, and so on.
// The long tail shares one shape: a keyword, its plain-text arguments, and
// the action blocks it carries.
struct Directive : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  vector<string_view> args;
  vector<Action*> actions;
};

using SectionItem = std::variant<SymbolDecl*, CodePropsDecl*, Directive*>;
using GrammarItem =
    std::variant<Production*, SymbolDecl*, CodePropsDecl*, Directive*>;

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<SectionItem> prologue;
  vector<GrammarItem> grammarSection;
  std::optional<string_view> epilogue;
};

}  // namespace bison::ast
