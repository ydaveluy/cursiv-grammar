#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "star/core/ast.hpp"
namespace star {
inline constexpr std::string_view heading_name(std::string_view text) {
  return text.substr(5);
}
struct Star : cursiv::GrammarSpec {
  static constexpr auto eol =
      lit<"\r\n"> | lit<"\n"> | (lit<"\r"> + !lit<"\n">);
  static constexpr auto space = cr<" \t\r\n\v\f">;
  static constexpr auto nonspace = ~space;
  static constexpr auto boundary = !nonspace;
  static constexpr auto quoteEnd = boundary | &cr<"[]{}:,">;
  struct WS : terminal<some(cr<" \t\v\f"> | (eol + !lit<";">))> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<"^\r\n">)> {};
  struct DATA : terminal<as<heading_name>(cursiv::Lit<"data_", true>{} +
                                          many(nonspace))> {};
  struct SAVE : terminal<as<heading_name>(cursiv::Lit<"save_", true>{} +
                                          some(nonspace))> {};
  struct SAVE_END : terminal<cursiv::Lit<"save_", true>{} + boundary> {};
  struct LOOP : terminal<cursiv::Lit<"loop_", true>{} + boundary> {};
  struct STOP : terminal<cursiv::Lit<"stop_", true>{} + boundary> {};
  struct GLOBAL : terminal<cursiv::Lit<"global_", true>{} + boundary> {};
  struct DATANAME : terminal<lit<"_"> + many(nonspace)> {};
  struct STRING1
      : terminal<lit<"'"> + many(!(lit<"'"> + quoteEnd) + cr<"^\r\n">) +
                 lit<"'"> + quoteEnd> {};
  struct STRING2
      : terminal<lit<"\""> + many(!(lit<"\""> + quoteEnd) + cr<"^\r\n">) +
                 lit<"\""> + quoteEnd> {};
  struct TRIPLE1 : terminal<through(lit<"'''">, lit<"'''">)> {};
  struct TRIPLE2 : terminal<through(lit<"\"\"\"">, lit<"\"\"\"">)> {};
  struct TEXT : terminal<eol + lit<";"> + many(!(eol + lit<";">)+any) + eol +
                         lit<";">> {};
  static constexpr auto bareStart =
      !(cursiv::Lit<"data_", true>{} | cursiv::Lit<"save_", true>{} |
        cursiv::Lit<"global_", true>{}) +
      !ref<LOOP> + !ref<STOP> + !cr<"_#\"'[]{}">;
  struct LITERAL : terminal<bareStart + some(cr<"^ \t\r\n\v\f[]{}">)> {};
  struct ATOM
      : terminal<bareStart + !lit<","> + some(cr<"^ \t\r\n\v\f[]{},">)> {};
  struct String : rule<create<ast::StringLit>(assign<&ast::StringLit::raw>(
                          ref<TRIPLE1> | ref<TRIPLE2> | ref<STRING1> |
                          ref<STRING2> | ref<TEXT>))> {};
  struct Literal
      : rule<assign<&ast::Literal::text>(ref<LITERAL>)> {
  };
  struct Value;
  struct DelimitedValue;
  struct Atom
      : rule<assign<&ast::Literal::text>(ref<ATOM>)> {};
  struct Loop;
  struct List
      : rule<create<ast::List>(
            lit<"["> +
            many(append<&ast::List::values>(ref<DelimitedValue>) +
                 option(lit<",">)) +
            lit<"]">)> {};
  struct TableEntry
      : rule<create<ast::TableEntry>(
            assign<&ast::TableEntry::key>(ref<String>) + lit<":"> +
            option(assign<&ast::TableEntry::value>(ref<DelimitedValue>)))> {
  };
  struct Table
      : rule<create<ast::Table>(
            lit<"{"> +
            many(append<&ast::Table::entries>(ref<TableEntry>) +
                 option(lit<",">)) +
            lit<"}">)> {};
  struct DelimitedValue
      : rule<ref<String> | ref<Atom> | ref<List> | ref<Table>> {
  };
  struct Scalar : rule<ref<String> | ref<Literal> | ref<List> |
                           ref<Table>> {};
  struct Stop : rule<create<ast::Stop>(ref<STOP>)> {};
  struct NestedHeader;
  static constexpr auto headerBody =
      ref<LOOP> + many(append<&ast::LoopHeader::items>(ref<DATANAME> |
                                                       ref<NestedHeader>));
  struct LoopHeader : rule<create<ast::LoopHeader>(headerBody)> {};
  struct NestedHeader
      : rule<create<ast::LoopHeader>(
            headerBody +
            option(flag<&ast::LoopHeader::closed>(ref<STOP>)))> {};
  struct Loop
      : rule<create<ast::Loop>(assign<&ast::Loop::header>(ref<LoopHeader>) +
                               many(append<&ast::Loop::values>(
                                   ref<Scalar> | ref<Stop>)))> {};
  struct Value : rule<ref<Scalar> | ref<Loop>> {};
  struct KeyVal
      : rule<create<ast::KeyVal>(
            assign<&ast::KeyVal::name>(ref<DATANAME>) +
            option(assign<&ast::KeyVal::value>(ref<Value>)))> {};
  struct SaveFrame;
  static constexpr auto element =
      ref<KeyVal> | ref<SaveFrame> | ref<Loop>;
  struct SaveFrame
      : rule<create<ast::SaveFrame>(
            assign<&ast::SaveFrame::name>(ref<SAVE>) +
            many(append<&ast::Content::elements>(element)) +
            option(flag<&ast::SaveFrame::closed>(ref<SAVE_END>)))> {};
  struct DataBlock : rule<create<ast::DataBlock>(
                             assign<&ast::DataBlock::name>(ref<DATA>) +
                             many(append<&ast::Content::elements>(element)))> {
  };
  struct Global
      : rule<create<ast::Global>(
            ref<GLOBAL> + many(append<&ast::Content::elements>(element)))> {};
  struct Doc : rule<create<ast::Doc>(many(append<&ast::Doc::blocks>(
                            ref<DataBlock> | ref<Global>))) +
                        eof> {};
  using entry = Doc;
  using ignored = list<WS, COMMENT>;
};
} // namespace star
