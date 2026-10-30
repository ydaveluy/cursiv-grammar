#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "vhdl/core/ast.hpp"

namespace vhdl {

struct Vhdl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"--"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  static constexpr auto ReservedWord =
      ikw<"mod"> | ikw<"rem"> | ikw<"abs"> | ikw<"not"> | ikw<"access"> |
      ikw<"after"> | ikw<"alias"> | ikw<"all"> | ikw<"architecture"> | ikw<"array"> |
      ikw<"assert"> | ikw<"attribute"> | ikw<"begin"> | ikw<"block"> | ikw<"body"> |
      ikw<"buffer"> | ikw<"bus"> | ikw<"case"> | ikw<"component"> | ikw<"configuration"> |
      ikw<"constant"> | ikw<"disconnect"> | ikw<"downto"> | ikw<"else"> | ikw<"elsif"> |
      ikw<"end"> | ikw<"entity"> | ikw<"exit"> | ikw<"file"> | ikw<"for"> |
      ikw<"function"> | ikw<"generate"> | ikw<"generic"> | ikw<"guarded"> | ikw<"if"> |
      ikw<"in"> | ikw<"inout"> | ikw<"is"> | ikw<"label"> | ikw<"library"> |
      ikw<"linkage"> | ikw<"loop"> | ikw<"map"> | ikw<"new"> | ikw<"next"> |
      ikw<"null"> | ikw<"of"> | ikw<"on"> | ikw<"open"> | ikw<"others"> |
      ikw<"out"> | ikw<"package"> | ikw<"port"> | ikw<"procedure"> | ikw<"process"> |
      ikw<"range"> | ikw<"record"> | ikw<"register"> | ikw<"report"> | ikw<"return"> |
      ikw<"select"> | ikw<"severity"> | ikw<"signal"> | ikw<"subtype"> | ikw<"then"> |
      ikw<"to"> | ikw<"transport"> | ikw<"type"> | ikw<"units"> | ikw<"until"> |
      ikw<"use"> | ikw<"variable"> | ikw<"wait"> | ikw<"when"> | ikw<"while"> |
      ikw<"with"> | ikw<"and"> | ikw<"or"> | ikw<"xor"> | ikw<"nand"> |
      ikw<"nor"> | ikw<"xnor"> | ikw<"group"> | ikw<"impure"> | ikw<"inertial"> |
      ikw<"literal"> | ikw<"postponed"> | ikw<"pure"> | ikw<"reject"> | ikw<"shared"> |
      ikw<"unaffected"> | ikw<"sll"> | ikw<"sla"> | ikw<"sra"> | ikw<"srl"> |
      ikw<"rol"> | ikw<"ror"> | ikw<"protected"> | ikw<"assume"> | ikw<"context"> |
      ikw<"cover"> | ikw<"default"> | ikw<"force"> | ikw<"parameter"> | ikw<"property"> |
      ikw<"release"> | ikw<"restrict"> | ikw<"restrict_guarantee"> | ikw<"sequence"> | ikw<"inherit"> |
      ikw<"vmode"> | ikw<"vprop"> | ikw<"vunit">;
  struct BASIC_ID
      : terminal<!ReservedWord + cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};
  // A doubled backslash escapes a literal backslash inside the delimiters.
  struct EXT_ID : terminal<lit<"\\"> + some(lit<"\\\\"> | cr<R"(^\\)">) + lit<"\\">> {};

  struct STRING_LIT : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(^"\r\n)">) + lit<"\"">> {};
  struct CHAR_LIT : terminal<lit<"'"> + any + lit<"'">> {};

  static constexpr auto IntRun = d + many(cr<"0-9_">);
  static constexpr auto ExtDigitRun = cr<"0-9a-zA-Z"> + many(cr<"0-9a-zA-Z_">);
  static constexpr auto ExponentLit =
      cr<"eE"> + option(cr<"+-">) + IntRun + option(lit<"."> + IntRun);

  struct NUMBER
      : terminal<IntRun + option(lit<"."> + IntRun) + option(ExponentLit)> {};
  struct BASED_NUMBER
      : terminal<IntRun + lit<"#"> + ExtDigitRun +
                 option(lit<"."> + ExtDigitRun) + lit<"#"> +
                 option(ExponentLit)> {};

  struct BIT_STRING_LIT
      : terminal<option(IntRun) +
                 ((option(cr<"sSuU">) + cr<"bBoOxX">) | cr<"dD">) +
                 lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">> {};

  static constexpr auto Identifier = ref<BASIC_ID> | ref<EXT_ID>;
  static constexpr auto NameAtomText =
      ref<BASIC_ID> | ref<EXT_ID> | ref<STRING_LIT> | ref<CHAR_LIT>;

  // ---------------- expressions & names ----------------
  struct ExternalName;
  struct Signature;
  struct NameAtom
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(Identifier)) |
             create<ast::Literal>(assign<&ast::Literal::text>(
                 ref<STRING_LIT> | ref<CHAR_LIT>)) |
             ref<ExternalName>> {};

  struct OthersMarker
      : rule<create<ast::Ident>(assign<&ast::Ident::name>(ikw<"OTHERS">))> {};

  struct ExplicitRange;
  struct Expression;
  struct DiscreteRange;

  static constexpr auto Choice =
      ref<ExplicitRange> | ref<Expression> | ref<OthersMarker>;

  static constexpr auto AttrDesignatorSpelling =
      ref<BASIC_ID> | ref<EXT_ID> | ikw<"RANGE"> | ikw<"REVERSE_RANGE"> |
      ikw<"ACROSS"> | ikw<"THROUGH"> | ikw<"REFERENCE"> | ikw<"TOLERANCE"> |
      ikw<"SUBTYPE">;

  struct Assoc;

  static constexpr auto SelectionTail =
      nest<ast::SelectedName, &ast::SelectedName::prefix>(
          lit<"."> + (flag<&ast::SelectedName::all>(ikw<"ALL">) |
                      assign<&ast::SelectedName::suffix>(NameAtomText)));
  struct EntityName : rule<ref<NameAtom> + many(SelectionTail)> {};

  struct NameExpr
      : rule<ref<NameAtom> +
             many(
                 SelectionTail  //
                 | nest<ast::CallName, &ast::CallName::prefix>(
                       lit<"("> +
                       some(append<&ast::CallName::args>(ref<Assoc>),
                            lit<",">) +
                       lit<")">)  //
                 | nest<ast::AttributeName, &ast::AttributeName::prefix>(
                       option(assign<&ast::AttributeName::signature>(ref<Signature>)) +
                       lit<"'"> +
                       assign<&ast::AttributeName::designator>(
                           AttrDesignatorSpelling) +
                       option(lit<"("> +
                              assign<&ast::AttributeName::arg>(
                                  ref<Expression>) +
                              lit<")">))  //
                 | nest<ast::RangeConstraintName,
                        &ast::RangeConstraintName::prefix>(
                       ikw<"RANGE"> +
                       assign<&ast::RangeConstraintName::range>(
                           ref<DiscreteRange>)))> {};

  struct ResolutionIndication
      : rule<ref<NameExpr> |
             (lit<"("> + ref<ResolutionIndication> + lit<")">)> {};

  struct SubtypeIndication
      : rule<create<ast::SubtypeIndication>(
                 option(assign<&ast::SubtypeIndication::resolution>(
                     ref<ResolutionIndication>)) +
                 assign<&ast::SubtypeIndication::typeMark>(ref<NameExpr>))> {
  };

  struct ExternalRoot : rule<as<[](std::string_view text) { return text; }>(
                            lit<"."> | (lit<"@"> + Identifier + lit<".">) |
                            some(lit<"^"> + lit<".">))> {};
  struct ExternalName
      : rule<create<ast::ExternalName>(
                 lit<"<<"> + assign<&ast::ExternalName::kind>(
                     ikw<"CONSTANT"> | ikw<"SIGNAL"> | ikw<"VARIABLE">) +
                 option(assign<&ast::ExternalName::root>(ref<ExternalRoot>)) +
                 assign<&ast::ExternalName::path>(ref<NameExpr>) + lit<":"> +
                 assign<&ast::ExternalName::subtype>(ref<SubtypeIndication>) +
                 lit<">>">)> {};

  struct ExplicitRange
      : rule<create<ast::RangeExpr>(
                 assign<&ast::RangeExpr::low>(ref<Expression>) +
                 assign<&ast::RangeExpr::direction>(ikw<"TO"> |
                                                     ikw<"DOWNTO">) +
                 assign<&ast::RangeExpr::high>(ref<Expression>))> {};

  struct DiscreteRange : rule<ref<ExplicitRange> | ref<SubtypeIndication>> {};

  struct OpenMarker : rule<create<ast::OpenMark> + ikw<"OPEN">> {};

  struct Assoc
      : rule<create<ast::Assoc>(
                 option(assign<&ast::Assoc::formal>(ref<NameExpr>) +
                        lit<"=>">) +
                 (assign<&ast::Assoc::actual>(ref<OpenMarker>)  //
                  | assign<&ast::Assoc::actual>(ref<ExplicitRange>)  //
                  | (option(flag<&ast::Assoc::inertial>(
                        ikw<"INERTIAL">)) +
                     assign<&ast::Assoc::actual>(ref<Expression>))))> {};

  struct ElementAssoc
      : rule<create<ast::ElementAssoc>(
                 option(some(append<&ast::ElementAssoc::choices>(Choice),
                             lit<"|">) +
                        lit<"=>">) +
                 assign<&ast::ElementAssoc::value>(ref<Expression>))> {};

  struct Aggregate
      : rule<create<ast::Aggregate>(
                 lit<"("> +
                 some(append<&ast::Aggregate::elements>(ref<ElementAssoc>),
                      lit<",">) +
                 lit<")">)> {};

  struct QualifiedExpr
      : rule<create<ast::QualifiedExpr>(
                 assign<&ast::QualifiedExpr::typeMark>(ref<NameExpr>) +
                 lit<"'"> +
                 assign<&ast::QualifiedExpr::value>(ref<Aggregate>))> {};

  struct AllocatorExpr
      : rule<create<ast::AllocatorExpr>(
                 ikw<"NEW"> +
                 assign<&ast::AllocatorExpr::target>(
                     ref<QualifiedExpr> | ref<SubtypeIndication>))> {};

  struct NumericLiteral
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::text>(ref<BASED_NUMBER> |
                                              ref<NUMBER>) +
                 option(!gap::none + &Identifier + assign<&ast::Literal::unit>(ref<NameExpr>)))> {};

  struct BitStrLiteral
      : rule<create<ast::Literal>(
                 assign<&ast::Literal::text>(ref<BIT_STRING_LIT>))> {};

  struct NullLiteral
      : rule<create<ast::Literal>(assign<&ast::Literal::text>(ikw<"NULL">))> {
  };

  struct Primary : rule<ref<NumericLiteral>  //
                        | ref<BitStrLiteral>  //
                        | ref<NullLiteral>    //
                        | ref<Aggregate>      //
                        | ref<QualifiedExpr>  //
                        | ref<AllocatorExpr>  //
                        | ref<NameExpr>> {};

  static constexpr auto OpLogical = ikw<"AND"> | ikw<"OR"> | ikw<"NAND"> |
                                    ikw<"NOR"> | ikw<"XOR"> | ikw<"XNOR">;

  struct UnaryFactor
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(ikw<"ABS"> | ikw<"NOT"> |
                                             OpLogical) +
                 assign<&ast::UnaryExpr::operand>(ref<Primary>))  //
             | ref<Primary>> {};

  struct PowerExpr
      : rule<ref<UnaryFactor> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(lit<"**">) +
                 assign<&ast::BinaryExpr::right>(ref<Primary>)))> {};

  static constexpr auto OpMul = lit<"*"> | lit<"/"> | ikw<"MOD"> | ikw<"REM">;
  static constexpr auto OpAdd = lit<"+"> | lit<"-"> | lit<"&">;
  static constexpr auto OpShift = ikw<"SLL"> | ikw<"SRL"> | ikw<"SLA"> |
                                  ikw<"SRA"> | ikw<"ROL"> | ikw<"ROR">;
  static constexpr auto OpRel =
      lit<"?<="> | lit<"?>="> | lit<"?/="> | lit<"?="> | lit<"?<"> |
      lit<"?>"> | lit<"/="> | lit<"<="> | lit<">="> | lit<"="> | lit<"<"> |
      lit<">">;

  struct Term : infix<PowerExpr, ast::BinaryExpr,
                       left(OpMul)> {};
  struct SignedTerm
      : rule<create<ast::UnaryExpr>(
                 assign<&ast::UnaryExpr::op>(lit<"+"> | lit<"-">) +
                 assign<&ast::UnaryExpr::operand>(ref<Term>)) | ref<Term>> {};
  struct SimpleExpr
      : rule<ref<SignedTerm> +
             many(nest<ast::BinaryExpr, &ast::BinaryExpr::left>(
                 assign<&ast::BinaryExpr::op>(OpAdd) +
                 assign<&ast::BinaryExpr::right>(ref<Term>)))> {};
  struct ShiftExpr : infix<SimpleExpr, ast::BinaryExpr,
                            left(OpShift)> {};
  struct RelationExpr : infix<ShiftExpr, ast::BinaryExpr,
                               left(OpRel)> {};
  struct LogicalExpr : infix<RelationExpr, ast::BinaryExpr,
                              left(OpLogical)> {};

  struct Expression
      : rule<create<ast::CondOpExpr>(
                 lit<"??"> +
                 assign<&ast::CondOpExpr::operand>(ref<Primary>))  //
             | ref<LogicalExpr>> {};

  struct CondExprEntry
      : rule<create<ast::CondExprEntry>(
                 assign<&ast::CondExprEntry::value>(ref<Expression>) +
                 ikw<"WHEN"> +
                 assign<&ast::CondExprEntry::condition>(ref<Expression>))> {};

  struct ConditionalExpr
      : rule<create<ast::ConditionalExpr>(
                 append<&ast::ConditionalExpr::entries>(
                     ref<CondExprEntry>) +
                 many(ikw<"ELSE"> +
                      append<&ast::ConditionalExpr::entries>(
                          ref<CondExprEntry>)) +
                 option(ikw<"ELSE"> +
                        assign<&ast::ConditionalExpr::elseValue>(
                            ref<Expression>)))> {};

  struct SelExprEntry
      : rule<create<ast::SelExprEntry>(
                 assign<&ast::SelExprEntry::value>(ref<Expression>) +
                 ikw<"WHEN"> +
                 some(append<&ast::SelExprEntry::choices>(Choice),
                      lit<"|">))> {};

  struct SelectedExpr
      : rule<create<ast::SelectedExpr>(
                 some(append<&ast::SelectedExpr::entries>(
                          ref<SelExprEntry>),
                      lit<",">))> {};

  // ---------------- waveforms & delay ----------------
  struct WaveformElement
      : rule<create<ast::WaveformElement>(
                 (flag<&ast::WaveformElement::isNull>(ikw<"NULL">) |
                  assign<&ast::WaveformElement::value>(ref<Expression>)) +
                 option(ikw<"AFTER"> +
                        assign<&ast::WaveformElement::afterTime>(
                            ref<Expression>)))> {};

  struct Waveform
      : rule<create<ast::Waveform>(
                 flag<&ast::Waveform::unaffected>(ikw<"UNAFFECTED">) |
                 some(append<&ast::Waveform::elements>(
                          ref<WaveformElement>),
                      lit<",">))> {};

  struct DelayMechanism
      : rule<create<ast::DelayMechanism>(
                 flag<&ast::DelayMechanism::transport>(
                     ikw<"TRANSPORT">) |
                 (option(ikw<"REJECT"> +
                         assign<&ast::DelayMechanism::rejectExpr>(
                             ref<Expression>)) +
                  flag<&ast::DelayMechanism::inertial>(
                      ikw<"INERTIAL">)))> {};

  // The WHEN is optional on the first entry alone: a plain (non-conditional)
  // waveform is this same shape with one unconditioned entry.
  struct CondWaveEntry
      : rule<create<ast::CondWaveEntry>(
                 assign<&ast::CondWaveEntry::wave>(ref<Waveform>) +
                 option(ikw<"WHEN"> +
                        assign<&ast::CondWaveEntry::condition>(
                            ref<Expression>)))> {};

  struct CondWaveAssign
      : rule<create<ast::CondWaveAssign>(
                 some(append<&ast::CondWaveAssign::entries>(
                          ref<CondWaveEntry>),
                      ikw<"ELSE">))> {};

  struct SelWaveEntry
      : rule<create<ast::SelWaveEntry>(
                 assign<&ast::SelWaveEntry::wave>(ref<Waveform>) +
                 ikw<"WHEN"> +
                 some(append<&ast::SelWaveEntry::choices>(Choice),
                      lit<"|">))> {};

  struct SelectedWaveforms
      : rule<create<ast::SelectedWaveforms>(
                 some(append<&ast::SelectedWaveforms::entries>(
                          ref<SelWaveEntry>),
                      lit<",">))> {};

  // ---------------- declarative items ----------------
  struct DeclarativeItem;
  struct SequentialStatement;
  struct ConcurrentStatement;

  struct LibraryClause
      : rule<create<ast::LibraryClause>(
                 ikw<"LIBRARY"> +
                 some(append<&ast::LibraryClause::names>(Identifier),
                      lit<",">) +
                 lit<";">)> {};

  struct SelectedNameText
      : rule<ref<NameExpr>> {};

  struct UseClause
      : rule<create<ast::UseClause>(
                 ikw<"USE"> +
                 some(append<&ast::UseClause::names>(ref<SelectedNameText>),
                      lit<",">) +
                 lit<";">)> {};

  struct ContextRef
      : rule<create<ast::ContextRef>(
                 ikw<"CONTEXT"> +
                 some(append<&ast::ContextRef::names>(ref<SelectedNameText>),
                      lit<",">) +
                 lit<";">)> {};

  static constexpr auto ModeKw = ikw<"IN"> | ikw<"OUT"> | ikw<"INOUT"> |
                                 ikw<"BUFFER"> | ikw<"LINKAGE">;

  struct InterfaceObjectDecl
      : rule<create<ast::InterfaceObject>(
                 option(assign<&ast::InterfaceObject::kind>(
                     ikw<"CONSTANT"> | ikw<"SIGNAL"> | ikw<"VARIABLE"> |
                     ikw<"FILE">)) +
                 some(append<&ast::InterfaceObject::names>(Identifier),
                      lit<",">) +
                 lit<":"> + option(assign<&ast::InterfaceObject::mode>(ModeKw)) +
                 assign<&ast::InterfaceObject::subtype>(ref<SubtypeIndication>) +
                 option(flag<&ast::InterfaceObject::bus>(ikw<"BUS">)) +
                 option(lit<":="> +
                        assign<&ast::InterfaceObject::defaultValue>(
                            ref<Expression>)))> {};

  struct InterfaceType
      : rule<create<ast::InterfaceType>(
                 ikw<"TYPE"> +
                 assign<&ast::InterfaceType::name>(Identifier))> {};

  struct InterfaceDeclaration;

  struct InterfaceSubprogram
      : rule<create<ast::InterfaceSubprogram>(
                 ((assign<&ast::InterfaceSubprogram::kind>(ikw<"PROCEDURE">))  //
                  | (option(flag<&ast::InterfaceSubprogram::pure>(
                         ikw<"PURE">) |
                     flag<&ast::InterfaceSubprogram::impure>(
                         ikw<"IMPURE">)) +
                     assign<&ast::InterfaceSubprogram::kind>(
                         ikw<"FUNCTION">))) +
                 assign<&ast::InterfaceSubprogram::name>(NameAtomText) +
                 option(option(ikw<"PARAMETER">) + lit<"("> +
                        some(append<&ast::InterfaceSubprogram::params>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")">) +
                 option(ikw<"RETURN"> +
                        assign<&ast::InterfaceSubprogram::returnType>(
                            ref<NameExpr>)) +
                 option(ikw<"IS"> +
                        (flag<&ast::InterfaceSubprogram::defaultBox>(
                             lit<"<>">) |
                         assign<&ast::InterfaceSubprogram::defaultName>(
                             ref<NameExpr>))))> {};

  struct GenericMapAspect;

  struct InterfacePackage
      : rule<create<ast::InterfacePackage>(
                 ikw<"PACKAGE"> +
                 assign<&ast::InterfacePackage::name>(Identifier) +
                 ikw<"IS"> + ikw<"NEW"> +
                 assign<&ast::InterfacePackage::uninstantiated>(ref<NameExpr>) +
                 ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                 (flag<&ast::InterfacePackage::genericBox>(lit<"<>">) |
                  flag<&ast::InterfacePackage::genericDefault>(
                      ikw<"DEFAULT">) |
                  some(append<&ast::InterfacePackage::generics>(
                           ref<Assoc>),
                       lit<",">)) +
                 lit<")">)> {};

  struct InterfaceDeclaration
      : rule<ref<InterfaceSubprogram>  //
            | ref<InterfacePackage>    //
            | ref<InterfaceType>       //
            | ref<InterfaceObjectDecl>> {};

  struct EnumTypeDef
      : rule<create<ast::EnumTypeDef>(
                 lit<"("> +
                 some(append<&ast::EnumTypeDef::literals>(
                          ref<NameAtom>),
                      lit<",">) +
                 lit<")">)> {};

  struct RangeTypeDef
      : rule<create<ast::RangeTypeDef>(
                 ikw<"RANGE"> +
                 assign<&ast::RangeTypeDef::range>(ref<DiscreteRange>))> {};

  struct UnitDecl
      : rule<create<ast::UnitDecl>(
                 assign<&ast::UnitDecl::name>(Identifier) + lit<";">)> {};

  struct SecondaryUnitDecl
      : rule<create<ast::UnitDecl>(
                 assign<&ast::UnitDecl::name>(Identifier) + lit<"="> +
                 assign<&ast::UnitDecl::value>(ref<NumericLiteral>) +
                 lit<";">)> {};

  struct PhysicalTypeDef
      : rule<create<ast::PhysicalTypeDef>(
                 ikw<"RANGE"> +
                 assign<&ast::PhysicalTypeDef::range>(ref<DiscreteRange>) +
                 ikw<"UNITS"> +
                 append<&ast::PhysicalTypeDef::units>(ref<UnitDecl>) +
                 many(append<&ast::PhysicalTypeDef::units>(
                     ref<SecondaryUnitDecl>)) +
                 ikw<"END"> + ikw<"UNITS"> + option(assign<&ast::Item::endName>(Identifier)))> {};

  struct ScalarTypeDef
      : rule<ref<EnumTypeDef> | ref<PhysicalTypeDef> | ref<RangeTypeDef>> {};

  struct UnboundedArrayDef
      : rule<create<ast::ArrayTypeDef>(
                 flag<&ast::ArrayTypeDef::unbounded>(ikw<"ARRAY">) +
                 lit<"("> +
                 some(append<&ast::ArrayTypeDef::indexTypes>(ref<NameExpr>) +
                          ikw<"RANGE"> + lit<"<>">,
                      lit<",">) +
                 lit<")"> + ikw<"OF"> +
                 assign<&ast::ArrayTypeDef::element>(ref<SubtypeIndication>))> {
  };

  struct ConstrainedArrayDef
      : rule<create<ast::ArrayTypeDef>(
                 ikw<"ARRAY"> + lit<"("> +
                 some(append<&ast::ArrayTypeDef::indexRanges>(
                          ref<DiscreteRange>),
                      lit<",">) +
                 lit<")"> + ikw<"OF"> +
                 assign<&ast::ArrayTypeDef::element>(ref<SubtypeIndication>))> {
  };

  struct ArrayTypeDef
      : rule<ref<UnboundedArrayDef> | ref<ConstrainedArrayDef>> {};

  struct RecordElementDecl
      : rule<create<ast::RecordElementDecl>(
                 some(append<&ast::RecordElementDecl::names>(Identifier),
                      lit<",">) +
                 lit<":"> +
                 assign<&ast::RecordElementDecl::subtype>(
                     ref<SubtypeIndication>) +
                 lit<";">)> {};

  struct RecordTypeDef
      : rule<create<ast::RecordTypeDef>(
                 ikw<"RECORD"> +
                 some(append<&ast::RecordTypeDef::elements>(
                     ref<RecordElementDecl>)) +
                 ikw<"END"> + ikw<"RECORD"> + option(assign<&ast::Item::endName>(Identifier)))> {};

  struct CompositeTypeDef : rule<ref<ArrayTypeDef> | ref<RecordTypeDef>> {};

  struct AccessTypeDef
      : rule<create<ast::AccessTypeDef>(
                 ikw<"ACCESS"> +
                 assign<&ast::AccessTypeDef::subtype>(
                     ref<SubtypeIndication>))> {};

  struct FileTypeDef
      : rule<create<ast::FileTypeDef>(
                 ikw<"FILE"> + ikw<"OF"> +
                 assign<&ast::FileTypeDef::subtype>(
                     ref<SubtypeIndication>))> {};

  struct ProtectedTypeBody
      : rule<create<ast::ProtectedTypeDef>(
                 flag<&ast::ProtectedTypeDef::isBody>(ikw<"BODY">) +
                 many(append<&ast::ProtectedTypeDef::decls>(
                     ref<DeclarativeItem>)) +
                 ikw<"END"> + ikw<"PROTECTED"> + ikw<"BODY"> +
                 option(assign<&ast::Item::endName>(Identifier)))> {};

  struct ProtectedTypeDecl
      : rule<create<ast::ProtectedTypeDef>(
                 many(append<&ast::ProtectedTypeDef::decls>(
                     ref<DeclarativeItem>)) +
                 ikw<"END"> + ikw<"PROTECTED"> + option(assign<&ast::Item::endName>(Identifier)))> {};

  struct ProtectedTypeDef
      : rule<ikw<"PROTECTED"> +
             (ref<ProtectedTypeBody> | ref<ProtectedTypeDecl>)> {};

  struct TypeDefinition
      : rule<ref<ScalarTypeDef>      //
            | ref<CompositeTypeDef>  //
            | ref<AccessTypeDef>     //
            | ref<FileTypeDef>       //
            | ref<ProtectedTypeDef>> {};

  struct TypeDecl
      : rule<create<ast::TypeDecl>(
                 ikw<"TYPE"> + assign<&ast::TypeDecl::name>(Identifier) +
                 option(ikw<"IS"> +
                        assign<&ast::TypeDecl::definition>(
                            ref<TypeDefinition>)) +
                 lit<";">)> {};

  struct SubtypeDecl
      : rule<create<ast::SubtypeDecl>(
                 ikw<"SUBTYPE"> + assign<&ast::SubtypeDecl::name>(Identifier) +
                 ikw<"IS"> +
                 assign<&ast::SubtypeDecl::subtype>(ref<SubtypeIndication>) +
                 lit<";">)> {};

  struct FileOpenInfo
      : rule<create<ast::FileOpenInfo>(
                 option(ikw<"OPEN"> +
                        assign<&ast::FileOpenInfo::openKind>(
                            ref<Expression>)) +
                 ikw<"IS"> +
                 assign<&ast::FileOpenInfo::logicalName>(ref<Expression>))> {
  };

  struct ObjectDecl
      : rule<create<ast::ObjectDecl>(
                 (assign<&ast::ObjectDecl::kind>(ikw<"CONSTANT">) |
                  assign<&ast::ObjectDecl::kind>(ikw<"SIGNAL">) |
                  (option(flag<&ast::ObjectDecl::shared>(
                       ikw<"SHARED">)) +
                   assign<&ast::ObjectDecl::kind>(ikw<"VARIABLE">)) |
                  assign<&ast::ObjectDecl::kind>(ikw<"FILE">)) +
                 some(append<&ast::ObjectDecl::names>(Identifier), lit<",">) +
                 lit<":"> +
                 assign<&ast::ObjectDecl::subtype>(ref<SubtypeIndication>) +
                 option(assign<&ast::ObjectDecl::signalKind>(
                     ikw<"REGISTER"> | ikw<"BUS">)) +
                 option(assign<&ast::ObjectDecl::fileOpen>(
                     ref<FileOpenInfo>)) +
                 option(lit<":="> +
                        assign<&ast::ObjectDecl::defaultValue>(
                            ref<Expression>)) +
                 lit<";">)> {};

  struct Signature
      : rule<create<ast::Signature>(
                 lit<"["> +
                 many(append<&ast::Signature::params>(ref<NameExpr>), lit<",">) +
                 option(ikw<"RETURN"> +
                        assign<&ast::Signature::returnType>(ref<NameExpr>)) +
                 lit<"]">)> {};

  struct AliasDecl
      : rule<create<ast::AliasDecl>(
                 ikw<"ALIAS"> + assign<&ast::AliasDecl::name>(NameAtomText) +
                 option(lit<":"> +
                        assign<&ast::AliasDecl::subtype>(
                            ref<SubtypeIndication>)) +
                 ikw<"IS"> + assign<&ast::AliasDecl::target>(ref<NameExpr>) +
                 option(assign<&ast::AliasDecl::signature>(
                     ref<Signature>)) +
                 lit<";">)> {};

  struct ComponentDecl
      : rule<create<ast::ComponentDecl>(
                 ikw<"COMPONENT"> +
                 assign<&ast::ComponentDecl::name>(Identifier) +
                 option(ikw<"IS">) +
                 option(ikw<"GENERIC"> + lit<"("> +
                        some(append<&ast::ComponentDecl::generics>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";">) +
                 option(ikw<"PORT"> + lit<"("> +
                        some(append<&ast::ComponentDecl::ports>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";">) +
                 ikw<"END"> + ikw<"COMPONENT"> + option(assign<&ast::Item::endName>(Identifier)) +
                 lit<";">)> {};

  struct AttributeDecl
      : rule<create<ast::AttributeDecl>(
                 ikw<"ATTRIBUTE"> +
                 assign<&ast::AttributeDecl::name>(Identifier) + lit<":"> +
                 assign<&ast::AttributeDecl::typeMark>(ref<NameExpr>) +
                 lit<";">)> {};

  static constexpr auto EntityClass =
      ikw<"ENTITY"> | ikw<"ARCHITECTURE"> | ikw<"CONFIGURATION"> |
      ikw<"PROCEDURE"> | ikw<"FUNCTION"> | ikw<"PACKAGE"> | ikw<"TYPE"> |
      ikw<"SUBTYPE"> | ikw<"CONSTANT"> | ikw<"SIGNAL"> | ikw<"VARIABLE"> |
      ikw<"COMPONENT"> | ikw<"LABEL"> | ikw<"LITERAL"> | ikw<"UNITS"> |
      ikw<"GROUP"> | ikw<"FILE">;

  struct EntityDesignator
      : rule<assign<&ast::EntityDesignator::name>(NameAtomText) +
             option(assign<&ast::EntityDesignator::signature>(ref<Signature>))> {};

  struct AttributeSpec
      : rule<create<ast::AttributeSpec>(
                 ikw<"ATTRIBUTE"> +
                 assign<&ast::AttributeSpec::designator>(
                     AttrDesignatorSpelling) +
                 ikw<"OF"> +
                 (flag<&ast::AttributeSpec::othersEntities>(
                      ikw<"OTHERS">) |
                  flag<&ast::AttributeSpec::allEntities>(ikw<"ALL">) |
                  some(append<&ast::AttributeSpec::entities>(ref<EntityDesignator>),
                       lit<",">)) +
                 lit<":"> +
                 assign<&ast::AttributeSpec::entityClass>(EntityClass) +
                 ikw<"IS"> +
                 assign<&ast::AttributeSpec::value>(ref<Expression>) +
                 lit<";">)> {};

  static constexpr auto EntityAspect =
      (assign<&ast::BindingIndication::aspectKind>(ikw<"ENTITY">) +
       assign<&ast::BindingIndication::aspectName>(ref<EntityName>) +
       option(lit<"("> + assign<&ast::BindingIndication::archId>(Identifier) +
              lit<")">))  //
      | (assign<&ast::BindingIndication::aspectKind>(ikw<"CONFIGURATION">) +
         assign<&ast::BindingIndication::aspectName>(ref<EntityName>))  //
      | assign<&ast::BindingIndication::aspectKind>(ikw<"OPEN">);

  struct BindingIndication
      : rule<create<ast::BindingIndication>(
                 option(ikw<"USE"> + EntityAspect) +
                 option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::BindingIndication::generics>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">) +
                 option(ikw<"PORT"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::BindingIndication::ports>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">))> {};

  struct ConfigSpec
      : rule<create<ast::ConfigSpec>(
                 ikw<"FOR"> +
                 (flag<&ast::ConfigSpec::allInstances>(ikw<"ALL">) |
                  flag<&ast::ConfigSpec::othersInstances>(
                      ikw<"OTHERS">) |
                  some(append<&ast::ConfigSpec::instances>(Identifier),
                       lit<",">)) +
                 lit<":"> +
                 assign<&ast::ConfigSpec::componentName>(ref<NameExpr>) +
                 assign<&ast::ConfigSpec::binding>(
                     ref<BindingIndication>) +
                 lit<";">)> {};

  struct DisconnectSpec
      : rule<create<ast::DisconnectSpec>(
                 ikw<"DISCONNECT"> +
                 (flag<&ast::DisconnectSpec::allSignals>(ikw<"ALL">) |
                  flag<&ast::DisconnectSpec::othersSignals>(
                      ikw<"OTHERS">) |
                  some(append<&ast::DisconnectSpec::signals>(ref<NameExpr>),
                       lit<",">)) +
                 lit<":"> +
                 assign<&ast::DisconnectSpec::typeMark>(ref<NameExpr>) +
                 ikw<"AFTER"> +
                 assign<&ast::DisconnectSpec::afterTime>(ref<Expression>) +
                 lit<";">)> {};

  struct GroupTemplateEntry
      : rule<create<ast::GroupTemplateEntry>(
                 assign<&ast::GroupTemplateEntry::entityClass>(
                     EntityClass) +
                 option(flag<&ast::GroupTemplateEntry::repeated>(
                     lit<"<>">)))> {};

  struct GroupTemplateDecl
      : rule<create<ast::GroupTemplateDecl>(
                 ikw<"GROUP"> +
                 assign<&ast::GroupTemplateDecl::name>(Identifier) +
                 ikw<"IS"> + lit<"("> +
                 some(append<&ast::GroupTemplateDecl::entries>(
                          ref<GroupTemplateEntry>),
                      lit<",">) +
                 lit<")"> + lit<";">)> {};

  struct GroupDecl
      : rule<create<ast::GroupDecl>(
                 ikw<"GROUP"> + assign<&ast::GroupDecl::name>(Identifier) +
                 lit<":"> +
                 assign<&ast::GroupDecl::templateName>(ref<NameExpr>) +
                 lit<"("> +
                 some(append<&ast::GroupDecl::constituents>(ref<NameExpr>),
                      lit<",">) +
                 lit<")"> + lit<";">)> {};

  static constexpr auto SubprogramHeader =
      option(ikw<"GENERIC"> + lit<"("> +
             some(append<&ast::SubprogramSpec::headerGenerics>(
                      ref<InterfaceDeclaration>),
                  lit<";">) +
             lit<")"> +
             option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                    some(append<&ast::SubprogramSpec::headerGenericMap>(
                             ref<Assoc>),
                         lit<",">) +
                    lit<")">));

  struct SubprogramSpec
      : rule<create<ast::SubprogramSpec>(
                 option(flag<&ast::SubprogramSpec::pure>(ikw<"PURE">) |
                        flag<&ast::SubprogramSpec::impure>(ikw<"IMPURE">)) +
                 assign<&ast::SubprogramSpec::kind>(ikw<"PROCEDURE"> | ikw<"FUNCTION">) +
                 assign<&ast::SubprogramSpec::name>(NameAtomText) + SubprogramHeader +
                 option(option(ikw<"PARAMETER">) + lit<"("> +
                        many(append<&ast::SubprogramSpec::params>(ref<InterfaceDeclaration>),
                             lit<";">) + lit<")">) +
                 option(ikw<"RETURN"> +
                        assign<&ast::SubprogramSpec::returnType>(ref<NameExpr>)))> {};

  struct SubprogramDecl
      : rule<create<ast::SubprogramDecl>(
                 assign<&ast::SubprogramDecl::spec>(ref<SubprogramSpec>) +
                 lit<";">)> {};

  struct SubprogramBody
      : rule<create<ast::SubprogramBody>(
                 assign<&ast::SubprogramBody::spec>(ref<SubprogramSpec>) +
                 ikw<"IS"> +
                 many(append<&ast::SubprogramBody::decls>(
                     ref<DeclarativeItem>)) +
                 ikw<"BEGIN"> +
                 many(append<&ast::SubprogramBody::statements>(
                     ref<SequentialStatement>)) +
                 ikw<"END"> + option(assign<&ast::SubprogramBody::endKind>(ikw<"PROCEDURE"> | ikw<"FUNCTION">)) +
                 option(assign<&ast::Item::endName>(NameAtomText)) + lit<";">)> {};

  struct SubprogramInstDecl
      : rule<create<ast::SubprogramInstDecl>(
                 assign<&ast::SubprogramInstDecl::kind>(ikw<"PROCEDURE"> |
                                                        ikw<"FUNCTION">) +
                 assign<&ast::SubprogramInstDecl::name>(Identifier) +
                 ikw<"IS"> + ikw<"NEW"> +
                 assign<&ast::SubprogramInstDecl::uninstantiated>(
                     ref<NameExpr>) +
                 option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::SubprogramInstDecl::generics>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">) +
                 lit<";">)> {};

  struct PackageDecl;
  struct PackageBody;
  struct PackageInstDecl;

  struct PackageDecl
      : rule<create<ast::PackageDecl>(
                 ikw<"PACKAGE"> + assign<&ast::PackageDecl::name>(Identifier) +
                 ikw<"IS"> +
                 option(ikw<"GENERIC"> + lit<"("> +
                        some(append<&ast::PackageDecl::headerGenerics>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";"> +
                        option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                               some(append<&ast::PackageDecl::headerGenericMap>(
                                        ref<Assoc>),
                                    lit<",">) +
                               lit<")"> + lit<";">)) +
                 many(append<&ast::PackageDecl::decls>(ref<DeclarativeItem>)) +
                 ikw<"END"> + option(ikw<"PACKAGE">) +
                 option(assign<&ast::Item::endName>(Identifier)) + lit<";">)> {};

  struct PackageBody
      : rule<create<ast::PackageBody>(
                 ikw<"PACKAGE"> + ikw<"BODY"> +
                 assign<&ast::PackageBody::name>(Identifier) + ikw<"IS"> +
                 many(append<&ast::PackageBody::decls>(ref<DeclarativeItem>)) +
                 ikw<"END"> + option(ikw<"PACKAGE"> + ikw<"BODY">) +
                 option(assign<&ast::Item::endName>(Identifier)) + lit<";">)> {};

  // A package instantiation's generic map is optional, one pair of
  // parentheses around the association list -- matching
  // `interface_package_generic_map_aspect` elsewhere in this grammar.
  struct PackageInstDecl
      : rule<create<ast::PackageInstDecl>(
                 ikw<"PACKAGE"> +
                 assign<&ast::PackageInstDecl::name>(Identifier) + ikw<"IS"> +
                 ikw<"NEW"> +
                 assign<&ast::PackageInstDecl::uninstantiated>(ref<NameExpr>) +
                 option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::PackageInstDecl::generics>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">) +
                 lit<";">)> {};

  struct DeclarativeItem
      : rule<ref<SubprogramBody>       //
            | ref<SubprogramDecl>      //
            | ref<SubprogramInstDecl>  //
            | ref<PackageInstDecl>     //
            | ref<PackageBody>         //
            | ref<PackageDecl>         //
            | ref<TypeDecl>            //
            | ref<SubtypeDecl>         //
            | ref<ObjectDecl>          //
            | ref<AliasDecl>           //
            | ref<ComponentDecl>       //
            | ref<AttributeSpec>       //
            | ref<AttributeDecl>       //
            | ref<ConfigSpec>          //
            | ref<DisconnectSpec>      //
            | ref<UseClause>           //
            | ref<GroupTemplateDecl>   //
            | ref<GroupDecl>> {};

  // ---------------- concurrent & sequential statements ----------------
  struct SeqIfBranch
      : rule<create<ast::IfBranch>(
                 assign<&ast::IfBranch::condition>(ref<Expression>) +
                 ikw<"THEN"> +
                 many(append<&ast::IfBranch::body>(ref<SequentialStatement>)))> {
  };

  struct IfStatement
      : rule<create<ast::IfStmt>(
                 option(assign<&ast::IfStmt::label>(Identifier) + lit<":">) +
                 ikw<"IF"> +
                 append<&ast::IfStmt::branches>(ref<SeqIfBranch>) +
                 many(ikw<"ELSIF"> +
                      append<&ast::IfStmt::branches>(ref<SeqIfBranch>)) +
                 option(ikw<"ELSE"> +
                        many(append<&ast::IfStmt::elseBody>(
                            ref<SequentialStatement>))) +
                 ikw<"END"> + ikw<"IF"> + option(assign<&ast::SeqStmt::endLabel>(Identifier)) + lit<";">)> {};

  struct CaseAlt
      : rule<create<ast::CaseAlt>(
                 ikw<"WHEN"> +
                 some(append<&ast::CaseAlt::choices>(Choice),
                      lit<"|">) +
                 lit<"=>"> +
                 many(append<&ast::CaseAlt::body>(ref<SequentialStatement>)))> {
  };

  struct CaseStatement
      : rule<create<ast::CaseStmt>(
                 option(assign<&ast::CaseStmt::label>(Identifier) + lit<":">) +
                 ikw<"CASE">
                 + option(flag<&ast::CaseStmt::questioned>(lit<"?">)) +
                 assign<&ast::CaseStmt::selector>(ref<Expression>) +
                 ikw<"IS"> +
                 some(append<&ast::CaseStmt::alternatives>(ref<CaseAlt>)) +
                 ikw<"END"> + ikw<"CASE"> +
                 option(flag<&ast::CaseStmt::endQuestioned>(lit<"?">)) +
                 option(assign<&ast::SeqStmt::endLabel>(Identifier)) + lit<";">)> {};

  static constexpr auto IterationScheme =
      (assign<&ast::LoopStmt::kind>(ikw<"WHILE">) +
       assign<&ast::LoopStmt::whileCond>(ref<Expression>))  //
      | (assign<&ast::LoopStmt::kind>(ikw<"FOR">) +
         assign<&ast::LoopStmt::forParamName>(Identifier) + ikw<"IN"> +
         assign<&ast::LoopStmt::forRange>(ref<DiscreteRange>));

  struct LoopStatement
      : rule<create<ast::LoopStmt>(
                 option(assign<&ast::LoopStmt::label>(Identifier) + lit<":">) +
                 option(IterationScheme) + ikw<"LOOP"> +
                 many(append<&ast::LoopStmt::body>(ref<SequentialStatement>)) +
                 ikw<"END"> + ikw<"LOOP"> + option(assign<&ast::SeqStmt::endLabel>(Identifier)) + lit<";">)> {};

  struct WaitStatement
      : rule<create<ast::WaitStmt>(
                 option(assign<&ast::WaitStmt::label>(Identifier) + lit<":">) +
                 ikw<"WAIT"> +
                 option(ikw<"ON"> +
                        some(append<&ast::WaitStmt::sensitivity>(
                                 ref<NameExpr>),
                             lit<",">)) +
                 option(ikw<"UNTIL"> +
                        assign<&ast::WaitStmt::untilCondition>(
                            ref<Expression>)) +
                 option(ikw<"FOR"> +
                        assign<&ast::WaitStmt::forExpr>(ref<Expression>)) +
                 lit<";">)> {};

  struct AssertStatement
      : rule<create<ast::AssertStmt>(
                 option(assign<&ast::AssertStmt::label>(Identifier) +
                        lit<":">) +
                 ikw<"ASSERT"> +
                 assign<&ast::AssertStmt::condition>(ref<Expression>) +
                 option(ikw<"REPORT"> +
                        assign<&ast::AssertStmt::report>(ref<Expression>)) +
                 option(ikw<"SEVERITY"> +
                        assign<&ast::AssertStmt::severity>(ref<Expression>)) +
                 lit<";">)> {};

  struct ReportStatement
      : rule<create<ast::ReportStmt>(
                 option(assign<&ast::ReportStmt::label>(Identifier) +
                        lit<":">) +
                 ikw<"REPORT"> +
                 assign<&ast::ReportStmt::report>(ref<Expression>) +
                 option(ikw<"SEVERITY"> +
                        assign<&ast::ReportStmt::severity>(ref<Expression>)) +
                 lit<";">)> {};

  struct SeqSignalAssignStatement
      : rule<create<ast::SeqSignalAssign>(
                 option(assign<&ast::SeqSignalAssign::label>(Identifier) +
                        lit<":">) +
                 option(ikw<"WITH"> +
                        assign<&ast::SeqSignalAssign::selector>(
                            ref<Expression>) +
                        ikw<"SELECT"> +
                        option(flag<&ast::SeqSignalAssign::questioned>(lit<"?">))) +
                 assign<&ast::SeqSignalAssign::target>(ref<NameExpr> |
                                                       ref<Aggregate>) +
                 lit<"<="> +
                 ((flag<&ast::SeqSignalAssign::force>(ikw<"FORCE">) +
                   option(assign<&ast::SeqSignalAssign::forceMode>(
                       ikw<"IN"> | ikw<"OUT">)) +
                   assign<&ast::SeqSignalAssign::forceValue>(
                       ref<ConditionalExpr> | ref<SelectedExpr> |
                       ref<Expression>))  //
                  | (flag<&ast::SeqSignalAssign::release>(
                         ikw<"RELEASE">) +
                     option(assign<&ast::SeqSignalAssign::forceMode>(
                         ikw<"IN"> | ikw<"OUT">)))  //
                  | (option(assign<&ast::SeqSignalAssign::delay>(
                         ref<DelayMechanism>)) +
                     (assign<&ast::SeqSignalAssign::selWave>(
                          ref<SelectedWaveforms>) |
                      assign<&ast::SeqSignalAssign::condWave>(
                          ref<CondWaveAssign>)))) +
                 lit<";">)> {};

  struct SeqVarAssignStatement
      : rule<create<ast::SeqVarAssign>(
                 option(assign<&ast::SeqVarAssign::label>(Identifier) +
                        lit<":">) +
                 option(ikw<"WITH"> +
                        assign<&ast::SeqVarAssign::selector>(
                            ref<Expression>) +
                        ikw<"SELECT"> +
                        option(flag<&ast::SeqVarAssign::questioned>(lit<"?">))) +
                 assign<&ast::SeqVarAssign::target>(ref<NameExpr> |
                                                    ref<Aggregate>) +
                 lit<":="> +
                 (assign<&ast::SeqVarAssign::selExpr>(ref<SelectedExpr>) |
                  assign<&ast::SeqVarAssign::condExpr>(ref<ConditionalExpr>) |
                  assign<&ast::SeqVarAssign::simpleValue>(ref<Expression>)) +
                 lit<";">)> {};

  struct ProcCallStatement
      : rule<create<ast::ProcCallStmt>(
                 option(assign<&ast::ProcCallStmt::label>(Identifier) +
                        lit<":">) +
                 assign<&ast::ProcCallStmt::call>(ref<NameExpr>) + lit<";">)> {
  };

  struct NextStatement
      : rule<create<ast::NextStmt>(
                 option(assign<&ast::NextStmt::label>(Identifier) + lit<":">) +
                 ikw<"NEXT"> + option(assign<&ast::NextStmt::target>(
                                          Identifier)) +
                 option(ikw<"WHEN"> +
                        assign<&ast::NextStmt::condition>(ref<Expression>)) +
                 lit<";">)> {};

  struct ExitStatement
      : rule<create<ast::ExitStmt>(
                 option(assign<&ast::ExitStmt::label>(Identifier) + lit<":">) +
                 ikw<"EXIT"> + option(assign<&ast::ExitStmt::target>(
                                          Identifier)) +
                 option(ikw<"WHEN"> +
                        assign<&ast::ExitStmt::condition>(ref<Expression>)) +
                 lit<";">)> {};

  struct ReturnStatement
      : rule<create<ast::ReturnStmt>(
                 option(assign<&ast::ReturnStmt::label>(Identifier) +
                        lit<":">) +
                 ikw<"RETURN"> +
                 option(assign<&ast::ReturnStmt::value>(ref<Expression>)) +
                 lit<";">)> {};

  struct NullStatement
      : rule<create<ast::NullStmt>(
                 option(assign<&ast::NullStmt::label>(Identifier) + lit<":">) +
                 ikw<"NULL"> + lit<";">)> {};

  struct SequentialStatement
      : rule<ref<WaitStatement>              //
            | ref<AssertStatement>           //
            | ref<ReportStatement>           //
            | ref<IfStatement>               //
            | ref<CaseStatement>              //
            | ref<LoopStatement>              //
            | ref<NextStatement>              //
            | ref<ExitStatement>              //
            | ref<ReturnStatement>            //
            | ref<NullStatement>              //
            | ref<SeqSignalAssignStatement>   //
            | ref<SeqVarAssignStatement>      //
            | ref<ProcCallStatement>> {};

  static constexpr auto SensitivityListOrAll =
      flag<&ast::ProcessStmt::allSensitivity>(ikw<"ALL">)  //
      | some(append<&ast::ProcessStmt::sensitivity>(ref<NameExpr>), lit<",">);

  struct ProcessStatement
      : rule<create<ast::ProcessStmt>(
                 option(assign<&ast::ProcessStmt::label>(Identifier) +
                        lit<":">) +
                 option(flag<&ast::ProcessStmt::postponed>(
                     ikw<"POSTPONED">)) +
                 ikw<"PROCESS"> +
                 option(lit<"("> + SensitivityListOrAll + lit<")">) +
                 option(ikw<"IS">) +
                 many(append<&ast::ProcessStmt::decls>(ref<DeclarativeItem>)) +
                 ikw<"BEGIN"> +
                 many(append<&ast::ProcessStmt::statements>(
                     ref<SequentialStatement>)) +
                 ikw<"END"> + option(flag<&ast::ProcessStmt::endPostponed>(ikw<"POSTPONED">)) + ikw<"PROCESS"> +
                 option(assign<&ast::ConcStmt::endLabel>(Identifier)) + lit<";">)> {};

  struct ConcProcCallStatement
      : rule<create<ast::ConcProcCall>(
                 option(assign<&ast::ConcProcCall::label>(Identifier) +
                        lit<":">) +
                 option(flag<&ast::ConcProcCall::postponed>(
                     ikw<"POSTPONED">)) +
                 assign<&ast::ConcProcCall::call>(ref<NameExpr>) + lit<";">)> {
  };

  struct ConcAssertStatement
      : rule<create<ast::ConcAssert>(
                 option(assign<&ast::ConcAssert::label>(Identifier) +
                        lit<":">) +
                 option(flag<&ast::ConcAssert::postponed>(
                     ikw<"POSTPONED">)) +
                 ikw<"ASSERT"> +
                 assign<&ast::ConcAssert::condition>(ref<Expression>) +
                 option(ikw<"REPORT"> +
                        assign<&ast::ConcAssert::report>(ref<Expression>)) +
                 option(ikw<"SEVERITY"> +
                        assign<&ast::ConcAssert::severity>(ref<Expression>)) +
                 lit<";">)> {};

  struct ConcSignalAssignStatement
      : rule<create<ast::SignalAssign>(
                 option(assign<&ast::SignalAssign::label>(Identifier) +
                        lit<":">) +
                 option(flag<&ast::SignalAssign::postponed>(
                     ikw<"POSTPONED">)) +
                 option(ikw<"WITH"> +
                        assign<&ast::SignalAssign::selector>(
                            ref<Expression>) +
                        ikw<"SELECT"> +
                        option(flag<&ast::SignalAssign::questioned>(lit<"?">))) +
                 assign<&ast::SignalAssign::target>(ref<NameExpr> |
                                                    ref<Aggregate>) +
                 lit<"<="> +
                 option(flag<&ast::SignalAssign::guarded>(
                     ikw<"GUARDED">)) +
                 ((flag<&ast::SignalAssign::force>(ikw<"FORCE">) +
                   option(assign<&ast::SignalAssign::forceMode>(
                       ikw<"IN"> | ikw<"OUT">)) +
                   assign<&ast::SignalAssign::forceValue>(
                       ref<ConditionalExpr> | ref<SelectedExpr> |
                       ref<Expression>))  //
                  | (option(assign<&ast::SignalAssign::delay>(
                         ref<DelayMechanism>)) +
                     (assign<&ast::SignalAssign::selWave>(
                          ref<SelectedWaveforms>) |
                      assign<&ast::SignalAssign::condWave>(
                          ref<CondWaveAssign>)))) +
                 lit<";">)> {};

  static constexpr auto InstantiatedUnit =
      (option(assign<&ast::ComponentInst::kind>(ikw<"COMPONENT">)) +
       assign<&ast::ComponentInst::name>(ref<EntityName>))  //
      | (assign<&ast::ComponentInst::kind>(ikw<"ENTITY">) +
         assign<&ast::ComponentInst::name>(ref<EntityName>) +
         option(lit<"("> + assign<&ast::ComponentInst::archId>(Identifier) +
                lit<")">))  //
      | (assign<&ast::ComponentInst::kind>(ikw<"CONFIGURATION">) +
         assign<&ast::ComponentInst::name>(ref<EntityName>));

  struct ComponentInstStatement
      : rule<create<ast::ComponentInst>(
                 assign<&ast::ComponentInst::label>(Identifier) + lit<":"> +
                 InstantiatedUnit +
                 option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::ComponentInst::generics>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">) +
                 option(ikw<"PORT"> + ikw<"MAP"> + lit<"("> +
                        some(append<&ast::ComponentInst::ports>(
                                 ref<Assoc>),
                             lit<",">) +
                        lit<")">) +
                 lit<";">)> {};

  struct GenBody
      : rule<create<ast::GenBody>(
                 option(many(append<&ast::GenBody::decls>(ref<DeclarativeItem>)) +
                        ikw<"BEGIN">) +
                 many(append<&ast::GenBody::statements>(ref<ConcurrentStatement>)) +
                 option(ikw<"END"> + !ikw<"GENERATE"> +
                        option(assign<&ast::Item::endName>(Identifier)) + lit<";">))> {};

  struct GenerateIfBranch
      : rule<create<ast::GenerateBranch>(
                 option(assign<&ast::GenerateBranch::label>(Identifier) + lit<":">) +
                 assign<&ast::GenerateBranch::condition>(ref<Expression>) +
                 ikw<"GENERATE"> + assign<&ast::GenerateBranch::body>(ref<GenBody>))> {};
  struct GenerateElseBranch
      : rule<create<ast::GenerateBranch>(
                 option(assign<&ast::GenerateBranch::label>(Identifier) + lit<":">) +
                 ikw<"GENERATE"> + assign<&ast::GenerateBranch::body>(ref<GenBody>))> {};

  static constexpr auto ForGenerateStatement =
      ikw<"FOR"> + assign<&ast::GenerateStmt::paramName>(Identifier) +
      ikw<"IN"> +
      assign<&ast::GenerateStmt::paramRange>(ref<DiscreteRange>) +
      ikw<"GENERATE"> + assign<&ast::GenerateStmt::body>(ref<GenBody>);

  static constexpr auto IfGenerateStatement =
      ikw<"IF"> + append<&ast::GenerateStmt::branches>(ref<GenerateIfBranch>) +
      many(ikw<"ELSIF"> + append<&ast::GenerateStmt::branches>(ref<GenerateIfBranch>)) +
      option(ikw<"ELSE"> + append<&ast::GenerateStmt::branches>(ref<GenerateElseBranch>));

  struct CaseGenAlt
      : rule<create<ast::CaseGenAlt>(
                 ikw<"WHEN"> +
                 option(assign<&ast::CaseGenAlt::label>(Identifier) + lit<":">) +
                 some(append<&ast::CaseGenAlt::choices>(Choice),
                      lit<"|">) +
                 lit<"=>"> +
                 assign<&ast::CaseGenAlt::body>(ref<GenBody>))> {};

  static constexpr auto CaseGenerateStatement =
      ikw<"CASE"> + assign<&ast::GenerateStmt::selector>(ref<Expression>) +
      ikw<"GENERATE"> +
      some(append<&ast::GenerateStmt::caseAlts>(ref<CaseGenAlt>));

  struct GenerateStatement
      : rule<create<ast::GenerateStmt>(
                 option(assign<&ast::GenerateStmt::label>(Identifier) +
                        lit<":">) +
                 (ForGenerateStatement       //
                  | IfGenerateStatement      //
                  | CaseGenerateStatement) +
                 ikw<"END"> + ikw<"GENERATE"> + option(assign<&ast::ConcStmt::endLabel>(Identifier)) +
                 lit<";">)> {};

  struct BlockStatement
      : rule<create<ast::BlockStmt>(
                 assign<&ast::BlockStmt::label>(Identifier) + lit<":"> +
                 ikw<"BLOCK"> +
                 option(lit<"("> +
                        assign<&ast::BlockStmt::guardExpr>(ref<Expression>) +
                        lit<")">) +
                 option(ikw<"IS">) +
                 option(ikw<"GENERIC"> + lit<"("> +
                        some(append<&ast::BlockStmt::generics>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";"> +
                        option(ikw<"GENERIC"> + ikw<"MAP"> + lit<"("> +
                               some(append<&ast::BlockStmt::genericMap>(
                                        ref<Assoc>),
                                    lit<",">) +
                               lit<")"> + lit<";">)) +
                 option(ikw<"PORT"> + lit<"("> +
                        some(append<&ast::BlockStmt::ports>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";"> +
                        option(ikw<"PORT"> + ikw<"MAP"> + lit<"("> +
                               some(append<&ast::BlockStmt::portMap>(
                                        ref<Assoc>),
                                    lit<",">) +
                               lit<")"> + lit<";">)) +
                 many(append<&ast::BlockStmt::decls>(ref<DeclarativeItem>)) +
                 ikw<"BEGIN"> +
                 many(append<&ast::BlockStmt::statements>(
                     ref<ConcurrentStatement>)) +
                 ikw<"END"> + ikw<"BLOCK"> + option(assign<&ast::ConcStmt::endLabel>(Identifier)) + lit<";">)> {
  };

  struct ConcurrentStatement
      : rule<ref<BlockStatement>              //
            | ref<ProcessStatement>           //
            | ref<ComponentInstStatement>     //
            | ref<GenerateStatement>          //
            | ref<ConcAssertStatement>        //
            | ref<ConcProcCallStatement>      //
            | ref<ConcSignalAssignStatement>> {};

  // ---------------- design units ----------------
  struct ContextItem
      : rule<ref<LibraryClause> | ref<UseClause> | ref<ContextRef>> {};

  struct ContextDecl
      : rule<create<ast::ContextDecl>(
                 ikw<"CONTEXT"> + assign<&ast::ContextDecl::name>(Identifier) +
                 ikw<"IS"> +
                 many(append<&ast::ContextDecl::context>(
                     ref<ContextItem>)) +
                 ikw<"END"> + option(ikw<"CONTEXT">) + option(assign<&ast::Item::endName>(Identifier)) +
                 lit<";">)> {};

  struct EntityDecl
      : rule<create<ast::EntityDecl>(
                 ikw<"ENTITY"> + assign<&ast::EntityDecl::name>(Identifier) +
                 ikw<"IS"> +
                 option(ikw<"GENERIC"> + lit<"("> +
                        some(append<&ast::EntityDecl::generics>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";">) +
                 option(ikw<"PORT"> + lit<"("> +
                        some(append<&ast::EntityDecl::ports>(
                                 ref<InterfaceDeclaration>),
                             lit<";">) +
                        lit<")"> + lit<";">) +
                 many(append<&ast::EntityDecl::decls>(ref<DeclarativeItem>)) +
                 option(ikw<"BEGIN"> +
                        many(append<&ast::EntityDecl::statements>(
                            ref<ConcurrentStatement>))) +
                 ikw<"END"> + option(ikw<"ENTITY">) + option(assign<&ast::Item::endName>(Identifier)) +
                 lit<";">)> {};

  struct ArchitectureBody
      : rule<create<ast::ArchitectureBody>(
                 ikw<"ARCHITECTURE"> +
                 assign<&ast::ArchitectureBody::name>(Identifier) +
                 ikw<"OF"> +
                 assign<&ast::ArchitectureBody::entityName>(ref<NameExpr>) +
                 ikw<"IS"> +
                 many(append<&ast::ArchitectureBody::decls>(
                     ref<DeclarativeItem>)) +
                 ikw<"BEGIN"> +
                 many(append<&ast::ArchitectureBody::statements>(
                     ref<ConcurrentStatement>)) +
                 ikw<"END"> + option(ikw<"ARCHITECTURE">) +
                 option(assign<&ast::Item::endName>(Identifier)) + lit<";">)> {};

  struct BlockConfig;

  struct ComponentConfig
      : rule<create<ast::ComponentConfig>(
                 ikw<"FOR"> +
                 (flag<&ast::ComponentConfig::allInstances>(
                      ikw<"ALL">) |
                  flag<&ast::ComponentConfig::othersInstances>(
                      ikw<"OTHERS">) |
                  some(append<&ast::ComponentConfig::instances>(Identifier),
                       lit<",">)) +
                 lit<":"> +
                 assign<&ast::ComponentConfig::componentName>(ref<NameExpr>) +
                 option(assign<&ast::ComponentConfig::binding>(
                            ref<BindingIndication>) +
                        lit<";">) +
                 option(assign<&ast::ComponentConfig::block>(
                     ref<BlockConfig>)) +
                 ikw<"END"> + ikw<"FOR"> + lit<";">)> {};

  struct ConfigItem
      : rule<ref<BlockConfig> | ref<ComponentConfig>> {};

  struct BlockConfig
      : rule<create<ast::BlockConfig>(
                 ikw<"FOR"> +
                 assign<&ast::BlockConfig::target>(ref<NameExpr>) +
                 many(append<&ast::BlockConfig::uses>(ref<UseClause>)) +
                 many(append<&ast::BlockConfig::items>(ref<ConfigItem>)) +
                 ikw<"END"> + ikw<"FOR"> + lit<";">)> {};

  struct ConfigurationDecl
      : rule<create<ast::ConfigurationDecl>(
                 ikw<"CONFIGURATION"> +
                 assign<&ast::ConfigurationDecl::name>(Identifier) +
                 ikw<"OF"> +
                 assign<&ast::ConfigurationDecl::entityName>(ref<NameExpr>) +
                 ikw<"IS"> +
                 many(append<&ast::ConfigurationDecl::decls>(
                     ref<DeclarativeItem>)) +
                 assign<&ast::ConfigurationDecl::body>(ref<BlockConfig>) +
                 ikw<"END"> + option(ikw<"CONFIGURATION">) +
                 option(assign<&ast::Item::endName>(Identifier)) + lit<";">)> {};

  struct PrimaryUnit
      : rule<ref<EntityDecl>             //
            | ref<ConfigurationDecl>     //
            | ref<PackageInstDecl>       //
            | ref<PackageDecl>           //
            | ref<ContextDecl>> {};

  struct SecondaryUnit
      : rule<ref<ArchitectureBody> | ref<PackageBody>> {};

  struct LibraryUnit : rule<ref<PrimaryUnit> | ref<SecondaryUnit>> {};

  struct DesignUnit
      : rule<create<ast::DesignUnit>(
                 many(append<&ast::DesignUnit::context>(
                     ref<ContextItem>)) +
                 assign<&ast::DesignUnit::unit>(ref<LibraryUnit>))> {};

  struct DesignFile
      : rule<create<ast::DesignFile>(
                 many(append<&ast::DesignFile::units>(ref<DesignUnit>)))> {
  };

  using entry = DesignFile;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace vhdl
