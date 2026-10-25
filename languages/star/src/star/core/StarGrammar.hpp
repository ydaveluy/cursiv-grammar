#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "star/core/ast.hpp"
namespace star {
inline constexpr std::string_view heading_name(std::string_view text) {
  return text.substr(5);
}
using HeadingName = cursiv::text_conv<heading_name>;
struct Star : cursiv::GrammarSpec {
  static constexpr auto eol =
      lit<"\r\n"> | lit<"\n"> | (lit<"\r"> + !lit<"\n">);
  static constexpr auto space = cr<" \t\r\n\v\f">;
  static constexpr auto nonspace = ~space;
  static constexpr auto boundary = !nonspace;
  static constexpr auto quoteEnd = boundary | &cr<"[]{}:,">;
  struct WS : terminal<some(cr<" \t\v\f"> | (eol + !lit<";">))> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<"^\r\n">)> {};
  struct DATA
      : terminal<cursiv::Lit<"data_", true>{} + many(nonspace), HeadingName> {};
  struct SAVE
      : terminal<cursiv::Lit<"save_", true>{} + some(nonspace), HeadingName> {};
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
  struct StringRule : rule<create<ast::StringLit>(assign<&ast::StringLit::raw>(
                          ref<TRIPLE1> | ref<TRIPLE2> | ref<STRING1> |
                          ref<STRING2> | ref<TEXT>))> {};
  struct LiteralRule
      : rule<create<ast::Literal>(assign<&ast::Literal::text>(ref<LITERAL>))> {
  };
  struct ValueRule;
  struct DelimitedValueRule;
  struct AtomRule
      : rule<create<ast::Literal>(assign<&ast::Literal::text>(ref<ATOM>))> {};
  struct LoopRule;
  struct ListRule
      : rule<create<ast::List>(
            lit<"["> +
            many(append<&ast::List::values>(ref<DelimitedValueRule>) +
                 option(lit<",">)) +
            lit<"]">)> {};
  struct TableEntryRule
      : rule<create<ast::TableEntry>(
            assign<&ast::TableEntry::key>(ref<StringRule>) + lit<":"> +
            option(assign<&ast::TableEntry::value>(ref<DelimitedValueRule>)))> {
  };
  struct TableRule
      : rule<create<ast::Table>(
            lit<"{"> +
            many(append<&ast::Table::entries>(ref<TableEntryRule>) +
                 option(lit<",">)) +
            lit<"}">)> {};
  struct DelimitedValueRule
      : rule<ref<StringRule> | ref<AtomRule> | ref<ListRule> | ref<TableRule>> {
  };
  struct ScalarRule : rule<ref<StringRule> | ref<LiteralRule> | ref<ListRule> |
                           ref<TableRule>> {};
  struct StopRule : rule<create<ast::Stop>(ref<STOP>)> {};
  struct NestedHeaderRule;
  static constexpr auto headerBody =
      ref<LOOP> + many(append<&ast::LoopHeader::items>(ref<DATANAME> |
                                                       ref<NestedHeaderRule>));
  struct LoopHeaderRule : rule<create<ast::LoopHeader>(headerBody)> {};
  struct NestedHeaderRule
      : rule<create<ast::LoopHeader>(
            headerBody +
            option(flag<&ast::LoopHeader::closed>(ref<STOP>)))> {};
  struct LoopRule
      : rule<create<ast::Loop>(assign<&ast::Loop::header>(ref<LoopHeaderRule>) +
                               many(append<&ast::Loop::values>(
                                   ref<ScalarRule> | ref<StopRule>)))> {};
  struct ValueRule : rule<ref<ScalarRule> | ref<LoopRule>> {};
  struct KeyValRule
      : rule<create<ast::KeyVal>(
            assign<&ast::KeyVal::name>(ref<DATANAME>) +
            option(assign<&ast::KeyVal::value>(ref<ValueRule>)))> {};
  struct SaveFrameRule;
  static constexpr auto element =
      ref<KeyValRule> | ref<SaveFrameRule> | ref<LoopRule>;
  struct SaveFrameRule
      : rule<create<ast::SaveFrame>(
            assign<&ast::SaveFrame::name>(ref<SAVE>) +
            many(append<&ast::Content::elements>(element)) +
            option(flag<&ast::SaveFrame::closed>(ref<SAVE_END>)))> {};
  struct DataBlockRule : rule<create<ast::DataBlock>(
                             assign<&ast::DataBlock::name>(ref<DATA>) +
                             many(append<&ast::Content::elements>(element)))> {
  };
  struct GlobalRule
      : rule<create<ast::Global>(
            ref<GLOBAL> + many(append<&ast::Content::elements>(element)))> {};
  struct DocRule : rule<create<ast::Doc>(many(append<&ast::Doc::blocks>(
                            ref<DataBlockRule> | ref<GlobalRule>))) +
                        eof> {};
  using entry = DocRule;
  using ignore = list<WS, COMMENT>;
};
} // namespace star
