#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "d2/core/ast.hpp"

namespace d2 {
inline constexpr auto unquote = [](std::string_view text) {
  return text.size() >= 2 ? text.substr(1, text.size() - 2)
                          : std::string_view{};
};

struct D2 : cursiv::GrammarSpec {
  static constexpr auto hws = !cr<R"(\n)"> + uc::White_Space;
  static constexpr auto escape = lit<"\\"> + any;
  static constexpr auto end = cr<R"(\n;#{}[])"> | eof;
  static constexpr auto edge_start = lit<"--"> | lit<"->"> | lit<"-*">;
  static constexpr auto key_end = end | cr<".:<>&"> | edge_start;
  static constexpr auto group_end =
      lit<")"> + &(many(hws) + (end | cr<".:{[">));
  static constexpr auto key_char = escape | (!key_end + any);
  static constexpr auto substitution_open = lit<"$"> + many(hws) + lit<"{">;
  static constexpr auto value_char = escape | (!(end | substitution_open) + any);
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
      : terminal<some(escape | (!(cr<R"("\n)"> | substitution_open) + any))> {};
  struct KEY_QUOTED
      : terminal<as<unquote>(lit<"\""> + many(escape | cr<R"(^"\\\n)">) +
                             lit<"\"">)> {};
  struct SINGLE_QUOTED
      : terminal<as<unquote>(
            lit<"'"> + many(lit<"''"> | (lit<"\\"> + lit<"\n">) |
                           cr<R"(^'\n)">) + lit<"'">)> {
  };
  struct Quote;
  struct BLOCK
      : terminal<through(lit<"|"> + mark<Quote>(many(~(uc::White_Space | uc::L | uc::Nd | cr<"_">))),
                         same<Quote> + lit<"|">)> {};
  static constexpr auto continuation =
      lit<"\\"> + many(hws) + lit<"\n"> + many(hws);
  struct OP
      : terminal<(cr<"<*"> + many(lit<"-"> | continuation) + option(cr<">*">)) |
                 (lit<"-"> + many(lit<"-"> | continuation) +
                  option(cr<">*">))> {};
  struct INDEX : terminal<some(d) | lit<"*">> {};
  struct PREFIX : terminal<some(cr<"./">)> {};

  struct Path;
  struct Value;
  struct Map;
  struct Array;
  struct Key;
  struct Import;
  struct Substitution;
  struct Spread;

  struct KeyChunk : rule<assign<&ast::Chunk::raw>(ref<KEY_TEXT>)> {};
  struct GroupChunk : rule<assign<&ast::Chunk::raw>(ref<GROUP_TEXT>)> {};
  struct QuotedKeyChunk : rule<assign<&ast::Chunk::raw>(ref<KEY_QUOTED>)> {};
  struct SingleChunk : rule<assign<&ast::Chunk::raw>(ref<SINGLE_QUOTED>)> {};
  struct BlockChunk : rule<assign<&ast::Chunk::raw>(ref<BLOCK>)> {};
  struct ValueChunk : rule<assign<&ast::Chunk::raw>(ref<VALUE_TEXT>)> {};
  struct FirstValueChunk
      : rule<assign<&ast::Chunk::raw>(ref<FIRST_VALUE_TEXT>)> {};
  struct DoubleChunk : rule<assign<&ast::Chunk::raw>(ref<DOUBLE_TEXT>)> {};
  struct Single : rule<create<ast::SingleText> +
                           append<&ast::Text::parts>(ref<SingleChunk>)> {};
  struct Block : rule<create<ast::BlockText> +
                          append<&ast::Text::parts>(ref<BlockChunk>)> {};
  struct QuotedKey : rule<create<ast::QuotedText> +
                              append<&ast::Text::parts>(ref<QuotedKeyChunk>)> {
  };
  struct PlainKey : rule<append<&ast::Text::parts>(ref<KeyChunk>)> {};
  struct GroupKey : rule<append<&ast::Text::parts>(ref<GroupChunk>)> {};
  static constexpr auto quoted_key =
      ref<QuotedKey> | ref<Single> | ref<Block>;
  struct KeyText : rule<quoted_key | ref<PlainKey>> {};
  struct GroupText : rule<quoted_key | ref<GroupKey>> {};
  static constexpr auto key_begin = !cr<"(\n;#{}[]:.<>&">;
  struct Path : rule<append<&ast::KeyPath::parts>(ref<KeyText>) +
                         many(lit<"."> + key_begin +
                              append<&ast::KeyPath::parts>(ref<KeyText>))> {
  };
  struct GroupPath
      : rule<append<&ast::KeyPath::parts>(ref<GroupText>) +
             many(lit<"."> + key_begin +
                  append<&ast::KeyPath::parts>(ref<GroupText>))> {};
  struct Substitution
      : rule<substitution_open +
             skip(ignore<WS>, hide<COMMENT, BLOCKCOMMENT>,
                  assign<&ast::Substitution::path>(ref<Path>) + lit<"}">)> {
  };
  struct Plain : rule<append<&ast::Text::parts>(ref<Substitution> |
                                                    ref<FirstValueChunk>) +
                          many(append<&ast::Text::parts>(ref<Substitution> |
                                                         ref<ValueChunk>))> {};
  struct Quoted : rule<create<ast::QuotedText> + lit<"\""> +
                           skip(ignore<>, hide<>,
                                many(append<&ast::Text::parts>(
                                    ref<Substitution> | ref<DoubleChunk>)) +
                                    lit<"\"">)> {};
  struct Text : rule<ref<Quoted> | ref<Single> | ref<Block> |
                         ref<Plain>> {};
  struct Import
      : rule<lit<"@"> + option(assign<&ast::Import::prefix>(ref<PREFIX>)) +
             option(assign<&ast::Import::path>(ref<Path>))> {};
  struct SpreadSubstitution
      : rule<flag<&ast::Substitution::spread>(lit<"...">) + substitution_open +
             assign<&ast::Substitution::path>(ref<Path>) + lit<"}">> {};
  struct SpreadImport
      : rule<flag<&ast::Import::spread>(lit<"...">) + lit<"@"> +
             option(assign<&ast::Import::prefix>(ref<PREFIX>)) +
             option(assign<&ast::Import::path>(ref<Path>))> {};
  struct Spread
      : rule<ref<SpreadSubstitution> | ref<SpreadImport>> {};
  struct Value
      : rule<ref<Map> | ref<Array> | ref<Import> | ref<Text>> {
  };
  struct Hop : rule<assign<&ast::Hop::op>(ref<OP>) +
                        assign<&ast::Hop::target>(ref<Path>)> {};
  struct GroupHop : rule<assign<&ast::Hop::op>(ref<OP>) +
                             assign<&ast::Hop::target>(ref<GroupPath>)> {};
  struct Key
      : rule<create<ast::Key> +
             option(flag<&ast::Key::filter>(lit<"&">) |
                    flag<&ast::Key::negated>(lit<"!&">)) +
             ((flag<&ast::Key::grouped>(
                   (assign<&ast::Key::scope>(ref<Path>) + ref<DOT_GROUP>) |
                   lit<"(">) +
               assign<&ast::Key::path>(ref<GroupPath>) +
               some(append<&ast::Key::hops>(ref<GroupHop>)) + lit<")"> +
               option(lit<"["> + assign<&ast::Key::index>(ref<INDEX>) +
                      lit<"]">) +
               option(lit<"."> + assign<&ast::Key::property>(ref<Path>))) |
              (assign<&ast::Key::path>(ref<Path>) +
               many(append<&ast::Key::hops>(ref<Hop>)))) +
             option((flag<&ast::Key::colon>(ref<COLON>) +
                     option((assign<&ast::Key::value>(ref<Text>) +
                             option(assign<&ast::Key::block>(ref<Map>))) |
                            assign<&ast::Key::value>(ref<Map> | ref<Array> |
                                                    ref<Import>))) |
                    assign<&ast::Key::block>(ref<Map>))> {};
  static constexpr auto separator = ref<NL> | ref<SEMICOLON>;
  static constexpr auto statement =
      ref<Spread> | ref<Key>;
  struct Map
      : rule<lit<"{"> +
             many(separator) +
             many(append<&ast::Map::statements>(statement) +
                  (some(separator) | &lit<"}">)) +
             lit<"}">> {};
  struct Array
      : rule<ref<OPEN_ARRAY> +
             many(separator) +
             many(append<&ast::Array::values>(ref<Spread> | ref<Value>) +
                  (some(separator) | &lit<"]">)) +
             lit<"]">> {};
  struct Diagram
      : rule<create<ast::Diagram> +
             many(separator) +
             many(append<&ast::Diagram::statements>(statement) +
                  (some(separator) | eof))> {};
  using entry = Diagram;
  using ignored = WS;
  using hidden = list<COMMENT, BLOCKCOMMENT>;
};
}  // namespace d2
