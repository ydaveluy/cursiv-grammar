#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace metric::ast {

enum class Prefix : std::uint8_t {
  Q,
  R,
  Y,
  Z,
  E,
  P,
  T,
  G,
  M,
  k,
  h,
  da,
  d,
  c,
  m,
  micro,
  n,
  p,
  f,
  a,
  z,
  y,
  r,
  q,
};

enum class Unit : std::uint8_t {
  m,
  g,
  s,
  A,
  K,
  mol,
  cd,
  rad,
  Hz,
  sr,
  N,
  Pa,
  J,
  W,
  C,
  V,
  F,
  ohm,
  S,
  Wb,
  T,
  H,
  degC,
  lm,
  lx,
  Bq,
  Gy,
  Sv,
  kat,
  dimensionless,
};

enum class Operator : std::uint8_t { multiply, divide };

struct Uom : cursiv::Extends<cursiv::AstNode> {};

struct Measure : cursiv::Extends<Uom> {
  optional<Prefix> prefix;
  Unit unit;
};

struct PowerUom : cursiv::Extends<Uom> {
  Uom *base;
  string_view exponent;
};

struct BinaryUom : cursiv::Extends<Uom> {
  Uom *left;
  Operator op;
  Uom *right;
};

} // namespace metric::ast
