// The asn language: its grammar, for the reduced ASN.1 module syntax this
// port covers (X.680/681/682/683: no type tagging, no EXTERNAL/
// EMBEDDED PDV/CHARACTER STRING/INSTANCE OF). A bare
// IDENTIFIER is genuinely ambiguous in several ASN.1 productions; the
// engine is LR with GLR forks for the positions not folded below.
#pragma once

#include "asn/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace asn {

struct Asn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  struct COMMENT : terminal<through(lit<"--">, lit<"--"> | &eol | eof)> {};
  struct BLOCK_COMMENT
      : terminal<lit<"/*"> +
                 many(ref<BLOCK_COMMENT> | (!lit<"/*"> + !lit<"*/"> + any)) +
                 lit<"*/">> {};

  // Keep names permissive, but leave a leading minus to SignedNumber.
  static constexpr auto word_boundary = uc::L | uc::Nd | cr<"_$-">;
  struct ID : terminal<(uc::L | cr<"_$">) + many(uc::L | uc::Nd | cr<"_$-">)> {
  };

  struct NUMBER : terminal<some(d)> {};

  struct CSTRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<"^\"">) + lit<"\"">> {};
  struct BSTRING : terminal<lit<"'"> + many(cr<R"(01 \t\r\n\f)">) + lit<"'B">> {};
  // HSTRING joins CSTRING/BSTRING as a builtin value.
  struct HSTRING : terminal<lit<"'"> + many(cr<R"(0-9a-fA-F \t\r\n\f)">) + lit<"'H">> {};
  static constexpr auto SignedNumber = option(lit<"-">) + ref<NUMBER>;

  // --- forward declarations --------------------------------------------
  struct Constraint;
  struct NameValue;
  struct IntegerValue;
  struct BooleanValue;
  struct StringValue;
  struct ChoiceValue;
  struct ObjectIdentifierValue;
  struct NamedType;
  struct Component;
  struct SequenceOrSetType;
  struct SequenceOrSetOfType;
  struct ChoiceType;
  struct EnumeratedType;
  struct IntegerType;
  struct BitStringType;
  struct OctetStringType;
  struct ObjectIdType;
  struct RealType;
  struct BooleanType;
  struct NullType;
  struct ObjectClassFieldType;
  struct ReferencedType;
  struct ActualParameter;
  struct DefinedObjectClass;
  struct TypeIdentifierOrAbstractSyntax;
  struct FieldName;
  struct ValueSet;
  struct ObjectSet;
  struct Object_;
  struct ObjectClassDefn;
  struct FieldSpec;
  struct SyntaxLiteral;
  struct SyntaxFieldRef;
  struct SyntaxOptionalGroup;
  struct Parameter;
  struct ElementSetSpec;
  struct IntersectionGroup;
  struct RangeElement;
  struct SizeElement;
  struct PatternElement;
  struct ValueElement;
  struct UserDefinedConstraintParam;
  struct AtNotation;
  struct ComponentPresence;
  struct Symbol;
  struct ImportsFromModule;
  struct Assignment;

  // --- shared fragments (forward-declared refs only) ---------------------
  static constexpr auto any_value =
      ref<NameValue> | ref<IntegerValue> | ref<ChoiceValue> |
      ref<ObjectIdentifierValue> | ref<BooleanValue> |
      ref<StringValue>;

  static constexpr auto asn_type =
      ref<SequenceOrSetType> | ref<SequenceOrSetOfType> |
      ref<ChoiceType> | ref<EnumeratedType> | ref<IntegerType> |
      ref<BitStringType> | ref<OctetStringType> |
      ref<ObjectIdType> | ref<ObjectClassFieldType> |
      ref<BooleanType> | ref<NullType> | ref<RealType> |
      ref<ReferencedType>;

  static constexpr auto with_constraints =
      many(append<&ast::Type::constraints>(ref<Constraint>));

  static constexpr auto subtype_elements =
      ref<RangeElement> | ref<SizeElement> | ref<PatternElement> |
      ref<ValueElement>;
  // An EXCEPT operand's own EXCEPT is accepted here: it reads through the
  // same rule as a top-level element, a slightly wider read.
  static constexpr auto exclusion_tail =
      option(kw<"EXCEPT"> + assign<&ast::Element::excluded>(subtype_elements));

  static constexpr auto token_or_group_spec =
      ref<SyntaxLiteral> | ref<SyntaxFieldRef> |
      ref<SyntaxOptionalGroup>;

  // --- values --------------------------------------------------------------

  // An enumeration value and a named integer both read a bare IDENTIFIER
  // and cannot be told apart without a symbol table: one
  // node covers both.
  struct NameValue
      : rule<create<ast::NameValue>(
            assign<&ast::NameValue::name>(ref<ID>))> {};
  struct IntegerValue
      : rule<create<ast::IntegerValue>(
            assign<&ast::IntegerValue::text>(SignedNumber))> {};
  struct BooleanValue
      : rule<create<ast::BooleanValue>(
            flag<&ast::BooleanValue::value>(kw<"TRUE"> | kw<"true">) |
            (kw<"FALSE"> | kw<"false">))> {};
  struct StringValue
      : rule<create<ast::StringValue>(assign<&ast::StringValue::text>(
            ref<CSTRING> | ref<BSTRING> | ref<HSTRING>))> {};
  struct ChoiceValue
      : rule<create<ast::ChoiceValue>(
            assign<&ast::ChoiceValue::alternative>(ref<ID>) + lit<":"> +
            assign<&ast::ChoiceValue::inner>(any_value))> {};

  // An OID component's value may in principle be a dot-qualified,
  // parameterized defined value; its qualification and parameter list are
  // dropped, folding it into the plain-IDENTIFIER shape already covered
  // below.
  struct ObjIdComponent
      : rule<create<ast::ObjIdComponent>(
            assign<&ast::ObjIdComponent::value>(ref<NUMBER>) |
            (assign<&ast::ObjIdComponent::name>(ref<ID>) +
             option(lit<"("> +
                    assign<&ast::ObjIdComponent::value>(ref<NUMBER> |
                                                          ref<ID>) +
                    lit<")">)))> {};
  struct ObjectIdentifierValue
      : rule<create<ast::ObjectIdentifierValue>(
            lit<"{"> +
            some(append<&ast::ObjectIdentifierValue::components>(
                ref<ObjIdComponent>)) +
            lit<"}">)> {};

  // --- constraint element sets ---------------------------------------------

  struct RangeElement
      : rule<create<ast::RangeElement>(
            (assign<&ast::RangeElement::lower>(any_value) |
             flag<&ast::RangeElement::lowerIsMin>(kw<"MIN">)) +
            option(flag<&ast::RangeElement::lowerExclusive>(lit<"<">)) +
            lit<".."> +
            option(flag<&ast::RangeElement::upperExclusive>(lit<"<">)) +
            (assign<&ast::RangeElement::upper>(any_value) |
             flag<&ast::RangeElement::upperIsMax>(kw<"MAX">)) +
            exclusion_tail)> {};
  struct SizeElement
      : rule<create<ast::SizeElement>(
            kw<"SIZE"> + assign<&ast::SizeElement::size>(ref<Constraint>) +
            exclusion_tail)> {};
  struct PatternElement
      : rule<create<ast::PatternElement>(
            kw<"PATTERN"> + assign<&ast::PatternElement::pattern>(any_value) +
            exclusion_tail)> {};
  struct ValueElement
      : rule<create<ast::ValueElement>(
            assign<&ast::ValueElement::value>(any_value) + exclusion_tail)> {
  };

  struct IntersectionGroup
      : rule<create<ast::IntersectionGroup>(
            append<&ast::IntersectionGroup::elements>(subtype_elements) +
            many((kw<"INTERSECTION"> | lit<"^">) +
                 append<&ast::IntersectionGroup::elements>(
                     subtype_elements)))> {};

  struct ElementSetSpec
      : rule<create<ast::ElementSetSpec>(
            (flag<&ast::ElementSetSpec::allExcept>(kw<"ALL">) +
             kw<"EXCEPT"> +
             assign<&ast::ElementSetSpec::allExceptOperand>(
                 subtype_elements)) |
            (append<&ast::ElementSetSpec::unions>(ref<IntersectionGroup>) +
             many((kw<"UNION"> | lit<"|">) +
                  append<&ast::ElementSetSpec::unions>(
                      ref<IntersectionGroup>)) +
             option(
                 lit<","> +
                 flag<&ast::ElementSetSpec::extensible>(lit<"...">) +
                 option(
                     lit<","> +
                     append<&ast::ElementSetSpec::additionalUnions>(
                         ref<IntersectionGroup>) +
                     many((kw<"UNION"> | lit<"|">) +
                          append<&ast::ElementSetSpec::additionalUnions>(
                              ref<IntersectionGroup>))))))> {};

  struct ValueSet
      : rule<create<ast::ValueSet>(
            lit<"{"> + assign<&ast::ValueSet::spec>(ref<ElementSetSpec>) +
            lit<"}">)> {};
  // An object set spec's bare-ELLIPSIS-with-no-root alternative (`{...}`
  // alone) is not ported: exceedingly rare, and `ElementSetSpec`
  // already requires at least one element.
  struct ObjectSet
      : rule<create<ast::ObjectSet>(
            lit<"{"> + assign<&ast::ObjectSet::spec>(ref<ElementSetSpec>) +
            lit<"}">)> {};

  // --- general constraints -----------------------------------------------

  struct UserDefinedConstraintParam
      : rule<create<ast::UserDefinedConstraintParam>(
            assign<&ast::UserDefinedConstraintParam::governorType>(
                asn_type) +
            option((lit<":"> +
                    assign<&ast::UserDefinedConstraintParam::value>(
                        any_value)) |
                   assign<&ast::UserDefinedConstraintParam::valueSet>(
                       ref<ValueSet>) |
                   assign<&ast::UserDefinedConstraintParam::object_>(
                       ref<Object_>) |
                   assign<&ast::UserDefinedConstraintParam::objectSet>(
                       ref<ObjectSet>)))> {};

  // The at-notation's extra dots after `@.` are consumed and not counted:
  // `relative` alone says which form was used.
  struct AtNotation
      : rule<create<ast::AtNotation>(
            (flag<&ast::AtNotation::relative>(lit<"@.">) +
             many(lit<".">)) |
            (lit<"@"> +
            append<&ast::AtNotation::path>(ref<ID>) +
            many(lit<"."> + append<&ast::AtNotation::path>(ref<ID>))))> {};

  struct ComponentPresence
      : rule<create<ast::ComponentPresence>(
            assign<&ast::ComponentPresence::name>(ref<ID>) +
            (flag<&ast::ComponentPresence::present>(kw<"PRESENT">) |
             kw<"ABSENT">))> {};

  static constexpr auto component_presence_list =
      option(append<&ast::ConstraintSpec::presences>(ref<ComponentPresence>) +
             many(lit<","> + append<&ast::ConstraintSpec::presences>(
                                  ref<ComponentPresence>))) +
      option(
          lit<","> +
          flag<&ast::ConstraintSpec::presencesExtensible>(lit<"...">) +
          option(lit<","> +
                 append<&ast::ConstraintSpec::additionalPresences>(
                     ref<ComponentPresence>) +
                 many(lit<","> +
                      append<&ast::ConstraintSpec::additionalPresences>(
                          ref<ComponentPresence>))));

  // A component-relation constraint's object-set name drops its optional
  // DOT-qualification, using the same fold as `ObjIdComponent`.
  struct Constraint
      : rule<create<ast::ConstraintSpec>(
            lit<"("> +
            (assign<&ast::ConstraintSpec::spec>(ref<ElementSetSpec>) |
             (kw<"CONSTRAINED"> + kw<"BY"> + lit<"{"> +
              append<&ast::ConstraintSpec::userDefined>(
                  ref<UserDefinedConstraintParam>) +
              many(lit<","> + append<&ast::ConstraintSpec::userDefined>(
                                   ref<UserDefinedConstraintParam>)) +
              lit<"}">) |
             (lit<"{"> +
              assign<&ast::ConstraintSpec::tableObjectSet>(ref<ID>) +
              lit<"}"> +
              option(lit<"{"> +
                     append<&ast::ConstraintSpec::tableAtNotations>(
                         ref<AtNotation>) +
                     many(lit<","> + append<&ast::ConstraintSpec::tableAtNotations>(
                                          ref<AtNotation>)) +
                     lit<"}">)) |
             (kw<"CONTAINING"> +
              assign<&ast::ConstraintSpec::containingType>(asn_type) +
              option(kw<"ENCODED"> + kw<"BY"> +
                     assign<&ast::ConstraintSpec::encodedBy>(any_value))) |
             (kw<"ENCODED"> + kw<"BY"> +
              assign<&ast::ConstraintSpec::encodedBy>(any_value)) |
             (kw<"WITH"> + kw<"COMPONENTS"> + lit<"{"> +
              component_presence_list + lit<"}">)) +
            option(lit<"!"> +
                   ((assign<&ast::ConstraintSpec::exceptionType>(asn_type) +
                     lit<":"> +
                     assign<&ast::ConstraintSpec::exceptionValue>(
                         any_value)) |
                    assign<&ast::ConstraintSpec::exceptionValue>(
                        any_value))) +
            lit<")">)> {};

  // --- types ---------------------------------------------------------------

  struct BooleanType
      : rule<create<ast::BooleanType>(kw<"BOOLEAN"> + with_constraints)> {};
  struct NullType
      : rule<create<ast::NullType>(kw<"NULL"> + with_constraints)> {};
  // REAL: ported next to BOOLEAN/NULL, the two
  // bare-keyword types it sits beside in the lexer.
  struct RealType
      : rule<create<ast::RealType>(kw<"REAL"> + with_constraints)> {};
  struct OctetStringType
      : rule<create<ast::OctetStringType>(
            kw<"OCTET"> + kw<"STRING"> + with_constraints)> {};
  struct ObjectIdType
      : rule<create<ast::ObjectIdentifierType>(
            kw<"OBJECT"> + kw<"IDENTIFIER"> + with_constraints)> {};

  struct NamedNumber
      : rule<create<ast::NamedNumber>(
            assign<&ast::NamedNumber::name>(ref<ID>) + lit<"("> +
            assign<&ast::NamedNumber::value>(SignedNumber | ref<ID>) +
            lit<")">)> {};
  struct IntegerType
      : rule<create<ast::IntegerType>(
            kw<"INTEGER"> +
            option(lit<"{"> +
                   some(append<&ast::IntegerType::namedNumbers>(
                            ref<NamedNumber>),
                        lit<",">) +
                   lit<"}">) +
            with_constraints)> {};

  struct NamedBit
      : rule<create<ast::NamedBit>(
            assign<&ast::NamedBit::name>(ref<ID>) + lit<"("> +
            assign<&ast::NamedBit::value>(ref<NUMBER> | ref<ID>) +
            lit<")">)> {};
  struct BitStringType
      : rule<create<ast::BitStringType>(
            kw<"BIT"> + kw<"STRING"> +
            option(lit<"{"> +
                   some(append<&ast::BitStringType::namedBits>(
                            ref<NamedBit>),
                        lit<",">) +
                   lit<"}">) +
            with_constraints)> {};

  // An ENUMERATED item's bare-value alternative is dropped: see ast.hpp's
  // `Enumerator` comment.
  struct EnumeratorItem
      : rule<create<ast::Enumerator>(
            assign<&ast::Enumerator::numbered>(ref<NamedNumber>) |
            assign<&ast::Enumerator::name>(ref<ID>))> {};
  struct EnumeratedType
      : rule<create<ast::EnumeratedType>(
            kw<"ENUMERATED"> + lit<"{"> +
            some(append<&ast::EnumeratedType::items>(ref<EnumeratorItem>),
                 lit<",">) +
            option(lit<","> +
                   flag<&ast::EnumeratedType::extensible>(lit<"...">) +
                   option(lit<","> +
                          some(append<&ast::EnumeratedType::additionalItems>(
                                   ref<EnumeratorItem>),
                               lit<",">))) +
            lit<"}"> + with_constraints)> {};

  struct NamedType
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(ref<ID>) +
            assign<&ast::NamedType::type>(asn_type))> {};

  struct Component
      : rule<create<ast::Component>(
            (assign<&ast::Component::item>(ref<NamedType>) +
             option(flag<&ast::Component::optional_>(kw<"OPTIONAL">) |
                    (kw<"DEFAULT"> +
                     assign<&ast::Component::default_>(any_value)))) |
            (kw<"COMPONENTS"> + kw<"OF"> +
             assign<&ast::Component::componentsOf>(asn_type)))> {};

  struct SequenceExtension
      : rule<create<ast::ExtensionAddition>(
                 append<&ast::ExtensionAddition::items>(ref<Component>)) |
             create<ast::ExtensionAddition>(
                 flag<&ast::ExtensionAddition::grouped>(lit<"[[">) + option(assign<&ast::ExtensionAddition::version>(ref<NUMBER>) + lit<":">) +
                 many(append<&ast::ExtensionAddition::items>(
                ref<Component>),
            lit<",">) +
       lit<"]]">)> {};
  static constexpr auto seq_set_ext_item =
      append<&ast::SequenceType::extensions>(ref<SequenceExtension>);
  static constexpr auto seq_set_ext_tail =
      flag<&ast::SequenceType::extensible>(lit<"...">) +
      option(lit<","> + some(seq_set_ext_item, lit<",">));
  struct SequenceOrSetType
      : rule<create<ast::SequenceType>(
            assign<&ast::SequenceType::kind>(as<ast::SeqSetKind::sequence_>(kw<"SEQUENCE">) | as<ast::SeqSetKind::set_>(kw<"SET">)) +
            lit<"{"> +
            ((some(append<&ast::SequenceType::components>(ref<Component>),
                   lit<",">) +
              option(lit<","> + seq_set_ext_tail)) |
             option(seq_set_ext_tail)) +
            lit<"}"> + with_constraints)> {};

  struct SequenceOrSetOfType
      : rule<create<ast::SequenceOfType>(
            assign<&ast::SequenceOfType::kind>(as<ast::SeqSetOfKind::sequenceOf_>(kw<"SEQUENCE">) |
                                                as<ast::SeqSetOfKind::setOf_>(kw<"SET">)) +
            option(option(kw<"SIZE">) +
                   assign<&ast::SequenceOfType::constraint>(
                       ref<Constraint>)) +
            kw<"OF"> +
            (assign<&ast::SequenceOfType::element>(asn_type) |
             assign<&ast::SequenceOfType::namedElement>(ref<NamedType>)) +
            with_constraints)> {};

  struct ChoiceExtension
      : rule<create<ast::ExtensionAddition>(
                 append<&ast::ExtensionAddition::items>(ref<NamedType>)) |
             create<ast::ExtensionAddition>(
                 flag<&ast::ExtensionAddition::grouped>(lit<"[[">) + option(assign<&ast::ExtensionAddition::version>(ref<NUMBER>) + lit<":">) +
                 many(append<&ast::ExtensionAddition::items>(
                ref<NamedType>),
            lit<",">) +
       lit<"]]">)> {};
  static constexpr auto choice_ext_item =
      append<&ast::ChoiceType::extensions>(ref<ChoiceExtension>);
  struct ChoiceType
      : rule<create<ast::ChoiceType>(
            kw<"CHOICE"> + lit<"{"> +
            some(append<&ast::ChoiceType::alternatives>(ref<NamedType>),
                 lit<",">) +
            option(lit<","> +
                   flag<&ast::ChoiceType::extensible>(lit<"...">) +
                   option(lit<","> + some(choice_ext_item, lit<",">))) +
            lit<"}"> + with_constraints)> {};

  struct FieldName
      : rule<create<ast::FieldNameRef>(
            lit<"&"> + append<&ast::FieldNameRef::path>(ref<ID>) +
            many(lit<"."> + lit<"&"> +
                 append<&ast::FieldNameRef::path>(ref<ID>)))> {};
  struct DefinedObjectClass
      : rule<create<ast::DefinedObjectClassRef>(
            (option(assign<&ast::DefinedObjectClassRef::moduleRef>(
                        ref<ID>) +
                    lit<".">) +
             assign<&ast::DefinedObjectClassRef::name>(ref<ID>)) |
            flag<&ast::DefinedObjectClassRef::typeIdentifier>(
                kw<"TYPE-Identifier">) |
            flag<&ast::DefinedObjectClassRef::abstractSyntax>(
                kw<"ABSTRACT-SYNTAX">))> {};
  struct TypeIdentifierOrAbstractSyntax
      : rule<create<ast::DefinedObjectClassRef>(
            flag<&ast::DefinedObjectClassRef::typeIdentifier>(
                kw<"TYPE-Identifier">) |
            flag<&ast::DefinedObjectClassRef::abstractSyntax>(
                kw<"ABSTRACT-SYNTAX">))> {};
  struct ObjectClassFieldType
      : rule<create<ast::ObjectClassFieldType>(
            assign<&ast::ObjectClassFieldType::class_>(
                ref<DefinedObjectClass>) +
            lit<"."> +
            assign<&ast::ObjectClassFieldType::field>(ref<FieldName>) +
            with_constraints)> {};

  struct ActualParameter
      : rule<create<ast::ActualParameter>(
            assign<&ast::ActualParameter::asType>(asn_type) |
            assign<&ast::ActualParameter::asValue>(any_value))> {};
  struct ReferencedType
      : rule<create<ast::ReferencedType>(
            (option(assign<&ast::ReferencedType::moduleRef>(ref<ID>) +
                    lit<".">) +
             assign<&ast::ReferencedType::name>(ref<ID>)) +
            option(lit<"{"> +
                   append<&ast::ReferencedType::actualParameters>(
                       ref<ActualParameter>) +
                   many(lit<","> +
                        append<&ast::ReferencedType::actualParameters>(
                            ref<ActualParameter>)) +
                   lit<"}">) +
            with_constraints)> {};

  // --- object-class definitions (X.681) ------------------------------------

  struct SyntaxLiteral
      : rule<create<ast::SyntaxLiteral>(
            assign<&ast::SyntaxLiteral::text>(ref<ID>))> {};
  struct SyntaxFieldRef
      : rule<create<ast::SyntaxFieldRef>(
            lit<"&"> + assign<&ast::SyntaxFieldRef::name>(ref<ID>))> {};
  struct SyntaxOptionalGroup
      : rule<create<ast::SyntaxOptionalGroup>(
            lit<"["> +
            some(append<&ast::SyntaxOptionalGroup::items>(
                token_or_group_spec)) +
            lit<"]">)> {};

  // A bare governing type and a bare governing class both read as one
  // IDENTIFIER; the class-governed alternative here only
  // takes the two distinctive keyword forms, folding the bare-identifier
  // case into the type-governed alternative -- the same fold as
  // `ObjectClassAssignment` below.
  struct FieldSpec
      : rule<create<ast::FieldSpec>(
            lit<"&"> + assign<&ast::FieldSpec::name>(ref<ID>) +
            option(
                (assign<&ast::FieldSpec::fieldRef>(ref<FieldName>) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultValueSet>(
                           ref<ValueSet>) |
                       assign<&ast::FieldSpec::defaultValue>(any_value))))) |
                (assign<&ast::FieldSpec::classRef>(
                     ref<TypeIdentifierOrAbstractSyntax>) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultObjectSet>(
                           ref<ObjectSet>) |
                       assign<&ast::FieldSpec::defaultObject>(
                           ref<Object_>))))) |
                (assign<&ast::FieldSpec::type>(asn_type) +
                 option(flag<&ast::FieldSpec::unique_>(kw<"UNIQUE">)) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultValueSet>(
                           ref<ValueSet>) |
                       assign<&ast::FieldSpec::defaultObjectSet>(
                           ref<ObjectSet>) |
                       assign<&ast::FieldSpec::defaultValue>(any_value))))) |
                flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                (kw<"DEFAULT"> +
                 assign<&ast::FieldSpec::defaultType>(asn_type))))> {};

  struct ObjectClassDefn
      : rule<create<ast::ObjectClassDefn>(
            kw<"CLASS"> + lit<"{"> +
            append<&ast::ObjectClassDefn::fields>(ref<FieldSpec>) +
            many(lit<","> +
                 append<&ast::ObjectClassDefn::fields>(ref<FieldSpec>)) +
            lit<"}"> +
            option(kw<"WITH"> + kw<"SYNTAX"> + lit<"{"> +
                   some(append<&ast::ObjectClassDefn::syntax>(
                       token_or_group_spec)) +
                   lit<"}">))> {};

  struct Object_
      : rule<create<ast::ParameterizedObject>(
            (option(assign<&ast::ParameterizedObject::moduleRef>(ref<ID>) +
                    lit<".">) +
             assign<&ast::ParameterizedObject::name>(ref<ID>)) +
            option(lit<"{"> +
                   append<&ast::ParameterizedObject::actualParameters>(
                       ref<ActualParameter>) +
                   many(lit<","> +
                        append<&ast::ParameterizedObject::actualParameters>(
                            ref<ActualParameter>)) +
                   lit<"}">))> {};

  // --- module-level declarations -------------------------------------------

  // A governor written as a bare class name would read the same IDENTIFIER
  // as a referenced type; a parameter's governor is always read as a type
  // (see ast.hpp).
  struct Parameter
      : rule<create<ast::Parameter>(
            option(assign<&ast::Parameter::governorType>(asn_type) +
                   lit<":">) +
            assign<&ast::Parameter::name>(ref<ID>))> {};

  struct TypeAssignment
      : rule<create<ast::TypeAssignment>(
            assign<&ast::TypeAssignment::name>(ref<ID>) + lit<"::="> +
            assign<&ast::TypeAssignment::type>(asn_type))> {};
  struct ValueAssignment
      : rule<create<ast::ValueAssignment>(
            assign<&ast::ValueAssignment::name>(ref<ID>) +
            assign<&ast::ValueAssignment::type>(asn_type) + lit<"::="> +
            assign<&ast::ValueAssignment::value>(any_value))> {};
  struct ParameterizedAssignment
      : rule<create<ast::ParameterizedAssignment>(
            assign<&ast::ParameterizedAssignment::name>(ref<ID>) + lit<"{"> +
            append<&ast::ParameterizedAssignment::params>(ref<Parameter>) +
            many(lit<","> + append<&ast::ParameterizedAssignment::params>(
                                 ref<Parameter>)) +
            lit<"}"> + lit<"::="> +
            (assign<&ast::ParameterizedAssignment::asType>(asn_type) |
             assign<&ast::ParameterizedAssignment::asValue>(any_value) |
             assign<&ast::ParameterizedAssignment::asValueSet>(
                 ref<ValueSet>)))> {};
  // Only the two distinctive keyword forms of a defined object class: a
  // bare identifier here reads the same as a type assignment without a
  // symbol table, so it is read as one instead.
  struct ObjectClassAssignment
      : rule<create<ast::ObjectClassAssignment>(
            assign<&ast::ObjectClassAssignment::name>(ref<ID>) + lit<"::="> +
            assign<&ast::ObjectClassAssignment::value>(
                ref<ObjectClassDefn> |
                ref<TypeIdentifierOrAbstractSyntax>))> {};

  // Order matters only for readability: the four shapes are lexically
  // disjoint at the first token after the leading IDENTIFIER once the
  // folds above are applied.
  struct Assignment
      : rule<ref<ParameterizedAssignment> | ref<ValueAssignment> |
             ref<TypeAssignment> | ref<ObjectClassAssignment>> {};

  struct Symbol
      : rule<create<ast::Symbol>(assign<&ast::Symbol::name>(ref<ID>) +
                                  option(lit<"{"> + lit<"}">))> {};
  struct ImportsFromModule
      : rule<create<ast::ImportsFromModule>(
            append<&ast::ImportsFromModule::symbols>(ref<Symbol>) +
            many(lit<","> +
                 append<&ast::ImportsFromModule::symbols>(ref<Symbol>)) +
            kw<"FROM"> +
            assign<&ast::ImportsFromModule::moduleName>(ref<ID>))> {};


  static constexpr auto exports_clause =
      option(flag<&ast::Module::hasExports>(kw<"EXPORTS">) +
             (flag<&ast::Module::exportsAll>(kw<"ALL">) |
              option(append<&ast::Module::exports_>(ref<Symbol>) +
                     many(lit<","> +
                          append<&ast::Module::exports_>(ref<Symbol>)))) +
             lit<";">);
  static constexpr auto imports_clause =
      option(kw<"IMPORTS"> +
             option(append<&ast::Module::imports>(ref<ImportsFromModule>) +
                    many(append<&ast::Module::imports>(
                        ref<ImportsFromModule>))) +
             lit<";">);

  // The module header's `{ iso(1) member-body(2) }` object identifier is
  // parsed and discarded: it names the module, not its content.
  struct Module
      : rule<create<ast::Module>(
            assign<&ast::Module::name>(ref<ID>) +
            option(lit<"{"> + many(ref<ID> + lit<"("> + ref<NUMBER> + lit<")">) +
                   lit<"}">) +
            kw<"DEFINITIONS"> +
            option(assign<&ast::Module::tagDefault>(
                as<ast::TagMode::explicit_>(kw<"EXPLICIT"> + kw<"TAGS">) | as<ast::TagMode::implicit_>(kw<"IMPLICIT"> + kw<"TAGS">) |
                as<ast::TagMode::automatic_>(kw<"AUTOMATIC"> + kw<"TAGS">))) +
            option(flag<&ast::Module::extensibilityImplied>(
                kw<"EXTENSIBILITY"> + kw<"IMPLIED">)) +
            lit<"::="> + kw<"BEGIN"> + exports_clause + imports_clause +
            many(append<&ast::Module::assignments>(ref<Assignment>)) +
            kw<"END">)> {};

  struct Modules
      : rule<create<ast::Modules>(
            some(append<&ast::Modules::modules>(ref<Module>)))> {};

  using entry = Modules;
  using ignored = WS;
  using hidden = list<COMMENT, BLOCK_COMMENT>;
};

}  // namespace asn
