// The joss language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "joss/core/ast.hpp"

namespace joss {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
inline constexpr auto form_layout = [](std::string_view t) {
  const auto line = t.find_first_of("\r\n");
  const auto width = t.substr(line).starts_with("\r\n") ? 2u : 1u;
  return t.substr(line + width);
};

struct Joss : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto NzDigit = cr<"1-9">;
  static constexpr auto DecimalPart = some(cr<"0-9">);
  static constexpr auto IntPart = lit<"0"> | (NzDigit + option(DecimalPart));
  static constexpr auto PartNumberBody = NzDigit + option(IntPart);

  struct NUMBER : terminal<as<double>(option(cr<"+-">) +
                                      ((IntPart + option(lit<"."> + DecimalPart)) |
                                       (lit<"."> + DecimalPart)))> {};
  struct PARTNUMBER : terminal<PartNumberBody> {};
  struct STEPNUMBER : terminal<PartNumberBody + lit<"."> + some(d)> {};

  struct ALPHA : terminal<cr<"A-Za-z">> {};

  // Native double quotes, plus the port's legacy single-quoted spelling.
  struct QSTRING : terminal<as<unquote>(through(lit<"\"">, lit<"\"">) |
                                        through(lit<"'">, lit<"'">))> {};
  struct FORM_LAYOUT : terminal<as<form_layout>(many(cr<R"( \t)">) + eol +
                                                some(cr<R"(^\r\n)">))> {};

  static constexpr auto BoolCompTok =
      as<ast::BoolCompOp::ne>(lit<"≠"> | lit<"!=">) | as<ast::BoolCompOp::le>(lit<"≤"> | lit<"<=">) | as<ast::BoolCompOp::ge>(lit<"≥"> | lit<"=>"> | lit<">=">) | as<ast::BoolCompOp::eq>(lit<"=">) | as<ast::BoolCompOp::lt>(lit<"<">) | as<ast::BoolCompOp::gt>(lit<">">);

  static constexpr auto BoolOpTok = as<ast::BoolOp::and_>(kw<"and">) | as<ast::BoolOp::or_>(kw<"or">);


  struct MathExpr;
  struct Variable;
  struct RangeCall;
  static constexpr auto UnaryFunction =
      as<ast::UnaryFn::sqrt_>(kw<"sqrt">) | as<ast::UnaryFn::log_>(kw<"log">) |
      as<ast::UnaryFn::exp_>(kw<"exp">) | as<ast::UnaryFn::sin_>(kw<"sin">) |
      as<ast::UnaryFn::cos_>(kw<"cos">) | as<ast::UnaryFn::ip_>(kw<"ip">) |
      as<ast::UnaryFn::fp_>(kw<"fp">) | as<ast::UnaryFn::dp_>(kw<"dp">) |
      as<ast::UnaryFn::xp_>(kw<"xp">) | as<ast::UnaryFn::sgn_>(kw<"sgn">);
  static constexpr auto UnaryArgument = assign<&ast::UnaryCall::arg>(ref<MathExpr>);
  static constexpr auto NaryFunction = as<ast::NaryFn::max_>(kw<"max">) |
      as<ast::NaryFn::min_>(kw<"min">) | as<ast::NaryFn::sum_>(kw<"sum">) |
      as<ast::NaryFn::prod_>(kw<"prod">);
  static constexpr auto NaryArguments = assign<&ast::NaryCall::first>(ref<MathExpr>) +
      lit<","> + some(append<&ast::NaryCall::rest>(ref<MathExpr>), lit<",">);

  struct Variable
      : rule<create<ast::Variable>(
            assign<&ast::Variable::name>(ref<ALPHA>) +
            option(lit<"("> + assign<&ast::Variable::index>(ref<MathExpr>) +
                   option(lit<","> + assign<&ast::Variable::secondIndex>(ref<MathExpr>)) +
                   lit<")">))> {};

  struct PrimaryMath
      : rule<create<ast::GroupedMath>(
                 lit<"("> + assign<&ast::GroupedMath::inner>(ref<MathExpr>) +
                 lit<")">)  //
             | create<ast::GroupedMath>(
                   lit<"["> + assign<&ast::GroupedMath::inner>(ref<MathExpr>) +
                   lit<"]">)  //
             | create<ast::NumberLit>(
                   assign<&ast::NumberLit::value>(ref<NUMBER>))  //
             | create<ast::AbsoluteMath>(lit<"|"> +
                   assign<&ast::AbsoluteMath::inner>(ref<MathExpr>) + lit<"|">)
             | ref<Variable>                                 //
             | ref<RangeCall>
             | create<ast::UnaryCall>(
                   assign<&ast::UnaryCall::fn>(UnaryFunction) +
                   ((lit<"("> + UnaryArgument + lit<")">) |
                    (lit<"["> + UnaryArgument + lit<"]">)))  //
             | create<ast::NaryCall>(
                   assign<&ast::NaryCall::fn>(NaryFunction) +
                   ((lit<"("> + NaryArguments + lit<")">) |
                    (lit<"["> + NaryArguments + lit<"]">)))> {};

  struct MathExpr
      : infix<PrimaryMath, ast::BinMath,
              levels(left(as<ast::MathOp::power>(lit<"*">)),
                     left(as<ast::MathOp::mul>(lit<"·"> | lit<"•">) | as<ast::MathOp::div>(lit<"/">)),
                     left(as<ast::MathOp::add>(lit<"+">) | as<ast::MathOp::sub>(lit<"-">)))> {};

  struct Assignment
      : rule<assign<&ast::Assignment::target>(ref<Variable>) + lit<"="> +
             assign<&ast::Assignment::value>(ref<MathExpr>)> {};

  struct Comparison
      : rule<assign<&ast::Comparison::left>(ref<MathExpr>) +
             assign<&ast::Comparison::op>(BoolCompTok) +
             assign<&ast::Comparison::right>(ref<MathExpr>)> {};

  struct BoolExpr;
  struct BoolExpr
      : rule<create<ast::GroupedBool>(
                 lit<"("> +
                 assign<&ast::GroupedBool::inner>(ref<BoolExpr>) +
                 lit<")">)  //
             | create<ast::GroupedBool>(
                   lit<"["> +
                   assign<&ast::GroupedBool::inner>(ref<BoolExpr>) +
                   lit<"]">)  //
             | (ref<Comparison> +
                option(nest<ast::BoolChain, &ast::BoolChain::left>(
                    assign<&ast::BoolChain::op>(BoolOpTok) +
                    assign<&ast::BoolChain::right>(ref<BoolExpr>))))> {};

  struct RangeVal;
  struct RangeVal
      : rule<create<ast::RangeVal>(
            assign<&ast::RangeVal::start>(ref<MathExpr>) +
            option(lit<"("> + assign<&ast::RangeVal::step>(ref<MathExpr>) +
                   lit<")"> +
                   assign<&ast::RangeVal::rest>(ref<RangeVal>)))> {};

  struct IfCond
      : rule<create<ast::IfCond>(kw<"if"> +
                                 assign<&ast::IfCond::cond>(ref<BoolExpr>))> {
  };
  static constexpr auto RangeBindingBody = assign<&ast::ForCond::var>(ref<Variable>) +
      lit<"="> + some(append<&ast::ForCond::values>(ref<RangeVal>), lit<",">);
  struct RangeBinding : rule<create<ast::ForCond>(RangeBindingBody)> {};
  struct RangeCall : rule<create<ast::RangeCall>(
      assign<&ast::RangeCall::fn>(NaryFunction) + lit<"["> +
      assign<&ast::RangeCall::range>(ref<RangeBinding>) + lit<":"> +
      assign<&ast::RangeCall::value>(ref<MathExpr>) + lit<"]">)> {};
  struct ForCond
      : rule<create<ast::ForCond>(
            kw<"for"> + RangeBindingBody)> {
  };
  static constexpr auto ConditionalExpr = ref<IfCond> | ref<ForCond>;

  static constexpr auto StepSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(as<ast::SelKind::step_>(kw<"step">)) +
      assign<&ast::RefSelector::number>(ref<STEPNUMBER>));
  static constexpr auto PartSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(as<ast::SelKind::part_>(kw<"part">)) +
      assign<&ast::RefSelector::number>(ref<PARTNUMBER>));
  static constexpr auto FormSelExpr = create<ast::RefSelector>(
      assign<&ast::RefSelector::kind>(as<ast::SelKind::form_>(kw<"form">)) +
      assign<&ast::RefSelector::number>(ref<PARTNUMBER>));

  // toSel/doSel: no 'form', no 'all'.
  static constexpr auto RefSel2 = StepSelExpr | PartSelExpr;
  // delCmdSel/typeSel's step/part/form part.
  static constexpr auto RefSel3 = StepSelExpr | PartSelExpr | FormSelExpr;

  static constexpr auto AllSel3 = create<ast::AllSelector>(
      kw<"all"> + option(assign<&ast::AllSelector::restriction>(
                      as<ast::AllKind::steps_>(kw<"steps">) | as<ast::AllKind::parts_>(kw<"parts">) | as<ast::AllKind::forms_>(kw<"forms">))));
  static constexpr auto AllSel4 = create<ast::AllSelector>(
      kw<"all"> +
      option(assign<&ast::AllSelector::restriction>(
          as<ast::AllKind::steps_>(kw<"steps">) | as<ast::AllKind::parts_>(kw<"parts">) | as<ast::AllKind::forms_>(kw<"forms">) | as<ast::AllKind::values_>(kw<"values">))));

  struct GoCmd : rule<create<ast::GoCmd>(kw<"Go">)> {};
  struct CancelCmd : rule<create<ast::CancelCmd>(kw<"Cancel">)> {};
  struct DeleteCmdCmd
      : rule<create<ast::DeleteCmdCmd>(
            kw<"Delete"> +
            assign<&ast::DeleteCmdCmd::target>(RefSel3 | AllSel3))> {};

  struct DoneCmd : rule<create<ast::DoneCmd>(kw<"Done">)> {};
  struct StopCmd : rule<create<ast::StopCmd>(kw<"Stop">)> {};
  struct DemandCmd
      : rule<create<ast::DemandCmd>(
            kw<"Demand"> + assign<&ast::DemandCmd::var>(ref<Variable>) +
            option(kw<"as"> + assign<&ast::DemandCmd::prompt>(ref<QSTRING>)))> {};
  struct ToCmd
      : rule<create<ast::ToCmd>(kw<"To"> +
                                assign<&ast::ToCmd::target>(RefSel2))> {};

  struct SetCmd
      : rule<create<ast::SetCmd>(
            kw<"Set"> +
            assign<&ast::SetCmd::assignment>(ref<Assignment>))> {};
  struct LetCmd : rule<create<ast::LetCmd>(kw<"Let"> +
      assign<&ast::LetCmd::assignment>(ref<Assignment>))> {};
  struct DoCmd
      : rule<create<ast::DoCmd>(
            kw<"Do"> + assign<&ast::DoCmd::target>(RefSel2))> {};

  struct TypeContent
      : rule<create<ast::TypeFormContent>(
                 some(append<&ast::TypeFormContent::vars>(ref<MathExpr>),
                      lit<",">) +
                 kw<"in"> +
                 assign<&ast::TypeFormContent::form>(FormSelExpr))  //
             | create<ast::TypeDataContent>(
                   some(append<&ast::TypeDataContent::items>(
                            ref<QSTRING> | ref<MathExpr> | create<ast::TypeLineBreak>(lit<"_">)),
                        lit<",">))  //
             | create<ast::TypeSelContent>(assign<&ast::TypeSelContent::sel>(
                   RefSel3 | AllSel4))  //
             | create<ast::TypeSysContent>(assign<&ast::TypeSysContent::kind>(
                   as<ast::TypeSysKind::size_>(kw<"size">) | as<ast::TypeSysKind::time_>(kw<"time">) | as<ast::TypeSysKind::users_>(kw<"users">)))> {};
  struct TypeCmd
      : rule<create<ast::TypeCmd>(
            kw<"Type"> +
            assign<&ast::TypeCmd::content>(ref<TypeContent>))> {};

  struct DeleteValCmd
      : rule<create<ast::DeleteValCmd>(
            option(kw<"Delete">) + (
            some(append<&ast::DeleteValCmd::vars>(ref<Variable>),
                 lit<",">)  //
            | (kw<"all"> +
               flag<&ast::DeleteValCmd::allValues>(kw<"values">))))> {};

  struct LineCmd : rule<create<ast::LineCmd>(kw<"Line">)> {};
  struct PageCmd : rule<create<ast::PageCmd>(kw<"Page">)> {};

  static constexpr auto ArbitraryCmd =
      ref<SetCmd> | ref<LetCmd> | ref<DoCmd> | ref<TypeCmd> |
      ref<DeleteValCmd> | ref<LineCmd> | ref<PageCmd>;
  static constexpr auto DirectCmd =
      ref<CancelCmd> | ref<DeleteCmdCmd> | ref<GoCmd> | ArbitraryCmd;
  static constexpr auto IndirectCmd = ref<ToCmd> | ref<DoneCmd> |
                                      ref<StopCmd> | ref<DemandCmd> |
                                      ArbitraryCmd;
  struct DirectCommand : rule<DirectCmd + option(assign<&ast::Command::cond>(ConditionalExpr))> {};
  struct IndirectCommand : rule<IndirectCmd + option(assign<&ast::Command::cond>(ConditionalExpr))> {};

  struct DirectStmt
      : rule<create<ast::DirectStmt>(
            assign<&ast::DirectStmt::command>(ref<DirectCommand>) + lit<".">)> {};
  struct IndirectStmt
      : rule<create<ast::IndirectStmt>(
            assign<&ast::IndirectStmt::stepNumber>(ref<STEPNUMBER>) +
            assign<&ast::IndirectStmt::command>(ref<IndirectCommand>) + lit<".">)> {};
  struct FormStmt
      : rule<create<ast::FormStmt>(
            kw<"Form"> + assign<&ast::FormStmt::partNumber>(ref<PARTNUMBER>) +
            lit<":"> + option(gap::none + assign<&ast::FormStmt::layout>(ref<FORM_LAYOUT>)))> {};
  struct AssignStmt
      : rule<create<ast::AssignStmt>(
            assign<&ast::AssignStmt::assignment>(ref<Assignment>))> {};

  struct Statement
      : rule<ref<DirectStmt> | ref<IndirectStmt> | ref<FormStmt> |
             ref<AssignStmt>> {};

  struct Program
      : rule<create<ast::Program> +
             some(append<&ast::Program::statements>(ref<Statement>))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace joss
