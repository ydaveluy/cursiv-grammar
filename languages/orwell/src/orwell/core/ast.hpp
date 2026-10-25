// The orwell language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace orwell::ast {

// The expression family: everything a `term` can be.
struct Term : cursiv::Extends<cursiv::AstNode> {};

struct IntLit : cursiv::Extends<Term> {
  int64 value;
};
struct FloatLit : cursiv::Extends<Term> {
  float64 value;
};
struct StringLit : cursiv::Extends<Term> {
  string_view value;
};
struct VarRef : cursiv::Extends<Term> {
  string_view name;
};
struct TupleTerm : cursiv::Extends<Term> {
  vector<Term*> items;
};
struct ListTerm : cursiv::Extends<Term> {
  vector<Term*> items;
};
// `[from, step .. to]`: `step` and `to` are null where the source omits them.
struct ListRange : cursiv::Extends<Term> {
  Term* from;
  Term* step;
  Term* to;
};

struct Qualifier : cursiv::Extends<cursiv::AstNode> {};
struct GuardQualifier : cursiv::Extends<Qualifier> {
  Term* condition;
};
struct Pat;
struct GeneratorQualifier : cursiv::Extends<Qualifier> {
  Pat* pat;
  Term* source;
};
struct ListComp : cursiv::Extends<Term> {
  Term* head;
  vector<Qualifier*> qualifiers;
};

// `f x y`: application by juxtaposition, no explicit call syntax.
struct Apply : cursiv::Extends<Term> {
  Term* fn;
  vector<Term*> args;
};
struct PrefixTerm : cursiv::Extends<Term> {
  string_view op;
  Term* operand;
};
struct BinaryTerm : cursiv::Extends<Term> {
  Term* left;
  string_view op;
  Term* right;
};
// An operator used as a value: `(+)`, `(1+)`, `(+1)`. Both operands null is
// the bare operator; one set picks the partial application's side.
struct Section : cursiv::Extends<Term> {
  string_view op;
  Term* leftOperand;
  Term* rightOperand;
};

// The pattern family: everything a `pat` can be.
struct Pat : cursiv::Extends<cursiv::AstNode> {};

struct PatVar : cursiv::Extends<Pat> {
  string_view name;
};
struct PatInt : cursiv::Extends<Pat> {
  int64 value;
};
struct PatString : cursiv::Extends<Pat> {
  string_view value;
};
struct PatTuple : cursiv::Extends<Pat> {
  vector<Pat*> items;
};
struct PatList : cursiv::Extends<Pat> {
  vector<Pat*> items;
};
// A constructor pattern applied to sub-patterns: `Cons x xs`.
struct PatApply : cursiv::Extends<Pat> {
  Pat* ctor;
  vector<Pat*> args;
};
struct PatPrefix : cursiv::Extends<Pat> {
  string_view op;
  Pat* operand;
};
struct PatBinary : cursiv::Extends<Pat> {
  Pat* left;
  string_view op;
  Pat* right;
};
struct PatSection : cursiv::Extends<Pat> {
  string_view op;
  Pat* leftOperand;
  Pat* rightOperand;
};

// The type family: everything a `type_` (or a `tylhs`, which shares its
// shape) can be.
struct Type : cursiv::Extends<cursiv::AstNode> {};

struct TypeName : cursiv::Extends<Type> {
  string_view name;
};
struct TypeApply : cursiv::Extends<Type> {
  Type* head;
  vector<Type*> args;
};
struct TypeTuple : cursiv::Extends<Type> {
  vector<Type*> items;
};
struct TypeList : cursiv::Extends<Type> {
  Type* element;
};
struct TypePrefix : cursiv::Extends<Type> {
  string_view op;
  Type* operand;
};
struct TypeBinary : cursiv::Extends<Type> {
  Type* left;
  string_view op;
  Type* right;
};
struct TypeSection : cursiv::Extends<Type> {
  string_view op;
  Type* leftOperand;
  Type* rightOperand;
};
// `tyname '(' (tylhs | tylhssection) ')'`, the head of a `tylhs1`.
struct TyLhsPrimary : cursiv::Extends<cursiv::AstNode> {
  string_view tyname;
  Type* inner;
};
struct TyLhs1 : cursiv::Extends<Type> {
  TyLhsPrimary* primary;
  vector<string_view> tyvars;
};

// `var_ '(' (var_ prefix | infix) ')'`: the name of an operator's type
// signature. `param` is empty for the bare-infix branch.
struct Name : cursiv::Extends<cursiv::AstNode> {
  string_view var;
  string_view param;
  string_view op;
};

// A data declaration's right-hand side: `con typrimary*`, `typrimary infix
// typrimary`, or `prefix typrimary`.
struct Construct : cursiv::Extends<cursiv::AstNode> {};
struct ConApply : cursiv::Extends<Construct> {
  string_view con;
  vector<Type*> args;
};
struct ConBinary : cursiv::Extends<Construct> {
  Type* left;
  string_view op;
  Type* right;
};
struct ConPrefix : cursiv::Extends<Construct> {
  string_view op;
  Type* operand;
};

// Top-level declarations.
struct Decl : cursiv::Extends<cursiv::AstNode> {};

// `tylhs '==' type_`: a type synonym.
struct SynDecl : cursiv::Extends<Decl> {
  Type* lhs;
  Type* rhs;
};

// `tylhs ':==' construct ('|' construct)*`: a data type.
struct ConDecl : cursiv::Extends<Decl> {
  Type* lhs;
  vector<Construct*> constructs;
};

// `name (',' name)* '::' type_`: an operator's type signature.
struct TypeDecl : cursiv::Extends<Decl> {
  vector<Name*> names;
  Type* type;
};

// `opkind OP+`: a fixity declaration. `precedence` holds the digit of the
// `assoc DIGIT` form; unset for `%prefix`/`%prefixcon`, which carry none.
struct OpDecl : cursiv::Extends<Decl> {
  string_view kind;
  optional<int8> precedence;
  vector<string_view> ops;
};

// One `result, if condition` branch of a guarded right-hand side.
struct GuardedResult : cursiv::Extends<cursiv::AstNode> {
  Term* result;
  Term* condition;
};

// `rhs: (term | conditional) wherepart?`. `simple` is set for a plain term;
// `guards`/`otherwise` for a conditional. `whereDecls` may be empty either way.
struct Rhs : cursiv::Extends<cursiv::AstNode> {
  Term* simple;
  vector<GuardedResult*> guards;
  Term* otherwise;
  vector<Decl*> whereDecls;
};

// One `pat = rhs` clause of a (possibly multi-clause) definition.
struct DefClause : cursiv::Extends<cursiv::AstNode> {
  Pat* pat;
  Rhs* rhs;
  boolean hasElse;  // preceded by `%else`; never set on the first clause
};

// `pat '=' rhs ('%else'? pat '=' rhs)*`.
struct Def : cursiv::Extends<Decl> {
  vector<DefClause*> clauses;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
};

}  // namespace orwell::ast
