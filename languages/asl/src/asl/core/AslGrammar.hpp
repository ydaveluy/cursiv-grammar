// The ASL language: its grammar.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string_view>

#include "asl/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace asl {

inline constexpr auto unescape_string =
    [](std::string_view t, std::pmr::memory_resource *mem) -> std::string_view {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('\\') == std::string_view::npos)
    return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\\' && i + 1 < body.size()) {
      ++i;
      switch (body[i]) {
      case 'r':
        room[n++] = '\r';
        break;
      case 'n':
        room[n++] = '\n';
        break;
      case 't':
        room[n++] = '\t';
        break;
      default:
        room[n++] = body[i];
        break;
      }
    } else {
      room[n++] = body[i];
    }
  }
  return std::string_view(room.data(), n);
};
using Unescaped = cursiv::text_conv<unescape_string>;

struct Asl : cursiv::GrammarSpec {
  // Trivia. A comment or a line continuation swallows its own trailing
  // newline, so it vanishes wherever it sits, including before the first
  // statement of the document.
  static constexpr auto EolUnit = (option(lit<"\r">) + lit<"\n">) | lit<"\r">;

  struct NEWLINE : terminal<some(EolUnit)> {};
  struct WHITESPACE : terminal<some(cr<R"( \t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + some(EolUnit)> {};
  struct CONTINUATION : terminal<lit<"\\"> + some(EolUnit)> {};
  // Embedded native code remains an explicit statement with its full text.
  struct NATIVE_LANGUAGE
      : terminal<through(lit<"$INLINE"> + some(EolUnit), lit<"$ENDINLINE">)> {};

  // Identifiers. Several of ASL's upper/lower-leading ID spellings overlap;
  // the widest always wins a maximal-munch tie at a shared position, so one
  // terminal each covers what several similarly-named forms spell
  // separately.
  struct VAR_ID : terminal<cr<"a-z"> + many(cr<"a-z0-9">) +
                           many(lit<"_"> + some(cr<"a-z0-9">))> {};
  struct OBJECT_ID : terminal<cr<"A-Z"> + many(cr<"A-Z">) +
                              many(lit<"_"> + some(cr<"A-Z">))> {};
  struct ATTR_ID : terminal<cr<"A-Z"> + many(cr<"A-Za-z">) +
                            many(lit<"_"> + some(cr<"A-Za-z">))> {};
  struct ENUM_ID : terminal<cr<"A-Z"> + many(cr<"A-Z0-9">) +
                            many(lit<"_"> + some(cr<"A-Z0-9">))> {};
  struct EVENT_CODE_ID
      : terminal<some(cr<"A-Z">) + many(lit<"_"> + some(cr<"A-Z">)) +
                 some(cr<"0-9">)> {};
  struct R_NUMBER : terminal<lit<"R"> + some(cr<"0-9">)> {};
  struct INSTANCE_ID_KW : terminal<kw<"instance_id">> {};

  // Literals.
  struct INTEGER_VALUE : terminal<option(lit<"-">) + some(d), std::int64_t> {};
  struct REAL_VALUE
      : terminal<option(lit<"-">) + some(d) + lit<"."> + some(d), double> {};
  struct DATE_VALUE : terminal<repeat<4>(d) + lit<"."> + repeat<2>(d) +
                               lit<"."> + repeat<2>(d)> {};
  struct TIME_VALUE : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) +
                               lit<":"> + repeat<2>(d)> {};
  struct BOOLEAN_VALUE
      : terminal<kw<"TRUE"> | kw<"FALSE">,
                 text_conv<[](std::string_view t) { return t == "TRUE"; }>> {};
  // One STRING terminal covers ASL's plain, escaped and empty text forms
  // alike: the widest already accepts everything the narrower two do.
  struct STRING
      : terminal<lit<"\""> +
                     many((lit<"\\"> + (lit<"\\"> | lit<"\""> | lit<"r"> |
                                        lit<"n"> | lit<"t">)) |
                          cr<R"(^"\\\r\n)">) +
                     lit<"\"">,
                 Unescaped> {};

  // Operators: one token per meaning, so a reader compares `op`, never text.
  struct EQ_OP : terminal<lit<"=">, value<ast::CompareOp::eq>> {};
  struct NEQ_OP : terminal<lit<"!=">, value<ast::CompareOp::neq>> {};
  struct EQUALS_OP : terminal<kw<"equals">, value<ast::CompareOp::eq>> {};
  struct NOT_EQUALS_OP
      : terminal<kw<"not-equals">, value<ast::CompareOp::neq>> {};
  struct LTE_OP : terminal<lit<"<=">, value<ast::CompareOp::lte>> {};
  struct GTE_OP : terminal<lit<">=">, value<ast::CompareOp::gte>> {};
  struct LT_OP : terminal<lit<"<">, value<ast::CompareOp::lt>> {};
  struct GT_OP : terminal<lit<">">, value<ast::CompareOp::gt>> {};
  struct LESS_THAN_OP : terminal<kw<"less-than">, value<ast::CompareOp::lt>> {};
  struct GREATER_THAN_OP
      : terminal<kw<"greater-than">, value<ast::CompareOp::gt>> {};
  struct AND_KW : terminal<kw<"and">, value<ast::LogicalOp::and_>> {};
  struct OR_KW : terminal<kw<"or">, value<ast::LogicalOp::or_>> {};
  struct AND_SYM : terminal<lit<"&">, value<ast::LogicalOp::and_>> {};
  struct OR_SYM : terminal<lit<"|">, value<ast::LogicalOp::or_>> {};
  struct ADD_OP : terminal<lit<"+">, value<ast::ArithOp::add>> {};
  struct SUB_OP : terminal<lit<"-">, value<ast::ArithOp::subtract>> {};
  struct MUL_OP : terminal<lit<"*">, value<ast::ArithOp::multiply>> {};
  struct DIV_OP : terminal<lit<"/">, value<ast::ArithOp::divide>> {};
  struct UNION_OF_OP
      : terminal<kw<"union-of">, value<ast::SetCombineOp::union_of>> {};
  struct DISUNION_OF_OP
      : terminal<kw<"disunion-of">, value<ast::SetCombineOp::disunion_of>> {};
  struct INTERSECTION_OF_OP
      : terminal<kw<"intersection-of">,
                 value<ast::SetCombineOp::intersection_of>> {};

  static constexpr auto EqualityOp =
      ref<EQ_OP> | ref<NEQ_OP> | ref<EQUALS_OP> | ref<NOT_EQUALS_OP>;
  static constexpr auto LogicalOperatorOp =
      EqualityOp | ref<LTE_OP> | ref<GTE_OP> | ref<LT_OP> | ref<GT_OP> |
      ref<LESS_THAN_OP> | ref<GREATER_THAN_OP>;

  // Forward declarations: the grammar is mutually recursive throughout
  // (a statement's body holds statements; an expression holds expressions).
  struct PlainVariable;
  struct VariableOrThis;
  struct AttributeRefRule;
  struct ObjectAttributeRule;
  struct StructureFieldRule;
  struct SetVariableRule;
  struct UndefinedValueRule;
  struct CurrentDateValueRule;
  struct CurrentTimeValueRule;
  struct ConstantRule;
  struct RNumberSpecRule;
  struct QualifiedRelationshipSpecRule;
  struct RelationshipRoleSpecRule;
  struct RelationshipSpecRule;
  struct CountOfRule;
  struct ArithOperand;
  struct ArithmeticExprRule;
  struct Operand;
  struct SimpleConditionInner;
  struct SimpleConditionRule;
  struct MaybeNotSimple;
  struct CompoundConditionInner;
  struct CompoundConditionRule;
  struct MaybeNotCompound;
  struct LogicalConditionRule;
  struct ObjectComparisonRule;
  struct ObjectConditionRule;
  struct StructComparisonRule;
  struct StructConditionRule;
  struct AssignmentRule;
  struct AttributeAssignmentRule;
  struct CreateStatementRule;
  struct DeleteStatementRule;
  struct FindInstanceStatementRule;
  struct FindSetStatementRule;
  struct RelationshipNavigationRule;
  struct AssociativeRelationshipNavigationRule;
  struct RelationshipLinkRule;
  struct RelationshipUnlinkRule;
  struct EventGenerationRule;
  struct OperationInvocationRule;
  struct TimerCreationRule;
  struct TimerDeletionRule;
  struct SetRelativeTimerRule;
  struct SetAbsoluteTimerRule;
  struct SetRecurringTimerRule;
  struct TimerResetRule;
  struct UniqueSetRule;
  struct SetCombinationRule;
  struct SetDifferenceRule;
  struct StructMemberRule;
  struct StructTypeRule;
  struct UserDefinedTypeRule;
  struct MemberTypeRule;
  struct StructDefinitionRule;
  struct StructInstantiationRule;
  struct StructAssemblyRule;
  struct ForLoopVarRule;
  struct StructForLoopRule;
  struct OrderStructRule;
  struct StructSubsetRule;
  struct CaseClauseRule;
  struct SwitchStatementRule;
  struct IfStatementRule;
  struct ForLoopRule;
  struct BreakStatementRule;
  struct LoopStatementRule;
  struct StatementDispatch;
  struct StatementRule;
  struct ModelRule;

  // Values.
  struct PlainVariable : rule<assign<&ast::VariableRef::name>(ref<VAR_ID>)> {};
  struct VariableOrThis
      : rule<assign<&ast::VariableRef::name>(ref<VAR_ID>) |
             flag<&ast::VariableRef::is_this>(kw<"this">)> {};
  struct AttributeRefRule : rule<assign<&ast::AttributeRef::name>(
                                ref<ATTR_ID> | ref<INSTANCE_ID_KW>)> {};
  struct ObjectAttributeRule
      : rule<create<ast::ObjectAttribute>(
            (assign<&ast::ObjectAttribute::base>(ref<VAR_ID>) |
             flag<&ast::ObjectAttribute::base_is_this>(kw<"this">)) +
            lit<"."> +
            assign<&ast::ObjectAttribute::attribute>(ref<ATTR_ID> |
                                                     ref<INSTANCE_ID_KW>))> {};
  struct StructureFieldRule
      : rule<create<ast::StructureField>(
            assign<&ast::StructureField::base>(ref<VAR_ID>) + lit<"."> +
            assign<&ast::StructureField::field>(ref<VAR_ID>))> {};
  struct SetVariableRule
      : rule<create<ast::SetVariable>(
            lit<"{"> + assign<&ast::SetVariable::name>(ref<VAR_ID>) +
            lit<"}">)> {};
  struct UndefinedValueRule
      : rule<create<ast::UndefinedValue> + kw<"UNDEFINED">> {};
  struct CurrentDateValueRule
      : rule<create<ast::CurrentDateValue> + kw<"current-date">> {};
  struct CurrentTimeValueRule
      : rule<create<ast::CurrentTimeValue> + kw<"current-time">> {};

  struct ConstantRule
      : rule<create<ast::IntegerConstant>(
                 assign<&ast::IntegerConstant::value>(ref<INTEGER_VALUE>)) |
             create<ast::RealConstant>(
                 assign<&ast::RealConstant::value>(ref<REAL_VALUE>)) |
             create<ast::BooleanConstant>(
                 assign<&ast::BooleanConstant::value>(ref<BOOLEAN_VALUE>)) |
             create<ast::TextConstant>(
                 assign<&ast::TextConstant::value>(ref<STRING>)) |
             create<ast::EnumConstant>(
                 assign<&ast::EnumConstant::value>(ref<ENUM_ID>)) |
             create<ast::DateConstant>(
                 assign<&ast::DateConstant::value>(ref<DATE_VALUE>)) |
             create<ast::TimeConstant>(
                 assign<&ast::TimeConstant::value>(ref<TIME_VALUE>))> {};

  struct RNumberSpecRule
      : rule<create<ast::RelationshipSpec>(
            assign<&ast::RelationshipSpec::number>(ref<R_NUMBER>))> {};
  struct QualifiedRelationshipSpecRule
      : rule<create<ast::QualifiedRelationshipSpec>(
            assign<&ast::QualifiedRelationshipSpec::number>(ref<R_NUMBER>) +
            lit<"."> +
            assign<&ast::QualifiedRelationshipSpec::object_name>(
                ref<OBJECT_ID>))> {};
  struct RelationshipRoleSpecRule
      : rule<create<ast::RelationshipRoleSpec>(
            assign<&ast::RelationshipRoleSpec::number>(ref<R_NUMBER>) +
            lit<"."> + assign<&ast::RelationshipRoleSpec::role>(ref<STRING>))> {
  };
  struct RelationshipSpecRule
      : rule<ref<QualifiedRelationshipSpecRule> |
             ref<RelationshipRoleSpecRule> | ref<RNumberSpecRule>> {};

  static constexpr auto QualifiedOrRoleSpec =
      ref<QualifiedRelationshipSpecRule> | ref<RelationshipRoleSpecRule>;

  struct CountOfRule
      : rule<kw<"countof"> +
             (create<ast::CountOf>(
                  assign<&ast::CountOf::set>(ref<SetVariableRule>)) |
              create<ast::CountOf>(
                  assign<&ast::CountOf::base>(ref<VariableOrThis>) + lit<"->"> +
                  assign<&ast::CountOf::spec>(ref<RelationshipSpecRule>)))> {};

  struct ArithOperand : rule<ref<CountOfRule> | ref<ObjectAttributeRule> |
                             ref<VariableOrThis> | ref<ConstantRule>> {};

  static constexpr auto AnyArithOp =
      ref<ADD_OP> | ref<SUB_OP> | ref<MUL_OP> | ref<DIV_OP>;

  struct ArithmeticExprRule
      : rule<create<ast::ArithmeticExpr>(
                 assign<&ast::ArithmeticExpr::left>(ref<ArithOperand>) +
                 assign<&ast::ArithmeticExpr::op>(AnyArithOp) +
                 assign<&ast::ArithmeticExpr::right>(ref<ArithOperand> |
                                                     ref<ArithmeticExprRule>)) |
             (lit<"("> + ref<ArithmeticExprRule> + lit<")">) |
             ref<CountOfRule>> {};

  // ASL has three near-identical operand shapes (one adds `this`, one a
  // narrower `countof`, one a bare set/struct); one dispatcher accepts
  // their union.
  struct Operand : rule<ref<ArithmeticExprRule> | ref<CountOfRule> |
                        ref<ObjectAttributeRule> | ref<SetVariableRule> |
                        ref<VariableOrThis> | ref<ConstantRule>> {};

  static constexpr auto AssignTarget =
      ref<ObjectAttributeRule> | ref<PlainVariable>;
  static constexpr auto AnyValue =
      ref<ArithmeticExprRule> | ref<CurrentDateValueRule> |
      ref<CurrentTimeValueRule> | ref<CountOfRule> | ref<ObjectAttributeRule> |
      ref<VariableOrThis> | ref<ConstantRule> | ref<UndefinedValueRule>;

  // Conditions.
  struct SimpleConditionInner
      : rule<create<ast::BoolCheck>(
                 assign<&ast::BoolCheck::value>(ref<PlainVariable>)) |
             create<ast::Comparison>(
                 assign<&ast::Comparison::left>(ref<Operand>) +
                 assign<&ast::Comparison::op>(LogicalOperatorOp) +
                 assign<&ast::Comparison::right>(ref<Operand> |
                                                 ref<UndefinedValueRule>))> {};
  struct LogicalConditionRule;
  struct SimpleConditionRule
      : rule<(lit<"("> + ref<LogicalConditionRule> + lit<")">) |
             ref<SimpleConditionInner>> {};
  struct MaybeNotSimple
      : rule<create<ast::LogicalNot>(
                 (kw<"not"> | lit<"!">)+assign<&ast::LogicalNot::operand>(
                     ref<MaybeNotSimple>)) |
             ref<SimpleConditionRule>> {};
  static constexpr auto AnyLogicalOp =
      ref<AND_KW> | ref<OR_KW> | ref<AND_SYM> | ref<OR_SYM>;
  struct LogicalConditionRule
      : infix<MaybeNotSimple, &ast::LogicalCombine::left,
              &ast::LogicalCombine::op, &ast::LogicalCombine::right,
              left(AnyLogicalOp)> {};

  static constexpr auto ObjCondRHS = ref<ObjectAttributeRule> |
                                     ref<VariableOrThis> | ref<ConstantRule> |
                                     ref<UndefinedValueRule>;

  struct ObjectComparisonRule
      : rule<(lit<"("> +
              create<ast::Comparison>(
                  assign<&ast::Comparison::left>(ref<AttributeRefRule>) +
                  assign<&ast::Comparison::op>(LogicalOperatorOp) +
                  assign<&ast::Comparison::right>(ObjCondRHS)) +
              lit<")">) |
             create<ast::Comparison>(
                 assign<&ast::Comparison::left>(ref<AttributeRefRule>) +
                 assign<&ast::Comparison::op>(LogicalOperatorOp) +
                 assign<&ast::Comparison::right>(ObjCondRHS))> {};
  struct ObjectConditionRule
      : infix<ObjectComparisonRule, &ast::LogicalCombine::left,
              &ast::LogicalCombine::op, &ast::LogicalCombine::right,
              (left(ref<AND_SYM> | ref<OR_SYM>))> {};

  struct StructComparisonRule
      : rule<create<ast::Comparison>(
            assign<&ast::Comparison::left>(ref<PlainVariable>) +
            assign<&ast::Comparison::op>(LogicalOperatorOp) +
            assign<&ast::Comparison::right>(ref<Operand>))> {};
  struct StructConditionRule
      : infix<StructComparisonRule, &ast::LogicalCombine::left,
              &ast::LogicalCombine::op, &ast::LogicalCombine::right,
              (left(AnyLogicalOp))> {};

  // Statements.
  struct AssignmentRule
      : rule<create<ast::Assignment>(
            assign<&ast::Assignment::target>(AssignTarget) + lit<"="> +
            assign<&ast::Assignment::value>(AnyValue))> {};

  struct AttributeAssignmentRule
      : rule<create<ast::AttributeAssignment>(
            assign<&ast::AttributeAssignment::attribute>(ref<ATTR_ID> |
                                                         ref<INSTANCE_ID_KW>) +
            lit<"="> + assign<&ast::AttributeAssignment::value>(AnyValue))> {};

  struct CreateStatementRule
      : rule<create<ast::CreateStatement>(
            assign<&ast::CreateStatement::target>(ref<VariableOrThis>) +
            lit<"="> + kw<"create"> +
            option(flag<&ast::CreateStatement::is_unique>(kw<"unique">)) +
            assign<&ast::CreateStatement::object_name>(ref<OBJECT_ID>) +
            option(kw<"with"> + some(append<&ast::CreateStatement::attributes>(
                                         ref<AttributeAssignmentRule>),
                                     lit<"&">)))> {};

  struct DeleteStatementRule
      : rule<create<ast::DeleteStatement>(
            kw<"delete"> +
            assign<&ast::DeleteStatement::target>(ref<VariableOrThis>))> {};

  struct FindInstanceStatementRule
      : rule<create<ast::FindInstanceStatement>(
            assign<&ast::FindInstanceStatement::variable>(ref<VAR_ID>) +
            lit<"="> +
            (flag<&ast::FindInstanceStatement::find_only>(
                 kw<"find-only">) |
             kw<"find-one">)+(assign<&ast::FindInstanceStatement::
                                         object_name>(ref<OBJECT_ID>) |
                              assign<&ast::FindInstanceStatement::source>(
                                  ref<SetVariableRule>)) +
            option(kw<"where"> + assign<&ast::FindInstanceStatement::where>(
                                     ref<ObjectConditionRule>)))> {};

  struct FindSetStatementRule
      : rule<create<ast::FindSetStatement>(
            assign<&ast::FindSetStatement::target>(ref<SetVariableRule>) +
            lit<"="> +
            (((kw<"find"> +
               assign<&ast::FindSetStatement::object_name>(ref<OBJECT_ID>)) |
              (option(kw<"find">) +
               assign<&ast::FindSetStatement::source>(ref<SetVariableRule>))) +
                 kw<"where"> +
                 assign<&ast::FindSetStatement::where>(
                     ref<ObjectConditionRule>) |
             (flag<&ast::FindSetStatement::find_all>(kw<"find-all">) +
              assign<&ast::FindSetStatement::object_name>(ref<OBJECT_ID>))) +
            option(
                (flag<&ast::FindSetStatement::reverse_ordered>(
                     kw<"reverse ordered by">) |
                 flag<&ast::FindSetStatement::ordered>(kw<"ordered by">)) +
                assign<&ast::FindSetStatement::order_by>(
                    ref<ATTR_ID> | ref<INSTANCE_ID_KW>)))> {};

  struct RelationshipNavigationRule
      : rule<create<ast::RelationshipNavigation>(
            assign<&ast::RelationshipNavigation::target>(ref<SetVariableRule> |
                                                         ref<PlainVariable>) +
            lit<"="> +
            assign<&ast::RelationshipNavigation::source>(ref<SetVariableRule> |
                                                         ref<VariableOrThis>) +
            some(lit<"->"> + append<&ast::RelationshipNavigation::path>(
                                 ref<RelationshipSpecRule>)) +
            option(kw<"where"> + assign<&ast::RelationshipNavigation::where>(
                                     ref<ObjectConditionRule>)) +
            option((flag<&ast::RelationshipNavigation::reverse_ordered>(
                        kw<"reverse ordered by">) |
                    flag<&ast::RelationshipNavigation::ordered>(
                        kw<"ordered by">)) +
                   assign<&ast::RelationshipNavigation::order_by>(
                       ref<ATTR_ID> | ref<INSTANCE_ID_KW>)))> {};

  struct AssociativeRelationshipNavigationRule
      : rule<create<ast::AssociativeRelationshipNavigation>(
            assign<&ast::AssociativeRelationshipNavigation::target>(
                ref<VAR_ID>) +
            lit<"="> +
            assign<&ast::AssociativeRelationshipNavigation::left>(
                ref<VariableOrThis>) +
            kw<"and"> +
            assign<&ast::AssociativeRelationshipNavigation::right>(
                ref<VariableOrThis>) +
            lit<"->"> +
            assign<&ast::AssociativeRelationshipNavigation::spec>(
                QualifiedOrRoleSpec))> {};

  struct RelationshipLinkRule
      : rule<create<ast::RelationshipLink>(
            kw<"link"> +
            assign<&ast::RelationshipLink::from>(ref<VariableOrThis>) +
            assign<&ast::RelationshipLink::spec>(ref<RelationshipSpecRule>) +
            assign<&ast::RelationshipLink::to>(ref<VariableOrThis>) +
            option(
                flag<&ast::RelationshipLink::has_using>(kw<"using">) +
                assign<&ast::RelationshipLink::using_handle>(ref<VAR_ID>)))> {};

  struct RelationshipUnlinkRule
      : rule<create<ast::RelationshipUnlink>(
            kw<"unlink"> +
            assign<&ast::RelationshipUnlink::from>(ref<VariableOrThis>) +
            assign<&ast::RelationshipUnlink::spec>(ref<RelationshipSpecRule>) +
            assign<&ast::RelationshipUnlink::to>(ref<VariableOrThis>))> {};

  static constexpr auto EventParam =
      ref<StructureFieldRule> | ref<ObjectAttributeRule> | ref<VariableOrThis> |
      ref<ConstantRule> | ref<UndefinedValueRule>;

  struct EventGenerationRule
      : rule<create<ast::EventGeneration>(
            kw<"%generate"> +
            assign<&ast::EventGeneration::code>(ref<EVENT_CODE_ID>) + lit<":"> +
            assign<&ast::EventGeneration::name>(ref<ATTR_ID>) + lit<"("> +
            option(some(append<&ast::EventGeneration::args>(EventParam),
                        lit<",">)) +
            lit<")"> + lit<";"> +
            option(kw<"to"> + assign<&ast::EventGeneration::target>(
                                  ref<VariableOrThis>)))> {};

  static constexpr auto OutputParam =
      ref<ObjectAttributeRule> | ref<SetVariableRule> | ref<VariableOrThis>;
  static constexpr auto InputParam = ref<ObjectAttributeRule> |
                                     ref<SetVariableRule> | ref<ConstantRule> |
                                     ref<VariableOrThis>;

  struct OperationInvocationRule
      : rule<create<ast::OperationInvocation>(
            lit<"["> +
            option(some(append<&ast::OperationInvocation::outputs>(OutputParam),
                        lit<",">)) +
            lit<"]"> + lit<"="> +
            assign<&ast::OperationInvocation::code>(ref<EVENT_CODE_ID>) +
            lit<":"> + assign<&ast::OperationInvocation::name>(ref<ATTR_ID>) +
            lit<"["> +
            option(some(append<&ast::OperationInvocation::inputs>(InputParam),
                        lit<",">)) +
            lit<"]">)> {};

  static constexpr auto TimerIdOperand =
      ref<ObjectAttributeRule> | ref<PlainVariable>;
  static constexpr auto TimeFieldOperand =
      ref<ObjectAttributeRule> | ref<PlainVariable> | ref<ConstantRule>;

  struct TimerCreationRule
      : rule<create<ast::TimerCreation>(
            lit<"["> + assign<&ast::TimerCreation::timer_id>(TimerIdOperand) +
            lit<"]"> + lit<"="> + kw<"Create_Timer"> + lit<"["> + lit<"]">)> {};

  struct TimerDeletionRule
      : rule<create<ast::TimerDeletion>(
            lit<"["> + lit<"]"> + lit<"="> + kw<"Delete_Timer"> + lit<"["> +
            assign<&ast::TimerDeletion::timer_id>(TimerIdOperand) + lit<"]">)> {
  };

  struct SetRelativeTimerRule
      : rule<create<ast::SetRelativeTimer>(
            kw<"%generate"> + kw<"TIM1:Set_Timer"> + lit<"("> +
            assign<&ast::SetRelativeTimer::time_to_fire>(TimeFieldOperand) +
            lit<","> +
            assign<&ast::SetRelativeTimer::return_event>(ref<EVENT_CODE_ID>) +
            lit<","> +
            assign<&ast::SetRelativeTimer::target_instance>(
                ref<VariableOrThis>) +
            lit<")"> + lit<";"> + kw<"to"> +
            assign<&ast::SetRelativeTimer::timer_id>(TimerIdOperand))> {};

  struct SetAbsoluteTimerRule
      : rule<create<ast::SetAbsoluteTimer>(
            kw<"%generate"> + kw<"TIM10:Set_Absolute_Timer"> + lit<"("> +
            assign<&ast::SetAbsoluteTimer::year>(TimeFieldOperand) + lit<","> +
            assign<&ast::SetAbsoluteTimer::month>(TimeFieldOperand) + lit<","> +
            assign<&ast::SetAbsoluteTimer::day>(TimeFieldOperand) + lit<","> +
            assign<&ast::SetAbsoluteTimer::hour>(TimeFieldOperand) + lit<","> +
            assign<&ast::SetAbsoluteTimer::minute>(TimeFieldOperand) +
            lit<","> +
            assign<&ast::SetAbsoluteTimer::second>(TimeFieldOperand) +
            lit<","> +
            assign<&ast::SetAbsoluteTimer::return_event>(ref<EVENT_CODE_ID>) +
            lit<","> +
            assign<&ast::SetAbsoluteTimer::target_instance>(
                ref<VariableOrThis>) +
            lit<")"> + lit<";"> + kw<"to"> +
            assign<&ast::SetAbsoluteTimer::timer_id>(TimerIdOperand))> {};

  struct SetRecurringTimerRule
      : rule<create<ast::SetRecurringTimer>(
            kw<"%generate"> + kw<"TIM3:Set_Chimer"> + lit<"("> +
            assign<&ast::SetRecurringTimer::start_time>(TimeFieldOperand) +
            lit<","> +
            assign<&ast::SetRecurringTimer::period>(TimeFieldOperand) +
            lit<","> +
            assign<&ast::SetRecurringTimer::return_event>(ref<EVENT_CODE_ID>) +
            lit<","> +
            assign<&ast::SetRecurringTimer::target_instance>(
                ref<VariableOrThis>) +
            lit<")"> + lit<";"> + kw<"to"> +
            assign<&ast::SetRecurringTimer::timer_id>(TimerIdOperand))> {};

  struct TimerResetRule
      : rule<create<ast::TimerReset>(
            kw<"%generate"> + kw<"TIM2:Reset_Timer"> + lit<"("> + lit<")"> +
            lit<";"> + kw<"to"> +
            assign<&ast::TimerReset::timer_id>(TimerIdOperand))> {};

  struct UniqueSetRule
      : rule<create<ast::UniqueSet>(
            assign<&ast::UniqueSet::target>(ref<SetVariableRule>) + lit<"="> +
            kw<"unique"> +
            assign<&ast::UniqueSet::source>(ref<SetVariableRule>))> {};

  struct SetCombinationRule
      : rule<create<ast::SetCombination>(
            assign<&ast::SetCombination::target>(ref<SetVariableRule>) +
            lit<"="> +
            assign<&ast::SetCombination::op>(ref<UNION_OF_OP> |
                                             ref<DISUNION_OF_OP> |
                                             ref<INTERSECTION_OF_OP>) +
            assign<&ast::SetCombination::a>(ref<SetVariableRule>) + kw<"and"> +
            assign<&ast::SetCombination::b>(ref<SetVariableRule>))> {};

  struct SetDifferenceRule
      : rule<create<ast::SetDifference>(
            assign<&ast::SetDifference::target>(ref<SetVariableRule>) +
            lit<"="> + assign<&ast::SetDifference::a>(ref<SetVariableRule>) +
            kw<"not-in"> +
            assign<&ast::SetDifference::b>(ref<SetVariableRule>))> {};

  struct StructMemberRule
      : rule<create<ast::StructMember>(
            assign<&ast::StructMember::name>(ref<VAR_ID>) +
            assign<&ast::StructMember::type_text>(ref<MemberTypeRule>))> {};

  struct StructTypeRule
      : rule<(ref<ATTR_ID> + lit<"."> + ref<VAR_ID>) | ref<VAR_ID>> {};
  struct UserDefinedTypeRule : rule<ref<OBJECT_ID> + lit<"."> + ref<ATTR_ID>> {
  };
  struct MemberTypeRule : rule<kw<"Integer"> | kw<"Real"> | kw<"Boolean"> |
                               kw<"Text"> | kw<"Date"> | kw<"Time_of_Day"> |
                               ref<StructTypeRule> | ref<UserDefinedTypeRule>> {
  };

  struct StructDefinitionRule
      : rule<create<ast::StructDefinition>(
            kw<"define"> + kw<"structure"> +
            assign<&ast::StructDefinition::name>(ref<StructTypeRule>) +
            ref<NEWLINE> +
            some(
                append<&ast::StructDefinition::members>(ref<StructMemberRule>) +
                ref<NEWLINE>) +
            kw<"enddefine">)> {};

  struct StructInstantiationRule
      : rule<create<ast::StructInstantiation>(
            assign<&ast::StructInstantiation::target>(ref<SetVariableRule>) +
            kw<"is"> +
            assign<&ast::StructInstantiation::type_name>(
                ref<StructTypeRule>))> {};

  struct StructAssemblyRule
      : rule<create<ast::StructAssembly>(
            kw<"append"> + lit<"["> +
            some(append<&ast::StructAssembly::values>(ref<Operand>), lit<",">) +
            lit<"]"> + kw<"to"> +
            assign<&ast::StructAssembly::target>(ref<SetVariableRule>) +
            option(
                (flag<&ast::StructAssembly::reverse_ordered>(
                     kw<"reverse ordered by">) |
                 flag<&ast::StructAssembly::ordered>(kw<"ordered by">)) +
                assign<&ast::StructAssembly::order_by>(ref<VAR_ID>)))> {};

  struct ForLoopVarRule
      : rule<(flag<&ast::ForLoopVar::is_set>(lit<"{">) +
              assign<&ast::ForLoopVar::name>(ref<VAR_ID>) + lit<"}">) |
             assign<&ast::ForLoopVar::name>(ref<VAR_ID>)> {};

  struct BreakStatementRule
      : rule<create<ast::BreakStatement>(
            (kw<"break"> |
             (kw<"breakif"> + assign<&ast::BreakStatement::condition>(
                                  ref<LogicalConditionRule>))) +
            ref<NEWLINE>)> {};

  struct StructForLoopRule
      : rule<create<ast::StructForLoop>(
            kw<"for"> + lit<"["> +
            some(append<&ast::StructForLoop::vars>(ref<ForLoopVarRule>),
                 lit<",">) +
            lit<"]"> + kw<"in"> +
            assign<&ast::StructForLoop::source>(ref<SetVariableRule>) +
            kw<"do"> + ref<NEWLINE> +
            many(append<&ast::StructForLoop::body>(ref<StatementRule>)) +
            option(
                append<&ast::StructForLoop::body>(ref<BreakStatementRule>) +
                many(append<&ast::StructForLoop::body>(ref<StatementRule>))) +
            kw<"endfor">)> {};

  struct OrderStructRule
      : rule<create<ast::OrderStruct>(
            assign<&ast::OrderStruct::target>(ref<SetVariableRule>) + lit<"="> +
            assign<&ast::OrderStruct::source>(ref<SetVariableRule>) +
            (flag<&ast::OrderStruct::reverse_ordered>(
                 kw<"reverse ordered by">) |
             kw<"ordered by">)+assign<&ast::OrderStruct::
                                          member>(ref<VAR_ID>))> {};

  struct StructSubsetRule
      : rule<create<ast::StructSubset>(
            assign<&ast::StructSubset::target>(ref<SetVariableRule>) +
            lit<"="> +
            assign<&ast::StructSubset::source>(ref<SetVariableRule>) +
            kw<"where"> +
            assign<&ast::StructSubset::where>(ref<StructConditionRule>))> {};

  struct CaseClauseRule
      : rule<create<ast::CaseClause>(
            kw<"case"> + assign<&ast::CaseClause::value>(ref<ConstantRule>) +
            ref<NEWLINE> +
            many(append<&ast::CaseClause::body>(ref<StatementRule>)))> {};

  struct SwitchStatementRule
      : rule<create<ast::SwitchStatement>(
            kw<"switch"> +
            assign<&ast::SwitchStatement::target>(ref<ObjectAttributeRule> |
                                                  ref<PlainVariable>) +
            ref<NEWLINE> +
            some(append<&ast::SwitchStatement::cases>(ref<CaseClauseRule>)) +
            option(
                flag<&ast::SwitchStatement::has_default>(kw<"default">) +
                ref<NEWLINE> +
                some(append<&ast::SwitchStatement::default_body>(
                    ref<StatementRule>))) +
            kw<"endswitch">)> {};

  struct IfStatementRule
      : rule<create<ast::IfStatement>(
            kw<"if"> +
            assign<&ast::IfStatement::condition>(
                (lit<"("> + ref<LogicalConditionRule> + lit<")">) |
                ref<LogicalConditionRule>) +
            kw<"then"> + ref<NEWLINE> +
            some(append<&ast::IfStatement::then_body>(ref<StatementRule>)) +
            option(flag<&ast::IfStatement::has_else>(kw<"else">) +
                   ref<NEWLINE> +
                   some(append<&ast::IfStatement::else_body>(
                       ref<StatementRule>))) +
            kw<"endif">)> {};

  struct ForLoopRule
      : rule<create<ast::ForLoop>(
            kw<"for"> + assign<&ast::ForLoop::variable>(ref<VAR_ID>) +
            kw<"in"> + assign<&ast::ForLoop::source>(ref<SetVariableRule>) +
            kw<"do"> + ref<NEWLINE> +
            many(append<&ast::ForLoop::body>(ref<StatementRule>)) +
            option(append<&ast::ForLoop::body>(ref<BreakStatementRule>) +
                   many(append<&ast::ForLoop::body>(ref<StatementRule>))) +
            kw<"endfor">)> {};

  struct LoopStatementRule
      : rule<create<ast::LoopStatement>(
            kw<"loop"> + ref<NEWLINE> +
            many(append<&ast::LoopStatement::body>(ref<StatementRule>)) +
            append<&ast::LoopStatement::body>(ref<BreakStatementRule>) +
            many(append<&ast::LoopStatement::body>(ref<BreakStatementRule> |
                                                   ref<StatementRule>)) +
            kw<"endloop">)> {};

  struct NativeStatementRule
      : rule<assign<&ast::NativeStatement::text>(ref<NATIVE_LANGUAGE>)> {};

  struct StatementDispatch
      : rule<ref<NativeStatementRule> | ref<AssignmentRule> |
             ref<CreateStatementRule> | ref<DeleteStatementRule> |
             ref<FindInstanceStatementRule> | ref<FindSetStatementRule> |
             ref<RelationshipNavigationRule> |
             ref<AssociativeRelationshipNavigationRule> |
             ref<RelationshipLinkRule> | ref<RelationshipUnlinkRule> |
             ref<EventGenerationRule> | ref<OperationInvocationRule> |
             ref<TimerCreationRule> | ref<TimerDeletionRule> |
             ref<SetRelativeTimerRule> | ref<SetAbsoluteTimerRule> |
             ref<SetRecurringTimerRule> | ref<TimerResetRule> |
             ref<UniqueSetRule> | ref<SetCombinationRule> |
             ref<SetDifferenceRule> | ref<StructDefinitionRule> |
             ref<StructInstantiationRule> | ref<StructAssemblyRule> |
             ref<StructForLoopRule> | ref<OrderStructRule> |
             ref<StructSubsetRule> | ref<SwitchStatementRule> |
             ref<IfStatementRule> | ref<ForLoopRule> | ref<LoopStatementRule>> {
  };

  struct StatementRule : rule<ref<StatementDispatch> + ref<NEWLINE>> {};

  struct ModelRule
      : rule<create<ast::Model> +
             some(append<&ast::Model::statements>(ref<StatementRule>))> {};

  using entry = ModelRule;
  using ignore = list<WHITESPACE, COMMENT, CONTINUATION>;
};

} // namespace asl
