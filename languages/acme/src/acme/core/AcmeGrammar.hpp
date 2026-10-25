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
using Unquoted = cursiv::text_conv<unquote>;

struct Acme : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINE_COMMENT : terminal<lit<"//"> + many(cr<R"(^\r\n)">)> {};
  struct BLOCK_COMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};

  // Acme identifiers are case-insensitive and may contain '-': both
  // `ClientAndServerFam` and `max-concurrent-requests` lex as one word.
  struct IDENTIFIER : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};

  static constexpr auto word_boundary = cr<"a-zA-Z0-9_-">;

  struct INTEGER_LITERAL : terminal<some(d), std::int64_t> {};
  struct FLOATING_POINT_LITERAL
      : terminal<some(d) + lit<"."> + some(d), double> {};
  // Only `acmePropertyValueDeclaration` reads a signed float; folding the
  // sign into the terminal keeps the converter correct (json's NUMBER does
  // the same for its own leading `-`).
  struct SIGNED_FLOAT
      : terminal<many(cr<"+-">) + some(d) + lit<"."> + some(d), double> {};
  struct STRING_LITERAL
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};
  struct BOOL_LITERAL : terminal<ikw<"true"> | ikw<"false">, bool> {};

  static constexpr auto FileSep = cr<".:+/$%-"> | lit<"\\\\"> | lit<"\\">;
  struct FilenameRule
      : rule<option(cr<"$%">) + ref<IDENTIFIER> +
             many(some(FileSep) + ref<IDENTIFIER>)> {};

  static constexpr auto ImportTargetE =
      ref<STRING_LITERAL> | ref<FilenameRule>;

  struct ImportRule
      : rule<ikw<"import"> + assign<&ast::Import::target>(ImportTargetE) +
             lit<";">> {};

  // A qualified reference is a name, optionally dotted once (Family.Type);
  // a few sites below accept unlimited dot depth instead, matching
  // InstanceRefRule's shape, where that reads easier.
  struct QRefRule
      : rule<ref<IDENTIFIER> + option(lit<"."> + ref<IDENTIFIER>)> {};
  struct InstanceRefRule : rule<some(ref<IDENTIFIER>, lit<".">)> {};

  static constexpr auto FamilyRefE = ref<QRefRule>;
  static constexpr auto TypeRefNameE = ref<QRefRule>;
  static constexpr auto InstTypeRefE = ref<QRefRule>;

  // ---- property / design-rule type structure -----------------------------

  struct PropTypeFieldRule;
  struct PropTypeRecordRule;
  struct PropTypeSetRule;
  struct PropTypeSequenceRule;
  struct PropTypeEnumRule;
  struct PropTypeRefNameRule;
  struct PropTypeRefKwRule;
  struct PropTypeSimpleWordRule;

  static constexpr auto PropTypeE =
      ref<PropTypeSimpleWordRule> | ref<PropTypeRecordRule> |
      ref<PropTypeSetRule> | ref<PropTypeSequenceRule> |
      ref<PropTypeEnumRule> | ref<PropTypeRefNameRule>;

  // `acmeTypeRef`: broader than a property type -- adds the bare meta-kind
  // keywords (`type`, `component`, ...) design rules read, e.g. `set {type}`
  // in `mustBeOfType`.
  static constexpr auto TypeRefE = ref<PropTypeRefKwRule> | PropTypeE;

  struct PropTypeSimpleWord
      : rule<ikw<"any"> | ikw<"int"> | ikw<"integer"> | ikw<"float"> |
             ikw<"double"> | ikw<"string"> | ikw<"true"> | ikw<"false">> {};
  struct PropTypeSimpleWordRule
      : rule<assign<&ast::PropTypeSimple::kind>(ref<PropTypeSimpleWord>)> {};

  struct PropTypeRefNameRule
      : rule<assign<&ast::PropTypeRef::name>(TypeRefNameE)> {};

  struct AcmeTypeRefWord
      : rule<ikw<"system"> | ikw<"component"> | ikw<"group"> |
             ikw<"connector"> | ikw<"port"> | ikw<"role"> | ikw<"property"> |
             ikw<"element"> | ikw<"type"> | ikw<"representation">> {};
  struct PropTypeRefKwRule
      : rule<assign<&ast::PropTypeSimple::kind>(ref<AcmeTypeRefWord>)> {};

  struct PropTypeFieldRule
      : rule<assign<&ast::PropTypeField::name>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::PropTypeField::type>(TypeRefE) + lit<";">> {};

  struct PropTypeRecordRule
      : rule<ikw<"record"> + lit<"["> +
             many(append<&ast::PropTypeRecord::fields>(
                 ref<PropTypeFieldRule>)) +
             lit<"]">> {};

  struct PropTypeSetRule
      : rule<ikw<"set"> + lit<"{"> +
             option(assign<&ast::PropTypeSet::element>(TypeRefE)) +
             lit<"}">> {};

  struct PropTypeSequenceRule
      : rule<(ikw<"sequence"> | ikw<"seq">) + lit<"<"> +
             option(assign<&ast::PropTypeSequence::element>(TypeRefE)) +
             lit<">">> {};

  struct PropTypeEnumRule
      : rule<ikw<"enum"> + lit<"{"> +
             some(append<&ast::PropTypeEnum::values>(ref<IDENTIFIER>),
                  lit<",">) +
             lit<"}">> {};

  // ---- property values -----------------------------------------------

  struct PropSetRule;
  struct PropSeqRule;
  struct PropRecordRule;

  static constexpr auto PropValueE =
      ref<INTEGER_LITERAL> | ref<SIGNED_FLOAT> | ref<STRING_LITERAL> |
      ref<BOOL_LITERAL> | ref<PropSetRule> | ref<PropRecordRule> |
      ref<PropSeqRule> | ref<IDENTIFIER>;

  struct PropSetRule
      : rule<lit<"{"> +
             option(some(append<&ast::PropSet::items>(PropValueE),
                         lit<",">)) +
             lit<"}">> {};

  struct PropSeqRule
      : rule<lit<"<"> +
             option(some(append<&ast::PropSeq::items>(PropValueE),
                         lit<",">)) +
             lit<">">> {};

  struct PropRecordEntryRule
      : rule<assign<&ast::PropRecordEntry::name>(ref<IDENTIFIER>) +
             option(lit<":"> +
                    assign<&ast::PropRecordEntry::type>(TypeRefE)) +
             lit<"="> +
             assign<&ast::PropRecordEntry::value>(PropValueE)> {};

  struct PropRecordRule
      : rule<lit<"["> +
             option(some(append<&ast::PropRecord::entries>(
                             ref<PropRecordEntryRule>),
                         lit<";">) +
                    option(lit<";">)) +
             lit<"]">> {};

  // ---- design-rule expression language --------------------------------

  struct DesignRuleExprRule;
  struct ArithExprRule;
  struct NegateExprRule;
  struct NegativeExprRule;
  struct PrimitiveRule;
  struct ReferenceRule;
  struct QuantifiedRule;
  struct SetExprRule;
  struct PathExprRule;
  struct SequenceExprRule;
  struct LiteralSetRule;
  struct LiteralSeqRule;
  struct LiteralRecordRule;
  struct SetConstructorRule;
  struct VarDeclRule;
  struct DRPathStepRule;

  struct OpMul : terminal<lit<"*">, value<ast::DROp::mul>> {};
  struct OpDiv : terminal<lit<"/">, value<ast::DROp::div>> {};
  struct OpRem : terminal<lit<"%">, value<ast::DROp::rem>> {};
  struct OpPow : terminal<ikw<"power">, value<ast::DROp::pow>> {};
  struct OpAdd : terminal<lit<"+">, value<ast::DROp::add>> {};
  struct OpSub : terminal<lit<"-">, value<ast::DROp::sub>> {};
  struct OpEq : terminal<lit<"==">, value<ast::DROp::eq>> {};
  struct OpNe : terminal<lit<"!=">, value<ast::DROp::ne>> {};
  struct OpLe : terminal<lit<"<=">, value<ast::DROp::le>> {};
  struct OpGe : terminal<lit<">=">, value<ast::DROp::ge>> {};
  struct OpLt : terminal<lit<"<">, value<ast::DROp::lt>> {};
  struct OpGt : terminal<lit<">">, value<ast::DROp::gt>> {};
  struct OpAnd : terminal<ikw<"and">, value<ast::DROp::logical_and>> {};
  struct OpIff : terminal<lit<"<->">, value<ast::DROp::iff>> {};
  struct OpImplies : terminal<lit<"->">, value<ast::DROp::implies>> {};
  struct OpOr : terminal<ikw<"or">, value<ast::DROp::logical_or>> {};

  static constexpr auto OpMulLevel =
      ref<OpMul> | ref<OpDiv> | ref<OpRem> | ref<OpPow>;
  static constexpr auto OpAddLevel = ref<OpAdd> | ref<OpSub>;
  static constexpr auto OpRelLevel =
      ref<OpLe> | ref<OpGe> | ref<OpLt> | ref<OpGt>;
  static constexpr auto OpEqLevel = ref<OpEq> | ref<OpNe>;

  struct NegativeExprRule
      : rule<(lit<"-"> +
              assign<&ast::DRNeg::operand>(ref<NegativeExprRule>)) |
             ref<PrimitiveRule>> {};

  struct ArithExprRule
      : infix<NegativeExprRule, &ast::DRBinary::left, &ast::DRBinary::op,
              &ast::DRBinary::right,
              (left(OpMulLevel) >> left(OpAddLevel) >> left(OpRelLevel) >>
               left(OpEqLevel))> {};

  struct NegateExprRule
      : rule<(lit<"!"> +
              assign<&ast::DRNot::operand>(ref<NegateExprRule>)) |
             ref<ArithExprRule>> {};

  static constexpr auto OpAndLevel = ref<OpAnd>;
  static constexpr auto OpIffLevel = ref<OpIff>;
  static constexpr auto OpImpliesLevel = ref<OpImplies>;
  static constexpr auto OpOrLevel = ref<OpOr>;

  struct DesignRuleExprRule
      : infix<NegateExprRule, &ast::DRBinary::left, &ast::DRBinary::op,
              &ast::DRBinary::right,
              (left(OpAndLevel) >> left(OpIffLevel) >> left(OpImpliesLevel) >>
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

  struct LiteralConstRule
      : rule<assign<&ast::DRLiteral::value>(
                 ref<INTEGER_LITERAL> | ref<SIGNED_FLOAT> |
                 ref<STRING_LITERAL> | ref<BOOL_LITERAL> |
                 ref<MetaTypeWord>)> {};

  struct ActualParamsRule
      : rule<lit<"("> +
             option(some(append<&ast::DRArgs::args>(ref<DesignRuleExprRule>),
                         lit<",">)) +
             lit<")">> {};

  struct ReferenceRule
      : rule<assign<&ast::DRRef::name>(ref<IDENTIFIER>) +
             many(lit<"."> +
                  append<&ast::DRRef::path>(ref<IDENTIFIER> |
                                            ref<SetRefWord>)) +
             option(assign<&ast::DRRef::callArgs>(ref<ActualParamsRule>))> {
  };

  static constexpr auto InClauseE = ref<SetExprRule> | ref<ReferenceRule>;

  struct VarDeclRule
      : rule<option(flag<&ast::DRVarDecl::distinct>(ikw<"distinct">)) +
             some(append<&ast::DRVarDecl::names>(ref<IDENTIFIER>), lit<",">) +
             option((lit<":"> | lit<":!">) +
                    assign<&ast::DRVarDecl::type>(TypeRefE)) +
             ikw<"in"> + assign<&ast::DRVarDecl::in>(InClauseE)> {};

  struct QuantifiedRule
      : rule<((flag<&ast::DRQuantified::exists>(ikw<"exists">) +
               option(flag<&ast::DRQuantified::unique>(
                   ikw<"unique">))) |
              ikw<"forall">) +
             some(append<&ast::DRQuantified::vars>(ref<VarDeclRule>),
                  lit<",">) +
             lit<"|"> +
             assign<&ast::DRQuantified::body>(ref<DesignRuleExprRule>)> {};

  struct LiteralSetRule
      : rule<lit<"{"> +
             option(some(append<&ast::DRLiteralSet::items>(
                             ref<LiteralConstRule> | ref<ReferenceRule>),
                         lit<",">)) +
             lit<"}">> {};

  struct LiteralSeqRule
      : rule<lit<"<"> +
             option(some(append<&ast::DRLiteralSeq::items>(
                             ref<LiteralConstRule> | ref<ReferenceRule>),
                         lit<",">)) +
             lit<">">> {};

  struct LiteralRecordEntryRule
      : rule<assign<&ast::DRRecordEntry::name>(ref<IDENTIFIER>) +
             option(lit<":"> + assign<&ast::DRRecordEntry::type>(TypeRefE)) +
             lit<"="> +
             assign<&ast::DRRecordEntry::value>(ref<LiteralConstRule>)> {};

  struct LiteralRecordRule
      : rule<lit<"["> +
             option(some(append<&ast::DRLiteralRecord::entries>(
                             ref<LiteralRecordEntryRule>),
                         lit<";">) +
                    option(lit<";">)) +
             lit<"]">> {};

  struct CollectVarRule
      : rule<append<&ast::DRVarDecl::names>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::DRVarDecl::type>(TypeRefE) + ikw<"in"> +
             assign<&ast::DRVarDecl::in>(InClauseE)> {};

  struct SetConstructorRule
      : rule<option(lit<"{">) +
             ((ikw<"select"> +
               assign<&ast::DRSetBuilder::var>(ref<VarDeclRule>)) |
              (flag<&ast::DRSetBuilder::isCollect>(ikw<"collect">) +
               assign<&ast::DRSetBuilder::var>(ref<CollectVarRule>))) +
             lit<"|"> +
             assign<&ast::DRSetBuilder::body>(ref<DesignRuleExprRule>) +
             option(lit<"}">)> {};

  struct DRPathStepRule
      : rule<(option(flag<&ast::DRPathStep::ellipsis>(lit<"...">)) +
              assign<&ast::DRPathStep::ref>(ref<ReferenceRule>)) |
             (assign<&ast::DRPathStep::setKeyword>(ref<SetRefWord>) +
              option((lit<":"> | lit<":!">) +
                     assign<&ast::DRPathStep::type>(TypeRefE)) +
              option(lit<"["> +
                     assign<&ast::DRPathStep::filter>(
                         ref<DesignRuleExprRule>) +
                     lit<"]">))> {};

  struct PathExprRule
      : rule<lit<"/"> + assign<&ast::DRPath::head>(ref<ReferenceRule>) +
             option((lit<":"> | lit<":!">) +
                    assign<&ast::DRPath::type>(TypeRefE)) +
             option(lit<"["> +
                    assign<&ast::DRPath::filter>(ref<DesignRuleExprRule>) +
                    lit<"]">) +
             many(lit<"/"> +
                  append<&ast::DRPath::steps>(ref<DRPathStepRule>))> {};

  struct SequenceExprRule
      : rule<lit<"<"> + assign<&ast::DRSequenceExpr::path>(ref<PathExprRule>) +
             lit<">">> {};

  struct SetExprRule
      : rule<ref<LiteralSetRule> | ref<SetConstructorRule> |
             ref<PathExprRule>> {};

  struct PrimitiveRule
      : rule<(lit<"("> + ref<DesignRuleExprRule> + lit<")">) |
             ref<LiteralConstRule> | ref<QuantifiedRule> |
             ref<LiteralRecordRule> | ref<SequenceExprRule> |
             ref<LiteralSeqRule> | ref<SetExprRule> | ref<ReferenceRule>> {};

  // ---- property block `<< ... >>` and property declarations -----------

  struct PropertyBlockEntryRule
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

  struct PropertyDeclRule
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
                        ref<PropertyBlockEntryRule>)) +
                    lit<">>">) +
             lit<";">> {};

  // ---- design rules & analyses -----------------------------------------

  struct DesignRuleDeclRule
      : rule<option(ikw<"design">) +
             option(ikw<"rule"> +
                    assign<&ast::DesignRuleDecl::name>(ref<IDENTIFIER>) +
                    option(lit<"=">)) +
             option((flag<&ast::DesignRuleDecl::isHeuristic>(
                         ikw<"heuristic">) |
                     ikw<"invariant">) +
                    assign<&ast::DesignRuleDecl::expr>(
                        ref<DesignRuleExprRule>)) +
             option(lit<"<<"> +
                    some(append<&ast::DesignRuleDecl::annotations>(
                        ref<PropertyBlockEntryRule>)) +
                    lit<">>">) +
             lit<";">> {};

  struct FormalParamRule
      : rule<assign<&ast::FormalParam::name>(ref<IDENTIFIER>) + lit<":"> +
             assign<&ast::FormalParam::type>(TypeRefE)> {};

  struct DesignAnalysisDeclRule
      : rule<(option(ikw<"design">) + ikw<"analysis"> +
              assign<&ast::DesignAnalysisDecl::name>(ref<IDENTIFIER>) +
              lit<"("> +
              option(some(append<&ast::DesignAnalysisDecl::params>(
                              ref<FormalParamRule>),
                          lit<",">)) +
              lit<")"> + lit<":"> +
              assign<&ast::DesignAnalysisDecl::returnType>(TypeRefE) +
              lit<"="> +
              assign<&ast::DesignAnalysisDecl::body>(
                  ref<DesignRuleExprRule>) +
              option(lit<"<<"> +
                     some(append<&ast::DesignAnalysisDecl::annotations>(
                         ref<PropertyBlockEntryRule>)) +
                     lit<">>">) +
              lit<";">) |
             (flag<&ast::DesignAnalysisDecl::isExternal>(
                  ikw<"external">) +
              option(ikw<"design">) + ikw<"analysis"> +
              assign<&ast::DesignAnalysisDecl::name>(ref<IDENTIFIER>) +
              lit<"("> +
              option(some(append<&ast::DesignAnalysisDecl::params>(
                              ref<FormalParamRule>),
                          lit<",">)) +
              lit<")"> + lit<":"> +
              assign<&ast::DesignAnalysisDecl::returnType>(TypeRefE) +
              lit<"="> +
              assign<&ast::DesignAnalysisDecl::externalCode>(
                  ref<STRING_LITERAL> | ref<InstanceRefRule>) +
              lit<";">)> {};

  // ---- structural declarations ------------------------------------------

  struct RepresentationDeclRule;
  struct PropertyTypeDeclRule;
  struct PortTypeDeclRule;
  struct RoleTypeDeclRule;
  struct ComponentTypeDeclRule;
  struct ConnectorTypeDeclRule;
  struct ElementTypeDeclRule;
  struct GroupTypeDeclRule;
  struct PortDeclRule;
  struct RoleDeclRule;
  struct ComponentDeclRule;
  struct ConnectorDeclRule;
  struct GroupDeclRule;
  struct MembersBlockRule;
  struct AttachmentDeclRule;
  struct SystemDeclRule;

  static constexpr auto SystemMemberE =
      ref<PropertyDeclRule> | ref<ComponentDeclRule> |
      ref<ConnectorDeclRule> | ref<AttachmentDeclRule> | ref<GroupDeclRule> |
      ref<DesignRuleDeclRule>;
  static constexpr auto PortRoleMemberE = ref<PropertyDeclRule> |
                                          ref<DesignRuleDeclRule> |
                                          ref<RepresentationDeclRule>;
  static constexpr auto ComponentMemberE =
      ref<PropertyDeclRule> | ref<PortDeclRule> | ref<DesignRuleDeclRule> |
      ref<RepresentationDeclRule>;
  static constexpr auto ConnectorMemberE =
      ref<PropertyDeclRule> | ref<RoleDeclRule> | ref<DesignRuleDeclRule> |
      ref<RepresentationDeclRule>;
  static constexpr auto ElementMemberE =
      ref<PropertyDeclRule> | ref<DesignRuleDeclRule>;
  static constexpr auto GroupMemberE = ref<MembersBlockRule> |
                                       ref<PropertyDeclRule> |
                                       ref<DesignRuleDeclRule>;
  static constexpr auto FamilyMemberE =
      ref<PortTypeDeclRule> | ref<RoleTypeDeclRule> |
      ref<ComponentTypeDeclRule> | ref<ConnectorTypeDeclRule> |
      ref<ElementTypeDeclRule> | ref<PropertyTypeDeclRule> |
      ref<GroupTypeDeclRule> | ref<DesignAnalysisDeclRule> |
      ref<DesignRuleDeclRule> | ref<PortDeclRule> | ref<RoleDeclRule> |
      ref<ComponentDeclRule> | ref<ConnectorDeclRule> |
      ref<PropertyDeclRule> | ref<GroupDeclRule> | ref<AttachmentDeclRule>;

  template <auto Field>
  static constexpr auto TypeModifiers = many(append<Field>(ikw<"public"> | ikw<"private"> | ikw<"final"> | ikw<"abstract">));

  struct PropertyTypeDeclRule
      : rule<ikw<"property"> + ikw<"type"> +
             assign<&ast::PropertyTypeDecl::name>(ref<IDENTIFIER>) +
             (lit<";"> |
              (lit<"="> +
               assign<&ast::PropertyTypeDecl::type>(PropTypeE) + lit<";">))> {
  };

  struct PortTypeDeclRule
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

  struct RoleTypeDeclRule
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

  struct ComponentTypeDeclRule
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

  struct ConnectorTypeDeclRule
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

  struct ElementTypeDeclRule
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

  struct GroupTypeDeclRule
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

  struct PortDeclRule
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

  struct RoleDeclRule
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

  struct ComponentDeclRule
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

  struct ConnectorDeclRule
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

  struct MembersBlockRule
      : rule<ikw<"members"> + lit<"{"> +
             option(some(append<&ast::MembersBlock::refs>(
                             ref<InstanceRefRule>),
                         lit<",">)) +
             lit<"}"> + option(lit<";">)> {};

  struct GroupDeclRule
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

  struct AttachmentDeclRule
      : rule<ikw<"attachment"> +
             assign<&ast::AttachmentDecl::from>(ref<InstanceRefRule>) +
             ikw<"to"> +
             assign<&ast::AttachmentDecl::to>(ref<InstanceRefRule>) +
             lit<";">> {};

  struct BindingDeclRule
      : rule<assign<&ast::BindingDecl::from>(ref<InstanceRefRule>) +
             ikw<"to"> +
             assign<&ast::BindingDecl::to>(ref<InstanceRefRule>) +
             option(lit<"{"> +
                    assign<&ast::BindingDecl::property>(
                        ref<PropertyDeclRule>) +
                    lit<"<<"> +
                    some(append<&ast::BindingDecl::block>(
                        ref<PropertyBlockEntryRule>)) +
                    lit<">>"> + lit<"}">) +
             lit<";">> {};

  struct RepresentationDeclRule
      : rule<ikw<"representation"> +
             option(assign<&ast::RepresentationDecl::label>(
                        ref<IDENTIFIER>) +
                    lit<"=">) +
             lit<"{"> +
             assign<&ast::RepresentationDecl::system>(ref<SystemDeclRule>) +
             option(ikw<"bindings"> + lit<"{"> +
                    many(append<&ast::RepresentationDecl::bindings>(
                        ref<BindingDeclRule>)) +
                    lit<"}"> + option(lit<";">)) +
             lit<"}"> + option(lit<";">)> {};

  struct SystemDeclRule
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

  struct FamilyDeclRule
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

  struct DesignDeclRule : rule<create<ast::DesignDecl> + ikw<"design"> +
                               lit<";">> {};

  struct CompUnitRule
      : rule<many(append<&ast::CompUnit::imports>(ref<ImportRule>)) +
             some(append<&ast::CompUnit::decls>(
                 ref<SystemDeclRule> | ref<FamilyDeclRule> |
                 ref<DesignDeclRule>))> {};

  using entry = CompUnitRule;
  using ignore = WS;
  using hidden = list<LINE_COMMENT, BLOCK_COMMENT>;
};

}  // namespace acme
