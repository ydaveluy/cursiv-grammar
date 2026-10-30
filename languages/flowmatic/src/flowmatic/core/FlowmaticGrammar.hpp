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
  struct COMMENT : terminal<through(lit<"*">, &lit<"\n"> | eof)> {};
  struct NUM : terminal<some(d)> {};
  struct DECIMAL
      : terminal<option(lit<"-">) + some(d) + option(lit<"."> + some(d))> {};
  struct STRING : terminal<as<[](std::string_view s) {
    return s.substr(1, s.size() - 2);
  }>(through(lit<"\"">, lit<"\"">))> {};

  struct END : terminal<lit<"(END)">> {};
  static constexpr auto go_to = ikw<"GO"> + ikw<"TO">;

  struct EQUAL : terminal<as<ast::Cond::equal>(ikw<"EQUAL">)> {};
  struct GREATER : terminal<as<ast::Cond::greater>(ikw<"GREATER">)> {};
  struct LESS : terminal<as<ast::Cond::less>(ikw<"LESS">)> {};
  struct Add : terminal<as<ast::ArithmeticOp::add>(ikw<"ADD">)> {};

  struct FileBinding
      : rule<create<ast::FileBinding>(
            assign<&ast::FileBinding::filename>(ref<ID>) +
            assign<&ast::FileBinding::fileletter>(ref<ID>))> {};

  struct HspStmt : rule<create<ast::HspStmt>(
                           ikw<"HSP"> + assign<&ast::HspStmt::file>(ref<ID>))> {
  };

  struct OutputStmt
      : rule<create<ast::OutputStmt>(
            ikw<"OUTPUT"> +
            some(append<&ast::OutputStmt::files>(ref<FileBinding>)))> {};

  struct InputStmt
      : rule<create<ast::InputStmt>(
            ikw<"INPUT"> +
            some(append<&ast::InputStmt::files>(ref<FileBinding>)))> {};

  struct JumpToStmt
      : rule<create<ast::JumpToStmt>(
            ((ikw<"JUMP"> + ikw<"TO">) | go_to) + ikw<"OPERATION"> +
            assign<&ast::JumpToStmt::target>(ref<NUM>))> {};

  struct ReadItemStmt
      : rule<create<ast::ReadItemStmt>(
            ikw<"READ-ITEM"> + assign<&ast::ReadItemStmt::file>(ref<ID>))> {};

  struct WriteItemStmt
      : rule<create<ast::WriteItemStmt>(
            (ikw<"WRITE-ITEM"> |
             flag<&ast::WriteItemStmt::punched>(ikw<"PUNCH-ITEM">)) +
            assign<&ast::WriteItemStmt::file>(ref<ID>))> {};

  struct TransferStmt
      : rule<create<ast::TransferStmt>(
            ikw<"TRANSFER"> + assign<&ast::TransferStmt::from>(ref<ID>) +
            ikw<"TO"> + assign<&ast::TransferStmt::to>(ref<ID>))> {};

  struct IfStmt
      : rule<create<ast::IfStmt>(
            ikw<"IF"> +
            assign<&ast::IfStmt::cond>(ref<EQUAL> | ref<GREATER> | ref<LESS> |
                                       as<ast::Cond::end_of_data>(ikw<"END"> + ikw<"OF"> + ikw<"DATA">)) +
            go_to + ikw<"OPERATION"> +
            assign<&ast::IfStmt::target>(ref<NUM>))> {};

  struct OtherwiseStmt
      : rule<create<ast::OtherwiseStmt>(
            ikw<"OTHERWISE"> + go_to + ikw<"OPERATION"> +
            assign<&ast::OtherwiseStmt::target>(ref<NUM>))> {};

  struct RewindStmt
      : rule<create<ast::RewindStmt>(
            ikw<"REWIND"> + assign<&ast::RewindStmt::file>(ref<ID>))> {};

  struct StopStmt : rule<create<ast::StopStmt> + ikw<"STOP">> {};

  struct CompareStmt
      : rule<create<ast::CompareStmt>(
            ikw<"COMPARE"> + assign<&ast::CompareStmt::leftField>(ref<ID>) +
            lit<"("> + assign<&ast::CompareStmt::leftFile>(ref<ID>) + lit<")"> +
            ikw<"WITH"> + assign<&ast::CompareStmt::rightField>(ref<ID>) +
            lit<"("> + assign<&ast::CompareStmt::rightFile>(ref<ID>) +
            lit<")">)> {};

  struct Field : rule<create<ast::Value>(
                         assign<&ast::Value::text>(ref<ID>) + lit<"("> +
                         assign<&ast::Value::file>(ref<ID>) + lit<")">)> {};

  struct Value
      : rule<create<ast::Value>(
            (flag<&ast::Value::quoted>(
                 assign<&ast::Value::text>(ref<STRING>)) |
             assign<&ast::Value::text>(ref<DECIMAL> | ref<ID>)) +
            option(lit<"("> + assign<&ast::Value::file>(ref<ID>) + lit<")">))> {
  };

  struct MoveStmt
      : rule<create<ast::MoveStmt>(
            ikw<"MOVE"> + assign<&ast::MoveStmt::source>(ref<Value>) +
            ikw<"TO"> + assign<&ast::MoveStmt::destination>(ref<Field>))> {
  };

  struct ArithmeticStmt
      : rule<create<ast::ArithmeticStmt>(
            ((assign<&ast::ArithmeticStmt::op>(ref<Add>) +
                 assign<&ast::ArithmeticStmt::left>(ref<Value>) +
                 ikw<"TO">) |
             (assign<&ast::ArithmeticStmt::op>(as<ast::ArithmeticOp::subtract>(ikw<"SUBTRACT">)) +
                 assign<&ast::ArithmeticStmt::left>(ref<Value>) +
                 ikw<"FROM">) |
             (assign<&ast::ArithmeticStmt::op>(as<ast::ArithmeticOp::multiply>(ikw<"MULTIPLY">)) +
                 assign<&ast::ArithmeticStmt::left>(ref<Value>) +
                 ikw<"BY">) |
             (assign<&ast::ArithmeticStmt::op>(as<ast::ArithmeticOp::divide>(ikw<"DIVIDE">)) +
                 assign<&ast::ArithmeticStmt::left>(ref<Value>) +
                 ikw<"BY">))+assign<&ast::ArithmeticStmt::
                                       right>(ref<Value>) +
            option(ikw<"GIVING"> +
                   assign<&ast::ArithmeticStmt::result>(ref<Field>)))> {};

  struct PrintItemStmt
      : rule<create<ast::PrintItemStmt>(
            ikw<"PRINT-ITEM"> + assign<&ast::PrintItemStmt::file>(ref<ID>))> {};

  struct ExecuteStmt
      : rule<create<ast::ExecuteStmt>(
            ikw<"EXECUTE"> + ikw<"OPERATION"> +
            assign<&ast::ExecuteStmt::first>(ref<NUM>) +
            option(ikw<"THROUGH"> + ikw<"OPERATION"> +
                   assign<&ast::ExecuteStmt::last>(ref<NUM>)))> {};

  struct SetStmt
      : rule<create<ast::SetStmt>(ikw<"SET"> + ikw<"OPERATION"> +
                                  assign<&ast::SetStmt::which>(ref<NUM>) +
                                  ikw<"TO"> + go_to + ikw<"OPERATION"> +
                                  assign<&ast::SetStmt::target>(ref<NUM>))> {};

  struct TestStmt
      : rule<create<ast::TestStmt>(
            ikw<"TEST"> + assign<&ast::TestStmt::field>(ref<ID>) + lit<"("> +
            assign<&ast::TestStmt::file>(ref<ID>) + lit<")"> + ikw<"AGAINST"> +
            assign<&ast::TestStmt::against>(ref<Value>))> {};

  struct CloseOutStmt
      : rule<create<ast::CloseOutStmt>(
            ikw<"CLOSE-OUT"> +
            (ikw<"FILES"> |
             ikw<"FILE">)+append<&ast::CloseOutStmt::files>(ref<FILE_LETTER>) +
            many(option(lit<";"> | lit<",">) +
                 append<&ast::CloseOutStmt::files>(ref<FILE_LETTER>)))> {};

  struct Statement
      : rule<ref<HspStmt> | ref<OutputStmt> | ref<InputStmt> |
             ref<JumpToStmt> | ref<ReadItemStmt> |
             ref<WriteItemStmt> | ref<TransferStmt> | ref<IfStmt> |
             ref<OtherwiseStmt> | ref<RewindStmt> | ref<StopStmt> |
             ref<CompareStmt> | ref<MoveStmt> | ref<SetStmt> |
             ref<TestStmt> | ref<CloseOutStmt> |
             ref<ArithmeticStmt> | ref<PrintItemStmt> |
             ref<ExecuteStmt>> {};

  // `label statement (';' statement)* '.'`, where `statement` itself is zero
  // or more leaves run together with no separator: flattened here into one
  // list, since the ';' between groups carries no meaning of its own.
  static constexpr auto label =
      lit<"("> + assign<&ast::Line::label>(ref<NUM>) + lit<")">;

  struct Line
      : rule<label +
             many(append<&ast::Line::statements>(ref<Statement>) |
                  lit<";">) +
             option(lit<".">)> {};

  struct Program : rule<create<ast::Program> +
                            many(append<&ast::Program::lines>(ref<Line>)) +
                            option(flag<&ast::Program::hasEnd>(ref<END>))> {};

  using entry = Program;
  using ignored = WS;
  using hidden = COMMENT;
};

} // namespace flowmatic
