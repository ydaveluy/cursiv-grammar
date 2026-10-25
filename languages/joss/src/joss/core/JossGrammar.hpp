// The joss language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "joss/core/ast.hpp"

namespace joss {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Joss : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto NzDigit = cr<"1-9">;
  static constexpr auto DecimalPart = some(cr<"0-9">);
  static constexpr auto IntPart = lit<"0"> | (NzDigit + option(DecimalPart));
  static constexpr auto PartNumberBody = NzDigit + option(IntPart);

  struct NUMBER
      : terminal<option(cr<"+-">) + IntPart + option(lit<"."> + DecimalPart),
                 double> {};
  struct PARTNUMBER : terminal<PartNumberBody> {};
  struct STEPNUMBER : terminal<PartNumberBody + lit<"."> + PartNumberBody> {};

  struct ALPHA : terminal<cr<"A-Za-z">> {};

  // JOSS has no escape for an embedded apostrophe in a quoted string, so
  // `through` closes on the first one.
  struct QSTRING : terminal<through(lit<"'">, lit<"'">), Unquoted> {};

  struct STEP_KW : terminal<kw<"step">, value<ast::SelKind::step_>> {};
  struct PART_KW : terminal<kw<"part">, value<ast::SelKind::part_>> {};
  struct FORM_KW : terminal<kw<"form">, value<ast::SelKind::form_>> {};

  struct STEPS_KW : terminal<kw<"steps">, value<ast::AllKind::steps_>> {};
  struct PARTS_KW : terminal<kw<"parts">, value<ast::AllKind::parts_>> {};
  struct FORMS_KW : terminal<kw<"forms">, value<ast::AllKind::forms_>> {};
  struct VALUES_KW : terminal<kw<"values">, value<ast::AllKind::values_>> {};

  struct SIZE_KW : terminal<kw<"size">, value<ast::TypeSysKind::size_>> {};
  struct TIME_KW : terminal<kw<"time">, value<ast::TypeSysKind::time_>> {};
  struct USERS_KW : terminal<kw<"users">, value<ast::TypeSysKind::users_>> {};

  struct ADD : terminal<lit<"+">, value<ast::MathOp::add>> {};
  struct SUB : terminal<lit<"-">, value<ast::MathOp::sub>> {};
  struct MUL : terminal<lit<"*">, value<ast::MathOp::mul>> {};
  struct DIV : terminal<lit<"/">, value<ast::MathOp::div>> {};

  struct NE : terminal<lit<"!=">, value<ast::BoolCompOp::ne>> {};
  struct LE : terminal<lit<"<=">, value<ast::BoolCompOp::le>> {};
  struct GE : terminal<lit<"=>">, value<ast::BoolCompOp::ge>> {};
  struct EQ : terminal<lit<"=">, value<ast::BoolCompOp::eq>> {};
  struct LT : terminal<lit<"<">, value<ast::BoolCompOp::lt>> {};
  struct GT : terminal<lit<">">, value<ast::BoolCompOp::gt>> {};
  static constexpr auto BoolCompTok =
      ref<NE> | ref<LE> | ref<GE> | ref<EQ> | ref<LT> | ref<GT>;

  struct AND_KW : terminal<kw<"and">, value<ast::BoolOp::and_>> {};
  struct OR_KW : terminal<kw<"or">, value<ast::BoolOp::or_>> {};
  static constexpr auto BoolOpTok = ref<AND_KW> | ref<OR_KW>;

  struct SQRT_KW : terminal<lit<"sqrt(">, value<ast::UnaryFn::sqrt_>> {};
  struct LOG_KW : terminal<lit<"log(">, value<ast::UnaryFn::log_>> {};
  struct EXP_KW : terminal<lit<"exp(">, value<ast::UnaryFn::exp_>> {};
  struct SIN_KW : terminal<lit<"sin(">, value<ast::UnaryFn::sin_>> {};
  struct COS_KW : terminal<lit<"cos(">, value<ast::UnaryFn::cos_>> {};
  struct IP_KW : terminal<lit<"ip(">, value<ast::UnaryFn::ip_>> {};
  struct FP_KW : terminal<lit<"fp(">, value<ast::UnaryFn::fp_>> {};
  struct DP_KW : terminal<lit<"dp(">, value<ast::UnaryFn::dp_>> {};
  struct XP_KW : terminal<lit<"xp(">, value<ast::UnaryFn::xp_>> {};
  struct SGN_KW : terminal<lit<"sgn(">, value<ast::UnaryFn::sgn_>> {};
  struct MAX_KW : terminal<lit<"max(">, value<ast::NaryFn::max_>> {};
  struct MIN_KW : terminal<lit<"min(">, value<ast::NaryFn::min_>> {};

  struct MathExprRule;
  struct VariableRule;

  struct VariableRule
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<ALPHA>) +
            option(lit<"("> + assign<&ast::Variable::index>(ref<MathExprRule>) +
                   lit<")">))> {};

  struct PrimaryMath
      : rule<create<ast::GroupedMath>(
                 lit<"("> + assign<&ast::GroupedMath::inner>(ref<MathExprRule>) +
                 lit<")">)  //
             | create<ast::GroupedMath>(
                   lit<"["> + assign<&ast::GroupedMath::inner>(ref<MathExprRule>) +
                   lit<"]">)  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::value>(ref<NUMBER>))  //
             | ref<VariableRule>                                 //
             | create<ast::UnaryCall>(
                   assign<&ast::UnaryCall::fn>(
                       ref<SQRT_KW> | ref<LOG_KW> | ref<EXP_KW> | ref<SIN_KW> |
                       ref<COS_KW> | ref<IP_KW> | ref<FP_KW> | ref<DP_KW> |
                       ref<XP_KW> | ref<SGN_KW>) +
                   assign<&ast::UnaryCall::arg>(ref<MathExprRule>) + lit<")">)  //
             | create<ast::NaryCall>(
                   assign<&ast::NaryCall::fn>(ref<MAX_KW> | ref<MIN_KW>) +
                   assign<&ast::NaryCall::first>(ref<MathExprRule>) + lit<","> +
                   some(append<&ast::NaryCall::rest>(ref<MathExprRule>),
                        lit<",">) +
                   lit<")">)> {};

  struct MathExprRule
      : infix<PrimaryMath, &ast::BinMath::left, &ast::BinMath::op,
              &ast::BinMath::right,
              (left(ref<MUL> | ref<DIV>) >> left(ref<ADD> | ref<SUB>))> {};

  struct AssignmentRule
      : rule<assign<&ast::Assignment::target>(ref<VariableRule>) + lit<"="> +
             assign<&ast::Assignment::value>(ref<MathExprRule>)> {};

  struct ComparisonRule
      : rule<assign<&ast::Comparison::left>(ref<MathExprRule>) +
             assign<&ast::Comparison::op>(BoolCompTok) +
             assign<&ast::Comparison::right>(ref<MathExprRule>)> {};

  struct BoolExprRule;
  struct BoolExprRule
      : rule<create<ast::GroupedBool>(
                 lit<"("> +
                 assign<&ast::GroupedBool::inner>(ref<BoolExprRule>) +
                 lit<")">)  //
             | create<ast::GroupedBool>(
                   lit<"["> +
                   assign<&ast::GroupedBool::inner>(ref<BoolExprRule>) +
                   lit<"]">)  //
             | (ref<ComparisonRule> +
                option(nest<ast::BoolChain, &ast::BoolChain::left>(
                    assign<&ast::BoolChain::op>(BoolOpTok) +
                    assign<&ast::BoolChain::right>(ref<BoolExprRule>))))> {};

  struct RangeValRule;
  struct RangeValRule
      : rule<create<ast::RangeVal>(
            assign<&ast::RangeVal::start>(ref<MathExprRule>) +
            option(lit<"("> + assign<&ast::RangeVal::step>(ref<MathExprRule>) +
                   lit<")"> +
                   assign<&ast::RangeVal::rest>(ref<RangeValRule>)))> {};

  struct IfCondRule
      : rule<create<ast::IfCond>(kw<"if"> +
                                 assign<&ast::IfCond::cond>(ref<BoolExprRule>))> {
  };
  struct ForCondRule
      : rule<create<ast::ForCond>(
            kw<"for"> + assign<&ast::ForCond::var>(ref<VariableRule>) +
            lit<"="> +
            some(append<&ast::ForCond::values>(ref<RangeValRule>), lit<",">))> {
  };
  static constexpr auto ConditionalExpr = ref<IfCondRule> | ref<ForCondRule>;

  static constexpr auto StepSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(ref<STEP_KW>) +
      assign<&ast::RefSelector::number>(ref<STEPNUMBER>));
  static constexpr auto PartSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(ref<PART_KW>) +
      assign<&ast::RefSelector::number>(ref<PARTNUMBER>));
  static constexpr auto FormSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(ref<FORM_KW>) +
      assign<&ast::RefSelector::number>(ref<PARTNUMBER>));

  // toSel/doSel: no 'form', no 'all'.
  static constexpr auto RefSel2 = StepSelExpr | PartSelExpr;
  // delCmdSel/typeSel's step/part/form part.
  static constexpr auto RefSel3 = StepSelExpr | PartSelExpr | FormSelExpr;

  static constexpr auto AllSel3 = create<ast::AllSelector>(
      kw<"all"> + option(assign<&ast::AllSelector::restriction>(
                      ref<STEPS_KW> | ref<PARTS_KW> | ref<FORMS_KW>)));
  static constexpr auto AllSel4 = create<ast::AllSelector>(
      kw<"all"> +
      option(assign<&ast::AllSelector::restriction>(
          ref<STEPS_KW> | ref<PARTS_KW> | ref<FORMS_KW> | ref<VALUES_KW>)));

  struct GoCmdRule : rule<create<ast::GoCmd>(kw<"Go">)> {};
  struct CancelCmdRule : rule<create<ast::CancelCmd>(kw<"Cancel">)> {};
  struct DeleteCmdCmdRule
      : rule<create<ast::DeleteCmdCmd>(
            kw<"Delete"> +
            assign<&ast::DeleteCmdCmd::target>(RefSel3 | AllSel3))> {};

  struct DoneCmdRule : rule<create<ast::DoneCmd>(kw<"Done">)> {};
  struct StopCmdRule : rule<create<ast::StopCmd>(kw<"Stop">)> {};
  struct DemandCmdRule
      : rule<create<ast::DemandCmd>(
            kw<"Demand"> + assign<&ast::DemandCmd::var>(ref<VariableRule>))> {};
  struct ToCmdRule
      : rule<create<ast::ToCmd>(kw<"To"> +
                                assign<&ast::ToCmd::target>(RefSel2))> {};

  struct SetCmdRule
      : rule<create<ast::SetCmd>(
            kw<"Set"> +
            assign<&ast::SetCmd::assignment>(ref<AssignmentRule>))> {};
  struct DoCmdRule
      : rule<create<ast::DoCmd>(
            kw<"Do"> + assign<&ast::DoCmd::target>(RefSel2) +
            option(assign<&ast::DoCmd::cond>(ConditionalExpr)))> {};

  struct TypeContentRule
      : rule<create<ast::TypeFormContent>(
                 some(append<&ast::TypeFormContent::vars>(ref<VariableRule>),
                      lit<",">) +
                 kw<"in"> +
                 assign<&ast::TypeFormContent::form>(FormSelExpr))  //
             | create<ast::TypeDataContent>(
                   some(append<&ast::TypeDataContent::items>(
                            ref<QSTRING> | ref<VariableRule>),
                        lit<",">))  //
             | create<ast::TypeSelContent>(assign<&ast::TypeSelContent::sel>(
                   RefSel3 | AllSel4))  //
             | create<ast::TypeSysContent>(assign<&ast::TypeSysContent::kind>(
                   ref<SIZE_KW> | ref<TIME_KW> | ref<USERS_KW>))> {};
  struct TypeCmdRule
      : rule<create<ast::TypeCmd>(
            kw<"Type"> +
            assign<&ast::TypeCmd::content>(ref<TypeContentRule>))> {};

  struct DeleteValCmdRule
      : rule<create<ast::DeleteValCmd>(
            some(append<&ast::DeleteValCmd::vars>(ref<VariableRule>),
                 lit<",">)  //
            | (kw<"all"> +
               flag<&ast::DeleteValCmd::allValues>(kw<"values">)))> {};

  struct LineCmdRule : rule<create<ast::LineCmd>(kw<"Line">)> {};
  struct PageCmdRule : rule<create<ast::PageCmd>(kw<"Page">)> {};

  static constexpr auto ArbitraryCmd =
      ref<SetCmdRule> | ref<DoCmdRule> | ref<TypeCmdRule> |
      ref<DeleteValCmdRule> | ref<LineCmdRule> | ref<PageCmdRule>;
  static constexpr auto DirectCmd =
      ref<CancelCmdRule> | ref<DeleteCmdCmdRule> | ref<GoCmdRule> | ArbitraryCmd;
  static constexpr auto IndirectCmd = ref<ToCmdRule> | ref<DoneCmdRule> |
                                      ref<StopCmdRule> | ref<DemandCmdRule> |
                                      ArbitraryCmd;

  struct DirectStmtRule
      : rule<create<ast::DirectStmt>(
            assign<&ast::DirectStmt::command>(DirectCmd) + lit<".">)> {};
  struct IndirectStmtRule
      : rule<create<ast::IndirectStmt>(
            assign<&ast::IndirectStmt::stepNumber>(ref<STEPNUMBER>) +
            assign<&ast::IndirectStmt::command>(IndirectCmd) + lit<".">)> {};
  struct FormStmtRule
      : rule<create<ast::FormStmt>(
            kw<"Form"> + assign<&ast::FormStmt::partNumber>(ref<PARTNUMBER>) +
            lit<":">)> {};
  struct AssignStmtRule
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::assignment>(ref<AssignmentRule>))> {};

  struct StatementRule
      : rule<ref<DirectStmtRule> | ref<IndirectStmtRule> | ref<FormStmtRule> |
             ref<AssignStmtRule>> {};

  struct ProgramRule
      : rule<create<ast::Program> +
             some(append<&ast::Program::statements>(ref<StatementRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace joss
