#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc3080/core/ast.hpp"

namespace rfc3080 {

struct Rfc3080 : cursiv::GrammarSpec {
  struct SP : terminal<some(cr<R"( \t\v\f)">)> {};
  struct NUMBER : terminal<option(cr<"+-">) + some(d)> {};
  struct DOT : terminal<lit<".">, value<false>> {};
  struct STAR : terminal<lit<"*">, value<true>> {};
  struct MSG : terminal<kw<"MSG">, value<ast::Kind::Msg>> {};
  struct RPY : terminal<kw<"RPY">, value<ast::Kind::Rpy>> {};
  struct ANS : terminal<kw<"ANS">, value<ast::Kind::Ans>> {};
  struct ERR : terminal<kw<"ERR">, value<ast::Kind::Err>> {};
  struct NUL : terminal<kw<"NUL">, value<ast::Kind::Nul>> {};

  inline static constexpr auto payload_text = [](std::string_view text) {
    return text.substr(2, text.size() - 7);
  };
  struct PAYLOAD_TRAILER
      : terminal<lit<"\r\n"> + many(byte) + lit<"END\r\n"> + eof,
                 text_conv<payload_text>> {};

  struct FrameRule : rule<create<ast::Frame>(
      option(ref<SP>) +
      assign<&ast::Frame::kind>(ref<MSG> | ref<RPY> | ref<ANS> | ref<ERR> | ref<NUL>) + ref<SP> +
      assign<&ast::Frame::channel>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::msgno>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::more>(ref<DOT> | ref<STAR>) + ref<SP> +
      assign<&ast::Frame::seqno>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::size>(ref<NUMBER>) +
      option(ref<SP> + assign<&ast::Frame::ansno>(ref<NUMBER>)) + option(ref<SP>) +
      assign<&ast::Frame::payload>(ref<PAYLOAD_TRAILER>))> {};

  using entry = FrameRule;
};

}  // namespace rfc3080
