// The oncrpc language: its AST for XDR (RFC 4506) type/constant definitions
// and ONCRPCv2 (RFC 5531) program/version/procedure declarations.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace oncrpc::ast {

using ConstValue = std::string_view;

struct Definition : cursiv::Extends<cursiv::AstNode> {};

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

enum class Primitive : std::uint8_t {
  int_,
  unsigned_int,
  hyper,
  unsigned_hyper,
  float_,
  double_,
  quadruple,
  bool_,
  char_,
  unsigned_char,
  short_,
  unsigned_short,
  long_,
  unsigned_long,
};

struct PrimitiveType : cursiv::Extends<TypeSpec> {
  Primitive kind;
};

// An identifier standing for a type defined elsewhere (a `typedef`, or a
// named `enum`/`struct`/`union`): held as text, not resolved -- a `.x` file
// routinely names types an imported header supplies.
struct NamedType : cursiv::Extends<TypeSpec> {
  string_view name;
  string_view prefix;
};

struct OpaqueType : cursiv::Extends<TypeSpec> {};
struct StringType : cursiv::Extends<TypeSpec> {};

struct EnumConst : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ConstValue value;
};

struct EnumType : cursiv::Extends<TypeSpec> {
  vector<EnumConst*> consts;
};

struct Declaration : cursiv::Extends<cursiv::AstNode> {};

struct VoidDecl : cursiv::Extends<Declaration> {
  string_view name;
};

enum class Bound : std::uint8_t { plain, fixed, variable };

struct ValueDecl : cursiv::Extends<Declaration> {
  TypeSpec* type;
  string_view name;
  vector<string_view> extraNames;
  boolean pointer;
  Bound bound;
  optional<ConstValue> size;  // set for `fixed`, optional for `variable`
};

struct StructType : cursiv::Extends<TypeSpec> {
  vector<Declaration*> members;
};

struct CaseLabel : cursiv::Extends<cursiv::AstNode> {
  ConstValue value;
};

struct CaseSpec : cursiv::Extends<cursiv::AstNode> {
  vector<CaseLabel*> labels;
  Declaration* body;
};

struct UnionType : cursiv::Extends<TypeSpec> {
  Declaration* discriminant;
  vector<CaseSpec*> cases;
  Declaration* defaultCase;  // null when the union has no default arm
};

struct TypedefDef : cursiv::Extends<Definition> {
  Declaration* decl;
};

struct EnumDef : cursiv::Extends<Definition> {
  string_view name;
  EnumType* body;
};

struct StructDef : cursiv::Extends<Definition> {
  string_view name;
  StructType* body;
};

struct UnionDef : cursiv::Extends<Definition> {
  string_view name;
  UnionType* body;
};

struct ConstantDef : cursiv::Extends<Definition> {
  string_view name;
  ConstValue value;
};

struct ProcedureDef : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* returnType;  // null for `void`
  string_view name;
  vector<Declaration*> arguments;
  ConstValue number;
};

struct VersionDef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<ProcedureDef*> procedures;
  ConstValue number;
};

struct ProgramDef : cursiv::Extends<Definition> {
  string_view name;
  vector<VersionDef*> versions;
  ConstValue number;
};

struct Specification : cursiv::Extends<cursiv::AstNode> {
  vector<Definition*> items;
};

}  // namespace oncrpc::ast
