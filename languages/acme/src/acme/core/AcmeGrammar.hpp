// The acme language: its grammar. Acme (CMU ADL) -- families, systems and
// the components/connectors/ports/roles/groups they declare, plus a small
// design-rule expression language for invariants, heuristics and analyses.
#pragma once

#include <cstdint>

#include "acme/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace acme {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Acme : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // Acme identifiers are case-insensitive and may contain '-': both
  // `ClientAndServerFam` and `max-concurrent-requests` lex as one word.
  struct IDENTIFIER : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct INTEGER_LITERAL : terminal<as<std::int64_t>(some(d))> {};
  static constexpr auto FloatBody =
      some(d) + lit<"."> + some(d) +
      option(cr<"eE"> + option(cr<"+-">) + some(d));
  struct FLOATING_POINT_LITERAL
      : terminal<as<double>(FloatBody)> {};
  // Only `acmePropertyValueDeclaration` reads a signed float; folding the
  // sign into the terminal keeps the converter correct (json's NUMBER does
  // the same for its own leading `-`).
  struct SIGNED_FLOAT
      : terminal<as<double>(option(cr<"+-">) + FloatBody)> {};
  struct STRING_LITERAL
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};
  struct BOOL_LITERAL : terminal<as<bool>(ikw<"true"> | ikw<"false">)> {};

  static constexpr auto FileSep = cr<".:+/$%-"> | lit<"\\\\"> | lit<"\\">;
  struct Filename
      : rule<option(cr<"$%">) + ref<IDENTIFIER> +
             many(some(FileSep) + ref<IDENTIFIER>)> {};

  static constexpr auto ImportTargetE =
      ref<STRING_LITERAL> | ref<Filename>;

  struct Import
      : rule<ikw<"import"> + assign<&ast::Import::target>(ImportTargetE) +
             lit<";">> {};

  // A qualified reference is a name, optionally dotted once (Family.Type);
  // a few sites below accept unlimited dot depth instead, matching
  // InstanceRef's shape, where that reads easier.
  struct QRef
      : rule<ref<IDENTIFIER> + option(lit<"."> + ref<IDENTIFIER>)> {};
  struct InstanceRef : rule<some(ref<IDENTIFIER>, lit<".">)> {};

  static constexpr auto FamilyRefE = ref<QRef>;
  static constexpr auto TypeRefNameE = ref<QRef>;
  static constexpr auto InstTypeRefE = ref<QRef>;

  // ---- property / design-rule type structure -----------------------------

  struct PropTypeField;
  struct PropTypeRecord;
  struct PropTypeSet;
  struct PropTypeSequence;
  struct PropTypeEnum;
  struct PropTypeRefName;
  struct PropTypeRefKw;
  struct PropTypeSimpleWordRule;

  static constexpr auto PropTypeE =
      ref<PropTypeSimpleWordRule> | ref<PropTypeRecord> |
      ref<PropTypeSet> | ref<PropTypeSequence> |
      ref<PropTypeEnum> | ref<PropTypeRefName>;

  // `acmeTypeRef`: broader than a property type -- adds the bare meta-kind
  // keywords (`type`, `component`, ...) design rules read, e.g. `set {type}`
  // in `mustBeOfType`.
  static constexpr auto TypeRefE = ref<PropTypeRefKw> | PropTypeE;

  struct PropTypeSimpleWord
      : rule<ikw<"any"> | ikw<"int"> | ikw<"integer"> | ikw<"float"> |
             ikw<"double"> | ikw<"string"> | ikw<"boolean"> |
             ikw<"true"> | ikw<"false">> {};
  struct PropTypeSimpleWordRule
      : rule<assign<&ast::PropTypeSimple::kind>(ref<PropTypeSimpleWord>)> {};

  struct PropTypeRefName
      : rule<assign<&ast::PropTypeRef::name>(TypeRefNameE)> {};

  struct AcmeTypeRefWord
      : rule<ikw<"system"> | ikw<"component"> | ikw<"group"> |
             ikw<"connector"> | ikw<"port"> | ikw<"role"> | ikw<"property"> |
             ikw<"element"> | ikw<"type"> | ikw<"representation">> {};
  struct PropTypeRefKw
      : rule<assign<&ast::PropTypeSimple::kind>(ref<AcmeTypeRefWord>)> {};

  struct PropTypeField
      : rule<assign<&ast::PropTypeField::name>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::PropTypeField::type>(TypeRefE) + lit<";">> {};

  struct PropTypeRecord
      : rule<ikw<"record"> + lit<"["> +
             many(append<&ast::PropTypeRecord::fields>(
                 ref<PropTypeField>)) +
             lit<"]">> {};

  struct PropTypeSet
      : rule<ikw<"set"> + lit<"{"> +
             option(assign<&ast::PropTypeSet::element>(TypeRefE)) +
             lit<"}">> {};

  struct PropTypeSequence
      : rule<(ikw<"sequence"> | ikw<"seq">) + lit<"<"> +
             option(assign<&ast::PropTypeSequence::element>(TypeRefE)) +
             lit<">">> {};

  struct PropTypeEnum
      : rule<ikw<"enum"> + lit<"{"> +
             some(append<&ast::PropTypeEnum::values>(ref<IDENTIFIER>),
                  lit<",">) +
             lit<"}">> {};

  // ---- property values -----------------------------------------------

  struct PropSet;
  struct PropSeq;
  struct PropRecord;

  static constexpr auto PropValueE =
      ref<INTEGER_LITERAL> | ref<SIGNED_FLOAT> | ref<STRING_LITERAL> |
      ref<BOOL_LITERAL> | ref<PropSet> | ref<PropRecord> |
      ref<PropSeq> | ref<IDENTIFIER>;

  struct PropSet
      : rule<lit<"{"> +
             option(some(append<&ast::PropSet::items>(PropValueE),
                         lit<",">)) +
             lit<"}">> {};

  struct PropSeq
      : rule<lit<"<"> +
             option(some(append<&ast::PropSeq::items>(PropValueE),
                         lit<",">)) +
             lit<">">> {};

  struct PropRecordEntry
      : rule<assign<&ast::PropRecordEntry::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    assign<&ast::PropRecordEntry::type>(TypeRefE)) +
             lit<"="> +
             assign<&ast::PropRecordEntry::value>(PropValueE)> {};

  struct PropRecord
      : rule<lit<"["> +
             option(some(append<&ast::PropRecord::entries>(
                             ref<PropRecordEntry>),
                         lit<";">) +
                    option(lit<";">)) +
             lit<"]">> {};

  // ---- design-rule expression language --------------------------------

  struct DesignRuleExpr;
  struct ArithExpr;
  struct NegateExpr;
  struct NegativeExpr;
  struct Primitive;
  struct Reference;
  struct Quantified;
  struct SetExpr;
  struct PathExpr;
  struct SequenceExpr;
  struct LiteralSet;
  struct LiteralSeq;
  struct LiteralRecord;
  struct SetConstructor;
  struct VarDecl;
  struct DRPathStep;


  static constexpr auto OpMulLevel =
      as<ast::DROp::mul>(lit<"*">) | as<ast::DROp::div>(lit<"/">) | as<ast::DROp::rem>(lit<"%">) | as<ast::DROp::pow>(ikw<"power">);
  static constexpr auto OpAddLevel = as<ast::DROp::add>(lit<"+">) | as<ast::DROp::sub>(lit<"-">);
  static constexpr auto OpRelLevel =
      as<ast::DROp::le>(lit<"<=">) | as<ast::DROp::ge>(lit<">=">) | as<ast::DROp::lt>(lit<"<">) | as<ast::DROp::gt>(lit<">">);
  static constexpr auto OpEqLevel = as<ast::DROp::eq>(lit<"==">) | as<ast::DROp::ne>(lit<"!=">);

  struct NegativeExpr
      : rule<(lit<"-"> +
              assign<&ast::DRNeg::operand>(ref<NegativeExpr>)) |
             ref<Primitive>> {};

  struct ArithExpr
      : infix<NegativeExpr, ast::DRBinary,
              levels(left(OpMulLevel), left(OpAddLevel), left(OpRelLevel),
                     left(OpEqLevel))> {};

  struct NegateExpr
      : rule<(lit<"!"> +
              assign<&ast::DRNot::operand>(ref<NegateExpr>)) |
             ref<ArithExpr>> {};

  static constexpr auto OpAndLevel = as<ast::DROp::logical_and>(ikw<"and">);
  static constexpr auto OpIffLevel = as<ast::DROp::iff>(lit<"<->">);
  static constexpr auto OpImpliesLevel = as<ast::DROp::implies>(lit<"->">);
  static constexpr auto OpOrLevel = as<ast::DROp::logical_or>(ikw<"or">);

  struct DesignRuleExpr
      : infix<NegateExpr, ast::DRBinary,
              levels(left(OpAndLevel), left(OpIffLevel), left(OpImpliesLevel),
                     left(OpOrLevel))> {};

  struct SetRefWord
      : rule<ikw<"type"> | ikw<"components"> | ikw<"connectors"> |
             ikw<"ports"> | ikw<"roles"> | ikw<"groups"> | ikw<"members"> |
             ikw<"properties"> | ikw<"representations"> |
             ikw<"attachedports"> | ikw<"attachedroles">> {};

  struct MetaTypeWord
      : rule<ikw<"component"> | ikw<"group"> | ikw<"connector"> |
             ikw<"port"> | ikw<"role"> | ikw<"system"> | ikw<"element"> |
             ikw<"property"> | ikw<"int"> | ikw<"integer"> | ikw<"float"> |
             ikw<"double"> | ikw<"string"> | ikw<"enum"> | ikw<"set"> |
             ikw<"sequence"> | ikw<"seq"> | ikw<"record">> {};

  struct LiteralConst
      : rule<assign<&ast::DRLiteral::value>(
                 ref<INTEGER_LITERAL> | ref<SIGNED_FLOAT> |
                 ref<STRING_LITERAL> | ref<BOOL_LITERAL> |
                 ref<MetaTypeWord>)> {};

  struct ActualParams
      : rule<lit<"("> +
             option(some(append<&ast::DRArgs::args>(ref<DesignRuleExpr>),
                         lit<",">)) +
             lit<")">> {};

  struct Reference
      : rule<assign<&ast::DRRef::name>(ref<IDENTIFIER>) +
             many(lit<"."> +
                  append<&ast::DRRef::path>(ref<IDENTIFIER> |
                                            ref<SetRefWord>)) +
             option(assign<&ast::DRRef::callArgs>(ref<ActualParams>))> {
  };

  static constexpr auto InClauseE = ref<SetExpr> | ref<Reference>;

  struct VarDecl
      : rule<option(flag<&ast::DRVarDecl::distinct>(ikw<"distinct">)) +
             some(append<&ast::DRVarDecl::names>(ref<IDENTIFIER>), lit<",">) +
             option((lit<":"> | lit<":!">) +
                    assign<&ast::DRVarDecl::type>(TypeRefE)) +
             ikw<"in"> + assign<&ast::DRVarDecl::in>(InClauseE)> {};

  struct Quantified
      : rule<((flag<&ast::DRQuantified::exists>(ikw<"exists">) +
               option(flag<&ast::DRQuantified::unique>(
                   ikw<"unique">))) |
              ikw<"forall">) +
             some(append<&ast::DRQuantified::vars>(ref<VarDecl>),
                  lit<",">) +
             lit<"|"> +
             assign<&ast::DRQuantified::body>(ref<DesignRuleExpr>)> {};

  struct LiteralSet
      : rule<lit<"{"> +
             option(some(append<&ast::DRLiteralSet::items>(
                             ref<LiteralConst> | ref<Reference>),
                         lit<",">)) +
             lit<"}">> {};

  struct LiteralSeq
      : rule<lit<"<"> +
             option(some(append<&ast::DRLiteralSeq::items>(
                             ref<LiteralConst> | ref<Reference>),
                         lit<",">)) +
             lit<">">> {};

  struct LiteralRecordEntry
      : rule<assign<&ast::DRRecordEntry::name>(ref<IDENTIFIER>) +
             option(lit<":"> + assign<&ast::DRRecordEntry::type>(TypeRefE)) +
             lit<"="> +
             assign<&ast::DRRecordEntry::value>(ref<LiteralConst>)> {};

  struct LiteralRecord
      : rule<lit<"["> +
             option(some(append<&ast::DRLiteralRecord::entries>(
                             ref<LiteralRecordEntry>),
                         lit<";">) +
                    option(lit<";">)) +
             lit<"]">> {};

  struct CollectVar
      : rule<append<&ast::DRVarDecl::names>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::DRVarDecl::type>(TypeRefE) + ikw<"in"> +
             assign<&ast::DRVarDecl::in>(InClauseE)> {};

  struct SetConstructor
      : rule<option(lit<"{">) +
             ((ikw<"select"> +
               assign<&ast::DRSetBuilder::var>(ref<VarDecl>)) |
              (flag<&ast::DRSetBuilder::isCollect>(ikw<"collect">) +
               assign<&ast::DRSetBuilder::var>(ref<CollectVar>))) +
             lit<"|"> +
             assign<&ast::DRSetBuilder::body>(ref<DesignRuleExpr>) +
             option(lit<"}">)> {};

  struct DRPathStep
      : rule<(option(flag<&ast::DRPathStep::ellipsis>(lit<"...">)) +
              assign<&ast::DRPathStep::ref>(ref<Reference>)) |
             (assign<&ast::DRPathStep::setKeyword>(ref<SetRefWord>) +
              option((lit<":"> | lit<":!">) +
                     assign<&ast::DRPathStep::type>(TypeRefE)) +
              option(lit<"["> +
                     assign<&ast::DRPathStep::filter>(
                         ref<DesignRuleExpr>) +
                     lit<"]">))> {};

  struct PathExpr
      : rule<lit<"/"> + assign<&ast::DRPath::head>(ref<Reference>) +
             option((lit<":"> | lit<":!">) +
                    assign<&ast::DRPath::type>(TypeRefE)) +
             option(lit<"["> +
                    assign<&ast::DRPath::filter>(ref<DesignRuleExpr>) +
                    lit<"]">) +
             many(lit<"/"> +
                  append<&ast::DRPath::steps>(ref<DRPathStep>))> {};

  struct SequenceExpr
      : rule<lit<"<"> + assign<&ast::DRSequenceExpr::path>(ref<PathExpr>) +
             lit<">">> {};

  struct SetExpr
      : rule<ref<LiteralSet> | ref<SetConstructor> |
             ref<PathExpr>> {};

  struct Primitive
      : rule<(lit<"("> + ref<DesignRuleExpr> + lit<")">) |
             ref<LiteralConst> | ref<Quantified> |
             ref<LiteralRecord> | ref<SequenceExpr> |
             ref<LiteralSeq> | ref<SetExpr> | ref<Reference>> {};

  // ---- property block `<< ... >>` and property declarations -----------

  struct PropertyBlockEntry
      : rule<assign<&ast::PropertyBlockEntry::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    assign<&ast::PropertyBlockEntry::type>(TypeRefE)) +
             option(
                 (lit<"="> +
                  assign<&ast::PropertyBlockEntry::value>(PropValueE)) |
                 (flag<&ast::PropertyBlockEntry::containAssign>(
                      ikw<"containassign">) +
                  assign<&ast::PropertyBlockEntry::value>(PropValueE))) +
             lit<";">> {};

  struct PropertyDecl
      : rule<ikw<"property"> +
             assign<&ast::PropertyDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> + assign<&ast::PropertyDecl::type>(TypeRefE)) +
             option((lit<"="> +
                     assign<&ast::PropertyDecl::value>(PropValueE)) |
                    (flag<&ast::PropertyDecl::containAssign>(
                         ikw<"containassign">) +
                     assign<&ast::PropertyDecl::value>(PropValueE))) +
             option(lit<"<<"> +
                    some(append<&ast::PropertyDecl::annotations>(
                        ref<PropertyBlockEntry>)) +
                    lit<">>">) +
             lit<";">> {};

  // ---- design rules & analyses -----------------------------------------

  struct DesignRuleDecl
      : rule<option(ikw<"design">) +
             option(ikw<"rule"> +
                    assign<&ast::DesignRuleDecl::name>(ref<IDENTIFIER>) +
                    option(lit<"=">)) +
             option((flag<&ast::DesignRuleDecl::isHeuristic>(
                         ikw<"heuristic">) |
                     ikw<"invariant">) +
                    assign<&ast::DesignRuleDecl::expr>(
                        ref<DesignRuleExpr>)) +
             option(lit<"<<"> +
                    some(append<&ast::DesignRuleDecl::annotations>(
                        ref<PropertyBlockEntry>)) +
                    lit<">>">) +
             lit<";">> {};

  struct FormalParam
      : rule<assign<&ast::FormalParam::name>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::FormalParam::type>(TypeRefE)> {};

  struct DesignAnalysisDecl
      : rule<(option(ikw<"design">) + ikw<"analysis"> +
              assign<&ast::DesignAnalysisDecl::name>(ref<IDENTIFIER>) +
              lit<"("> +
              option(some(append<&ast::DesignAnalysisDecl::params>(
                              ref<FormalParam>),
                          lit<",">)) +
              lit<")"> + lit<":"> +
              assign<&ast::DesignAnalysisDecl::returnType>(TypeRefE) +
              lit<"="> +
              assign<&ast::DesignAnalysisDecl::body>(
                  ref<DesignRuleExpr>) +
              option(lit<"<<"> +
                     some(append<&ast::DesignAnalysisDecl::annotations>(
                         ref<PropertyBlockEntry>)) +
                     lit<">>">) +
              lit<";">) |
             (flag<&ast::DesignAnalysisDecl::isExternal>(
                  ikw<"external">) +
              option(ikw<"design">) + ikw<"analysis"> +
              assign<&ast::DesignAnalysisDecl::name>(ref<IDENTIFIER>) +
              lit<"("> +
              option(some(append<&ast::DesignAnalysisDecl::params>(
                              ref<FormalParam>),
                          lit<",">)) +
              lit<")"> + lit<":"> +
              assign<&ast::DesignAnalysisDecl::returnType>(TypeRefE) +
              lit<"="> +
              assign<&ast::DesignAnalysisDecl::externalCode>(
                  ref<STRING_LITERAL> | ref<InstanceRef>) +
              lit<";">)> {};

  // ---- structural declarations ------------------------------------------

  struct RepresentationDecl;
  struct PropertyTypeDecl;
  struct PortTypeDecl;
  struct RoleTypeDecl;
  struct ComponentTypeDecl;
  struct ConnectorTypeDecl;
  struct ElementTypeDecl;
  struct GroupTypeDecl;
  struct PortDecl;
  struct RoleDecl;
  struct ComponentDecl;
  struct ConnectorDecl;
  struct GroupDecl;
  struct MembersBlock;
  struct AttachmentDecl;
  struct SystemDecl;

  static constexpr auto SystemMemberE =
      ref<PropertyDecl> | ref<ComponentDecl> |
      ref<ConnectorDecl> | ref<AttachmentDecl> | ref<GroupDecl> |
      ref<RepresentationDecl> | ref<DesignRuleDecl>;
  static constexpr auto PortRoleMemberE = ref<PropertyDecl> |
                                          ref<DesignRuleDecl> |
                                          ref<RepresentationDecl>;
  static constexpr auto ComponentMemberE =
      ref<PropertyDecl> | ref<PortDecl> | ref<DesignRuleDecl> |
      ref<RepresentationDecl>;
  static constexpr auto ConnectorMemberE =
      ref<PropertyDecl> | ref<RoleDecl> | ref<DesignRuleDecl> |
      ref<RepresentationDecl>;
  static constexpr auto ElementMemberE =
      ref<PropertyDecl> | ref<DesignRuleDecl>;
  static constexpr auto GroupMemberE = ref<MembersBlock> |
                                       ref<PropertyDecl> |
                                       ref<DesignRuleDecl>;
  static constexpr auto FamilyMemberE =
      ref<PortTypeDecl> | ref<RoleTypeDecl> |
      ref<ComponentTypeDecl> | ref<ConnectorTypeDecl> |
      ref<ElementTypeDecl> | ref<PropertyTypeDecl> |
      ref<GroupTypeDecl> | ref<DesignAnalysisDecl> |
      ref<DesignRuleDecl> | ref<PortDecl> | ref<RoleDecl> |
      ref<ComponentDecl> | ref<ConnectorDecl> |
      ref<PropertyDecl> | ref<GroupDecl> | ref<AttachmentDecl>;

  template <auto Field>
  static constexpr auto TypeModifiers = many(append<Field>(ikw<"public"> | ikw<"private"> | ikw<"final"> | ikw<"abstract">));

  struct PropertyTypeDecl
      : rule<ikw<"property"> + ikw<"type"> +
             assign<&ast::PropertyTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> +
               assign<&ast::PropertyTypeDecl::type>(PropTypeE) + lit<";">))> {
  };

  struct PortTypeDecl
      : rule<TypeModifiers<&ast::PortTypeDecl::modifiers> + ikw<"port"> + ikw<"type"> +
             assign<&ast::PortTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::PortTypeDecl::members>(PortRoleMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::PortTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::PortTypeDecl::members>(
                     PortRoleMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct RoleTypeDecl
      : rule<TypeModifiers<&ast::RoleTypeDecl::modifiers> + ikw<"role"> + ikw<"type"> +
             assign<&ast::RoleTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::RoleTypeDecl::members>(PortRoleMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::RoleTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::RoleTypeDecl::members>(
                     PortRoleMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct ComponentTypeDecl
      : rule<TypeModifiers<&ast::ComponentTypeDecl::modifiers> + ikw<"component"> + ikw<"type"> +
             assign<&ast::ComponentTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::ComponentTypeDecl::members>(
                   ComponentMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::ComponentTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::ComponentTypeDecl::members>(
                     ComponentMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct ConnectorTypeDecl
      : rule<TypeModifiers<&ast::ConnectorTypeDecl::modifiers> + ikw<"connector"> + ikw<"type"> +
             assign<&ast::ConnectorTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::ConnectorTypeDecl::members>(
                   ConnectorMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::ConnectorTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::ConnectorTypeDecl::members>(
                     ConnectorMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct ElementTypeDecl
      : rule<TypeModifiers<&ast::ElementTypeDecl::modifiers> + ikw<"element"> + ikw<"type"> +
             assign<&ast::ElementTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::ElementTypeDecl::members>(
                   ElementMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::ElementTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::ElementTypeDecl::members>(
                     ElementMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct GroupTypeDecl
      : rule<TypeModifiers<&ast::GroupTypeDecl::modifiers> + ikw<"group"> + ikw<"type"> +
             assign<&ast::GroupTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::GroupTypeDecl::members>(GroupMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::GroupTypeDecl::extends>(TypeRefNameE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::GroupTypeDecl::members>(GroupMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct PortDecl
      : rule<ikw<"port"> + assign<&ast::PortDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::PortDecl::types>(TypeRefNameE),
                         lit<",">)) +
             (option(lit<";">) |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::PortDecl::members>(PortRoleMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::PortDecl::isNew>(ikw<"new">) +
                 some(append<&ast::PortDecl::newTypes>(InstTypeRefE),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::PortDecl::members>(PortRoleMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct RoleDecl
      : rule<ikw<"role"> + assign<&ast::RoleDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::RoleDecl::types>(TypeRefNameE),
                         lit<",">)) +
             (lit<";"> |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::RoleDecl::members>(PortRoleMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::RoleDecl::isNew>(ikw<"new">) +
                 some(append<&ast::RoleDecl::newTypes>(InstTypeRefE),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::RoleDecl::members>(PortRoleMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct ComponentDecl
      : rule<ikw<"component"> +
             assign<&ast::ComponentDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::ComponentDecl::types>(TypeRefNameE),
                         lit<",">)) +
             (lit<";"> |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::ComponentDecl::members>(
                     ComponentMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::ComponentDecl::isNew>(ikw<"new">) +
                 some(append<&ast::ComponentDecl::newTypes>(InstTypeRefE),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::ComponentDecl::members>(
                       ComponentMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct ConnectorDecl
      : rule<ikw<"connector"> +
             assign<&ast::ConnectorDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::ConnectorDecl::types>(TypeRefNameE),
                         lit<",">)) +
             (lit<";"> |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::ConnectorDecl::members>(
                     ConnectorMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::ConnectorDecl::isNew>(ikw<"new">) +
                 some(append<&ast::ConnectorDecl::newTypes>(InstTypeRefE),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::ConnectorDecl::members>(
                       ConnectorMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct MembersBlock
      : rule<ikw<"members"> + lit<"{"> +
             option(some(append<&ast::MembersBlock::refs>(
                             ref<InstanceRef>),
                         lit<",">)) +
             lit<"}"> + option(lit<";">)> {};

  struct GroupDecl
      : rule<ikw<"group"> + assign<&ast::GroupDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::GroupDecl::types>(TypeRefNameE),
                         lit<",">)) +
             (lit<";"> |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::GroupDecl::members>(GroupMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::GroupDecl::isNew>(ikw<"new">) +
                 some(append<&ast::GroupDecl::newTypes>(InstTypeRefE),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::GroupDecl::members>(GroupMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct AttachmentDecl
      : rule<ikw<"attachment"> +
             assign<&ast::AttachmentDecl::from>(ref<InstanceRef>) +
             ikw<"to"> +
             assign<&ast::AttachmentDecl::to>(ref<InstanceRef>) +
             lit<";">> {};

  struct BindingDecl
      : rule<assign<&ast::BindingDecl::from>(ref<InstanceRef>) +
             ikw<"to"> +
             assign<&ast::BindingDecl::to>(ref<InstanceRef>) +
             option(lit<"{"> +
                    assign<&ast::BindingDecl::property>(
                        ref<PropertyDecl>) +
                    lit<"<<"> +
                    some(append<&ast::BindingDecl::block>(
                        ref<PropertyBlockEntry>)) +
                    lit<">>"> + lit<"}">) +
             lit<";">> {};

  struct RepresentationDecl
      : rule<ikw<"representation"> +
             option(assign<&ast::RepresentationDecl::label>(
                        ref<IDENTIFIER>) +
                    lit<"=">) +
             lit<"{"> +
             assign<&ast::RepresentationDecl::system>(ref<SystemDecl>) +
             option(ikw<"bindings"> + lit<"{"> +
                    many(append<&ast::RepresentationDecl::bindings>(
                        ref<BindingDecl>)) +
                    lit<"}"> + option(lit<";">)) +
             lit<"}"> + option(lit<";">)> {};

  struct SystemDecl
      : rule<ikw<"system"> +
             assign<&ast::SystemDecl::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    some(append<&ast::SystemDecl::extends>(FamilyRefE),
                         lit<",">)) +
             (lit<";"> |
              (lit<"="> +
               ((lit<"{"> +
                 many(append<&ast::SystemDecl::members>(SystemMemberE)) +
                 lit<"}"> + option(lit<";">)) |
                (flag<&ast::SystemDecl::isNew>(ikw<"new">) +
                 some(append<&ast::SystemDecl::newFamilies>(
                          ref<IDENTIFIER>),
                      lit<",">) +
                 (lit<";"> |
                  (ikw<"extended"> + ikw<"with"> + lit<"{"> +
                   many(append<&ast::SystemDecl::members>(SystemMemberE)) +
                   lit<"}"> + option(lit<";">)))))))> {};

  struct FamilyDecl
      : rule<((flag<&ast::FamilyDecl::isStyle>(ikw<"style">)) |
              ikw<"family">) +
             assign<&ast::FamilyDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> + lit<"{"> +
               many(append<&ast::FamilyDecl::members>(FamilyMemberE)) +
               lit<"}"> + option(lit<";">)) |
              (ikw<"extends"> +
               some(append<&ast::FamilyDecl::extends>(FamilyRefE),
                    lit<",">) +
               (lit<";"> |
                (ikw<"with"> + lit<"{"> +
                 many(append<&ast::FamilyDecl::members>(FamilyMemberE)) +
                 lit<"}"> + option(lit<";">)))))> {};

  struct DesignDecl : rule<create<ast::DesignDecl> + ikw<"design"> +
                               lit<";">> {};

  struct CompUnit
      : rule<many(append<&ast::CompUnit::imports>(ref<Import>)) +
             some(append<&ast::CompUnit::decls>(
                 ref<SystemDecl> | ref<FamilyDecl> |
                 ref<DesignDecl>))> {};

  using entry = CompUnit;
  using ignored = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace acme
