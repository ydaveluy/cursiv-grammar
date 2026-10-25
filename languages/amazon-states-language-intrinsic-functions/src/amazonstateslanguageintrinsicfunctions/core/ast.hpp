// The Amazon States Language intrinsic-function AST: one call, its argument
// list, and one node class per argument shape.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace amazonstateslanguageintrinsicfunctions::ast {

// The function named, not the text that spells it (`States.` + name).
enum class Function : std::uint8_t {
  format,
  string_to_json,
  json_to_string,
  array,
  array_partition,
  array_contains,
  array_range,
  array_get_item,
  array_length,
  array_unique,
  base64_encode,
  base64_decode,
  hash,
  json_merge,
  math_random,
  math_add,
  string_split,
  uuid,
};

// An argument: a scalar, a $/$$ path, or a nested call.
struct Arg : cursiv::Extends<cursiv::AstNode> {};

struct StringArg : cursiv::Extends<Arg> {
  string_view value;
};

struct IntArg : cursiv::Extends<Arg> {
  string_view value;
};

struct FloatArg : cursiv::Extends<Arg> {
  string_view value;
};

struct BoolArg : cursiv::Extends<Arg> {
  boolean value;
};

// `$.a.b[0]`: a path into the state's input.
struct JsonPathArg : cursiv::Extends<Arg> {
  string_view path;
};

// `$$.a.b`: a path into the context object.
struct ContextPathArg : cursiv::Extends<Arg> {
  string_view path;
};

struct VariableArg : cursiv::Extends<Arg> {
  string_view expression;
};

struct FuncCall : cursiv::Extends<Arg> {
  Function function;
  vector<Arg *> args;
};

} // namespace amazonstateslanguageintrinsicfunctions::ast
