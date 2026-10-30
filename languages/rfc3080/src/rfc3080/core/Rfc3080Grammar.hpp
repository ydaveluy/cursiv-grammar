#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc3080/core/ast.hpp"

namespace rfc3080 {

struct Rfc3080 : cursiv::GrammarSpec {
  struct SP : terminal<some(cr<R"( \t\v\f)">)> {};
  struct NUMBER : terminal<option(cr<"+-">) + some(d)> {};
  struct MSG : terminal<as<ast::Kind::Msg>(kw<"MSG">)> {};
  struct RPY : terminal<as<ast::Kind::Rpy>(kw<"RPY">)> {};
  struct ANS : terminal<as<ast::Kind::Ans>(kw<"ANS">)> {};
  struct ERR : terminal<as<ast::Kind::Err>(kw<"ERR">)> {};
  struct NUL : terminal<as<ast::Kind::Nul>(kw<"NUL">)> {};

  inline static constexpr auto payload_text = [](std::string_view text) {
    return text.substr(2, text.size() - 7);
  };
  struct PAYLOAD_TRAILER : terminal<as<payload_text>(lit<"\r\n"> + many(byte) +
                                                     lit<"END\r\n"> + eof)> {};

  struct Frame : rule<create<ast::Frame>(
      option(ref<SP>) +
      assign<&ast::Frame::kind>(ref<MSG> | ref<RPY> | ref<ANS> | ref<ERR> | ref<NUL>) + ref<SP> +
      assign<&ast::Frame::channel>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::msgno>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::more>(as<false>(lit<".">) | as<true>(lit<"*">)) + ref<SP> +
      assign<&ast::Frame::seqno>(ref<NUMBER>) + ref<SP> +
      assign<&ast::Frame::size>(ref<NUMBER>) +
      option(ref<SP> + assign<&ast::Frame::ansno>(ref<NUMBER>)) + option(ref<SP>) +
      assign<&ast::Frame::payload>(ref<PAYLOAD_TRAILER>))> {};

  using entry = Frame;
};

}  // namespace rfc3080
