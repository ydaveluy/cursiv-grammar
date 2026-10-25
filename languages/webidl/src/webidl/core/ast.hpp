// The webidl language: its AST. WebIDL (Web IDL, Second Edition draft).
// Scoped/interface names are kept as text, never `reference<>`: a name may
// be declared in another IDL fragment entirely (an included spec, a DOM
// interface defined elsewhere), so nothing here resolves within one document.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace webidl::ast {

struct ExtendedAttribute;
struct Argument;
struct Type;
struct ConstValue;
struct ExtendedAttribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean hasArguments;
  boolean wildcard;
  boolean hasList;
  ConstValue* rhs;
  vector<ConstValue*> values;
  vector<Argument*> args;
};

enum class Primitive : std::uint8_t {
  short_,
  long_,
  long_long,
  unsigned_short,
  unsigned_long,
  unsigned_long_long,
  float_,
  unrestricted_float,
  double_,
  unrestricted_double,
  undefined_,
  void_,
  boolean_,
  byte_,
  octet_,
  bigint_,
};

// The type family: one class per distinguishable-type alternative, plus
// `any`, the promise type and the union type. `attrs` and `nullable` are on
// the shared base because most alternatives carry both.
struct Type : cursiv::Extends<cursiv::AstNode> {
  vector<ExtendedAttribute*> attrs;
  boolean nullable;
};

struct PrimitiveType : cursiv::Extends<Type> {
  Primitive kind;
};
// ByteString / DOMString / USVString.
struct StringType : cursiv::Extends<Type> {
  string_view name;
};
// A named user type: an interface, dictionary, enum, callback or typedef.
struct NamedType : cursiv::Extends<Type> {
  string_view name;
};
struct AnyType : cursiv::Extends<Type> {};
struct ObjectType : cursiv::Extends<Type> {};
struct SymbolType : cursiv::Extends<Type> {};
// ArrayBuffer, DataView, Int8Array, ... Float64Array.
struct BufferType : cursiv::Extends<Type> {
  string_view name;
};
struct SequenceType : cursiv::Extends<Type> {
  boolean async_;
  Type* element;
};
struct FrozenArrayType : cursiv::Extends<Type> {
  Type* element;
};
struct ObservableArrayType : cursiv::Extends<Type> {
  Type* element;
};
struct RecordType : cursiv::Extends<Type> {
  Type* keyType;    // always a StringType
  Type* valueType;
};
struct PromiseType : cursiv::Extends<Type> {
  Type* value;
};
struct UnionType : cursiv::Extends<Type> {
  vector<Type*> members;  // two or more
};

// One parameter: `optional type name = default` or `type... name` (variadic)
// or `type name`.
struct Argument : cursiv::Extends<cursiv::AstNode> {
  vector<ExtendedAttribute*> attrs;
  boolean optional_;
  boolean variadic;
  Type* type;
  string_view name;
  ConstValue* default_;  // set only when optional_ and a `= value` was written
};

// constValue / defaultValue, unified: the string/empty-list/empty-map/null
// forms only ever appear as a default value, never as a const's value.
struct ConstValue : cursiv::Extends<cursiv::AstNode> {};
struct IdentifierValue : cursiv::Extends<ConstValue> {
  string_view text;
};
struct BoolValue : cursiv::Extends<ConstValue> {
  boolean value;
};
struct IntValue : cursiv::Extends<ConstValue> {
  string_view text;
};
// DECIMAL, or one of Infinity / -Infinity / NaN.
struct FloatValue : cursiv::Extends<ConstValue> {
  string_view text;
};
struct StringValue : cursiv::Extends<ConstValue> {
  string_view text;  // quotes stripped
};
struct NullValue : cursiv::Extends<ConstValue> {};
struct EmptySequenceValue : cursiv::Extends<ConstValue> {};
struct EmptyDictionaryValue : cursiv::Extends<ConstValue> {};

// A member of an interface, mixin, callback interface or namespace body.
struct Member : cursiv::Extends<cursiv::AstNode> {
  vector<ExtendedAttribute*> attrs;
};

// A const's type is a primitive or named type only, never nullable or
// attributed.
struct ConstMember : cursiv::Extends<Member> {
  Type* type;
  string_view name;
  ConstValue* value;
};

enum class Special : std::uint8_t { none_, getter_, setter_, deleter_ };
struct Operation : cursiv::Extends<Member> {
  Special special;
  boolean static_;
  boolean stringifier;
  Type* returnType;
  string_view name;  // empty: unnamed (getter/setter/deleter often are)
  vector<Argument*> params;
};

struct Attribute : cursiv::Extends<Member> {
  boolean readonly_;
  boolean inherit_;
  boolean static_;
  boolean stringifier;
  Type* type;
  string_view name;
};

// interfaceMember only (never a mixin/namespace/callback-interface member).
struct Constructor : cursiv::Extends<Member> {
  vector<Argument*> params;
};

// The bare `stringifier;` form; `stringifier attribute` and `stringifier
// operation` are an Attribute/Operation with `stringifier` set instead.
struct Stringifier : cursiv::Extends<Member> {};

struct Iterable : cursiv::Extends<Member> {
  Type* keyType;
  Type* valueType;  // null: single-type form (value iterator only)
};
struct AsyncIterable : cursiv::Extends<Member> {
  boolean legacySyntax;
  Type* keyType;
  Type* valueType;
  vector<Argument*> params;
};
struct Maplike : cursiv::Extends<Member> {
  boolean readonly_;
  Type* keyType;
  Type* valueType;
};
struct Setlike : cursiv::Extends<Member> {
  boolean readonly_;
  Type* type;
};

// A definition at document scope: interface, mixin, callback (interface or
// function), namespace, dictionary, enum, typedef, includes statement.
struct Definition : cursiv::Extends<cursiv::AstNode> {
  vector<ExtendedAttribute*> attrs;
};

struct Interface : cursiv::Extends<Definition> {
  boolean partial_;
  string_view name;
  string_view base;  // empty: no `: Base`
  vector<Member*> members;
};

struct Mixin : cursiv::Extends<Definition> {
  boolean partial_;
  string_view name;
  vector<Member*> members;  // ConstMember, Operation, Attribute
};

struct CallbackInterface : cursiv::Extends<Definition> {
  string_view name;
  vector<Member*> members;  // ConstMember, Operation
};

// `callback F = type (args);` -- a callback function, not an interface.
struct Callback : cursiv::Extends<Definition> {
  string_view name;
  Type* returnType;
  vector<Argument*> params;
};

struct Namespace : cursiv::Extends<Definition> {
  boolean partial_;
  string_view name;
  vector<Member*> members;  // Operation, Attribute (readonly), ConstMember
};

struct DictionaryMember : cursiv::Extends<cursiv::AstNode> {
  vector<ExtendedAttribute*> attrs;
  boolean required_;
  Type* type;
  string_view name;
  ConstValue* default_;  // null: no `= value` (required members, or none given)
};

struct Dictionary : cursiv::Extends<Definition> {
  boolean partial_;
  string_view name;
  string_view base;  // empty: no `: Base`
  vector<DictionaryMember*> members;
};

struct Enum : cursiv::Extends<Definition> {
  string_view name;
  vector<string_view> values;  // decoded string literals, quotes stripped
};

struct Typedef : cursiv::Extends<Definition> {
  Type* type;
  string_view name;
};

// `A includes B;`
struct IncludesStatement : cursiv::Extends<Definition> {
  string_view target;
  string_view mixin;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Definition*> definitions;
};

}  // namespace webidl::ast
