// The asn language: its AST, for the reduced ASN.1 module syntax this port
// covers (X.680/681/682/683). Types, values and constraints
// are each one small class hierarchy; a constraint, a value set and an
// object set share one element-set shape, since ASN.1 reuses that grammar
// for all three.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace asn::ast {

struct Constraint;

// Every concrete type ends with zero or more trailing constraints; each
// alternative below appends into this inherited member rather than wrapping
// a separate node, so a reader never has to unwrap a generic "constrained
// type" layer.
struct Type : cursiv::Extends<cursiv::AstNode> {
  vector<Constraint*> constraints;
};
struct Value : cursiv::Extends<cursiv::AstNode> {};
struct Constraint : cursiv::Extends<cursiv::AstNode> {};
struct Object_ : cursiv::Extends<cursiv::AstNode> {};
struct ObjectClass : cursiv::Extends<cursiv::AstNode> {};

// --- values ----------------------------------------------------------------

// A bare IDENTIFIER standing for a value: an enumeration value, a named
// integer, and a defined value referring to another assignment all read the
// same token and cannot be told apart without a symbol table, so they share
// this one node.
struct NameValue : cursiv::Extends<Value> {
  string_view moduleRef;  // empty for an unqualified defined value
  string_view name;
};
struct IntegerValue : cursiv::Extends<Value> {
  string_view text;  // a signed number, minus sign included when present
};
struct RealNumberValue : cursiv::Extends<Value> {
  string_view text;  // decimal/exponent spelling, never rounded to double
};
struct BooleanValue : cursiv::Extends<Value> {
  boolean value;
};
enum class SpecialReal : std::uint8_t { plusInfinity, minusInfinity, notANumber };
struct SpecialRealValue : cursiv::Extends<Value> {
  SpecialReal kind;
};
// The REAL associated sequence's written shape; its governing type decides
// whether the REAL restrictions apply, rather than the field names alone.
struct CompositeRealValue : cursiv::Extends<Value> {
  Value* mantissa;
  Value* base;
  Value* exponent;
};
// CSTRING | BSTRING | HSTRING: the spelling kept verbatim, quotes/marks
// included, since the three notations are otherwise unrelated in shape.
struct StringValue : cursiv::Extends<Value> {
  string_view text;
};
struct ChoiceValue : cursiv::Extends<Value> {
  string_view alternative;
  Value* inner;
};
struct ObjIdComponent : cursiv::Extends<cursiv::AstNode> {
  string_view name;   // empty when the component is a bare NUMBER
  string_view value;  // a NUMBER or IDENTIFIER in parens after name, or the
                       // bare NUMBER's own text when name is empty
};
struct ObjectIdentifierValue : cursiv::Extends<Value> {
  vector<ObjIdComponent*> components;
};

// --- constraint element sets -------------------------------------------------
// A subtype constraint's element set and an object set's share this exact
// union-of-intersections shape: one struct serves constraints, value sets
// and object sets alike.

struct Element : cursiv::Extends<cursiv::AstNode> {
  Element* excluded;  // the operand of a trailing EXCEPT, or null
};
struct RangeElement : cursiv::Extends<Element> {
  Value* lower;  // null when lowerIsMin
  boolean lowerIsMin;
  boolean lowerExclusive;  // a `<` before the `..`
  Value* upper;  // null when upperIsMax
  boolean upperIsMax;
  boolean upperExclusive;  // a `<` after the `..`
};
struct SizeElement : cursiv::Extends<Element> {
  Constraint* size;
};
struct PatternElement : cursiv::Extends<Element> {
  Value* pattern;
};
struct ValueElement : cursiv::Extends<Element> {
  Value* value;
};
struct IntersectionGroup : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> elements;
};
struct ElementSetSpec : cursiv::Extends<cursiv::AstNode> {
  boolean allExcept;         // `ALL EXCEPT ...` form
  Element* allExceptOperand;
  vector<IntersectionGroup*> unions;
  boolean extensible;        // trailing `, ...`
  vector<IntersectionGroup*> additionalUnions;
};

struct ValueSet : cursiv::Extends<cursiv::AstNode> {
  ElementSetSpec* spec;
};
struct ObjectSet : cursiv::Extends<cursiv::AstNode> {
  ElementSetSpec* spec;
};

// --- general constraints -----------------------------------------------------

// A governor written as a bare class name is dropped here and on
// `Parameter` below: a bare IDENTIFIER is indistinguishable from a
// referenced type without a symbol table, and the form is rare.
struct UserDefinedConstraintParam : cursiv::Extends<cursiv::AstNode> {
  Type* governorType;
  Value* value;
  ValueSet* valueSet;
  Object_* object_;
  ObjectSet* objectSet;
};
struct AtNotation : cursiv::Extends<cursiv::AstNode> {
  boolean relative;  // `@.` (optionally more dots, not counted) vs plain `@`
  vector<string_view> path;
};
struct ComponentPresence : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean present;  // false: ABSENT
};

// One node for a constraint, subtype or general, plus its optional
// exception spec: exactly one of the shapes below is set at a time.
struct ConstraintSpec : cursiv::Extends<Constraint> {
  ElementSetSpec* spec;
  vector<UserDefinedConstraintParam*> userDefined;  // CONSTRAINED BY {...}
  string_view tableObjectSet;  // a component-relation constraint's {Name}
  vector<AtNotation*> tableAtNotations;              // empty: no `{@notation,...}` clause
  Type* containingType;   // CONTAINING <type>
  Value* encodedBy;       // ENCODED BY value
  vector<ComponentPresence*> presences;      // WITH COMPONENTS {...}
  boolean presencesExtensible;
  vector<ComponentPresence*> additionalPresences;

  Type* exceptionType;   // an exception spec's "type : value" form
  Value* exceptionValue;  // an exception spec's value (any form)
};

// --- types -------------------------------------------------------------------

enum class TagMode : std::uint8_t { none_, explicit_, implicit_, automatic_ };
enum class TagClass : std::uint8_t { contextSpecific, universal, application, private_ };
struct TaggedType : cursiv::Extends<Type> {
  boolean explicitEncodingReference = false;  // TAG: was written
  TagClass tagClass = TagClass::contextSpecific;
  Value* number;  // unsigned IntegerValue or a defined NameValue
  TagMode mode = TagMode::none_;  // inherit module/default rules, not resolved
  Type* inner;
};

struct BooleanType : cursiv::Extends<Type> {};
struct NullType : cursiv::Extends<Type> {};
struct OctetStringType : cursiv::Extends<Type> {};
struct ObjectIdentifierType : cursiv::Extends<Type> {};
// REAL: ported alongside BOOLEAN/NULL, the two
// bare-keyword types it sits next to in the lexer.
struct RealType : cursiv::Extends<Type> {};

struct NamedNumber : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;  // a signed number or a defined value, kept as text
};
struct IntegerType : cursiv::Extends<Type> {
  vector<NamedNumber*> namedNumbers;
};
struct NamedBit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};
struct BitStringType : cursiv::Extends<Type> {
  vector<NamedBit*> namedBits;
};

// An ENUMERATED item's bare-value alternative is dropped: it overlaps the
// bare-IDENTIFIER alternative already covered by `name` for every spelling
// a valid item actually uses.
struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  string_view name;    // the bare-IDENTIFIER form
  NamedNumber* numbered;
};
struct EnumeratedType : cursiv::Extends<Type> {
  vector<Enumerator*> items;
  boolean extensible;
  vector<Enumerator*> additionalItems;
};

struct NamedType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
};

enum class SeqSetKind : std::uint8_t { sequence_, set_ };
struct Component : cursiv::Extends<cursiv::AstNode> {
  NamedType* item;    // null when componentsOf is set
  boolean optional_;
  Value* default_;
  Type* componentsOf;  // COMPONENTS OF <type>
};
struct ExtensionAddition : cursiv::Extends<cursiv::AstNode> {
  boolean grouped;
  optional<string_view> version;
  vector<variant<Component *, NamedType *>> items;
};

struct SequenceType : cursiv::Extends<Type> {
  SeqSetKind kind;
  vector<Component*> components;
  boolean extensible;
  vector<ExtensionAddition *> extensions;
};

enum class SeqSetOfKind : std::uint8_t { sequenceOf_, setOf_ };
struct SequenceOfType : cursiv::Extends<Type> {
  SeqSetOfKind kind;
  Constraint* constraint;   // the optional `(SIZE(...))`/`(...)` before OF
  Type* element;            // the bare-type form
  NamedType* namedElement;  // the namedType form
};

// CHOICE extension additions preserve the same grouping as SEQUENCE.
struct ChoiceType : cursiv::Extends<Type> {
  vector<NamedType*> alternatives;
  boolean extensible;
  vector<ExtensionAddition *> extensions;
};

struct ActualParameter : cursiv::Extends<cursiv::AstNode> {
  Type* asType;
  Value* asValue;
};
struct ReferencedType : cursiv::Extends<Type> {
  string_view moduleRef;  // empty when unqualified
  string_view name;
  vector<ActualParameter*> actualParameters;
};

struct DefinedObjectClassRef : cursiv::Extends<ObjectClass> {
  string_view moduleRef;  // empty when unqualified
  string_view name;       // empty for the two useful-class keywords below
  boolean typeIdentifier;  // TYPE-Identifier
  boolean abstractSyntax;  // ABSTRACT-SYNTAX
};
struct FieldNameRef : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> path;  // one entry per `&Ident`
};
struct ObjectClassFieldType : cursiv::Extends<Type> {
  DefinedObjectClassRef* class_;
  FieldNameRef* field;
};

// --- object-class definitions (X.681) ---------------------------------------

struct SyntaxItem : cursiv::Extends<cursiv::AstNode> {};
struct SyntaxLiteral : cursiv::Extends<SyntaxItem> {
  string_view text;  // an IDENTIFIER token or a comma, spelled as-is
};
struct SyntaxFieldRef : cursiv::Extends<SyntaxItem> {
  string_view name;  // the &Ident, without the ampersand
};
struct SyntaxOptionalGroup : cursiv::Extends<SyntaxItem> {
  vector<SyntaxItem*> items;
};

struct FieldSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // the &Ident, without the ampersand
  Type* type;              // a governing type
  FieldNameRef* fieldRef;   // a governing field name
  DefinedObjectClassRef* classRef;  // a governing object-class reference
  boolean unique_;
  boolean optional_;
  Value* defaultValue;
  Type* defaultType;        // an optionality spec's DEFAULT <type>
  ValueSet* defaultValueSet;
  Object_* defaultObject;
  ObjectSet* defaultObjectSet;
};
struct ObjectClassDefn : cursiv::Extends<ObjectClass> {
  vector<FieldSpec*> fields;
  vector<SyntaxItem*> syntax;  // empty: no WITH SYNTAX clause
};

struct ParameterizedObject : cursiv::Extends<Object_> {
  string_view moduleRef;  // empty when unqualified
  string_view name;
  vector<ActualParameter*> actualParameters;  // may be empty
};

// --- module-level declarations
// ------------------------------------------------

struct Decl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct TypeAssignment : cursiv::Extends<Decl> {
  Type* type;
};
struct ValueAssignment : cursiv::Extends<Decl> {
  Type* type;
  Value* value;
};
struct ObjectClassAssignment : cursiv::Extends<Decl> {
  ObjectClass* value;  // TYPE-Identifier, ABSTRACT-SYNTAX or a CLASS body
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  Type* governorType;  // null: no `governor COLON` prefix
  string_view name;
};
// A parameterized assignment binds its parameter list to a type, a value or
// a value set: exactly one of asType/asValue/asValueSet is set.
struct ParameterizedAssignment : cursiv::Extends<Decl> {
  vector<Parameter*> params;
  Type* asType;
  Value* asValue;
  ValueSet* asValueSet;
};

struct Symbol : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};
struct ImportsFromModule : cursiv::Extends<cursiv::AstNode> {
  vector<Symbol*> symbols;
  string_view moduleName;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  TagMode tagDefault;
  boolean extensibilityImplied;
  boolean hasExports;
  boolean exportsAll;         // `EXPORTS ALL;`
  vector<Symbol*> exports_;
  vector<ImportsFromModule*> imports;
  vector<Decl*> assignments;
};

struct Modules : cursiv::Extends<cursiv::AstNode> {
  vector<Module*> modules;
};

}  // namespace asn::ast
