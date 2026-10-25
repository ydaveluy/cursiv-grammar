// The tinyosnesc language: its AST. nesC's bodies (event/task/command/plain
// statements) carry no real C expression grammar here, so most statement
// forms keep their signature/argument text as spans and only the
// nesC-specific shell (uses/provides, wiring, components) is structured.
#pragma once

#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tinyosnesc::ast {

struct Stat : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// The shared base of a top-level component: a module, a configuration or an
// interface declaration. A plain base (not a variant): the set of component
// kinds is not closed and statically-distinguishable otherwise.
struct Component : cursiv::Extends<cursiv::AstNode> {};

struct Include : cursiv::Extends<cursiv::AstNode> {
  string_view path;
};

// One `uses`/`provides` interface entry: `Timer<TMilli> as Timer0`,
// `IP as ICMP_RA[uint8_t code]`. `text` keeps the base name and any generic
// or parameter suffix together, as written.
struct InterfaceRef : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  string_view alias;  // after `as`, empty when absent
};

struct UsesProvides : cursiv::Extends<cursiv::AstNode> {
  boolean isProvides;
  vector<InterfaceRef*> interfaces;
};

// A `command`/`event` declaration inside a top-level `interface` block.
struct InterfaceMember : cursiv::Extends<cursiv::AstNode> {
  boolean isEvent;  // false: command
  string_view signature;
};

struct InterfaceDecl : cursiv::Extends<Component> {
  string_view name;
  vector<InterfaceMember*> members;
};

// `event`/`task`/`static`/plain: `KIND? signature(args) { body }?`. `kind`
// is empty for the keyword-less form (a plain function definition,
// declaration or call statement); `signature` carries the return type, name
// and parenthesized argument list together, as written.
struct FuncStat : cursiv::Extends<Stat> {
  string_view kind;
  string_view signature;
  vector<Stat*> body;
};

struct IfBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;  // null on the trailing `else` branch
  vector<Stat*> body;
};

struct IfStat : cursiv::Extends<Stat> {
  vector<IfBranch*> branches;
};

struct WhileStat : cursiv::Extends<Stat> {
  Expr* cond;
  vector<Stat*> body;
};

// The header is one raw span of the `for (...)` clauses, not split into
// init/cond/update.
struct ForStat : cursiv::Extends<Stat> {
  string_view header;
  vector<Stat*> body;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  boolean isDefault;
  string_view label;
  vector<Stat*> body;
};

struct SwitchStat : cursiv::Extends<Stat> {
  Expr* cond;
  vector<CaseClause*> cases;
};

struct AtomicStat : cursiv::Extends<Stat> {
  vector<Stat*> body;
};

struct EnumStat : cursiv::Extends<Stat> {
  vector<Expr*> items;
};

// `call`/`signal`: `call Timer0.startPeriodic(250);`, `signal RadioSend.ready();`.
struct CallStat : cursiv::Extends<Stat> {
  boolean isSignal;
  string_view target;
};

// `#define NAME value`, bounded to one line.
struct DefineStat : cursiv::Extends<Stat> {
  string_view name;
  string_view value;
};

// A packet/struct typedef (`typedef nx_struct foo { ... } foo_t;`-like):
// two header spans, a body of statements, and one mandatory trailing
// statement.
struct PacketDefine : cursiv::Extends<Stat> {
  string_view header;
  vector<Stat*> body;
  Stat* trailing;
};

struct UnknownCharStat : cursiv::Extends<Stat> {
  string_view text;
};

// A primary operand: one or more atoms (literals, names), or a
// brace-delimited array/matrix initializer -- kept as raw text, below the
// operator ladder (file header).
struct AtomExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct Module : cursiv::Extends<Component> {
  boolean hasParameters;
  string_view parameters;
  boolean isGeneric;
  string_view name;
  string_view attribute;  // raw `@attr(...)` text, empty when absent
  vector<UsesProvides*> signature;
  vector<Stat*> body;
};

struct ConfigItem : cursiv::Extends<cursiv::AstNode> {};

// `components A, B, new C() as D;` -- one line, kept as text per item: the
// reference itself does not structure the instantiation arguments.
struct ComponentsStat : cursiv::Extends<ConfigItem> {
  vector<string_view> names;
};

struct WiringStat : cursiv::Extends<ConfigItem> {
  string_view left;
  string_view arrow;  // "->" or "<-"
  string_view right;
};

struct Configuration : cursiv::Extends<Component> {
  boolean hasParameters;
  string_view parameters;
  boolean isGeneric;
  string_view name;
  string_view attribute;
  vector<UsesProvides*> signature;
  vector<ConfigItem*> items;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Include*> includes;
  vector<Component*> components;
};

}  // namespace tinyosnesc::ast
