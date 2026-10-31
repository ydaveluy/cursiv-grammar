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

struct Asl : cursiv::GrammarSpec {
  // Trivia. A comment or a line continuation swallows its own trailing
  // newline, so it vanishes wherever it sits, including before the first
  // statement of the document.
  static constexpr auto EolUnit = eol;

  struct NEWLINE : terminal<some(EolUnit)> {};
  struct WHITESPACE : terminal<some(cr<R"( \t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + some(EolUnit)> {};
  struct CONTINUATION : terminal<lit<"\\"> + some(EolUnit)> {};
  // Embedded native code remains an explicit statement with its full text.
  struct NATIVE_LANGUAGE
      : terminal<through(lit<"$INLINE"> + some(EolUnit), lit<"$ENDINLINE">)> {};

  // Identifier spellings overlap. The lexer takes the longest match;
  // equal-length pattern matches can each continue their parse.
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
  struct INTEGER_VALUE
      : terminal<as<std::int64_t>(option(lit<"-">) + some(d))> {};
  struct REAL_VALUE
      : terminal<as<double>(option(lit<"-">) + some(d) + lit<"."> + some(d))> {
  };
  struct DATE_VALUE : terminal<repeat<4>(d) + lit<"."> + repeat<2>(d) +
                               lit<"."> + repeat<2>(d)> {};
  struct TIME_VALUE : terminal<repeat<2>(d) + lit<":"> + repeat<2>(d) +
                               lit<":"> + repeat<2>(d)> {};
  struct BOOLEAN_VALUE
      : terminal<as<[](std::string_view t) { return t == "TRUE"; }>(
            kw<"TRUE"> | kw<"FALSE">)> {};
  // One STRING terminal covers ASL's plain, escaped and empty text forms
  // alike: the widest already accepts everything the narrower two do.
  struct STRING : terminal<as<unescape_string>(
                      lit<"\""> +
                      many((lit<"\\"> + (lit<"\\"> | lit<"\""> | lit<"r"> |
                                         lit<"n"> | lit<"t">)) |
                           cr<R"(^"\\\r\n)">) +
                      lit<"\"">)> {};

  // Operators: one token per meaning, so a reader compares `op`, never text.

  static constexpr auto EqualityOp =
      as<ast::CompareOp::eq>(lit<"=">) | as<ast::CompareOp::neq>(lit<"!=">) | as<ast::CompareOp::eq>(kw<"equals">) | as<ast::CompareOp::neq>(kw<"not-equals">);
  static constexpr auto LogicalOperatorOp =
      EqualityOp | as<ast::CompareOp::lte>(lit<"<=">) | as<ast::CompareOp::gte>(lit<">=">) | as<ast::CompareOp::lt>(lit<"<">) | as<ast::CompareOp::gt>(lit<">">) |
      as<ast::CompareOp::lt>(kw<"less-than">) | as<ast::CompareOp::gt>(kw<"greater-than">);

  // Forward declarations: the grammar is mutually recursive throughout
  // (a statement's body holds statements; an expression holds expressions).
  struct PlainVariable;
  struct VariableOrThis;
  struct AttributeRef;
  struct ObjectAttribute;
  struct StructureField;
  struct SetVariable;
  struct UndefinedValue;
  struct CurrentDateValue;
  struct CurrentTimeValue;
  struct Constant;
  struct RNumberSpec;
  struct QualifiedRelationshipSpec;
  struct RelationshipRoleSpec;
  struct RelationshipSpec;
  struct CountOf;
  struct ArithOperand;
  struct ArithmeticExpr;
  struct Operand;
  struct SimpleConditionInner;
  struct SimpleCondition;
  struct MaybeNotSimple;
  struct CompoundConditionInner;
  struct CompoundCondition;
  struct MaybeNotCompound;
  struct LogicalCondition;
  struct ObjectComparison;
  struct ObjectCondition;
  struct StructComparison;
  struct StructCondition;
  struct Assignment;
  struct AttributeAssignment;
  struct CreateStatement;
  struct DeleteStatement;
  struct FindInstanceStatement;
  struct FindSetStatement;
  struct RelationshipNavigation;
  struct AssociativeRelationshipNavigation;
  struct RelationshipLink;
  struct RelationshipUnlink;
  struct EventGeneration;
  struct OperationInvocation;
  struct TimerCreation;
  struct TimerDeletion;
  struct SetRelativeTimer;
  struct SetAbsoluteTimer;
  struct SetRecurringTimer;
  struct TimerReset;
  struct UniqueSet;
  struct SetCombination;
  struct SetDifference;
  struct StructMember;
  struct StructType;
  struct UserDefinedType;
  struct MemberType;
  struct StructDefinition;
  struct StructInstantiation;
  struct StructAssembly;
  struct ForLoopVar;
  struct StructForLoop;
  struct OrderStruct;
  struct StructSubset;
  struct CaseClause;
  struct SwitchStatement;
  struct IfStatement;
  struct ForLoop;
  struct BreakStatement;
  struct LoopStatement;
  struct StatementDispatch;
  struct Statement;
  struct Model;

  // Values.
  struct PlainVariable : rule<assign<&ast::VariableRef::name>(ref<VAR_ID>)> {};
  struct VariableOrThis
      : rule<assign<&ast::VariableRef::name>(ref<VAR_ID>) |
             flag<&ast::VariableRef::is_this>(kw<"this">)> {};
  struct AttributeRef : rule<assign<&ast::AttributeRef::name>(
                                ref<ATTR_ID> | ref<INSTANCE_ID_KW>)> {};
  struct ObjectAttribute
      : rule<create<ast::ObjectAttribute>(
            (assign<&ast::ObjectAttribute::base>(ref<VAR_ID>) |
             flag<&ast::ObjectAttribute::base_is_this>(kw<"this">)) +
            lit<"."> +
            assign<&ast::ObjectAttribute::attribute>(ref<ATTR_ID> |
                                                     ref<INSTANCE_ID_KW>))> {};
  struct StructureField
      : rule<create<ast::StructureField>(
            assign<&ast::StructureField::base>(ref<VAR_ID>) + lit<"."> +
            assign<&ast::StructureField::field>(ref<VAR_ID>))> {};
  struct SetVariable
      : rule<create<ast::SetVariable>(
            lit<"{"> + assign<&ast::SetVariable::name>(ref<VAR_ID>) +
            lit<"}">)> {};
  struct UndefinedValue
      : rule<create<ast::UndefinedValue> + kw<"UNDEFINED">> {};
  struct CurrentDateValue
      : rule<create<ast::CurrentDateValue> + kw<"current-date">> {};
  struct CurrentTimeValue
      : rule<create<ast::CurrentTimeValue> + kw<"current-time">> {};

  struct Constant
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

  struct RNumberSpec
      : rule<assign<&ast::RelationshipSpec::number>(ref<R_NUMBER>)> {};
  struct QualifiedRelationshipSpec
      : rule<create<ast::QualifiedRelationshipSpec>(
            assign<&ast::QualifiedRelationshipSpec::number>(ref<R_NUMBER>) +
            lit<"."> +
            assign<&ast::QualifiedRelationshipSpec::object_name>(
                ref<OBJECT_ID>))> {};
  struct RelationshipRoleSpec
      : rule<create<ast::RelationshipRoleSpec>(
            assign<&ast::RelationshipRoleSpec::number>(ref<R_NUMBER>) +
            lit<"."> + assign<&ast::RelationshipRoleSpec::role>(ref<STRING>))> {
  };
  struct RelationshipSpec
      : rule<ref<QualifiedRelationshipSpec> |
             ref<RelationshipRoleSpec> | ref<RNumberSpec>> {};

  static constexpr auto QualifiedOrRoleSpec =
      ref<QualifiedRelationshipSpec> | ref<RelationshipRoleSpec>;

  struct CountOf
      : rule<kw<"countof"> +
             (create<ast::CountOf>(
                  assign<&ast::CountOf::set>(ref<SetVariable>)) |
              create<ast::CountOf>(
                  assign<&ast::CountOf::base>(ref<VariableOrThis>) + lit<"->"> +
                  assign<&ast::CountOf::spec>(ref<RelationshipSpec>)))> {};

  struct ArithOperand : rule<ref<CountOf> | ref<ObjectAttribute> |
                             ref<VariableOrThis> | ref<Constant>> {};

  static constexpr auto AnyArithOp =
      as<ast::ArithOp::add>(lit<"+">) | as<ast::ArithOp::subtract>(lit<"-">) | as<ast::ArithOp::multiply>(lit<"*">) | as<ast::ArithOp::divide>(lit<"/">);

  struct ArithmeticExpr
      : rule<create<ast::ArithmeticExpr>(
                 assign<&ast::ArithmeticExpr::left>(ref<ArithOperand>) +
                 assign<&ast::ArithmeticExpr::op>(AnyArithOp) +
                 assign<&ast::ArithmeticExpr::right>(ref<ArithOperand> |
                                                     ref<ArithmeticExpr>)) |
             (lit<"("> + ref<ArithmeticExpr> + lit<")">) |
             ref<CountOf>> {};

  // ASL has three near-identical operand shapes (one adds `this`, one a
  // narrower `countof`, one a bare set/struct); one dispatcher accepts
  // their union.
  struct Operand : rule<ref<ArithmeticExpr> | ref<CountOf> |
                        ref<ObjectAttribute> | ref<SetVariable> |
                        ref<VariableOrThis> | ref<Constant>> {};

  static constexpr auto AssignTarget =
      ref<ObjectAttribute> | ref<PlainVariable>;
  static constexpr auto AnyValue =
      ref<ArithmeticExpr> | ref<CurrentDateValue> |
      ref<CurrentTimeValue> | ref<CountOf> | ref<ObjectAttribute> |
      ref<VariableOrThis> | ref<Constant> | ref<UndefinedValue>;

  // Conditions.
  struct SimpleConditionInner
      : rule<create<ast::BoolCheck>(
                 assign<&ast::BoolCheck::value>(ref<PlainVariable>)) |
             create<ast::Comparison>(
                 assign<&ast::Comparison::left>(ref<Operand>) +
                 assign<&ast::Comparison::op>(LogicalOperatorOp) +
                 assign<&ast::Comparison::right>(ref<Operand> |
                                                 ref<UndefinedValue>))> {};
  struct SimpleCondition
      : rule<(lit<"("> + ref<LogicalCondition> + lit<")">) |
             ref<SimpleConditionInner>> {};
  struct MaybeNotSimple
      : rule<create<ast::LogicalNot>(
                 (kw<"not"> | lit<"!">)+assign<&ast::LogicalNot::operand>(
                     ref<MaybeNotSimple>)) |
             ref<SimpleCondition>> {};
  static constexpr auto AnyLogicalOp =
      as<ast::LogicalOp::and_>(kw<"and">) | as<ast::LogicalOp::or_>(kw<"or">) | as<ast::LogicalOp::and_>(lit<"&">) | as<ast::LogicalOp::or_>(lit<"|">);
  struct LogicalCondition
      : infix<MaybeNotSimple, ast::LogicalCombine,
              left(AnyLogicalOp)> {};

  static constexpr auto ObjCondRHS = ref<ObjectAttribute> |
                                     ref<VariableOrThis> | ref<Constant> |
                                     ref<UndefinedValue>;

  struct ObjectComparison
      : rule<(lit<"("> +
              create<ast::Comparison>(
                  assign<&ast::Comparison::left>(ref<AttributeRef>) +
                  assign<&ast::Comparison::op>(LogicalOperatorOp) +
                  assign<&ast::Comparison::right>(ObjCondRHS)) +
              lit<")">) |
             create<ast::Comparison>(
                 assign<&ast::Comparison::left>(ref<AttributeRef>) +
                 assign<&ast::Comparison::op>(LogicalOperatorOp) +
                 assign<&ast::Comparison::right>(ObjCondRHS))> {};
  struct ObjectCondition
      : infix<ObjectComparison, ast::LogicalCombine,
              (left(as<ast::LogicalOp::and_>(lit<"&">) | as<ast::LogicalOp::or_>(lit<"|">)))> {};

  struct StructComparison
      : rule<create<ast::Comparison>(
            assign<&ast::Comparison::left>(ref<PlainVariable>) +
            assign<&ast::Comparison::op>(LogicalOperatorOp) +
            assign<&ast::Comparison::right>(ref<Operand>))> {};
  struct StructCondition
      : infix<StructComparison, ast::LogicalCombine,
              (left(AnyLogicalOp))> {};

  // Statements.
  struct Assignment
      : rule<create<ast::Assignment>(
            assign<&ast::Assignment::target>(AssignTarget) + lit<"="> +
            assign<&ast::Assignment::value>(AnyValue))> {};

  struct AttributeAssignment
      : rule<create<ast::AttributeAssignment>(
            assign<&ast::AttributeAssignment::attribute>(ref<ATTR_ID> |
                                                         ref<INSTANCE_ID_KW>) +
            lit<"="> + assign<&ast::AttributeAssignment::value>(AnyValue))> {};

  struct CreateStatement
      : rule<create<ast::CreateStatement>(
            assign<&ast::CreateStatement::target>(ref<VariableOrThis>) +
            lit<"="> + kw<"create"> +
            option(flag<&ast::CreateStatement::is_unique>(kw<"unique">)) +
            assign<&ast::CreateStatement::object_name>(ref<OBJECT_ID>) +
            option(kw<"with"> + some(append<&ast::CreateStatement::attributes>(
                                         ref<AttributeAssignment>),
                                     lit<"&">)))> {};

  struct DeleteStatement
      : rule<create<ast::DeleteStatement>(
            kw<"delete"> +
            assign<&ast::DeleteStatement::target>(ref<VariableOrThis>))> {};

  struct FindInstanceStatement
      : rule<create<ast::FindInstanceStatement>(
            assign<&ast::FindInstanceStatement::variable>(ref<VAR_ID>) +
            lit<"="> +
            (flag<&ast::FindInstanceStatement::find_only>(
                 kw<"find-only">) |
             kw<"find-one">)+(assign<&ast::FindInstanceStatement::
                                         object_name>(ref<OBJECT_ID>) |
                              assign<&ast::FindInstanceStatement::source>(
                                  ref<SetVariable>)) +
            option(kw<"where"> + assign<&ast::FindInstanceStatement::where>(
                                     ref<ObjectCondition>)))> {};

  struct FindSetStatement
      : rule<create<ast::FindSetStatement>(
            assign<&ast::FindSetStatement::target>(ref<SetVariable>) +
            lit<"="> +
            ((((kw<"find"> +
               assign<&ast::FindSetStatement::object_name>(ref<OBJECT_ID>)) |
              (option(kw<"find">) +
               assign<&ast::FindSetStatement::source>(ref<SetVariable>))) +
                 kw<"where"> +
                 assign<&ast::FindSetStatement::where>(
                     ref<ObjectCondition>)) |
             (flag<&ast::FindSetStatement::find_all>(kw<"find-all">) +
              assign<&ast::FindSetStatement::object_name>(ref<OBJECT_ID>))) +
            option(
                (flag<&ast::FindSetStatement::reverse_ordered>(
                     kw<"reverse ordered by">) |
                 flag<&ast::FindSetStatement::ordered>(kw<"ordered by">)) +
                assign<&ast::FindSetStatement::order_by>(
                    ref<ATTR_ID> | ref<INSTANCE_ID_KW>)))> {};

  struct RelationshipNavigation
      : rule<create<ast::RelationshipNavigation>(
            assign<&ast::RelationshipNavigation::target>(ref<SetVariable> |
                                                         ref<PlainVariable>) +
            lit<"="> +
            assign<&ast::RelationshipNavigation::source>(ref<SetVariable> |
                                                         ref<VariableOrThis>) +
            some(lit<"->"> + append<&ast::RelationshipNavigation::path>(
                                 ref<RelationshipSpec>)) +
            option(kw<"where"> + assign<&ast::RelationshipNavigation::where>(
                                     ref<ObjectCondition>)) +
            option((flag<&ast::RelationshipNavigation::reverse_ordered>(
                        kw<"reverse ordered by">) |
                    flag<&ast::RelationshipNavigation::ordered>(
                        kw<"ordered by">)) +
                   assign<&ast::RelationshipNavigation::order_by>(
                       ref<ATTR_ID> | ref<INSTANCE_ID_KW>)))> {};

  struct AssociativeRelationshipNavigation
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

  struct RelationshipLink
      : rule<create<ast::RelationshipLink>(
            kw<"link"> +
            assign<&ast::RelationshipLink::from>(ref<VariableOrThis>) +
            assign<&ast::RelationshipLink::spec>(ref<RelationshipSpec>) +
            assign<&ast::RelationshipLink::to>(ref<VariableOrThis>) +
            option(
                flag<&ast::RelationshipLink::has_using>(kw<"using">) +
                assign<&ast::RelationshipLink::using_handle>(ref<VAR_ID>)))> {};

  struct RelationshipUnlink
      : rule<create<ast::RelationshipUnlink>(
            kw<"unlink"> +
            assign<&ast::RelationshipUnlink::from>(ref<VariableOrThis>) +
            assign<&ast::RelationshipUnlink::spec>(ref<RelationshipSpec>) +
            assign<&ast::RelationshipUnlink::to>(ref<VariableOrThis>))> {};

  static constexpr auto EventParam =
      ref<StructureField> | ref<ObjectAttribute> | ref<VariableOrThis> |
      ref<Constant> | ref<UndefinedValue>;

  struct EventGeneration
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
      ref<ObjectAttribute> | ref<SetVariable> | ref<VariableOrThis>;
  static constexpr auto InputParam = ref<ObjectAttribute> |
                                     ref<SetVariable> | ref<Constant> |
                                     ref<VariableOrThis>;

  struct OperationInvocation
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
      ref<ObjectAttribute> | ref<PlainVariable>;
  static constexpr auto TimeFieldOperand =
      ref<ObjectAttribute> | ref<PlainVariable> | ref<Constant>;

  struct TimerCreation
      : rule<create<ast::TimerCreation>(
            lit<"["> + assign<&ast::TimerCreation::timer_id>(TimerIdOperand) +
            lit<"]"> + lit<"="> + kw<"Create_Timer"> + lit<"["> + lit<"]">)> {};

  struct TimerDeletion
      : rule<create<ast::TimerDeletion>(
            lit<"["> + lit<"]"> + lit<"="> + kw<"Delete_Timer"> + lit<"["> +
            assign<&ast::TimerDeletion::timer_id>(TimerIdOperand) + lit<"]">)> {
  };

  struct SetRelativeTimer
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

  struct SetAbsoluteTimer
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

  struct SetRecurringTimer
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

  struct TimerReset
      : rule<create<ast::TimerReset>(
            kw<"%generate"> + kw<"TIM2:Reset_Timer"> + lit<"("> + lit<")"> +
            lit<";"> + kw<"to"> +
            assign<&ast::TimerReset::timer_id>(TimerIdOperand))> {};

  struct UniqueSet
      : rule<create<ast::UniqueSet>(
            assign<&ast::UniqueSet::target>(ref<SetVariable>) + lit<"="> +
            kw<"unique"> +
            assign<&ast::UniqueSet::source>(ref<SetVariable>))> {};

  struct SetCombination
      : rule<create<ast::SetCombination>(
            assign<&ast::SetCombination::target>(ref<SetVariable>) +
            lit<"="> +
            assign<&ast::SetCombination::op>(as<ast::SetCombineOp::union_of>(kw<"union-of">) |
                                             as<ast::SetCombineOp::disunion_of>(kw<"disunion-of">) |
                                             as<ast::SetCombineOp::intersection_of>(kw<"intersection-of">)) +
            assign<&ast::SetCombination::a>(ref<SetVariable>) + kw<"and"> +
            assign<&ast::SetCombination::b>(ref<SetVariable>))> {};

  struct SetDifference
      : rule<create<ast::SetDifference>(
            assign<&ast::SetDifference::target>(ref<SetVariable>) +
            lit<"="> + assign<&ast::SetDifference::a>(ref<SetVariable>) +
            kw<"not-in"> +
            assign<&ast::SetDifference::b>(ref<SetVariable>))> {};

  struct StructMember
      : rule<create<ast::StructMember>(
            assign<&ast::StructMember::name>(ref<VAR_ID>) +
            assign<&ast::StructMember::type_text>(ref<MemberType>))> {};

  struct StructType
      : rule<(ref<ATTR_ID> + lit<"."> + ref<VAR_ID>) | ref<VAR_ID>> {};
  struct UserDefinedType : rule<ref<OBJECT_ID> + lit<"."> + ref<ATTR_ID>> {
  };
  struct MemberType : rule<kw<"Integer"> | kw<"Real"> | kw<"Boolean"> |
                               kw<"Text"> | kw<"Date"> | kw<"Time_of_Day"> |
                               ref<StructType> | ref<UserDefinedType>> {
  };

  struct StructDefinition
      : rule<create<ast::StructDefinition>(
            kw<"define"> + kw<"structure"> +
            assign<&ast::StructDefinition::name>(ref<StructType>) +
            ref<NEWLINE> +
            some(
                append<&ast::StructDefinition::members>(ref<StructMember>) +
                ref<NEWLINE>) +
            kw<"enddefine">)> {};

  struct StructInstantiation
      : rule<create<ast::StructInstantiation>(
            assign<&ast::StructInstantiation::target>(ref<SetVariable>) +
            kw<"is"> +
            assign<&ast::StructInstantiation::type_name>(
                ref<StructType>))> {};

  struct StructAssembly
      : rule<create<ast::StructAssembly>(
            kw<"append"> + lit<"["> +
            some(append<&ast::StructAssembly::values>(ref<Operand>), lit<",">) +
            lit<"]"> + kw<"to"> +
            assign<&ast::StructAssembly::target>(ref<SetVariable>) +
            option(
                (flag<&ast::StructAssembly::reverse_ordered>(
                     kw<"reverse ordered by">) |
                 flag<&ast::StructAssembly::ordered>(kw<"ordered by">)) +
                assign<&ast::StructAssembly::order_by>(ref<VAR_ID>)))> {};

  struct ForLoopVar
      : rule<(flag<&ast::ForLoopVar::is_set>(lit<"{">) +
              assign<&ast::ForLoopVar::name>(ref<VAR_ID>) + lit<"}">) |
             assign<&ast::ForLoopVar::name>(ref<VAR_ID>)> {};

  struct BreakStatement
      : rule<create<ast::BreakStatement>(
            (kw<"break"> |
             (kw<"breakif"> + assign<&ast::BreakStatement::condition>(
                                  ref<LogicalCondition>))) +
            ref<NEWLINE>)> {};

  struct StructForLoop
      : rule<create<ast::StructForLoop>(
            kw<"for"> + lit<"["> +
            some(append<&ast::StructForLoop::vars>(ref<ForLoopVar>),
                 lit<",">) +
            lit<"]"> + kw<"in"> +
            assign<&ast::StructForLoop::source>(ref<SetVariable>) +
            kw<"do"> + ref<NEWLINE> +
            many(append<&ast::StructForLoop::body>(ref<Statement>)) +
            option(
                append<&ast::StructForLoop::body>(ref<BreakStatement>) +
                many(append<&ast::StructForLoop::body>(ref<Statement>))) +
            kw<"endfor">)> {};

  struct OrderStruct
      : rule<create<ast::OrderStruct>(
            assign<&ast::OrderStruct::target>(ref<SetVariable>) + lit<"="> +
            assign<&ast::OrderStruct::source>(ref<SetVariable>) +
            (flag<&ast::OrderStruct::reverse_ordered>(
                 kw<"reverse ordered by">) |
             kw<"ordered by">)+assign<&ast::OrderStruct::
                                          member>(ref<VAR_ID>))> {};

  struct StructSubset
      : rule<create<ast::StructSubset>(
            assign<&ast::StructSubset::target>(ref<SetVariable>) +
            lit<"="> +
            assign<&ast::StructSubset::source>(ref<SetVariable>) +
            kw<"where"> +
            assign<&ast::StructSubset::where>(ref<StructCondition>))> {};

  struct CaseClause
      : rule<create<ast::CaseClause>(
            kw<"case"> + assign<&ast::CaseClause::value>(ref<Constant>) +
            ref<NEWLINE> +
            many(append<&ast::CaseClause::body>(ref<Statement>)))> {};

  struct SwitchStatement
      : rule<create<ast::SwitchStatement>(
            kw<"switch"> +
            assign<&ast::SwitchStatement::target>(ref<ObjectAttribute> |
                                                  ref<PlainVariable>) +
            ref<NEWLINE> +
            some(append<&ast::SwitchStatement::cases>(ref<CaseClause>)) +
            option(
                flag<&ast::SwitchStatement::has_default>(kw<"default">) +
                ref<NEWLINE> +
                some(append<&ast::SwitchStatement::default_body>(
                    ref<Statement>))) +
            kw<"endswitch">)> {};

  struct IfStatement
      : rule<create<ast::IfStatement>(
            kw<"if"> +
            assign<&ast::IfStatement::condition>(
                (lit<"("> + ref<LogicalCondition> + lit<")">) |
                ref<LogicalCondition>) +
            kw<"then"> + ref<NEWLINE> +
            some(append<&ast::IfStatement::then_body>(ref<Statement>)) +
            option(flag<&ast::IfStatement::has_else>(kw<"else">) +
                   ref<NEWLINE> +
                   some(append<&ast::IfStatement::else_body>(
                       ref<Statement>))) +
            kw<"endif">)> {};

  struct ForLoop
      : rule<create<ast::ForLoop>(
            kw<"for"> + assign<&ast::ForLoop::variable>(ref<VAR_ID>) +
            kw<"in"> + assign<&ast::ForLoop::source>(ref<SetVariable>) +
            kw<"do"> + ref<NEWLINE> +
            many(append<&ast::ForLoop::body>(ref<Statement>)) +
            option(append<&ast::ForLoop::body>(ref<BreakStatement>) +
                   many(append<&ast::ForLoop::body>(ref<Statement>))) +
            kw<"endfor">)> {};

  struct LoopStatement
      : rule<create<ast::LoopStatement>(
            kw<"loop"> + ref<NEWLINE> +
            many(append<&ast::LoopStatement::body>(ref<Statement>)) +
            append<&ast::LoopStatement::body>(ref<BreakStatement>) +
            many(append<&ast::LoopStatement::body>(ref<BreakStatement> |
                                                   ref<Statement>)) +
            kw<"endloop">)> {};

  struct NativeStatement
      : rule<assign<&ast::NativeStatement::text>(ref<NATIVE_LANGUAGE>)> {};

  struct StatementDispatch
      : rule<ref<NativeStatement> | ref<Assignment> |
             ref<CreateStatement> | ref<DeleteStatement> |
             ref<FindInstanceStatement> | ref<FindSetStatement> |
             ref<RelationshipNavigation> |
             ref<AssociativeRelationshipNavigation> |
             ref<RelationshipLink> | ref<RelationshipUnlink> |
             ref<EventGeneration> | ref<OperationInvocation> |
             ref<TimerCreation> | ref<TimerDeletion> |
             ref<SetRelativeTimer> | ref<SetAbsoluteTimer> |
             ref<SetRecurringTimer> | ref<TimerReset> |
             ref<UniqueSet> | ref<SetCombination> |
             ref<SetDifference> | ref<StructDefinition> |
             ref<StructInstantiation> | ref<StructAssembly> |
             ref<StructForLoop> | ref<OrderStruct> |
             ref<StructSubset> | ref<SwitchStatement> |
             ref<IfStatement> | ref<ForLoop> | ref<LoopStatement>> {
  };

  struct Statement : rule<ref<StatementDispatch> + ref<NEWLINE>> {};

  struct Model
      : rule<create<ast::Model> +
             some(append<&ast::Model::statements>(ref<Statement>))> {};

  using entry = Model;
  using ignored = list<WHITESPACE, COMMENT, CONTINUATION>;
};

} // namespace asl
