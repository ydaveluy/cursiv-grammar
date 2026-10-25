// The asn language: its grammar, for the reduced ASN.1 module syntax this
// port covers (X.680/681/682/683: no type tagging, no REAL/EXTERNAL/
// EMBEDDED PDV/CHARACTER STRING/INSTANCE OF). A bare
// IDENTIFIER is genuinely ambiguous in several ASN.1 productions; the
// engine is LR with GLR forks for the positions not folded below.
#pragma once

#include "asn/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace asn {

struct Asn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\f)">)> {};
  // No whitespace required before the newline: a comment with no trailing
  // newline (the last line of a file) still lexes as one token.
  struct COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">) + option(eol)> {};

  // ID's alphabet over-accepts a few scripts and includes
  // a bare hyphen as a letter, so a hyphen glued to digits (`-1`) lexes as
  // one IDENTIFIER, never as MINUS NUMBER.
  static constexpr auto word_boundary = uc::L | uc::Nd | cr<"_$-">;
  struct ID : terminal<(uc::L | cr<"_$-">) + many(uc::L | uc::Nd | cr<"_$-">)> {
  };

  struct NUMBER : terminal<some(d)> {};

  static constexpr auto escape_seq = lit<"\\"> + cr<R"(btnfr"'\\\\)">;
  struct CSTRING
      : terminal<lit<"\""> + many(escape_seq | cr<R"(^"\\\\)">) + lit<"\"">> {};
  struct BSTRING : terminal<lit<"'"> + many(cr<"01">) + lit<"'B">> {};
  // HSTRING joins CSTRING/BSTRING as a builtin value.
  struct HSTRING : terminal<lit<"'"> + many(cr<"0-9a-fA-F">) + lit<"'H">> {};

  // --- forward declarations --------------------------------------------
  struct ConstraintRule;
  struct NameValueRule;
  struct IntegerValueRule;
  struct BooleanValueRule;
  struct StringValueRule;
  struct ChoiceValueRule;
  struct ObjectIdentifierValueRule;
  struct NamedTypeRule;
  struct ComponentRule;
  struct SequenceOrSetTypeRule;
  struct SequenceOrSetOfTypeRule;
  struct ChoiceTypeRule;
  struct EnumeratedTypeRule;
  struct IntegerTypeRule;
  struct BitStringTypeRule;
  struct OctetStringTypeRule;
  struct ObjectIdTypeRule;
  struct RealTypeRule;
  struct BooleanTypeRule;
  struct NullTypeRule;
  struct ObjectClassFieldTypeRule;
  struct ReferencedTypeRule;
  struct ActualParameterRule;
  struct DefinedObjectClassRule;
  struct TypeIdentifierOrAbstractSyntaxRule;
  struct FieldNameRule;
  struct ValueSetRule;
  struct ObjectSetRule;
  struct Object_Rule;
  struct ObjectClassDefnRule;
  struct FieldSpecRule;
  struct SyntaxLiteralRule;
  struct SyntaxFieldRefRule;
  struct SyntaxOptionalGroupRule;
  struct ParameterRule;
  struct ElementSetSpecRule;
  struct IntersectionGroupRule;
  struct RangeElementRule;
  struct SizeElementRule;
  struct PatternElementRule;
  struct ValueElementRule;
  struct UserDefinedConstraintParamRule;
  struct AtNotationRule;
  struct ComponentPresenceRule;
  struct SymbolRule;
  struct ImportsFromModuleRule;
  struct AssignmentRule;

  // --- shared fragments (forward-declared refs only) ---------------------
  static constexpr auto any_value =
      ref<NameValueRule> | ref<IntegerValueRule> | ref<ChoiceValueRule> |
      ref<ObjectIdentifierValueRule> | ref<BooleanValueRule> |
      ref<StringValueRule>;

  static constexpr auto asn_type =
      ref<SequenceOrSetTypeRule> | ref<SequenceOrSetOfTypeRule> |
      ref<ChoiceTypeRule> | ref<EnumeratedTypeRule> | ref<IntegerTypeRule> |
      ref<BitStringTypeRule> | ref<OctetStringTypeRule> |
      ref<ObjectIdTypeRule> | ref<ObjectClassFieldTypeRule> |
      ref<BooleanTypeRule> | ref<NullTypeRule> | ref<RealTypeRule> |
      ref<ReferencedTypeRule>;

  static constexpr auto with_constraints =
      many(append<&ast::Type::constraints>(ref<ConstraintRule>));

  static constexpr auto subtype_elements =
      ref<RangeElementRule> | ref<SizeElementRule> | ref<PatternElementRule> |
      ref<ValueElementRule>;
  // An EXCEPT operand's own EXCEPT is accepted here: it reads through the
  // same rule as a top-level element, a slightly wider read.
  static constexpr auto exclusion_tail =
      option(kw<"EXCEPT"> + assign<&ast::Element::excluded>(subtype_elements));

  static constexpr auto token_or_group_spec =
      ref<SyntaxLiteralRule> | ref<SyntaxFieldRefRule> |
      ref<SyntaxOptionalGroupRule>;

  // --- values --------------------------------------------------------------

  // An enumeration value and a named integer both read a bare IDENTIFIER
  // and cannot be told apart without a symbol table: one
  // node covers both.
  struct NameValueRule
      : rule<create<ast::NameValue>(
            assign<&ast::NameValue::name>(ref<ID>))> {};
  // A signed number's leading MINUS is dead in practice: a hyphen glued to
  // digits already lexes as one IDENTIFIER (see `word_boundary` above), so
  // this only ever captures a plain NUMBER.
  struct IntegerValueRule
      : rule<create<ast::IntegerValue>(
            assign<&ast::IntegerValue::text>(ref<NUMBER>))> {};
  struct BooleanValueRule
      : rule<create<ast::BooleanValue>(
            flag<&ast::BooleanValue::value>(kw<"TRUE"> | kw<"true">) |
            (kw<"FALSE"> | kw<"false">))> {};
  struct StringValueRule
      : rule<create<ast::StringValue>(assign<&ast::StringValue::text>(
            ref<CSTRING> | ref<BSTRING> | ref<HSTRING>))> {};
  struct ChoiceValueRule
      : rule<create<ast::ChoiceValue>(
            assign<&ast::ChoiceValue::alternative>(ref<ID>) + lit<":"> +
            assign<&ast::ChoiceValue::inner>(any_value))> {};

  // An OID component's value may in principle be a dot-qualified,
  // parameterized defined value; its qualification and parameter list are
  // dropped, folding it into the plain-IDENTIFIER shape already covered
  // below.
  struct ObjIdComponentRule
      : rule<create<ast::ObjIdComponent>(
            assign<&ast::ObjIdComponent::value>(ref<NUMBER>) |
            (assign<&ast::ObjIdComponent::name>(ref<ID>) +
             option(lit<"("> +
                    assign<&ast::ObjIdComponent::value>(ref<NUMBER> |
                                                          ref<ID>) +
                    lit<")">)))> {};
  struct ObjectIdentifierValueRule
      : rule<create<ast::ObjectIdentifierValue>(
            lit<"{"> +
            some(append<&ast::ObjectIdentifierValue::components>(
                ref<ObjIdComponentRule>)) +
            lit<"}">)> {};

  // --- constraint element sets ---------------------------------------------

  struct RangeElementRule
      : rule<create<ast::RangeElement>(
            (assign<&ast::RangeElement::lower>(any_value) |
             flag<&ast::RangeElement::lowerIsMin>(kw<"MIN">)) +
            option(flag<&ast::RangeElement::lowerExclusive>(lit<"<">)) +
            lit<".."> +
            option(flag<&ast::RangeElement::upperExclusive>(lit<"<">)) +
            (assign<&ast::RangeElement::upper>(any_value) |
             flag<&ast::RangeElement::upperIsMax>(kw<"MAX">)) +
            exclusion_tail)> {};
  struct SizeElementRule
      : rule<create<ast::SizeElement>(
            kw<"SIZE"> + assign<&ast::SizeElement::size>(ref<ConstraintRule>) +
            exclusion_tail)> {};
  struct PatternElementRule
      : rule<create<ast::PatternElement>(
            kw<"PATTERN"> + assign<&ast::PatternElement::pattern>(any_value) +
            exclusion_tail)> {};
  struct ValueElementRule
      : rule<create<ast::ValueElement>(
            assign<&ast::ValueElement::value>(any_value) + exclusion_tail)> {
  };

  struct IntersectionGroupRule
      : rule<create<ast::IntersectionGroup>(
            append<&ast::IntersectionGroup::elements>(subtype_elements) +
            many((kw<"INTERSECTION"> | lit<"^">) +
                 append<&ast::IntersectionGroup::elements>(
                     subtype_elements)))> {};

  struct ElementSetSpecRule
      : rule<create<ast::ElementSetSpec>(
            (flag<&ast::ElementSetSpec::allExcept>(kw<"ALL">) +
             kw<"EXCEPT"> +
             assign<&ast::ElementSetSpec::allExceptOperand>(
                 subtype_elements)) |
            (append<&ast::ElementSetSpec::unions>(ref<IntersectionGroupRule>) +
             many((kw<"UNION"> | lit<"|">) +
                  append<&ast::ElementSetSpec::unions>(
                      ref<IntersectionGroupRule>)) +
             option(
                 lit<","> +
                 flag<&ast::ElementSetSpec::extensible>(lit<"...">) +
                 option(
                     lit<","> +
                     append<&ast::ElementSetSpec::additionalUnions>(
                         ref<IntersectionGroupRule>) +
                     many((kw<"UNION"> | lit<"|">) +
                          append<&ast::ElementSetSpec::additionalUnions>(
                              ref<IntersectionGroupRule>))))))> {};

  struct ValueSetRule
      : rule<create<ast::ValueSet>(
            lit<"{"> + assign<&ast::ValueSet::spec>(ref<ElementSetSpecRule>) +
            lit<"}">)> {};
  // An object set spec's bare-ELLIPSIS-with-no-root alternative (`{...}`
  // alone) is not ported: exceedingly rare, and `ElementSetSpecRule`
  // already requires at least one element.
  struct ObjectSetRule
      : rule<create<ast::ObjectSet>(
            lit<"{"> + assign<&ast::ObjectSet::spec>(ref<ElementSetSpecRule>) +
            lit<"}">)> {};

  // --- general constraints -----------------------------------------------

  struct UserDefinedConstraintParamRule
      : rule<create<ast::UserDefinedConstraintParam>(
            assign<&ast::UserDefinedConstraintParam::governorType>(
                asn_type) +
            option((lit<":"> +
                    assign<&ast::UserDefinedConstraintParam::value>(
                        any_value)) |
                   assign<&ast::UserDefinedConstraintParam::valueSet>(
                       ref<ValueSetRule>) |
                   assign<&ast::UserDefinedConstraintParam::object_>(
                       ref<Object_Rule>) |
                   assign<&ast::UserDefinedConstraintParam::objectSet>(
                       ref<ObjectSetRule>)))> {};

  // The at-notation's extra dots after `@.` are consumed and not counted:
  // `relative` alone says which form was used.
  struct AtNotationRule
      : rule<create<ast::AtNotation>(
            (flag<&ast::AtNotation::relative>(lit<"@.">) +
             many(lit<".">)) |
            lit<"@"> +
            append<&ast::AtNotation::path>(ref<ID>) +
            many(lit<"."> + append<&ast::AtNotation::path>(ref<ID>)))> {};

  struct ComponentPresenceRule
      : rule<create<ast::ComponentPresence>(
            assign<&ast::ComponentPresence::name>(ref<ID>) +
            (flag<&ast::ComponentPresence::present>(kw<"PRESENT">) |
             kw<"ABSENT">))> {};

  static constexpr auto component_presence_list =
      option(append<&ast::ConstraintSpec::presences>(ref<ComponentPresenceRule>) +
             many(lit<","> + append<&ast::ConstraintSpec::presences>(
                                  ref<ComponentPresenceRule>))) +
      option(
          lit<","> +
          flag<&ast::ConstraintSpec::presencesExtensible>(lit<"...">) +
          option(lit<","> +
                 append<&ast::ConstraintSpec::additionalPresences>(
                     ref<ComponentPresenceRule>) +
                 many(lit<","> +
                      append<&ast::ConstraintSpec::additionalPresences>(
                          ref<ComponentPresenceRule>))));

  // A component-relation constraint's object-set name drops its optional
  // DOT-qualification, using the same fold as `ObjIdComponentRule`.
  struct ConstraintRule
      : rule<create<ast::ConstraintSpec>(
            lit<"("> +
            (assign<&ast::ConstraintSpec::spec>(ref<ElementSetSpecRule>) |
             (kw<"CONSTRAINED"> + kw<"BY"> + lit<"{"> +
              append<&ast::ConstraintSpec::userDefined>(
                  ref<UserDefinedConstraintParamRule>) +
              many(lit<","> + append<&ast::ConstraintSpec::userDefined>(
                                   ref<UserDefinedConstraintParamRule>)) +
              lit<"}">) |
             (lit<"{"> +
              assign<&ast::ConstraintSpec::tableObjectSet>(ref<ID>) +
              lit<"}"> +
              option(lit<"{"> +
                     append<&ast::ConstraintSpec::tableAtNotations>(
                         ref<AtNotationRule>) +
                     many(lit<","> + append<&ast::ConstraintSpec::tableAtNotations>(
                                          ref<AtNotationRule>)) +
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

  struct BooleanTypeRule
      : rule<create<ast::BooleanType>(kw<"BOOLEAN"> + with_constraints)> {};
  struct NullTypeRule
      : rule<create<ast::NullType>(kw<"NULL"> + with_constraints)> {};
  // REAL: ported next to BOOLEAN/NULL, the two
  // bare-keyword types it sits beside in the lexer.
  struct RealTypeRule
      : rule<create<ast::RealType>(kw<"REAL"> + with_constraints)> {};
  struct OctetStringTypeRule
      : rule<create<ast::OctetStringType>(
            kw<"OCTET"> + kw<"STRING"> + with_constraints)> {};
  struct ObjectIdTypeRule
      : rule<create<ast::ObjectIdentifierType>(
            kw<"OBJECT"> + kw<"IDENTIFIER"> + with_constraints)> {};

  struct NamedNumberRule
      : rule<create<ast::NamedNumber>(
            assign<&ast::NamedNumber::name>(ref<ID>) + lit<"("> +
            assign<&ast::NamedNumber::value>(ref<NUMBER> | ref<ID>) +
            lit<")">)> {};
  struct IntegerTypeRule
      : rule<create<ast::IntegerType>(
            kw<"INTEGER"> +
            option(lit<"{"> +
                   some(append<&ast::IntegerType::namedNumbers>(
                            ref<NamedNumberRule>),
                        lit<",">) +
                   lit<"}">) +
            with_constraints)> {};

  struct NamedBitRule
      : rule<create<ast::NamedBit>(
            assign<&ast::NamedBit::name>(ref<ID>) + lit<"("> +
            assign<&ast::NamedBit::value>(ref<NUMBER> | ref<ID>) +
            lit<")">)> {};
  struct BitStringTypeRule
      : rule<create<ast::BitStringType>(
            kw<"BIT"> + kw<"STRING"> +
            option(lit<"{"> +
                   some(append<&ast::BitStringType::namedBits>(
                            ref<NamedBitRule>),
                        lit<",">) +
                   lit<"}">) +
            with_constraints)> {};

  // An ENUMERATED item's bare-value alternative is dropped: see ast.hpp's
  // `Enumerator` comment.
  struct EnumeratorItemRule
      : rule<create<ast::Enumerator>(
            assign<&ast::Enumerator::numbered>(ref<NamedNumberRule>) |
            assign<&ast::Enumerator::name>(ref<ID>))> {};
  struct EnumeratedTypeRule
      : rule<create<ast::EnumeratedType>(
            kw<"ENUMERATED"> + lit<"{"> +
            some(append<&ast::EnumeratedType::items>(ref<EnumeratorItemRule>),
                 lit<",">) +
            option(lit<","> +
                   flag<&ast::EnumeratedType::extensible>(lit<"...">) +
                   option(lit<","> +
                          some(append<&ast::EnumeratedType::additionalItems>(
                                   ref<EnumeratorItemRule>),
                               lit<",">))) +
            lit<"}"> + with_constraints)> {};

  struct NamedTypeRule
      : rule<create<ast::NamedType>(
            assign<&ast::NamedType::name>(ref<ID>) +
            assign<&ast::NamedType::type>(asn_type))> {};

  struct ComponentRule
      : rule<create<ast::Component>(
            (assign<&ast::Component::item>(ref<NamedTypeRule>) +
             option(flag<&ast::Component::optional_>(kw<"OPTIONAL">) |
                    (kw<"DEFAULT"> +
                     assign<&ast::Component::default_>(any_value)))) |
            (kw<"COMPONENTS"> + kw<"OF"> +
             assign<&ast::Component::componentsOf>(asn_type)))> {};

  struct KSequence
      : rule<kw<"SEQUENCE">, value<ast::SeqSetKind::sequence_>> {};
  struct KSet : rule<kw<"SET">, value<ast::SeqSetKind::set_>> {};
  struct SequenceExtensionRule
      : rule<create<ast::ExtensionAddition>(
                 append<&ast::ExtensionAddition::items>(ref<ComponentRule>)) |
             create<ast::ExtensionAddition>(
                 flag<&ast::ExtensionAddition::grouped>(lit<"[[">) + option(assign<&ast::ExtensionAddition::version>(ref<NUMBER>) + lit<":">) +
                 many(append<&ast::ExtensionAddition::items>(
                ref<ComponentRule>),
            lit<",">) +
       lit<"]]">)> {};
  static constexpr auto seq_set_ext_item =
      append<&ast::SequenceType::extensions>(ref<SequenceExtensionRule>);
  static constexpr auto seq_set_ext_tail =
      flag<&ast::SequenceType::extensible>(lit<"...">) +
      option(lit<","> + some(seq_set_ext_item, lit<",">));
  struct SequenceOrSetTypeRule
      : rule<create<ast::SequenceType>(
            assign<&ast::SequenceType::kind>(ref<KSequence> | ref<KSet>) +
            lit<"{"> +
            ((some(append<&ast::SequenceType::components>(ref<ComponentRule>),
                   lit<",">) +
              option(lit<","> + seq_set_ext_tail)) |
             option(seq_set_ext_tail)) +
            lit<"}"> + with_constraints)> {};

  struct KSequenceOf
      : rule<kw<"SEQUENCE">, value<ast::SeqSetOfKind::sequenceOf_>> {};
  struct KSetOf : rule<kw<"SET">, value<ast::SeqSetOfKind::setOf_>> {};
  struct SequenceOrSetOfTypeRule
      : rule<create<ast::SequenceOfType>(
            assign<&ast::SequenceOfType::kind>(ref<KSequenceOf> |
                                                ref<KSetOf>) +
            option(option(kw<"SIZE">) +
                   assign<&ast::SequenceOfType::constraint>(
                       ref<ConstraintRule>)) +
            kw<"OF"> +
            (assign<&ast::SequenceOfType::element>(asn_type) |
             assign<&ast::SequenceOfType::namedElement>(ref<NamedTypeRule>)) +
            with_constraints)> {};

  struct ChoiceExtensionRule
      : rule<create<ast::ExtensionAddition>(
                 append<&ast::ExtensionAddition::items>(ref<NamedTypeRule>)) |
             create<ast::ExtensionAddition>(
                 flag<&ast::ExtensionAddition::grouped>(lit<"[[">) + option(assign<&ast::ExtensionAddition::version>(ref<NUMBER>) + lit<":">) +
                 many(append<&ast::ExtensionAddition::items>(
                ref<NamedTypeRule>),
            lit<",">) +
       lit<"]]">)> {};
  static constexpr auto choice_ext_item =
      append<&ast::ChoiceType::extensions>(ref<ChoiceExtensionRule>);
  struct ChoiceTypeRule
      : rule<create<ast::ChoiceType>(
            kw<"CHOICE"> + lit<"{"> +
            some(append<&ast::ChoiceType::alternatives>(ref<NamedTypeRule>),
                 lit<",">) +
            option(lit<","> +
                   flag<&ast::ChoiceType::extensible>(lit<"...">) +
                   option(lit<","> + some(choice_ext_item, lit<",">))) +
            lit<"}"> + with_constraints)> {};

  struct FieldNameRule
      : rule<create<ast::FieldNameRef>(
            lit<"&"> + append<&ast::FieldNameRef::path>(ref<ID>) +
            many(lit<"."> + lit<"&"> +
                 append<&ast::FieldNameRef::path>(ref<ID>)))> {};
  struct DefinedObjectClassRule
      : rule<create<ast::DefinedObjectClassRef>(
            (option(assign<&ast::DefinedObjectClassRef::moduleRef>(
                        ref<ID>) +
                    lit<".">) +
             assign<&ast::DefinedObjectClassRef::name>(ref<ID>)) |
            flag<&ast::DefinedObjectClassRef::typeIdentifier>(
                kw<"TYPE-Identifier">) |
            flag<&ast::DefinedObjectClassRef::abstractSyntax>(
                kw<"ABSTRACT-SYNTAX">))> {};
  struct TypeIdentifierOrAbstractSyntaxRule
      : rule<create<ast::DefinedObjectClassRef>(
            flag<&ast::DefinedObjectClassRef::typeIdentifier>(
                kw<"TYPE-Identifier">) |
            flag<&ast::DefinedObjectClassRef::abstractSyntax>(
                kw<"ABSTRACT-SYNTAX">))> {};
  struct ObjectClassFieldTypeRule
      : rule<create<ast::ObjectClassFieldType>(
            assign<&ast::ObjectClassFieldType::class_>(
                ref<DefinedObjectClassRule>) +
            lit<"."> +
            assign<&ast::ObjectClassFieldType::field>(ref<FieldNameRule>) +
            with_constraints)> {};

  struct ActualParameterRule
      : rule<create<ast::ActualParameter>(
            assign<&ast::ActualParameter::asType>(asn_type) |
            assign<&ast::ActualParameter::asValue>(any_value))> {};
  struct ReferencedTypeRule
      : rule<create<ast::ReferencedType>(
            (option(assign<&ast::ReferencedType::moduleRef>(ref<ID>) +
                    lit<".">) +
             assign<&ast::ReferencedType::name>(ref<ID>)) +
            option(lit<"{"> +
                   append<&ast::ReferencedType::actualParameters>(
                       ref<ActualParameterRule>) +
                   many(lit<","> +
                        append<&ast::ReferencedType::actualParameters>(
                            ref<ActualParameterRule>)) +
                   lit<"}">) +
            with_constraints)> {};

  // --- object-class definitions (X.681) ------------------------------------

  struct SyntaxLiteralRule
      : rule<create<ast::SyntaxLiteral>(
            assign<&ast::SyntaxLiteral::text>(ref<ID>))> {};
  struct SyntaxFieldRefRule
      : rule<create<ast::SyntaxFieldRef>(
            lit<"&"> + assign<&ast::SyntaxFieldRef::name>(ref<ID>))> {};
  struct SyntaxOptionalGroupRule
      : rule<create<ast::SyntaxOptionalGroup>(
            lit<"["> +
            some(append<&ast::SyntaxOptionalGroup::items>(
                token_or_group_spec)) +
            lit<"]">)> {};

  // A bare governing type and a bare governing class both read as one
  // IDENTIFIER; the class-governed alternative here only
  // takes the two distinctive keyword forms, folding the bare-identifier
  // case into the type-governed alternative -- the same fold as
  // `ObjectClassAssignmentRule` below.
  struct FieldSpecRule
      : rule<create<ast::FieldSpec>(
            lit<"&"> + assign<&ast::FieldSpec::name>(ref<ID>) +
            option(
                (assign<&ast::FieldSpec::fieldRef>(ref<FieldNameRule>) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultValueSet>(
                           ref<ValueSetRule>) |
                       assign<&ast::FieldSpec::defaultValue>(any_value))))) |
                (assign<&ast::FieldSpec::classRef>(
                     ref<TypeIdentifierOrAbstractSyntaxRule>) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultObjectSet>(
                           ref<ObjectSetRule>) |
                       assign<&ast::FieldSpec::defaultObject>(
                           ref<Object_Rule>))))) |
                (assign<&ast::FieldSpec::type>(asn_type) +
                 option(flag<&ast::FieldSpec::unique_>(kw<"UNIQUE">)) +
                 option(
                     flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                     (kw<"DEFAULT"> +
                      (assign<&ast::FieldSpec::defaultValueSet>(
                           ref<ValueSetRule>) |
                       assign<&ast::FieldSpec::defaultObjectSet>(
                           ref<ObjectSetRule>) |
                       assign<&ast::FieldSpec::defaultValue>(any_value))))) |
                flag<&ast::FieldSpec::optional_>(kw<"OPTIONAL">) |
                (kw<"DEFAULT"> +
                 assign<&ast::FieldSpec::defaultType>(asn_type))))> {};

  struct ObjectClassDefnRule
      : rule<create<ast::ObjectClassDefn>(
            kw<"CLASS"> + lit<"{"> +
            append<&ast::ObjectClassDefn::fields>(ref<FieldSpecRule>) +
            many(lit<","> +
                 append<&ast::ObjectClassDefn::fields>(ref<FieldSpecRule>)) +
            lit<"}"> +
            option(kw<"WITH"> + kw<"SYNTAX"> + lit<"{"> +
                   some(append<&ast::ObjectClassDefn::syntax>(
                       token_or_group_spec)) +
                   lit<"}">))> {};

  struct Object_Rule
      : rule<create<ast::ParameterizedObject>(
            (option(assign<&ast::ParameterizedObject::moduleRef>(ref<ID>) +
                    lit<".">) +
             assign<&ast::ParameterizedObject::name>(ref<ID>)) +
            option(lit<"{"> +
                   append<&ast::ParameterizedObject::actualParameters>(
                       ref<ActualParameterRule>) +
                   many(lit<","> +
                        append<&ast::ParameterizedObject::actualParameters>(
                            ref<ActualParameterRule>)) +
                   lit<"}">))> {};

  // --- module-level declarations -------------------------------------------

  // A governor written as a bare class name would read the same IDENTIFIER
  // as a referenced type; a parameter's governor is always read as a type
  // (see ast.hpp).
  struct ParameterRule
      : rule<create<ast::Parameter>(
            option(assign<&ast::Parameter::governorType>(asn_type) +
                   lit<":">) +
            assign<&ast::Parameter::name>(ref<ID>))> {};

  struct TypeAssignmentRule
      : rule<create<ast::TypeAssignment>(
            assign<&ast::TypeAssignment::name>(ref<ID>) + lit<"::="> +
            assign<&ast::TypeAssignment::type>(asn_type))> {};
  struct ValueAssignmentRule
      : rule<create<ast::ValueAssignment>(
            assign<&ast::ValueAssignment::name>(ref<ID>) +
            assign<&ast::ValueAssignment::type>(asn_type) + lit<"::="> +
            assign<&ast::ValueAssignment::value>(any_value))> {};
  struct ParameterizedAssignmentRule
      : rule<create<ast::ParameterizedAssignment>(
            assign<&ast::ParameterizedAssignment::name>(ref<ID>) + lit<"{"> +
            append<&ast::ParameterizedAssignment::params>(ref<ParameterRule>) +
            many(lit<","> + append<&ast::ParameterizedAssignment::params>(
                                 ref<ParameterRule>)) +
            lit<"}"> + lit<"::="> +
            (assign<&ast::ParameterizedAssignment::asType>(asn_type) |
             assign<&ast::ParameterizedAssignment::asValue>(any_value) |
             assign<&ast::ParameterizedAssignment::asValueSet>(
                 ref<ValueSetRule>)))> {};
  // Only the two distinctive keyword forms of a defined object class: a
  // bare identifier here reads the same as a type assignment without a
  // symbol table, so it is read as one instead.
  struct ObjectClassAssignmentRule
      : rule<create<ast::ObjectClassAssignment>(
            assign<&ast::ObjectClassAssignment::name>(ref<ID>) + lit<"::="> +
            assign<&ast::ObjectClassAssignment::value>(
                ref<ObjectClassDefnRule> |
                ref<TypeIdentifierOrAbstractSyntaxRule>))> {};

  // Order matters only for readability: the four shapes are lexically
  // disjoint at the first token after the leading IDENTIFIER once the
  // folds above are applied.
  struct AssignmentRule
      : rule<ref<ParameterizedAssignmentRule> | ref<ValueAssignmentRule> |
             ref<TypeAssignmentRule> | ref<ObjectClassAssignmentRule>> {};

  struct SymbolRule
      : rule<create<ast::Symbol>(assign<&ast::Symbol::name>(ref<ID>) +
                                  option(lit<"{"> + lit<"}">))> {};
  struct ImportsFromModuleRule
      : rule<create<ast::ImportsFromModule>(
            append<&ast::ImportsFromModule::symbols>(ref<SymbolRule>) +
            many(lit<","> +
                 append<&ast::ImportsFromModule::symbols>(ref<SymbolRule>)) +
            kw<"FROM"> +
            assign<&ast::ImportsFromModule::moduleName>(ref<ID>))> {};

  struct KExplicitTags
      : rule<kw<"EXPLICIT"> + kw<"TAGS">, value<ast::TagMode::explicit_>> {};
  struct KImplicitTags
      : rule<kw<"IMPLICIT"> + kw<"TAGS">, value<ast::TagMode::implicit_>> {};
  struct KAutomaticTags
      : rule<kw<"AUTOMATIC"> + kw<"TAGS">, value<ast::TagMode::automatic_>> {
  };

  static constexpr auto exports_clause =
      option(flag<&ast::Module::hasExports>(kw<"EXPORTS">) +
             (flag<&ast::Module::exportsAll>(kw<"ALL">) |
              option(append<&ast::Module::exports_>(ref<SymbolRule>) +
                     many(lit<","> +
                          append<&ast::Module::exports_>(ref<SymbolRule>)))) +
             lit<";">);
  static constexpr auto imports_clause =
      option(kw<"IMPORTS"> +
             option(append<&ast::Module::imports>(ref<ImportsFromModuleRule>) +
                    many(append<&ast::Module::imports>(
                        ref<ImportsFromModuleRule>))) +
             lit<";">);

  // The module header's `{ iso(1) member-body(2) }` object identifier is
  // parsed and discarded: it names the module, not its content.
  struct ModuleRule
      : rule<create<ast::Module>(
            assign<&ast::Module::name>(ref<ID>) +
            option(lit<"{"> + many(ref<ID> + lit<"("> + ref<NUMBER> + lit<")">) +
                   lit<"}">) +
            kw<"DEFINITIONS"> +
            option(assign<&ast::Module::tagDefault>(
                ref<KExplicitTags> | ref<KImplicitTags> |
                ref<KAutomaticTags>)) +
            option(flag<&ast::Module::extensibilityImplied>(
                kw<"EXTENSIBILITY"> + kw<"IMPLIED">)) +
            lit<"::="> + kw<"BEGIN"> + exports_clause + imports_clause +
            many(append<&ast::Module::assignments>(ref<AssignmentRule>)) +
            kw<"END">)> {};

  struct ModulesRule
      : rule<create<ast::Modules>(
            some(append<&ast::Modules::modules>(ref<ModuleRule>)))> {};

  using entry = ModulesRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace asn
