// The dcm language: its AST. A DCM file is a header of optional metadata
// blocks (module info, function table, variant criteria) followed by a body
// of characteristic values -- scalars, blocks, curves, maps and
// support-point distributions, each carrying the same optional metadata
// block.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace dcm::ast {

struct VarAbh : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct KgrInfo : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> langname;
  optional<string_view> displayName;
  vector<VarAbh*> dependencies;
  vector<string_view> functions;
};

struct ModHeaderLine : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> values;
};

struct FunctionEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view version;
  string_view longName;
};

struct VariantCriterion : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> values;
};

// A row of a WERT list: one or more numbers on one line.
struct ValueRow : cursiv::Extends<cursiv::AstNode> {
  vector<double> values;
};

// One entry of a FESTWERTEBLOCK: either a WERT line of numbers or a TEXT
// line of strings, never both.
struct ValueBlock : cursiv::Extends<cursiv::AstNode> {
  vector<double> numbers;
  vector<string_view> texts;
};

// A support-point list along one axis (ST/X or ST_TX/X): numbers or text
// labels, never both.
struct SupportPoints : cursiv::Extends<cursiv::AstNode> {
  vector<double> numbers;
  vector<string_view> texts;
};

// One row of a 2D map: a Y support point (numeric or text) plus its WERT
// rows.
struct MapRow : cursiv::Extends<cursiv::AstNode> {
  optional<double> y;
  optional<string_view> yText;
  vector<ValueRow*> rows;
};

enum class CurveKind { Normal, Fixed, Group };

// Common base of the six kenngroesse forms, so a document holds them in
// source order without a variant.
struct Kenngroesse : cursiv::Extends<cursiv::AstNode> {};

struct Kennwert : cursiv::Extends<Kenngroesse> {
  string_view name;
  KgrInfo* info;
  optional<string_view> unit;
  optional<double> value;
  optional<string_view> text;
};

struct Kennwerteblock : cursiv::Extends<Kenngroesse> {
  string_view name;
  std::int64_t count;
  optional<std::int64_t> countY;  // present for a native matrix: x @ y
  KgrInfo* info;
  optional<string_view> unit;
  vector<ValueBlock*> blocks;
};

struct Kennlinie : cursiv::Extends<Kenngroesse> {
  CurveKind kind;
  string_view name;
  std::int64_t count;
  KgrInfo* info;
  optional<string_view> unitX;
  optional<string_view> unitW;
  vector<SupportPoints*> supportX;
  vector<ValueRow*> values;
};

struct Kennfeld : cursiv::Extends<Kenngroesse> {
  CurveKind kind;
  string_view name;
  std::int64_t countX;
  std::int64_t countY;
  KgrInfo* info;
  optional<string_view> unitX;
  optional<string_view> unitY;
  optional<string_view> unitW;
  vector<SupportPoints*> supportX;
  vector<MapRow*> rows;
};

struct Gruppenstuetzstellen : cursiv::Extends<Kenngroesse> {
  string_view name;
  std::int64_t count;
  KgrInfo* info;
  optional<string_view> unitX;
  vector<SupportPoints*> supportX;
};

struct Kenntext : cursiv::Extends<Kenngroesse> {
  string_view name;
  KgrInfo* info;
  string_view text;
};

// Absence of a header block (module info, functions, variants) is an empty
// list, not a separate flag: each block requires at least one entry once
// its keyword is read, so emptiness and absence coincide.
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<ModHeaderLine*> moduleHeader;
  vector<FunctionEntry*> functions;
  vector<VariantCriterion*> variants;
  vector<Kenngroesse*> items;
};

}  // namespace dcm::ast
