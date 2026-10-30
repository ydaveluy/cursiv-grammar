// The idl language: its AST. OMG CORBA IDL ("IDL v3.5/v4").
// Scoped names are kept as raw text, not resolved cross-references: a name
// may cross modules, interfaces and imports, so a `reference<>` field's own
// linker target is out of reach here -- a plain text span stands in for it.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace idl::ast {

struct Expr;
struct AnnotationArgument : cursiv::Extends<cursiv::AstNode> {
  string_view name; // empty for positional syntax
  Expr *value;
};

struct AnnotationUse : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<AnnotationArgument *> arguments;
  boolean hasArguments;
};
struct Annotated : cursiv::Extends<cursiv::AstNode> {
  vector<AnnotationUse *> annotations;
};

// Base of anything that can appear in a module/interface/value/... body.
struct Decl : cursiv::Extends<Annotated> {};

// Base of a type spelled in "simple" position (base type, scoped name,
// sequence/set/map/string/wstring/fixed). A `type_spec` (member, typedef,
// case element) may also hold a fresh `struct`/`union`/`enum`/`bitset`/
// `bitmask` declaration -- that alternative is a `Decl*` instead, so the
// full type slot is the variant `Type` below.
struct TypeSpec : cursiv::Extends<Annotated> {};

// Namespace scope, so the AstNode vocabulary is out of reach here.
using Type = std::variant<TypeSpec *, Decl *>;

struct Expr;

enum class BaseType : std::uint8_t {
  float_,
  double_,
  long_double,
  short_,
  long_,
  long_long,
  int8_,
  int16_,
  int32_,
  int64_,
  ushort_,
  ulong_,
  ulong_long,
  uint8_,
  uint16_,
  uint32_,
  uint64_,
  char_,
  wchar_,
  boolean_,
  octet_,
  any_,
  object_,
  value_base,
  void_,
  fixed_, // bare fixed in const_type
};

struct BaseTypeRef : cursiv::Extends<TypeSpec> {
  BaseType kind;
};

struct NamedTypeRef : cursiv::Extends<TypeSpec> {
  string_view name;
};

struct StringTypeRef : cursiv::Extends<TypeSpec> {
  Expr *bound; // null: unbounded
};
struct WStringTypeRef : cursiv::Extends<TypeSpec> {
  Expr *bound;
};
struct SequenceTypeRef : cursiv::Extends<TypeSpec> {
  TypeSpec *element;
  Expr *bound;
};
struct SetTypeRef : cursiv::Extends<TypeSpec> {
  TypeSpec *element;
  Expr *bound;
};
struct MapTypeRef : cursiv::Extends<TypeSpec> {
  TypeSpec *keyType;
  TypeSpec *valueType;
  Expr *bound;
};
struct FixedPtTypeRef : cursiv::Extends<TypeSpec> {
  Expr *digits;
  Expr *scale;
};

// Constant expressions: `const_exp` and `positive_int_const`.
struct Expr : cursiv::Extends<Annotated> {};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};
struct ParenExpr : cursiv::Extends<Expr> {
  Expr *inner;
};
struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FixedLit : cursiv::Extends<Expr> {
  string_view text;
};
struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};
struct WCharLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
  vector<string_view> continuations; // adjacent literals, with boundaries intact
};
struct WStringLit : cursiv::Extends<Expr> {
  string_view text;
  vector<string_view> continuations;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// Declarators: `simple_declarator` (no sizes) and `complex_declarator`
// (`array_declarator`) unify into one shape.
struct Declarator : cursiv::Extends<Annotated> {
  string_view name;
  vector<Expr *> arraySizes;
};

struct Member : cursiv::Extends<Annotated> {
  Type type;
  vector<Declarator *> declarators;
};

// `struct`/`union`/`enum`/`bitset`/`bitmask`, each a `Decl` and a `TypeSpec`
// only through the `Type` variant above, never a common node.
struct StructDecl : cursiv::Extends<Decl> {
  string_view name;
  string_view base; // empty: no `: scoped_name`
  vector<Member *> members;
};

struct CaseLabel : cursiv::Extends<Annotated> {
  Expr *value; // null when `default_`
  boolean default_;
};
struct ElementSpec : cursiv::Extends<Annotated> {
  Type type;
  Declarator *declarator;
};
struct CaseStmt : cursiv::Extends<Annotated> {
  vector<CaseLabel *> labels;
  ElementSpec *element;
};
struct UnionDecl : cursiv::Extends<Decl> {
  string_view name;
  Type switchType;
  vector<CaseStmt *> cases;
};

struct EnumeratorDecl : cursiv::Extends<Annotated> {
  string_view name;
};
struct EnumDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<EnumeratorDecl *> enumerators;
};

struct BitfieldSpec : cursiv::Extends<Annotated> {
  Expr *width;
  optional<BaseType> type;
  vector<string_view> names; // may be empty: an anonymous padding bit
};
struct BitsetDecl : cursiv::Extends<Decl> {
  string_view name;
  string_view base;
  vector<BitfieldSpec *> fields;
};

struct BitmaskDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<EnumeratorDecl *> values; // each bit value owns its annotations
};

struct ExceptionDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Member *> members;
};

struct TypedefDecl : cursiv::Extends<Decl> {
  Type type;
  vector<Declarator *> declarators;
};

struct NativeDecl : cursiv::Extends<Decl> {
  string_view name;
};
struct StructForwardDecl : cursiv::Extends<Decl> {
  string_view name;
};
struct UnionForwardDecl : cursiv::Extends<Decl> {
  string_view name;
};

struct ConstDecl : cursiv::Extends<Decl> {
  TypeSpec *type;
  string_view name;
  Expr *value;
};

struct AttrDecl : cursiv::Extends<Decl> {
  boolean readonly_;
  TypeSpec *type;
  vector<string_view> names;
  vector<string_view> getRaises; // meaningful only when names.size() == 1
  vector<string_view> setRaises; // never set when readonly_
};

enum class ParamDir : std::uint8_t { in_, out_, inout_ };
struct Param : cursiv::Extends<Annotated> {
  ParamDir dir;
  TypeSpec *type;
  string_view name;
};

struct OpDecl : cursiv::Extends<Decl> {
  boolean oneway_;
  TypeSpec *returnType; // `BaseTypeRef{kind: void_}` for `void`
  string_view name;
  vector<Param *> params;
  vector<string_view> raises;
  vector<string_view> context;
};

struct ImportDecl : cursiv::Extends<Decl> {
  string_view imported;
};
struct TypeIdDecl : cursiv::Extends<Decl> {
  string_view target;
  string_view repositoryId;
};
struct TypePrefixDecl : cursiv::Extends<Decl> {
  string_view target;
  string_view prefix;
};

struct AnnotationMember : cursiv::Extends<Decl> {
  TypeSpec *type;
  string_view name;
  Expr *defaultValue; // null: no `default`
};
struct AnnotationDecl : cursiv::Extends<Decl> {
  string_view name;
  string_view base;
  vector<Decl *> members; // AnnotationMember, EnumDecl, ConstDecl, TypedefDecl
};
struct AnnotationForwardDecl : cursiv::Extends<Decl> {
  string_view name;
};

struct InterfaceForwardDecl : cursiv::Extends<Decl> {
  boolean abstract_;
  boolean local_;
  string_view name;
};
struct InterfaceDecl : cursiv::Extends<Decl> {
  boolean abstract_;
  boolean local_;
  string_view name;
  vector<string_view> bases;
  vector<Decl *> exports;
};

struct ModuleDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Decl *> definitions;
};

// `valuetype`: box/abstract-forward/abstract-full/forward/full each their own
// shape, since each has a different set of clauses.
struct ValueForwardDecl : cursiv::Extends<Decl> {
  boolean abstract_;
  string_view name;
};
struct ValueBoxDecl : cursiv::Extends<Decl> {
  string_view name;
  Type type;
};
struct ValueAbsDecl : cursiv::Extends<Decl> {
  string_view name;
  boolean truncatable_;
  vector<string_view> valueBases;
  vector<string_view> supports_;
  vector<Decl *> exports;
};
struct StateMember : cursiv::Extends<Decl> {
  boolean public_;
  Type type;
  vector<Declarator *> declarators;
};
struct InitDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Param *> params;
  vector<string_view> raises;
};
struct ValueDecl : cursiv::Extends<Decl> {
  boolean custom_;
  string_view name;
  boolean truncatable_;
  vector<string_view> valueBases;
  vector<string_view> supports_;
  vector<Decl *> elements; // export_, StateMember, InitDecl
};

// `eventtype`: the same three shapes as valuetype, minus the box form.
struct EventForwardDecl : cursiv::Extends<Decl> {
  boolean abstract_;
  string_view name;
};
struct EventAbsDecl : cursiv::Extends<Decl> {
  string_view name;
  boolean truncatable_;
  vector<string_view> valueBases;
  vector<string_view> supports_;
  vector<Decl *> exports;
};
struct EventDecl : cursiv::Extends<Decl> {
  boolean custom_;
  string_view name;
  boolean truncatable_;
  vector<string_view> valueBases;
  vector<string_view> supports_;
  vector<Decl *> elements;
};

struct ProvidesDecl : cursiv::Extends<Decl> {
  string_view interfaceType;
  string_view name;
};
struct UsesDecl : cursiv::Extends<Decl> {
  boolean multiple_;
  string_view interfaceType;
  string_view name;
};
struct EmitsDecl : cursiv::Extends<Decl> {
  string_view eventType;
  string_view name;
};
struct PublishesDecl : cursiv::Extends<Decl> {
  string_view eventType;
  string_view name;
};
struct ConsumesDecl : cursiv::Extends<Decl> {
  string_view eventType;
  string_view name;
};

struct ComponentForwardDecl : cursiv::Extends<Decl> {
  string_view name;
};
struct ComponentDecl : cursiv::Extends<Decl> {
  string_view name;
  string_view base; // empty: no inheritance
  vector<string_view> supports_;
  vector<Decl *> exports;
};

struct FactoryDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Param *> params;
  vector<string_view> raises;
};
struct FinderDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Param *> params;
  vector<string_view> raises;
};
struct HomeDecl : cursiv::Extends<Decl> {
  string_view name;
  string_view base;
  vector<string_view> supports_;
  string_view manages_;
  string_view primaryKey; // empty: no primarykey clause
  vector<Decl *> exports; // export_, FactoryDecl, FinderDecl
};

struct Specification : cursiv::Extends<Annotated> {
  vector<ImportDecl *> imports;
  vector<Decl *> definitions;
};

} // namespace idl::ast
