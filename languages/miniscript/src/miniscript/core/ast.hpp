// The miniscript language: its AST. One base per BIP-379 fragment shape;
// operators are enums, keys and numbers stay text since a hash or a
// multi_a() count can run past any fixed-width integer.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace miniscript::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct Constant : cursiv::Extends<Expression> {
  string_view value;  // "0" or "1"
};

enum class KeyOp : std::uint8_t { pk, pkh, pk_k, pk_h };

struct KeyExpression : cursiv::Extends<Expression> {
  KeyOp op;
  string_view key;
};

enum class TimeOp : std::uint8_t { older, after };

struct TimeExpression : cursiv::Extends<Expression> {
  TimeOp op;
  string_view value;
};

enum class HashOp : std::uint8_t { sha256, hash256, hash160, ripemd160 };

struct HashExpression : cursiv::Extends<Expression> {
  HashOp op;
  string_view hash;
};

// multi() caps its key count at 20 (the legacy CHECKMULTISIG limit);
// multi_a() does not.
struct MultisigExpression : cursiv::Extends<Expression> {
  boolean isMultiA;
  string_view threshold;
  vector<string_view> keys;
};

enum class BinOp : std::uint8_t {
  and_v,
  and_b,
  and_n,
  or_b,
  or_c,
  or_d,
  or_i,
};

struct BinaryExpression : cursiv::Extends<Expression> {
  BinOp op;
  Expression* left;
  Expression* right;
};

struct TernaryExpression : cursiv::Extends<Expression> {
  Expression* a;
  Expression* b;
  Expression* c;
};

struct ThresholdExpression : cursiv::Extends<Expression> {
  string_view threshold;
  vector<Expression*> expressions;
};

// wrappers holds the letters alone, the colon dropped.
struct WrappedExpression : cursiv::Extends<Expression> {
  string_view wrappers;
  Expression* inner;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  Expression* expression;
};

}  // namespace miniscript::ast
