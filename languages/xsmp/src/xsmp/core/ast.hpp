// The xsmp language: its AST. Every class derives from
// cursiv::AstNode, through the abstract spine below or, for the three
// spine-less interfaces (Attribute, ReturnParameter, Multiplicity), directly.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace xsmp::ast {

// Forward declarations: needed by the mutually recursive fields below, and
// safe for reference<T> either way -- it only stores an AstNode*.

struct NamedElement;
struct Document;
struct VisibilityElement;
struct Type;
struct LanguageType;
struct ValueType;
struct SimpleType;
struct Component;
struct NamedElementWithMultiplicity;

struct Structure;
struct Class;
struct Exception;
struct Interface;
struct Model;
struct Service;
struct ArrayType;
struct ValueReference;
struct Integer;
struct Float;
struct EventType;
struct StringType;
struct PrimitiveType;
struct NativeType;
struct AttributeType;
struct Enumeration;

struct Catalogue;
struct Namespace;
struct Attribute;
struct EnumerationLiteral;
struct Constant;
struct Field;
struct Property;
struct Operation;
struct ReturnParameter;
struct Parameter;
struct Association;
struct Container;
struct Reference;
struct Multiplicity;
struct EntryPoint;
struct EventSink;
struct EventSource;

struct Expression;

// interface NamedElement { name: string; attributes: Metadata[] }
// NOTE: `Metadata` is dropped -- `Attribute` is its only implementation, so
// `attributes` is `vector<Attribute*>`, not `vector<Metadata*>`.
struct NamedElement : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Attribute*> attributes;
};

// interface Document extends NamedElement {}
struct Document : cursiv::Extends<NamedElement> {};

// interface VisibilityElement extends NamedElement { modifiers: string[] }
// `modifiers` holds the matched keywords verbatim ("public", "abstract",
// "input", "readOnly", ...).
struct VisibilityElement : cursiv::Extends<NamedElement> {
  vector<string_view> modifiers;
};

// interface Type extends VisibilityElement {}
struct Type : cursiv::Extends<VisibilityElement> {};

// interface LanguageType extends Type {}
struct LanguageType : cursiv::Extends<Type> {};

// interface ValueType extends LanguageType {}
struct ValueType : cursiv::Extends<LanguageType> {};

// interface SimpleType extends ValueType {}
struct SimpleType : cursiv::Extends<ValueType> {};

// `Expression` is a root of its own: it does not extend `NamedElement`.
struct Expression : cursiv::Extends<cursiv::AstNode> {};

// The operation a binary expression applies, not the text that denotes it:
// each operator token hands its constant over.
enum class OpBinary : std::uint8_t {
  logical_or,
  logical_and,
  bitwise_or,
  bitwise_xor,
  bitwise_and,
  equal,
  not_equal,
  greater_equal,
  less_equal,
  greater,
  less,
  shift_left,
  shift_right,
  add,
  subtract,
  multiply,
  divide,
  modulo,
};

// interface BinaryOperation extends Expression {
//     leftOperand: Expression; rightOperand: Expression; feature: OpBinary }
struct BinaryOperation : cursiv::Extends<Expression> {
  Expression* leftOperand;
  OpBinary feature;
  Expression* rightOperand;
};

// interface UnaryOperation extends Expression { operand: Expression; feature:
// OpUnary }
struct UnaryOperation : cursiv::Extends<Expression> {
  string_view feature;  // OpUnary: "!" "-" "+" "~"
  Expression* operand;
};

// interface NamedElementReference extends Expression { value:
// @ConstantOrEnumerationLiteral }
struct NamedElementReference : cursiv::Extends<Expression> {
  reference<Constant, EnumerationLiteral> value;
};

// interface ParenthesizedExpression extends Expression { expr?: Expression }
struct ParenthesizedExpression : cursiv::Extends<Expression> {
  Expression* expr;
};

// interface DesignatedInitializer extends Expression { field: @Field; expr:
// Expression }
// NOTE: `field` is scoped by ValidID, the only reference in the grammar that
// is not a dotted QualifiedName. NOTE: `expr` is nullable despite the
// interface: the grammar writes `('=' expr=...)?`.
struct DesignatedInitializer : cursiv::Extends<Expression> {
  reference<Field> field;  // [Field:ValidID]
  Expression* expr;
};

// interface CollectionLiteral extends Expression { elements: Expression[] }
struct CollectionLiteral : cursiv::Extends<Expression> {
  vector<Expression*> elements;
};

// interface IntegerLiteral extends Expression { text: string }
struct IntegerLiteral : cursiv::Extends<Expression> {
  string_view text;
};

// interface FloatingLiteral extends Expression { text: string }
struct FloatingLiteral : cursiv::Extends<Expression> {
  string_view text;
};

// interface StringLiteral extends Expression { value: string[] }
// A list because the grammar writes `value+=STRING+` (adjacent string
// literals concatenate, C-style). Each entry is decoded; source spelling stays in CST.
struct StringLiteral : cursiv::Extends<Expression> {
  vector<string_view> value;
};

// interface CharacterLiteral extends Expression { value: string }
struct CharacterLiteral : cursiv::Extends<Expression> {
  string_view value;  // decoded as StringLiteral::value
};

// interface BuiltInConstant extends Expression { name: string }
struct BuiltInConstant : cursiv::Extends<Expression> {
  string_view name;
};

// interface BuiltInFunction extends Expression { name: string; argument?:
// Expression }
struct BuiltInFunction : cursiv::Extends<Expression> {
  string_view name;
  Expression* argument;
};

// interface BooleanLiteral extends Expression { isTrue: boolean }
// A real bool, written by the grammar's `flag<&BooleanLiteral::isTrue>`
// on `isTrue?='true'`.
struct BooleanLiteral : cursiv::Extends<Expression> {
  boolean isTrue;
};

// interface KeywordExpression extends Expression { name: string }
struct KeywordExpression : cursiv::Extends<Expression> {
  string_view name;  // always "nullptr"
};

// interface Attribute extends Metadata { ^type: @Type; value?: Expression }
// NOTE: no `name` -- Attribute is metadata attached to its owner, not a
// symbol of its own; `Metadata` gone (see NamedElement), so its only base is
// cursiv::AstNode.
// NOTE: `type` shadows the base's own type tag (same for every `type` member
// below); the builder reads the tag through `cursiv::AstNode&`, so that stays
// harmless.
struct Attribute : cursiv::Extends<cursiv::AstNode> {
  reference<Type> type;  // [AttributeType:QualifiedName]
  Expression* value;
};

// entry Catalogue returns Catalogue { name; attributes+; elements+=Namespace* }
struct Catalogue : cursiv::Extends<Document> {
  vector<Namespace*> elements;
};

// interface Namespace extends NamedElement { elements: (Namespace|Type)[] }
// Both `Namespace` and `NestedNamespace` grammar rules build this type,
// differing only by the leading `namespace` keyword. NOTE: `elements` is
// `NamedElement*`, the closest common base of Namespace and every Type --
// same reasoning for every heterogeneous list below.
struct Namespace : cursiv::Extends<NamedElement> {
  vector<NamedElement*> elements;
};

// interface Structure extends ValueType {
//     elements: (Constant|Field|Property|Operation|Association)[] }
struct Structure : cursiv::Extends<ValueType> {
  vector<NamedElement*> elements;
};

// interface Class extends Structure { base?: @Type }   -- filter [Class:...]
struct Class : cursiv::Extends<Structure> {
  reference<Type> base;  // [Class:QualifiedName]
};

// interface Exception extends Class {}  -- reuses Class::base
struct Exception : cursiv::Extends<Class> {};

// interface Interface extends LanguageType {
//     base: @Type[];  elements: (Constant|Property|Operation)[] }
// NOTE: `base` is a plain vector of independently written names, one per
// `extends A, B, C` entry -- a list of references, not one name resolving to
// several targets (which would be a `multi_reference`).
struct Interface : cursiv::Extends<LanguageType> {
  vector<reference<Type>> base;  // [Interface:QualifiedName], `+=`
  vector<NamedElement*> elements;
};

// interface Component extends LanguageType {
//     ^interface: @Type[];  base?: @Type;
//     elements: (Constant|Field|Property|Operation|Association|Container|
//                Reference|EntryPoint|EventSink|EventSource)[] }
// NOTE: `interface` is renamed `interface_` -- it is a macro in <windows.h> and
// reads badly next to the `Interface` type. Same treatment as `default_` below.
struct Component : cursiv::Extends<LanguageType> {
  vector<reference<Type>> interface_;  // [Interface:QualifiedName], `+=`
  reference<Type> base;                // [Component:QualifiedName]
  vector<NamedElement*> elements;
};

// interface Model extends Component {}
struct Model : cursiv::Extends<Component> {};

// interface Service extends Component {}
struct Service : cursiv::Extends<Component> {};

// interface ArrayType extends ValueType { itemType: @Type; size: Expression }
// NOTE: `size` is declared non-optional but the grammar writes
// '[' size=Expression? ']' (`array A = T[]` is legal), so it is nullable.
struct ArrayType : cursiv::Extends<ValueType> {
  reference<Type> itemType;  // [ValueType:QualifiedName]
  Expression* size;
};

// interface ValueReference extends LanguageType { ^type: @Type }
struct ValueReference : cursiv::Extends<LanguageType> {
  reference<Type> type;  // [ValueType:QualifiedName]
};

// interface Integer extends SimpleType {
//     primitiveType?: @Type; minimum?: Expression; maximum?: Expression }
// NOTE: an unbounded end is written `*` in the source and assigns nothing, so
// "unbounded" is encoded as a null minimum/maximum.
struct Integer : cursiv::Extends<SimpleType> {
  reference<Type> primitiveType;  // [PrimitiveType:QualifiedName]
  Expression* minimum;
  Expression* maximum;
};

// interface Float extends SimpleType {
//     primitiveType?; minimum?; maximum?; range?: RangeKind }
struct Float : cursiv::Extends<SimpleType> {
  reference<Type> primitiveType;  // [PrimitiveType:QualifiedName]
  Expression* minimum;
  Expression* maximum;
  string_view range;  // RangeKind: "..." "..<" "<.." "<.<"
};

// interface EventType extends Type { eventArgs?: @Type }
struct EventType : cursiv::Extends<Type> {
  reference<Type> eventArgs;  // [SimpleType:QualifiedName]
};

// interface StringType extends SimpleType { length: Expression }
// NOTE: same as ArrayType::size -- grammatically optional, hence nullable.
struct StringType : cursiv::Extends<SimpleType> {
  Expression* length;
};

// interface PrimitiveType extends SimpleType {}
struct PrimitiveType : cursiv::Extends<SimpleType> {};

// interface NativeType extends LanguageType {}
struct NativeType : cursiv::Extends<LanguageType> {};

// interface AttributeType extends Type { ^type: @Type; default?: Expression }
// NOTE: `default` is a C++ keyword -> `default_`.
struct AttributeType : cursiv::Extends<Type> {
  reference<Type> type;  // [ValueType:QualifiedName]
  Expression* default_;
};

// interface Enumeration extends SimpleType { literal: EnumerationLiteral[] }
// NOTE: the property is singular (`literal`) although it is a list.
struct Enumeration : cursiv::Extends<SimpleType> {
  vector<EnumerationLiteral*> literal;
};

// interface EnumerationLiteral extends NamedElement { value: Expression }
struct EnumerationLiteral : cursiv::Extends<NamedElement> {
  Expression* value;
};

// interface Constant extends VisibilityElement { ^type: @Type; value:
// Expression } NOTE: `value` is grammatically optional ('=' value=Expression)?.
struct Constant : cursiv::Extends<VisibilityElement> {
  reference<Type> type;  // [SimpleType:QualifiedName]
  Expression* value;
};

// interface Field extends VisibilityElement, Resolvable {
//     ^type: @Type; default?: Expression }
// NOTE: `Resolvable` is an empty marker on `NamedElement`; it and every other
// `, Resolvable` in this file are dropped rather than kept as a second empty
// base (single inheritance only).
struct Field : cursiv::Extends<VisibilityElement> {
  reference<Type> type;  // [LanguageType:QualifiedName]
  Expression* default_;
};

// interface Property extends VisibilityElement, Resolvable {
//     ^type: @Type; attachedField?: @Field; getRaises: @Type[]; setRaises:
//     @Type[] }
struct Property : cursiv::Extends<VisibilityElement> {
  reference<Type> type;               // [ValueType:QualifiedName]
  vector<reference<Type>> getRaises;  // [Exception:QualifiedName], `+=`
  vector<reference<Type>> setRaises;  // [Exception:QualifiedName], `+=`
  reference<Field> attachedField;     // [Field:QualifiedName]
};

// interface Operation extends VisibilityElement, Resolvable {
//     returnParameter?: ReturnParameter; parameter: Parameter[];
//     raisedException: @Type[] }
// NOTE: a `def void f()` operation leaves returnParameter null -- the `void`
// alternative of the grammar assigns nothing.
struct Operation : cursiv::Extends<VisibilityElement> {
  ReturnParameter* returnParameter;
  vector<Parameter*> parameter;
  vector<reference<Type>> raisedException;  // [Exception:QualifiedName], `+=`
};

// interface ReturnParameter { name?: string; attributes: Attribute[]; ^type:
// @Type }
// NOTE: the one member-ish node that is not a NamedElement -- it declares
// its own `name`/`attributes` instead, so its only base is cursiv::AstNode.
struct ReturnParameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Attribute*> attributes;
  reference<Type> type;  // [LanguageType:QualifiedName]
};

// interface Parameter extends NamedElement {
//     ^type: @Type; default?: Expression; direction?: ParameterDirectionKind }
struct Parameter : cursiv::Extends<NamedElement> {
  string_view direction;  // ParameterDirectionKind: "in" "out" "inout"
  reference<Type> type;   // [LanguageType:QualifiedName]
  Expression* default_;
};

// interface Association extends VisibilityElement { ^type: @Type }
struct Association : cursiv::Extends<VisibilityElement> {
  reference<Type> type;  // [LanguageType:QualifiedName]
};

// interface Multiplicity { lower?: Expression; upper?: Expression; aux?:
// boolean } NOTE: not a NamedElement (no `name`), so cursiv::AstNode directly.
//   `[+]` and `[lower ... *]` set aux; `*`, `[*]`, `[]` do not.
struct Multiplicity : cursiv::Extends<cursiv::AstNode> {
  Expression* lower;
  Expression* upper;
  boolean aux;
};

// interface NamedElementWithMultiplicity extends NamedElement {
//     multiplicity?: Multiplicity; optional?: boolean }
// NOTE: `optional` is renamed `optional_` -- it is the base's own vocabulary
// alias for std::optional.
struct NamedElementWithMultiplicity : cursiv::Extends<NamedElement> {
  Multiplicity* multiplicity;
  boolean optional_;
};

// interface Container extends NamedElementWithMultiplicity, Resolvable {
//     ^type: @Type; defaultComponent?: @Type }
struct Container : cursiv::Extends<NamedElementWithMultiplicity> {
  reference<Interface, Component> type;  // [ReferenceType:QualifiedName]
  reference<Type> defaultComponent;  // [Component:QualifiedName]
};

// interface Reference extends NamedElementWithMultiplicity, Resolvable {
//     ^interface: @Type }
// NOTE: `interface` -> `interface_`, as on Component.
struct Reference : cursiv::Extends<NamedElementWithMultiplicity> {
  reference<Interface, Component> interface_;  // [ReferenceType:QualifiedName]
};

// interface EntryPoint extends NamedElement, Resolvable {
//     input: @Field[]; output: @Field[] }
struct EntryPoint : cursiv::Extends<NamedElement> {
  vector<reference<Field>> input;   // [Field:QualifiedName], `+=`
  vector<reference<Field>> output;  // [Field:QualifiedName], `+=`
};

// interface EventSink extends NamedElement, Resolvable { ^type: @Type }
struct EventSink : cursiv::Extends<NamedElement> {
  reference<Type> type;  // [EventType:QualifiedName]
};

// interface EventSource extends NamedElement, Resolvable { ^type: @Type }
struct EventSource : cursiv::Extends<NamedElement> {
  reference<Type> type;  // [EventType:QualifiedName]
};

}  // namespace xsmp::ast
