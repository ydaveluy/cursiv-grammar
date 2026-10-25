// The Z notation language (Unicode markup): its AST. Predicates and
// expressions share one hierarchy -- Z's own grammar treats a bare
// expression as a predicate (schema reference) and a relation chain as a
// predicate built from expressions, so splitting the two would duplicate
// every operator shape.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace z::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Unknown operator bindings are resolved against operator-template paragraphs at
// runtime.
struct OperatorStep : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  Expr* operand;
};
struct OperatorSequence : cursiv::Extends<Expr> {
  Expr* first;
  vector<OperatorStep*> steps;
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct OpNameRef : cursiv::Extends<Expr> {
  string_view text;
};

struct Number : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  bool value = false;
};

struct SetExtension : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct TupleExtension : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct SeqExtension : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct DeclNames : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Expr* type = nullptr;
};
struct DeclEq : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;
};
using DeclItem = std::variant<DeclNames*, DeclEq*, Expr*>;

// A schema's text: its declarations, conjoined, and the predicate they are
// constrained by. Both lists join their items with `;` or a bare newline --
// `implied<>` supplies the `;` a line break stands for.
struct SchemaText : cursiv::Extends<cursiv::AstNode> {
  vector<DeclItem> decls;
  vector<Expr*> where;
};

struct BindingExtension : cursiv::Extends<Expr> {
  vector<DeclEq*> items;
};

struct SchemaConstruction : cursiv::Extends<Expr> {
  SchemaText* text = nullptr;
};

struct SetComprehension : cursiv::Extends<Expr> {
  SchemaText* text = nullptr;
  Expr* body = nullptr;  // null for the characteristic-set form: {schemaText}
};

struct CharacteristicMu : cursiv::Extends<Expr> {
  SchemaText* text = nullptr;
};

struct GenericInstantiation : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

// A prefix operator: the core `pre`/`¬` and every prefix name the
// standard toolkit declares (`dom`, `ran`, unary `-`, `ℙ`, ...).
struct Prefix : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};

// A postfix operator: relational inverse, transitive/reflexive closure.
struct Postfix : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  string_view op;
};

// Relational image: `f(| S |)`, written `f ⦇ S ⦈`.
struct Image : cursiv::Extends<Expr> {
  Expr* fn = nullptr;
  Expr* arg = nullptr;
};

struct Hide : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<string_view> names;
};

// Juxtaposition: function application, `f x`.
struct Application : cursiv::Extends<Expr> {
  Expr* func = nullptr;
  Expr* arg = nullptr;
};

struct Decoration : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<string_view> strokes;
};

struct Selection : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  string_view field;
};

struct TupleSelection : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  string_view index;
};

struct BindingConstruction : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<string_view> strokes;
};

struct Rename : cursiv::Extends<cursiv::AstNode> {
  string_view newName;
  string_view oldName;
};

struct Renaming : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<Rename*> renames;
};

// Every binary operator: the schema calculus (`∧`, `∨`, `⇒`,
// `⇔`, `;`), schema composition/piping/projection, `×`, every
// relation the core language or the standard toolkit declares, and every
// infix name the toolkit declares. One shape for all of them -- `op` carries
// the spelling, never a node, so a converter never has to pick a member.
struct Infix : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  string_view op;
  Expr* right = nullptr;
};

// `∀`/`∃`/`∃₁`/`λ`/`μ` share a shape: a schema
// text binds names, and the body sees them. `let` binds by equations
// instead of a schema text.
struct Quantifier : cursiv::Extends<Expr> {
  string_view kind;  // the matched symbol: "∀", "∃", "λ", "μ"
  bool uniqueExists = false;
  SchemaText* text = nullptr;
  Expr* body = nullptr;
};

struct Let : cursiv::Extends<Expr> {
  vector<DeclEq*> bindings;
  Expr* body = nullptr;
};

struct IfThenElse : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* thenValue = nullptr;
  Expr* elseValue = nullptr;
};

struct BindingConstructionTheta : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  vector<string_view> strokes;
};

// A paragraph or a section: what a Z section body is a list of.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct GivenTypes : cursiv::Extends<Item> {
  vector<string_view> names;
};

struct SchemaDef : cursiv::Extends<Item> {
  bool isAxiomatic = false;  // AX (unnamed) vs SCH (named)
  string_view name;
  vector<string_view> formals;
  SchemaText* text = nullptr;
};

struct HorizontalDef : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> formals;
  Expr* value = nullptr;
};

struct FreeTypeBranch : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* payload = nullptr;
};

struct FreeType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<FreeTypeBranch*> branches;
};

struct FreeTypes : cursiv::Extends<Item> {
  vector<FreeType*> types;
};

struct Conjecture : cursiv::Extends<Item> {
  vector<string_view> formals;
  Expr* value = nullptr;
};

// An operator template declaration, available for runtime binding and precedence
// resolution of OperatorSequence nodes.
struct OperatorTemplate : cursiv::Extends<Item> {
  string_view category;  // "relation", "function", "generic"
  string_view text;      // the parenthesized name pattern, verbatim
  bool hasPrec = false;
  bool rightAssoc = false;
};

struct Section : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> parents;
  vector<Item*> body;
};

struct Specification : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace z::ast
