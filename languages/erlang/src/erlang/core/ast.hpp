#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace erlang::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct AtomLit : cursiv::Extends<Expr> {
  string_view name;
};

struct IntegerLit : cursiv::Extends<Expr> {
  string_view value;  // raw spelling, base kept as written
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view value;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view value;  // raw spelling, escapes not decoded
};

struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> parts;  // raw spelling of each adjacent string token
};

struct SigilLit : cursiv::Extends<Expr> {
  string_view spelling;
};

struct MacroExpr : cursiv::Extends<Expr> {
  string_view name;
  boolean stringify;
};

struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
  Expr* tail;  // the `| Tail` of `[H | T]`; null for a proper list
};

struct TupleLit : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct BinElement : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  Expr* size;  // the `:Size` part; null when absent
  string_view bitTypes;  // raw /type-type-... suffix for runtime interpretation
};

struct BinaryLit : cursiv::Extends<Expr> {
  vector<BinElement*> elements;
};

struct Qualifier : cursiv::Extends<cursiv::AstNode> {};

struct LcClause : cursiv::Extends<Qualifier> {
  Expr* left;
  Expr* mapValue;
  Expr* right;
  string_view op;
};

struct ZipQualifier : cursiv::Extends<Qualifier> {
  vector<LcClause*> generators;
};

struct ListCompr : cursiv::Extends<Expr> {
  Expr* head;
  vector<Qualifier*> quals;
};

struct BinCompr : cursiv::Extends<Expr> {
  Expr* head;
  vector<Qualifier*> quals;
};

struct MapField;
struct MapCompr : cursiv::Extends<Expr> {
  MapField* head;
  vector<Qualifier*> quals;
};

struct BeginEnd : cursiv::Extends<Expr> {
  vector<Expr*> body;
};

struct GuardGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> exprs;  // AND'ed
};

// A guard's `;`-separated groups are OR'ed; each group's `,`-separated
// exprs are AND'ed.
struct Guard : cursiv::Extends<cursiv::AstNode> {
  vector<GuardGroup*> groups;
};

struct IfClause : cursiv::Extends<cursiv::AstNode> {
  Guard* guard;
  vector<Expr*> body;
};

struct IfExpr : cursiv::Extends<Expr> {
  vector<IfClause*> clauses;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Expr* pattern;
  Guard* guard;  // null when no `when`
  vector<Expr*> body;
};

struct MaybeMatch : cursiv::Extends<Expr> {
  Expr* left;
  Expr* right;
};

struct MaybeExpr : cursiv::Extends<Expr> {
  vector<Expr*> body;
  boolean hasElse;
  vector<CaseClause*> elseClauses;
};

struct CaseExpr : cursiv::Extends<Expr> {
  Expr* subject;
  vector<CaseClause*> clauses;
};

struct ReceiveExpr : cursiv::Extends<Expr> {
  vector<CaseClause*> clauses;
  boolean hasAfter;
  Expr* afterTimeout;
  vector<Expr*> afterBody;
};

struct FunClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // the clause's own leading variable; empty when anonymous
  vector<Expr*> params;
  Guard* guard;
  vector<Expr*> body;
};

// The three shapes of `fun ...`: a local name/arity, a remote one, or an
// anonymous (optionally self-named per clause) set of clauses.
struct FunNameRef : cursiv::Extends<Expr> {
  string_view name;
  string_view arity;
};

struct FunRemoteRef : cursiv::Extends<Expr> {
  string_view module;   // atom or variable, raw
  string_view function;  // atom or variable, raw
  string_view arity;     // integer or variable, raw
};

struct FunExpr : cursiv::Extends<Expr> {
  vector<FunClause*> clauses;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  string_view exClass;  // atom or variable before ':'; empty when absent
  Expr* pattern;
  string_view stackVar;  // the `:Var` stacktrace binding; empty when absent
  Guard* guard;
  vector<Expr*> body;
};

struct TryExpr : cursiv::Extends<Expr> {
  vector<Expr*> body;
  vector<CaseClause*> ofClauses;
  boolean hasOf;
  vector<CatchClause*> catchClauses;
  boolean hasAfter;
  vector<Expr*> afterBody;
};

struct CatchExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

enum class Operator : std::uint8_t {
  match,
  send,
  orelse_,
  andalso_,
  eq,
  neq,
  le,
  lt,
  ge,
  gt,
  exact_eq,
  exact_neq,
  list_append,
  list_subtract,
  add,
  sub,
  bor_,
  bxor_,
  bsl_,
  bsr_,
  or_,
  xor_,
  divide,
  multiply,
  int_div,
  rem_,
  band_,
  and_,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

// `E`, `E:E` -- a bare value or a module-qualified one (`m:f`, then called).
struct ModuleRef : cursiv::Extends<Expr> {
  Expr* module;
  Expr* member;
};

struct Call : cursiv::Extends<Expr> {
  Expr* target;
  vector<Expr*> args;
};

struct MapField : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
  boolean exact;  // `:=` rather than `=>`
};

// A map literal, or an update/access when `base` is set.
struct MapExpr : cursiv::Extends<Expr> {
  Expr* base;  // null for a fresh map
  vector<MapField*> fields;
};

struct RecordField : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // atom or variable
  Expr* value;
};

// `#name{...}`, `#name.field`, or `Base#name{...}` / `Base#name.field`.
struct RecordExpr : cursiv::Extends<Expr> {
  Expr* base;  // null for a fresh record
  string_view name;
  string_view accessField;  // the `.field` form; empty otherwise
  vector<RecordField*> fields;
};

// Type syntax keeps its leaves in order, with explicit nested type nodes.
struct TypeExpr : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<string_view, TypeExpr*>> parts;  // ordered leaves and nested types
};

struct TypedField : cursiv::Extends<cursiv::AstNode> {
  Expr* field;
  TypeExpr* type;  // null for an untyped field in a typed list
};

struct TypeSig : cursiv::Extends<cursiv::AstNode> {
  string_view module;    // the optional `m:` of `specFun`; empty otherwise
  string_view function;
  vector<TypeExpr*> signatures;
};

// The two shapes a typed attribute value takes -- `expr :: type`, or a
// value plus a typed record's field list -- folded into one node.
struct TypedRecord : cursiv::Extends<cursiv::AstNode> {
  Expr* head;
  TypeExpr* type;             // set for the `::` shape
  vector<TypedField*> fields;  // set for the record-fields shape
};

// One `-name(...)`/`-name value.` form. `spec`/`typedRecord` are mutually
// exclusive with `args` and with each other, chosen by which grammar shape
// matched.
struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> args;
  TypeSig* spec;
  TypedRecord* typedRecord;
};

struct FunctionClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> params;
  Guard* guard;
  vector<Expr*> body;
};

struct FunctionDecl : cursiv::Extends<cursiv::AstNode> {
  vector<FunctionClause*> clauses;
};

struct Forms : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Attribute*, FunctionDecl*>> forms;
};

}  // namespace erlang::ast
