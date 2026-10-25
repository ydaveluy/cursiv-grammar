// The codeql language: its AST. Module/type/predicate declarations, formulas
// (the boolean sublanguage) and expressions (the value sublanguage) are kept
// as distinct node families; module paths, generic arguments and type
// annotations are captured as their matched text rather than decomposed.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace codeql::ast {

struct TopLevel : cursiv::Extends<cursiv::AstNode> {};
struct Formula : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Annotation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> args;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view name;
};

struct AsExpr : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
  optional<string_view> alias;
};

struct OrderByTerm : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean descending = false;
};

// ---- formulas ----

enum class LogicOp : uint8_t { and_, or_, implies };
enum class CompareOp : uint8_t { eq, ne, lt, gt, le, ge };
enum class QuantKind : uint8_t { exists, forall, forex };

struct BinFormula : cursiv::Extends<Formula> {
  Formula* left = nullptr;
  LogicOp op{};
  Formula* right = nullptr;
};

struct NegatedFormula : cursiv::Extends<Formula> {
  Formula* operand = nullptr;
};

struct IfThenFormula : cursiv::Extends<Formula> {
  Formula* cond = nullptr;
  Formula* then_ = nullptr;
  Formula* else_ = nullptr;
};

// `exists(expr)` alone sets `existsExpr`; every other quantified shape (the
// var_decls-headed exists/forall/forex) leaves it null.
struct QuantifiedFormula : cursiv::Extends<Formula> {
  QuantKind kind{};
  vector<Param*> vars;
  Expr* existsExpr = nullptr;
  optional<Formula*> restriction;
  optional<Formula*> body;
};

struct ComparisonFormula : cursiv::Extends<Formula> {
  Expr* left = nullptr;
  CompareOp op{};
  Expr* right = nullptr;
};

struct InstanceofFormula : cursiv::Extends<Formula> {
  Expr* operand = nullptr;
  string_view type;
};

struct InRangeFormula : cursiv::Extends<Formula> {
  Expr* operand = nullptr;
  Expr* range = nullptr;
};

struct CallFormula : cursiv::Extends<Formula> {
  string_view modulePath;
  string_view name;
  optional<string_view> closure;
  vector<Expr*> args;
};

struct MethodCallFormula : cursiv::Extends<Formula> {
  Expr* receiver = nullptr;
  string_view name;
  optional<string_view> closure;
  vector<Expr*> args;
};

// ---- expressions ----

enum class ArithOp : uint8_t { add, sub, mul, div, mod };
enum class SignOp : uint8_t { plus, minus };
enum class AggKind : uint8_t {
  avg,
  concat,
  count,
  max,
  min,
  rank,
  strictconcat,
  strictcount,
  strictsum,
  sum,
  unique
};
enum class VarKind : uint8_t { name, this_, result };

struct BinExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  ArithOp op{};
  Expr* right = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  SignOp op{};
  Expr* operand = nullptr;
};

struct CastExpr : cursiv::Extends<Expr> {
  string_view type;
  Expr* operand = nullptr;
};

struct DontCareExpr : cursiv::Extends<Expr> {};

struct NoneExpr : cursiv::Extends<Expr> {};

struct Literal : cursiv::Extends<Expr> {
  variant<boolean, int64, float64, string_view> value;
};

struct VariableExpr : cursiv::Extends<Expr> {
  VarKind kind{};
  string_view name;
};

struct SuperExpr : cursiv::Extends<Expr> {
  optional<string_view> type;
};

struct RangeExpr : cursiv::Extends<Expr> {
  Expr* lo = nullptr;
  Expr* hi = nullptr;
};

struct SetLiteralExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct AggOrderTerm : cursiv::Extends<cursiv::AstNode> {
  Expr* key = nullptr;
  boolean descending = false;
};

struct AggregationExpr : cursiv::Extends<Expr> {
  AggKind kind{};
  Expr* rankExpr = nullptr;
  vector<Param*> vars;
  optional<Formula*> restriction;
  vector<AsExpr*> results;
  vector<AggOrderTerm*> orderBys;
};

struct AnyExpr : cursiv::Extends<Expr> {
  vector<Param*> vars;
  optional<Formula*> restriction;
  Expr* result = nullptr;
};

struct PragmaExpr : cursiv::Extends<Expr> {
  boolean bindOut = false;
  Expr* operand = nullptr;
};

struct CallExpr : cursiv::Extends<Expr> {
  string_view modulePath;
  string_view name;
  optional<string_view> closure;
  vector<Expr*> args;
};

struct PostfixCastExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  string_view type;
};

struct MethodCallExpr : cursiv::Extends<Expr> {
  Expr* receiver = nullptr;
  string_view name;
  optional<string_view> closure;
  vector<Expr*> args;
};

// ---- top-level entities ----

struct ImportDecl : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view modulePath;
  optional<string_view> alias;
};

struct PredicateSignature : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  optional<string_view> resultType;
  string_view name;
  vector<Param*> params;
};

struct DefaultPredicate : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  optional<string_view> resultType;
  string_view name;
  vector<Param*> params;
  Formula* body = nullptr;
};

struct TypeSignature : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<string_view> extends_;
  vector<PredicateSignature*> members;
};

struct ModuleSignature : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<Param*> params;
  vector<TopLevel*> body;
};

struct AliasBody : cursiv::Extends<cursiv::AstNode> {
  string_view charPred;
  vector<string_view> charPredArgs;
  vector<Expr*> args;
};

struct Predicate : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  optional<string_view> resultType;
  string_view name;
  vector<Param*> params;
  optional<Formula*> body;
  optional<AliasBody*> aliasBody;
};

struct DtBranch : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Param*> params;
  optional<Formula*> restriction;
};

struct Datatype : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<DtBranch*> branches;
};

struct TypeUnion : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<string_view> branches;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view type;
  string_view name;
};

struct CharacteristicPredicate : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  Formula* body = nullptr;
};

struct Class_ : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<string_view> extends_;
  vector<string_view> instanceof_;
  vector<variant<CharacteristicPredicate*, Predicate*, Field*>> members;
};

struct Module : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  vector<Param*> params;
  vector<string_view> implements_;
  vector<TopLevel*> body;
};

struct AliasPredicate : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  string_view target;
};

struct AliasClass : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  string_view type;
};

struct AliasModule : cursiv::Extends<TopLevel> {
  vector<Annotation*> annotations;
  string_view name;
  string_view moduleExpr;
};

struct Select : cursiv::Extends<TopLevel> {
  vector<Param*> fromVars;
  optional<Formula*> where;
  vector<AsExpr*> results;
  vector<OrderByTerm*> orderBys;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<TopLevel*> body;
};

}  // namespace codeql::ast
