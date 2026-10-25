// The flowmatic language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "flowmatic/core/ast.hpp"

namespace flowmatic {

struct Flowmatic : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct ID : terminal<cr<"a-zA-Z-"> + many(cr<"a-zA-Z0-9-">)> {};
  struct FILE_LETTER : terminal<cr<"a-zA-Z"> + !word_boundary> {};
  struct COMMENT : terminal<lit<"*"> + many(!lit<"\n"> + any)> {};
  struct NUM : terminal<some(d)> {};
  struct DECIMAL
      : terminal<option(lit<"-">) + some(d) + option(lit<"."> + some(d))> {};
  struct STRING : terminal<lit<"\""> + many(!lit<"\""> + any) + lit<"\"">,
                           cursiv::text_conv<[](std::string_view s) {
                             return s.substr(1, s.size() - 2);
                           }>> {};

  struct END : terminal<lit<"(END)">> {};
  static constexpr auto go_to = ikw<"GO"> + ikw<"TO">;

  struct EQUAL : terminal<ikw<"EQUAL">, value<ast::Cond::equal>> {};
  struct GREATER : terminal<ikw<"GREATER">, value<ast::Cond::greater>> {};
  struct LESS : terminal<ikw<"LESS">, value<ast::Cond::less>> {};
  struct EndOfData : rule<ikw<"END"> + ikw<"OF"> + ikw<"DATA">,
                          value<ast::Cond::end_of_data>> {};
  struct Add : terminal<ikw<"ADD">, value<ast::ArithmeticOp::add>> {};
  struct Subtract
      : terminal<ikw<"SUBTRACT">, value<ast::ArithmeticOp::subtract>> {};
  struct Multiply
      : terminal<ikw<"MULTIPLY">, value<ast::ArithmeticOp::multiply>> {};
  struct Divide : terminal<ikw<"DIVIDE">, value<ast::ArithmeticOp::divide>> {};

  struct FileBindingRule
      : rule<create<ast::FileBinding>(
            assign<&ast::FileBinding::filename>(ref<ID>) +
            assign<&ast::FileBinding::fileletter>(ref<ID>))> {};

  struct HspStmtRule : rule<create<ast::HspStmt>(
                           ikw<"HSP"> + assign<&ast::HspStmt::file>(ref<ID>))> {
  };

  struct OutputStmtRule
      : rule<create<ast::OutputStmt>(
            ikw<"OUTPUT"> +
            some(append<&ast::OutputStmt::files>(ref<FileBindingRule>)))> {};

  struct InputStmtRule
      : rule<create<ast::InputStmt>(
            ikw<"INPUT"> +
            some(append<&ast::InputStmt::files>(ref<FileBindingRule>)))> {};

  struct JumpToStmtRule
      : rule<create<ast::JumpToStmt>(
            (ikw<"JUMP"> + ikw<"TO"> | go_to) + ikw<"OPERATION"> +
            assign<&ast::JumpToStmt::target>(ref<NUM>))> {};

  struct ReadItemStmtRule
      : rule<create<ast::ReadItemStmt>(
            ikw<"READ-ITEM"> + assign<&ast::ReadItemStmt::file>(ref<ID>))> {};

  struct WriteItemStmtRule
      : rule<create<ast::WriteItemStmt>(
            (ikw<"WRITE-ITEM"> |
             flag<&ast::WriteItemStmt::punched>(ikw<"PUNCH-ITEM">)) +
            assign<&ast::WriteItemStmt::file>(ref<ID>))> {};

  struct TransferStmtRule
      : rule<create<ast::TransferStmt>(
            ikw<"TRANSFER"> + assign<&ast::TransferStmt::from>(ref<ID>) +
            ikw<"TO"> + assign<&ast::TransferStmt::to>(ref<ID>))> {};

  struct IfStmtRule
      : rule<create<ast::IfStmt>(
            ikw<"IF"> +
            assign<&ast::IfStmt::cond>(ref<EQUAL> | ref<GREATER> | ref<LESS> |
                                       ref<EndOfData>) +
            go_to + ikw<"OPERATION"> +
            assign<&ast::IfStmt::target>(ref<NUM>))> {};

  struct OtherwiseStmtRule
      : rule<create<ast::OtherwiseStmt>(
            ikw<"OTHERWISE"> + go_to + ikw<"OPERATION"> +
            assign<&ast::OtherwiseStmt::target>(ref<NUM>))> {};

  struct RewindStmtRule
      : rule<create<ast::RewindStmt>(
            ikw<"REWIND"> + assign<&ast::RewindStmt::file>(ref<ID>))> {};

  struct StopStmtRule : rule<create<ast::StopStmt> + ikw<"STOP">> {};

  struct CompareStmtRule
      : rule<create<ast::CompareStmt>(
            ikw<"COMPARE"> + assign<&ast::CompareStmt::leftField>(ref<ID>) +
            lit<"("> + assign<&ast::CompareStmt::leftFile>(ref<ID>) + lit<")"> +
            ikw<"WITH"> + assign<&ast::CompareStmt::rightField>(ref<ID>) +
            lit<"("> + assign<&ast::CompareStmt::rightFile>(ref<ID>) +
            lit<")">)> {};

  struct FieldRule : rule<create<ast::Value>(
                         assign<&ast::Value::text>(ref<ID>) + lit<"("> +
                         assign<&ast::Value::file>(ref<ID>) + lit<")">)> {};

  struct ValueRule
      : rule<create<ast::Value>(
            (flag<&ast::Value::quoted>(
                 assign<&ast::Value::text>(ref<STRING>)) |
             assign<&ast::Value::text>(ref<DECIMAL> | ref<ID>)) +
            option(lit<"("> + assign<&ast::Value::file>(ref<ID>) + lit<")">))> {
  };

  struct MoveStmtRule
      : rule<create<ast::MoveStmt>(
            ikw<"MOVE"> + assign<&ast::MoveStmt::source>(ref<ValueRule>) +
            ikw<"TO"> + assign<&ast::MoveStmt::destination>(ref<FieldRule>))> {
  };

  struct ArithmeticStmtRule
      : rule<create<ast::ArithmeticStmt>(
            (assign<&ast::ArithmeticStmt::op>(ref<Add>) +
                 assign<&ast::ArithmeticStmt::left>(ref<ValueRule>) +
                 ikw<"TO"> |
             assign<&ast::ArithmeticStmt::op>(ref<Subtract>) +
                 assign<&ast::ArithmeticStmt::left>(ref<ValueRule>) +
                 ikw<"FROM"> |
             assign<&ast::ArithmeticStmt::op>(ref<Multiply>) +
                 assign<&ast::ArithmeticStmt::left>(ref<ValueRule>) +
                 ikw<"BY"> |
             assign<&ast::ArithmeticStmt::op>(ref<Divide>) +
                 assign<&ast::ArithmeticStmt::left>(ref<ValueRule>) +
                 ikw<"BY">)+assign<&ast::ArithmeticStmt::
                                       right>(ref<ValueRule>) +
            option(ikw<"GIVING"> +
                   assign<&ast::ArithmeticStmt::result>(ref<FieldRule>)))> {};

  struct PrintItemStmtRule
      : rule<create<ast::PrintItemStmt>(
            ikw<"PRINT-ITEM"> + assign<&ast::PrintItemStmt::file>(ref<ID>))> {};

  struct ExecuteStmtRule
      : rule<create<ast::ExecuteStmt>(
            ikw<"EXECUTE"> + ikw<"OPERATION"> +
            assign<&ast::ExecuteStmt::first>(ref<NUM>) +
            option(ikw<"THROUGH"> + ikw<"OPERATION"> +
                   assign<&ast::ExecuteStmt::last>(ref<NUM>)))> {};

  struct SetStmtRule
      : rule<create<ast::SetStmt>(ikw<"SET"> + ikw<"OPERATION"> +
                                  assign<&ast::SetStmt::which>(ref<NUM>) +
                                  ikw<"TO"> + go_to + ikw<"OPERATION"> +
                                  assign<&ast::SetStmt::target>(ref<NUM>))> {};

  struct TestStmtRule
      : rule<create<ast::TestStmt>(
            ikw<"TEST"> + assign<&ast::TestStmt::field>(ref<ID>) + lit<"("> +
            assign<&ast::TestStmt::file>(ref<ID>) + lit<")"> + ikw<"AGAINST"> +
            assign<&ast::TestStmt::against>(ref<ValueRule>))> {};

  struct CloseOutStmtRule
      : rule<create<ast::CloseOutStmt>(
            ikw<"CLOSE-OUT"> +
            (ikw<"FILES"> |
             ikw<"FILE">)+append<&ast::CloseOutStmt::files>(ref<FILE_LETTER>) +
            many(option(lit<";"> | lit<",">) +
                 append<&ast::CloseOutStmt::files>(ref<FILE_LETTER>)))> {};

  struct StatementRule
      : rule<ref<HspStmtRule> | ref<OutputStmtRule> | ref<InputStmtRule> |
             ref<JumpToStmtRule> | ref<ReadItemStmtRule> |
             ref<WriteItemStmtRule> | ref<TransferStmtRule> | ref<IfStmtRule> |
             ref<OtherwiseStmtRule> | ref<RewindStmtRule> | ref<StopStmtRule> |
             ref<CompareStmtRule> | ref<MoveStmtRule> | ref<SetStmtRule> |
             ref<TestStmtRule> | ref<CloseOutStmtRule> |
             ref<ArithmeticStmtRule> | ref<PrintItemStmtRule> |
             ref<ExecuteStmtRule>> {};

  // `label statement (';' statement)* '.'`, where `statement` itself is zero
  // or more leaves run together with no separator: flattened here into one
  // list, since the ';' between groups carries no meaning of its own.
  static constexpr auto label =
      lit<"("> + assign<&ast::Line::label>(ref<NUM>) + lit<")">;

  struct LineRule
      : rule<label +
             many(append<&ast::Line::statements>(ref<StatementRule>) |
                  lit<";">) +
             option(lit<".">)> {};

  struct ProgramRule : rule<create<ast::Program> +
                            many(append<&ast::Program::lines>(ref<LineRule>)) +
                            option(ref<END>)> {};

  using entry = ProgramRule;
  using ignore = WS;
  using hidden = COMMENT;
};

} // namespace flowmatic
