// The http language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "http/core/ast.hpp"

namespace http {

struct Http : cursiv::GrammarSpec {
  static constexpr auto crlf_bytes = lit<"\r\n"> | lit<"\n">;
  static constexpr auto ows_byte = cr<R"( \t)">;

  // tchar: RFC 7230 token character.
  static constexpr auto tchar = cr<"0-9A-Za-z!#$%&'*+.^_`|~-">;

  // field-vchar: VCHAR (%x21-7E) or obs-text (%x80-FF).
  // Complement ASCII controls so obs-text also admits isolated octets 80-FF,
  // rather than requiring them to form a valid Unicode scalar.
  static constexpr auto field_vchar = ~cr<R"(\x00-\x20\x7f)">;

  struct SP : terminal<lit<" ">> {};
  struct HTAB : terminal<lit<"\t">> {};
  struct CRLF : terminal<crlf_bytes> {};

  struct METHOD : terminal<some(tchar)> {};
  struct FIELD_NAME : terminal<some(tchar)> {};

  struct VERSION : terminal<some(d) + lit<"."> + some(d)> {};
  struct STATUS : terminal<some(d)> {};
  struct REASON : terminal<some(cr<R"(^\r\n)">)> {};

  // origin-form: absolute-path ('?' query)?, kept as one span -- the
  // language has no other request-target form to distinguish it from.
  struct REQUEST_TARGET : terminal<some(cr<R"(^ \t\r\n)">)> {};

  // field-value = *( field-content / obs-fold ); a terminal cannot match
  // nothing, so an absent value is `option()`-ed at the call site instead.
  struct FIELD_VALUE
      : terminal<some((field_vchar + many(some(ows_byte) + field_vchar)) |
                      (crlf_bytes + some(ows_byte)))> {};

  static constexpr auto ows = many(ref<SP> | ref<HTAB>);

  struct RequestLine
      : rule<assign<&ast::RequestLine::method>(ref<METHOD>) + ref<SP> +
             assign<&ast::RequestLine::target>(ref<REQUEST_TARGET>) + ref<SP> +
             lit<"HTTP"> + lit<"/"> +
             assign<&ast::RequestLine::version>(ref<VERSION>) + ref<CRLF>> {};

  struct StatusLine
      : rule<lit<"HTTP/"> + assign<&ast::StatusLine::version>(ref<VERSION>) +
             ref<SP> + assign<&ast::StatusLine::status>(ref<STATUS>) +
             option(ref<SP> + option(assign<&ast::StatusLine::reason>(ref<REASON>))) +
             ref<CRLF>> {};

  struct HeaderField
      : rule<assign<&ast::HeaderField::name>(ref<FIELD_NAME>) + lit<":"> + ows +
             option(assign<&ast::HeaderField::value>(ref<FIELD_VALUE>) + ows)> {
  };

  struct BODY : terminal<some(byte)> {};

  struct Message
      : rule<create<ast::Message> +
             (assign<&ast::Message::request_line>(ref<RequestLine>) |
              assign<&ast::Message::status_line>(ref<StatusLine>)) +
             many(append<&ast::Message::headers>(ref<HeaderField>) +
                  ref<CRLF>) +
             option(flag<&ast::Message::headersTerminated>(ref<CRLF>) +
                    option(assign<&ast::Message::body>(ref<BODY>)))> {};

  using entry = Message;
};

} // namespace http
