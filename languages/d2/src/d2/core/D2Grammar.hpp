#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "d2/core/ast.hpp"

namespace d2 {
inline constexpr auto unquote = [](std::string_view text) {
  return text.size() >= 2 ? text.substr(1, text.size() - 2)
                          : std::string_view{};
};
using Unquoted = cursiv::text_conv<unquote>;

struct D2 : cursiv::GrammarSpec {
  static constexpr auto hws = !cr<R"(\n)"> + uc::White_Space;
  static constexpr auto escape = lit<"\\"> + any;
  static constexpr auto end = cr<R"(\n;#{}[])"> | eof;
  static constexpr auto edge_start = lit<"--"> | lit<"->"> | lit<"-*">;
  static constexpr auto key_end = end | cr<".:<>&"> | edge_start;
  static constexpr auto group_end =
      lit<")"> + &(many(hws) + (end | cr<".:{[">));
  static constexpr auto key_char = escape | (!key_end + any);
  static constexpr auto value_char = escape | (!(end | lit<"${">)+any);
  struct WS : terminal<some(hws)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  struct BLOCKCOMMENT : terminal<through(lit<"\"\"\"">, lit<"\"\"\"">)> {};
  struct NL : terminal<lit<"\n"> + many(hws)> {};
  struct SEMICOLON : terminal<lit<";"> + many(hws)> {};
  struct COLON : terminal<lit<":"> + many(hws)> {};
  struct OPEN_ARRAY : terminal<lit<"["> + many(hws)> {};
  struct DOT_GROUP : terminal<lit<"."> + many(hws) + lit<"(">> {};
  struct KEY_TEXT : terminal<!hws + !cr<R"("'|()"> +
                             some(!(some(hws) + key_end) + key_char)> {};
  struct GROUP_TEXT
      : terminal<!hws + !cr<R"("'|()"> +
                 some(!(group_end | (some(hws) + (key_end | group_end))) +
                      key_char)> {};
  struct VALUE_TEXT : terminal<some(!(some(hws) + end) + value_char)> {};
  struct FIRST_VALUE_TEXT : terminal<!hws + !cr<R"("'|@)"> +
                                     some(!(some(hws) + end) + value_char)> {};
  struct DOUBLE_TEXT
      : terminal<some(escape | (!(cr<R"("\n)"> | lit<"${">)+any))> {};
  struct KEY_QUOTED
      : terminal<lit<"\""> + many(escape | cr<R"(^"\\\n)">) + lit<"\"">,
                 Unquoted> {};
  struct SINGLE_QUOTED
      : terminal<lit<"'"> + many(lit<"''"> | escape | cr<R"(^'\\\n)">) +
                     lit<"'">,
                 Unquoted> {};
  struct Quote;
  struct BLOCK
      : terminal<through(lit<"|"> + mark<Quote>(many(cr<R"(!-/:-@[-^`{-~)">)),
                         same<Quote> + lit<"|">)> {};
  static constexpr auto continuation =
      lit<"\\"> + many(hws) + lit<"\n"> + many(hws);
  struct OP
      : terminal<(cr<"<*"> + many(lit<"-"> | continuation) + option(cr<">*">)) |
                 (lit<"-"> + many(lit<"-"> | continuation) +
                  option(cr<">*">))> {};
  struct INDEX : terminal<some(d) | lit<"*">> {};
  struct PREFIX : terminal<some(cr<"./">)> {};

  struct PathRule;
  struct ValueRule;
  struct MapRule;
  struct ArrayRule;
  struct KeyRule;
  struct ImportRule;
  struct SubstitutionRule;
  struct SpreadRule;

  struct KeyChunk : rule<assign<&ast::Chunk::raw>(ref<KEY_TEXT>)> {};
  struct GroupChunk : rule<assign<&ast::Chunk::raw>(ref<GROUP_TEXT>)> {};
  struct QuotedKeyChunk : rule<assign<&ast::Chunk::raw>(ref<KEY_QUOTED>)> {};
  struct SingleChunk : rule<assign<&ast::Chunk::raw>(ref<SINGLE_QUOTED>)> {};
  struct BlockChunk : rule<assign<&ast::Chunk::raw>(ref<BLOCK>)> {};
  struct ValueChunk : rule<assign<&ast::Chunk::raw>(ref<VALUE_TEXT>)> {};
  struct FirstValueChunk
      : rule<assign<&ast::Chunk::raw>(ref<FIRST_VALUE_TEXT>)> {};
  struct DoubleChunk : rule<assign<&ast::Chunk::raw>(ref<DOUBLE_TEXT>)> {};
  struct SingleRule : rule<create<ast::SingleText> +
                           append<&ast::Text::parts>(ref<SingleChunk>)> {};
  struct BlockRule : rule<create<ast::BlockText> +
                          append<&ast::Text::parts>(ref<BlockChunk>)> {};
  struct QuotedKeyRule : rule<create<ast::QuotedText> +
                              append<&ast::Text::parts>(ref<QuotedKeyChunk>)> {
  };
  struct PlainKeyRule : rule<append<&ast::Text::parts>(ref<KeyChunk>)> {};
  struct GroupKeyRule : rule<append<&ast::Text::parts>(ref<GroupChunk>)> {};
  static constexpr auto quoted_key =
      ref<QuotedKeyRule> | ref<SingleRule> | ref<BlockRule>;
  struct KeyTextRule : rule<quoted_key | ref<PlainKeyRule>> {};
  struct GroupTextRule : rule<quoted_key | ref<GroupKeyRule>> {};
  static constexpr auto key_begin = !cr<"(\n;#{}[]:.<>&">;
  struct PathRule : rule<append<&ast::KeyPath::parts>(ref<KeyTextRule>) +
                         many(lit<"."> + key_begin +
                              append<&ast::KeyPath::parts>(ref<KeyTextRule>))> {
  };
  struct GroupPathRule
      : rule<append<&ast::KeyPath::parts>(ref<GroupTextRule>) +
             many(lit<"."> + key_begin +
                  append<&ast::KeyPath::parts>(ref<GroupTextRule>))> {};
  struct SubstitutionRule
      : rule<lit<"${"> +
             skip(ignored<WS>, hide<COMMENT, BLOCKCOMMENT>,
                  assign<&ast::Substitution::path>(ref<PathRule>) + lit<"}">)> {
  };
  struct PlainRule : rule<append<&ast::Text::parts>(ref<SubstitutionRule> |
                                                    ref<FirstValueChunk>) +
                          many(append<&ast::Text::parts>(ref<SubstitutionRule> |
                                                         ref<ValueChunk>))> {};
  struct QuotedRule : rule<create<ast::QuotedText> + lit<"\""> +
                           skip(ignored<>, hide<>,
                                many(append<&ast::Text::parts>(
                                    ref<SubstitutionRule> | ref<DoubleChunk>)) +
                                    lit<"\"">)> {};
  struct TextRule : rule<ref<QuotedRule> | ref<SingleRule> | ref<BlockRule> |
                         ref<PlainRule>> {};
  struct ImportRule
      : rule<lit<"@"> + option(assign<&ast::Import::prefix>(ref<PREFIX>)) +
             option(assign<&ast::Import::path>(ref<PathRule>))> {};
  struct SpreadSubstitutionRule
      : rule<flag<&ast::Substitution::spread>(lit<"...">) + lit<"${"> +
             assign<&ast::Substitution::path>(ref<PathRule>) + lit<"}">> {};
  struct SpreadImportRule
      : rule<flag<&ast::Import::spread>(lit<"...">) + lit<"@"> +
             option(assign<&ast::Import::prefix>(ref<PREFIX>)) +
             option(assign<&ast::Import::path>(ref<PathRule>))> {};
  struct SpreadRule
      : rule<ref<SpreadSubstitutionRule> | ref<SpreadImportRule>> {};
  struct ValueRule
      : rule<ref<MapRule> | ref<ArrayRule> | ref<ImportRule> | ref<TextRule>> {
  };
  struct HopRule : rule<assign<&ast::Hop::op>(ref<OP>) +
                        assign<&ast::Hop::target>(ref<PathRule>)> {};
  struct GroupHopRule : rule<assign<&ast::Hop::op>(ref<OP>) +
                             assign<&ast::Hop::target>(ref<GroupPathRule>)> {};
  struct KeyRule
      : rule<create<ast::Key> +
             option(flag<&ast::Key::filter>(lit<"&">) |
                    flag<&ast::Key::negated>(lit<"!&">)) +
             ((flag<&ast::Key::grouped>(
                   (assign<&ast::Key::scope>(ref<PathRule>) + ref<DOT_GROUP>) |
                   lit<"(">) +
               assign<&ast::Key::path>(ref<GroupPathRule>) +
               some(append<&ast::Key::hops>(ref<GroupHopRule>)) + lit<")"> +
               option(lit<"["> + assign<&ast::Key::index>(ref<INDEX>) +
                      lit<"]">) +
               option(lit<"."> + assign<&ast::Key::property>(ref<PathRule>))) |
              (assign<&ast::Key::path>(ref<PathRule>) +
               many(append<&ast::Key::hops>(ref<HopRule>)))) +
             option((flag<&ast::Key::colon>(ref<COLON>) +
                     option(assign<&ast::Key::value>(ref<ValueRule>)) +
                     option(assign<&ast::Key::block>(ref<MapRule>))) |
                    assign<&ast::Key::block>(ref<MapRule>))> {};
  static constexpr auto separator = ref<NL> | ref<SEMICOLON>;
  static constexpr auto statement =
      ref<SpreadRule> | ref<KeyRule> | ref<ArrayRule>;
  struct MapRule
      : rule<lit<"{"> +
             many(separator | append<&ast::Map::statements>(statement)) +
             lit<"}">> {};
  struct ArrayRule
      : rule<ref<OPEN_ARRAY> +
             many(separator | append<&ast::Array::values>(ref<SpreadRule> |
                                                          ref<ValueRule>)) +
             lit<"]">> {};
  struct DiagramRule
      : rule<create<ast::Diagram> +
             many(separator | append<&ast::Diagram::statements>(statement))> {};
  using entry = DiagramRule;
  using ignore = WS;
  using hidden = list<COMMENT, BLOCKCOMMENT>;
};
}  // namespace d2
