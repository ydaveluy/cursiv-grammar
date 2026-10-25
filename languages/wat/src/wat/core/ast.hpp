

#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wat::ast {

struct Instr;

using InstrArg = std::variant<Instr*, std::string_view>;

struct Instr : cursiv::Extends<cursiv::AstNode> {
  string_view op;
  optional<string_view> label;
  optional<string_view> endLabel;
  boolean folded;
  vector<InstrArg> args;
};

struct Binding : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  string_view valueType;
  boolean mutable_;
};

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view module_;
  string_view name;
  string_view kind;
  optional<string_view> indexType;
  optional<string_view> pageSize;
  boolean shared;
  optional<string_view> bindVar;
  optional<string_view> typeUse;
  vector<Binding*> params;
  vector<string_view> results;
  optional<string_view> min;
  optional<string_view> max;
  optional<string_view> elemType;
  boolean mutable_;
  optional<string_view> valueType;
};

struct Export : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view kind;
  string_view var_;
};

struct TypeDef : cursiv::Extends<cursiv::AstNode> {
  optional<boolean> final_;
  boolean shared;
  vector<string_view> supertypes;
  string_view kind;
  vector<Binding*> fields;
  optional<string_view> name;
  vector<Binding*> params;
  vector<string_view> results;
};

struct RecGroup : cursiv::Extends<cursiv::AstNode> {
  vector<TypeDef*> types;
};

struct Global : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  string_view valueType;
  boolean mutable_;
  Import* import_;
  vector<string_view> exports;
  vector<Instr*> init;
};

struct Table : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> indexType;
  boolean shared;
  vector<Instr*> inlineExprs;
  optional<string_view> name;
  optional<string_view> min;
  optional<string_view> max;
  string_view elemType;
  Import* import_;
  vector<string_view> exports;
  vector<string_view> inlineElems;
};

struct Memory : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> indexType;
  optional<string_view> pageSize;
  boolean shared;
  optional<string_view> name;
  optional<string_view> min;
  optional<string_view> max;
  Import* import_;
  vector<string_view> exports;
  vector<string_view> inlineData;
};

struct FunctionDecl : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  optional<string_view> typeUse;
  vector<Binding*> params;
  vector<string_view> results;
  vector<Binding*> locals;
  vector<Instr*> body;
  Import* import_;
  vector<string_view> exports;
};

struct Func : cursiv::Extends<FunctionDecl> {};
struct Tag : cursiv::Extends<FunctionDecl> {};

struct Elem : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  boolean declared;
  optional<string_view> elemType;
  vector<Instr*> expressions;
  optional<string_view> tableVar;
  vector<Instr*> offset;
  vector<string_view> funcVars;
};

struct Data : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  optional<string_view> memVar;
  vector<Instr*> offset;
  vector<string_view> strings;
};

struct Start : cursiv::Extends<cursiv::AstNode> {
  string_view func;
};

using ModuleField =
    std::variant<TypeDef*, RecGroup*, Global*, Table*, Memory*, Func*, Elem*,
                 Data*, Start*, Import*, Export*, Tag*, Instr*>;

struct Module : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  vector<ModuleField> fields;
};

struct BinQuoteModule : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> name;
  string_view kind;
  vector<string_view> strings;
};

using ScriptModule = std::variant<Module*, BinQuoteModule*>;

struct Wconst : cursiv::Extends<cursiv::AstNode> {
  string_view valueType;
  vector<string_view> literals;
};

struct Action : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  optional<string_view> moduleVar;
  string_view name;
  vector<Wconst*> args;
};

struct Assertion : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  ScriptModule module_;
  Action* action;
  vector<Wconst*> results;
  optional<string_view> message;
};

struct Register : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> var_;
};

using Cmd =
    std::variant<Action*, Assertion*, Module*, BinQuoteModule*, Register*>;

struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<Cmd> cmds;
  vector<ModuleField> fields;
};

}  // namespace wat::ast
