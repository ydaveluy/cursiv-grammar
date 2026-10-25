// The rfc1960 language: its AST for an LDAP search filter (RFC 1960).
// `Filter` is the base of every filter shape; `And`/`Or` hold nested
// filters, `Not` one, and `Simple`/`Substring` compare an attribute.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rfc1960::ast {

struct Filter : cursiv::Extends<cursiv::AstNode> {};

struct And : cursiv::Extends<Filter> {
  vector<Filter *> operands;
};

struct Or : cursiv::Extends<Filter> {
  vector<Filter *> operands;
};

struct Not : cursiv::Extends<Filter> {
  Filter *operand = nullptr;
};

enum class CompareOp : std::uint8_t {
  equal,
  approx,
  greater_or_equal,
  less_or_equal,
};

struct Simple : cursiv::Extends<Filter> {
  string_view attr;
  CompareOp op;
  string_view value;
};

struct Substring : cursiv::Extends<Filter> {
  string_view attr;
  optional<string_view> initial;
  vector<string_view> middle;
  optional<string_view> final_;
};

struct Extensible : cursiv::Extends<Filter> {
  optional<string_view> attr;
  vector<string_view> qualifiers;
  string_view value;
};

} // namespace rfc1960::ast
