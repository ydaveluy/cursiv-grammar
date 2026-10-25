// The rexx language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rexx::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Add/Sub also stand for a prefix `+`/`-`: the enclosing node (BinExpr vs.
// PrefixExpr) already says which, so one pair of values covers both.
enum class Op : std::uint8_t {
  Or, Xor, And,
  Eq, Neq, LtGt, GtLt, Lt, Gt, LtEq, GtEq, NGt, NLt,
  SEq, SNeq, SGt, SLt, SGtEq, SLtEq, SNGt, SNLt,
  Add, Sub, Mul, Div, IntDiv, Mod, Pow,
  Not,
};

// term
struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};
struct BinaryLit : cursiv::Extends<Expr> {
  string_view text;
};
struct HexLit : cursiv::Extends<Expr> {
  string_view text;
};
struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner = nullptr;
};

// call_parms / function_parameters: a slot may be omitted between commas.
struct Arg : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
};
struct Call : cursiv::Extends<Expr> {
  string_view name;
  vector<Arg*> args;
};

// (PLUS | MINUS | NOT)* term, one node per sign, right-nested.
struct PrefixExpr : cursiv::Extends<Expr> {
  Op op{};
  Expr* operand = nullptr;
};

// power_expression .. expression: one shape for every token-driven level.
struct BinExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  Op op{};
  Expr* right = nullptr;
};

// concatenation: the operator is not a token in two of its three readings --
// explicit (`||`), blank (a gap ending in whitespace) or abuttal (neither).
struct Concat : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  bool explicitOp = false;
  bool blank = false;
  Expr* right = nullptr;
};

// A name read where drop_/upper_/procedure_'s expose list may hold a plain
// var_symbol or a vref (the name to drop held indirectly in another var).
struct VarRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool indirect = false;
};

// ARG/PULL/PARSE templates: target_ and trigger_, one class per shape.
struct TplVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct TplDrop : cursiv::Extends<cursiv::AstNode> {};
struct TplStringPattern : cursiv::Extends<cursiv::AstNode> {
  string_view value;
};
struct TplVarPattern : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct TplAbsPos : cursiv::Extends<cursiv::AstNode> {
  string_view number;
};
struct TplAbsPosVar : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct TplRelPos : cursiv::Extends<cursiv::AstNode> {
  bool minus = false;
  string_view number;
};
struct TplRelPosVar : cursiv::Extends<cursiv::AstNode> {
  bool minus = false;
  string_view name;
};
// Namespace scope, so the AstNode vocabulary is out of reach and the std
// spelling is the only one (see json's ast.hpp for the same note).
using TplItem = std::variant<TplVar*, TplDrop*, TplStringPattern*,
                              TplVarPattern*, TplAbsPos*, TplAbsPosVar*,
                              TplRelPos*, TplRelPosVar*>;
// A slot of a template_list; empty (no items) when the source is skipped
// between commas.
struct Template : cursiv::Extends<cursiv::AstNode> {
  vector<TplItem> items;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

struct Label : cursiv::Extends<Stmt> {
  string_view name;
};
struct IncludeStmt : cursiv::Extends<Stmt> {
  string_view text;
};
struct Assignment : cursiv::Extends<Stmt> {
  string_view target;
  Expr* value = nullptr;
};
struct CommandStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};

struct DoClause : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  Expr* value = nullptr;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Assignment* counter = nullptr;
  bool forever = false;
  Expr* repeatExpr = nullptr;
  vector<DoClause*> clauses;
  bool isWhile = false;
  bool isUntil = false;
  Expr* condExpr = nullptr;
  Block* body = nullptr;
  string_view endName;
};
struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond = nullptr;
  Stmt* thenBranch = nullptr;
  Stmt* elseBranch = nullptr;
};
struct WhenClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond = nullptr;
  Stmt* body = nullptr;
};
struct SelectStmt : cursiv::Extends<Stmt> {
  vector<WhenClause*> whens;
  Block* otherwiseBody = nullptr;
  bool hasOtherwise = false;
};

struct AddressStmt : cursiv::Extends<Stmt> {
  string_view envName;
  Expr* envValue = nullptr;
  Expr* command = nullptr;
};
struct ArgStmt : cursiv::Extends<Stmt> {
  vector<Template*> templates;
};
struct CallStmt : cursiv::Extends<Stmt> {
  bool onCondition = false;
  bool off = false;
  string_view condition;
  string_view handlerName;
  string_view target;
  vector<Arg*> args;
};
struct DropStmt : cursiv::Extends<Stmt> {
  vector<VarRef*> vars;
};
struct ExitStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct InterpretStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct IterateStmt : cursiv::Extends<Stmt> {
  string_view label;
};
struct LeaveStmt : cursiv::Extends<Stmt> {
  string_view label;
};
struct NopStmt : cursiv::Extends<Stmt> {};
enum class NumericKind : std::uint8_t { Digits, Form, Fuzz };
struct NumericStmt : cursiv::Extends<Stmt> {
  NumericKind kind{};
  Expr* amount = nullptr;
  bool engineering = false;
  bool scientific = false;
  Expr* formValue = nullptr;
};
struct OptionsStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
enum class ParseSource : std::uint8_t {
  Arg, External, Numeric, Pull, Source, Version, Value, Var
};
struct ParseStmt : cursiv::Extends<Stmt> {
  bool upper = false;
  ParseSource source{};
  Expr* valueExpr = nullptr;
  string_view varName;
  vector<Template*> templates;
};
struct ProcedureStmt : cursiv::Extends<Stmt> {
  bool expose = false;
  vector<VarRef*> exposed;
};
struct PullStmt : cursiv::Extends<Stmt> {
  vector<Template*> templates;
};
struct PushStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct QueueStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct SayStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};
struct SignalStmt : cursiv::Extends<Stmt> {
  bool onCondition = false;
  bool off = false;
  string_view condition;
  string_view handlerName;
  string_view targetName;
  Expr* targetValue = nullptr;
};
struct TraceStmt : cursiv::Extends<Stmt> {
  string_view constant;
  Expr* value = nullptr;
};
struct UpperStmt : cursiv::Extends<Stmt> {
  vector<VarRef*> vars;
};

}  // namespace rexx::ast
