#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "swiftfin/core/ast.hpp"

namespace swiftfin {
struct SwiftFin : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct CRLF : terminal<eol> {};
  struct HeaderValue : terminal<some(cr<"^{}">)> {};
  struct MapKey : terminal<some(cr<"^{}:">)> {};
  struct UserId : terminal<cr<"035-9a-zA-Z">> {};

  struct Nested : terminal<
      (lit<"{4:"> + &(many(s) + lit<":">) +
       many(!lit<"\n-}"> + any) + lit<"\n-}">) |
      (lit<"{"> + many(ref<Nested> | cr<"^{}">) + lit<"}">)> {};
  struct MapValue : terminal<some(ref<Nested> | cr<"^{}">)> {};
  struct KeyValueRule : rule<lit<"{"> + option(assign<&ast::KeyValue::key>(ref<MapKey>)) +
      lit<":"> + option(assign<&ast::KeyValue::value>(ref<MapValue>)) + lit<"}">> {};

  static constexpr auto tagStart = lit<":"> + repeat<2>(d) + option(cr<"A-Z">) + lit<":">;
  struct TagName : terminal<some(cr<R"(^:\r\n)">)> {};
  struct FieldValue : terminal<some(!(eol + (tagStart | lit<"-}">)) + any)> {};
  struct TextFieldRule : rule<lit<":"> + assign<&ast::TextField::tag>(ref<TagName>) +
      lit<":"> + option(assign<&ast::TextField::value>(ref<FieldValue>))> {};

  static constexpr auto headerEnd = lit<":"> +
      option(assign<&ast::Header::value>(ref<HeaderValue>)) + lit<"}">;
  struct BasicHeader : rule<create<ast::Header> + lit<"{"> +
      assign<&ast::Block::id>(lit<"1">) + headerEnd> {};
  struct ApplicationHeader : rule<create<ast::Header> + lit<"{"> +
      assign<&ast::Block::id>(lit<"2">) + headerEnd> {};
  struct MapBlock : rule<create<ast::TagBlock> + lit<"{"> +
      assign<&ast::Block::id>(ref<UserId> | lit<"4">) + lit<":"> +
      many(append<&ast::TagBlock::tags>(ref<KeyValueRule>)) + lit<"}">> {};
  struct TextBlock : rule<create<ast::TextBlock> + lit<"{"> +
      assign<&ast::Block::id>(lit<"4">) + lit<":"> + many(ref<CRLF>) +
      many(append<&ast::TextBlock::fields>(ref<TextFieldRule>) + ref<CRLF>) + lit<"-}">> {};
  struct OtherBlock : rule<ref<ApplicationHeader> | ref<TextBlock> | ref<MapBlock>> {};
  struct AnyBlock : rule<ref<BasicHeader> | ref<OtherBlock>> {};
  struct MessageRule : rule<append<&ast::Message::blocks>(ref<AnyBlock>) +
      many(many(ref<WS>) + append<&ast::Message::blocks>(ref<OtherBlock>))> {};
  struct DocumentRule : rule<create<ast::Document> + many(ref<WS>) +
      many(append<&ast::Document::messages>(ref<MessageRule>) + many(ref<WS>))> {};
  using entry = DocumentRule;
};
}  // namespace swiftfin
