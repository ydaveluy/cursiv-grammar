// The nasm (x86-64) language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace nasm::ast {

// The expression family: unary/binary arithmetic, registers, names, segment
// qualification and the four literal kinds. Shared by operands, `equ`,
// `times` counts and data-declaration atoms -- every place NASM syntax
// takes a value.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct IntegerLit : cursiv::Extends<Expr> {
  string_view
      text; // exact spelling, including radix and arbitrarily large digits
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;  // quotes stripped; the token graph allows no escapes
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct RegisterRef : cursiv::Extends<Expr> {
  string_view name;
};

// `$` (this-line address) and `$$` (section start).
struct DollarExpr : cursiv::Extends<Expr> {
  boolean section;  // true for `$$`, false for `$`
};

// `?`, the reserve-with-no-value marker of a `db`/`dw`/... list.
struct QuestionLit : cursiv::Extends<Expr> {};

// `(register COLON)? (register | name)`: a segment override on a register
// or name target. Accepted both as an operand and inside a plain expression
// (`es:eax`, not just `es:foo`) -- a harmless broadening, real x86
// addressing allows a segment override anywhere a memory operand's target
// does.
struct QualifiedName : cursiv::Extends<Expr> {
  string_view segment;  // empty when no `reg:` prefix was written
  Expr* target;         // RegisterRef* or NameRef*
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // + - ~ !
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `cond ? then : else`. NASM restricts `then` to a bare integer; `then_`
// still types as `Expr*` since IntegerLit is one, but only an integer is
// ever parsed there.
struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* then_;
  Expr* else_;
};

struct ValueList;

// A `db`/`dw`/.../`dz` initializer value: a plain expression (or `?`), a
// nested `size` re-tag, or a list (`dup(...)`, `%(...)`, `size %(...)`).
struct Value : cursiv::Extends<cursiv::AstNode> {
  Expr* atom;      // set for the plain-value case
  string_view size;  // set for the "size value" case; `inner` then applies
  Value* inner;
  ValueList* list;  // set for the list case
};

// The three list shapes fold into one node, told apart by which fields are
// set: `dupCount` marks a `dup`; `size` alone (no `dupCount`) marks the
// `size [%] ( ... )` form; neither marks a bare `% ( ... )`.
struct ValueList : cursiv::Extends<cursiv::AstNode> {
  Expr* dupCount;
  string_view size;
  boolean percent;
  vector<Value*> items;
};

// An instruction/directive operand. `strict`/`size` are the optional
// prefix; `memory` is non-empty for a bracketed `[expr, expr...]` address,
// otherwise `value` holds the operand (register, name, literal or general
// expression) -- NASM's non-bracket operand forms are all ordinary
// expressions, so they collapse to one field here.
struct Operand : cursiv::Extends<cursiv::AstNode> {
  boolean strict;
  string_view size;
  vector<Expr*> memory;
  Expr* value;
};

// The family of one physical line's content, once past its optional label.
struct Stmt : cursiv::Extends<cursiv::AstNode> {};

// `times_prefix? (...)`: the repeat count, folded onto the statement it
// repeats rather than kept as its own wrapping node.
struct Repeatable : cursiv::Extends<Stmt> {
  Expr* times;
};

struct Instruction : cursiv::Extends<Repeatable> {
  string_view mnemonic;
  vector<Operand*> operands;
};

// `name (arg, arg...)` or `name arg, arg...` invoking a preprocessor macro.
struct MacroCall : cursiv::Extends<Repeatable> {
  string_view name;
  vector<Expr*> args;
};

// A `db`/`dw`/.../`dz` data declaration; `label` is the inline name NASM
// allows before it with no colon (`msg db "hi",0`).
struct DataDecl : cursiv::Extends<Repeatable> {
  string_view label;
  string_view kind;
  vector<Value*> values;
};

// `resb`/`resw`/.../`resz` -- reserve `count` units of `kind`.
struct ReserveDecl : cursiv::Extends<Repeatable> {
  string_view label;
  string_view kind;
  Expr* count;
};

// `incbin file [, start [, length]]` -- args hold whichever atoms were
// written (string/name/integer/float/`?`/expression, i.e. `Expr*`).
struct IncbinDecl : cursiv::Extends<Repeatable> {
  string_view label;
  vector<Expr*> args;
};

// `name equ value` -- a symbolic constant.
struct EquDecl : cursiv::Extends<Repeatable> {
  string_view label;
  Expr* value;
};

// Every directive (`section`, `extern`, `global`, `bits`, ...) in one class:
// each shape is a handful of names, flags and values, and none recurs
// often enough in real sources to earn its own node -- `kind` says which was
// written, the rest carry whichever parts of it apply.
struct Directive : cursiv::Extends<Stmt> {
  string_view kind;
  vector<string_view> names;  // every name and keyword-flag, in source order
  vector<Expr*> values;       // every integer/expression value, in source order
};

// One physical line: an optional label, then at most one statement.
struct Line : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  Stmt* stmt;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace nasm::ast
