// The RuleWorks language: its AST. Value is the root of every expression
// shape (literal, variable, call, arithmetic, comparison, action-as-value);
// Action is generic across its ~30 kinds -- which fields it fills depends on
// `kind`, since the source actions do not share one shape.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ruleworks::ast {

enum class ArithOp : uint8_t { add, sub, mul, div, mod };
enum class LogicalOp : uint8_t { logical_and, logical_or };

enum class ActionKind : uint8_t {
  add_state, after, at, bind, call_inherited, close_file, copy, debug,
  default_, for_each, function_call, if_, make, modify, open_file, quit,
  remove, remove_every, restore_state, return_, save_state, specialize,
  trace, while_, write,
  sql_attach, sql_commit, sql_delete, sql_detach, sql_fetch_each,
  sql_fetch_as_object, sql_insert, sql_insert_from_object, sql_rollback,
  sql_start, sql_update, sql_update_from_object,
};

struct Value : cursiv::Extends<cursiv::AstNode> {};

struct Literal : cursiv::Extends<Value> {
  variant<int64, double, string_view> value;
};

struct Variable : cursiv::Extends<Value> {
  string_view name;
};

// A built-in function, a user function call, or an rclass/index_num/db_spec
// wrapped form -- all `NAME arg*`, so one node covers them.
struct Call : cursiv::Extends<Value> {
  string_view name;
  vector<Value*> args;
};

// `left` is null for a unary minus; `op` is then `sub`.
struct ArithExpr : cursiv::Extends<Value> {
  Value* left = nullptr;
  ArithOp op{};
  Value* right = nullptr;
};

struct Compare : cursiv::Extends<Value> {
  Value* left = nullptr;
  string_view predicateText;
  Value* right = nullptr;
};

struct Logical : cursiv::Extends<Value> {
  Value* left = nullptr;
  LogicalOp op{};
  Value* right = nullptr;
};

struct Not : cursiv::Extends<Value> {
  Value* operand = nullptr;
};

// Populated fields depend on `kind`: values/terms/body/
// condition/elseBody each serve only some action kinds. `body`/`elseBody`
// hold `Value*` rather than `Action*` because a bare function call is a
// legal statement too.
struct Action : cursiv::Extends<Value> {
  ActionKind kind{};
  string_view name;
  vector<Value*> values;
  vector<struct RhsTerm*> terms;
  vector<Value*> body;
  Value* condition = nullptr;
  vector<Value*> elseBody;
};

struct RhsTerm : cursiv::Extends<cursiv::AstNode> {
  string_view attribute;
  Value* index = nullptr;
  Value* value = nullptr;
};

struct Restriction : cursiv::Extends<cursiv::AstNode> {
  string_view predicateText;
  vector<Value*> values;
};

struct LhsTerm : cursiv::Extends<cursiv::AstNode> {
  string_view attribute;
  Value* index = nullptr;
  vector<Restriction*> restrictions;
};

struct CeForm : cursiv::Extends<cursiv::AstNode> {
  string_view className;
  vector<LhsTerm*> terms;
  boolean openEnded;
};

struct Ce : cursiv::Extends<cursiv::AstNode> {
  boolean negated;
  vector<CeForm*> alternatives;
};

struct Rule : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;
  string_view name;
  vector<Ce*> conditions;
  vector<Value*> actions;
};

struct Catcher : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Value*> actions;
};

struct OnEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Value*> actions;
};
struct OnEmpty : cursiv::Extends<cursiv::AstNode> {
  vector<Value*> actions;
};
struct OnExit : cursiv::Extends<cursiv::AstNode> {
  vector<Value*> actions;
};
struct OnEvery : cursiv::Extends<cursiv::AstNode> {
  vector<Value*> actions;
};

struct DeclarationBlock : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct RuleGroupDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct EndRuleGroup : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
};

struct EndBlock : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
};

struct ParamDecl : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  Value* arrayLength = nullptr;
  boolean compound;
  string_view type;
  string_view mechanism;
};

struct EntryBlock : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<ParamDecl*> accepts;
  ParamDecl* returns = nullptr;
  vector<string_view> activates;
  vector<string_view> uses;
  string_view strategy;
};

struct RuleBlock : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> uses;
  string_view strategy;
};

struct AttributeModifier : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Value* value = nullptr;
};

struct AttributeDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean compound;
  string_view domain;
  string_view domainClass;
  vector<AttributeModifier*> modifiers;
};

struct ObjectClassDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view inheritsFrom;
  vector<AttributeDecl*> attributes;
};

struct MethodDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Value* whenVar = nullptr;
  string_view whenClassName;
  vector<ParamDecl*> params;
  ParamDecl* returns = nullptr;
  vector<Value*> actions;
};

struct GenericMethodDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Value* whenVar = nullptr;
  string_view whenClassName;
  vector<ParamDecl*> params;
  ParamDecl* returns = nullptr;
};

struct ExternalRoutineDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view alias;
  vector<ParamDecl*> accepts;
  ParamDecl* returns = nullptr;
};

using ProgramLine =
    std::variant<DeclarationBlock*, EntryBlock*, RuleBlock*, EndBlock*,
                 RuleGroupDecl*, EndRuleGroup*, ObjectClassDecl*, MethodDecl*,
                 GenericMethodDecl*, ExternalRoutineDecl*, OnEntry*, OnEmpty*,
                 OnExit*, OnEvery*, Rule*, Catcher*>;

struct ProgramConstruct : cursiv::Extends<cursiv::AstNode> {
  vector<ProgramLine> lines;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<ProgramConstruct*> constructs;
};

}  // namespace ruleworks::ast
