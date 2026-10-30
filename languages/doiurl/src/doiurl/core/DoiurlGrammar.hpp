// The doiurl language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "doiurl/core/ast.hpp"

namespace doiurl {

struct Doiurl : cursiv::GrammarSpec {
  // A character allowed unescaped: unreserved (letter, digit, mark) or a
  // reserved sub-delimiter, or a %-escape (either hex case).
  static constexpr auto pchar = cr<"a-zA-Z0-9_.!~*'();:@&=+$,-"> |
                                (lit<"%"> + cr<"0-9a-fA-F"> + cr<"0-9a-fA-F">);
  static constexpr auto query_char = pchar | cr<"/?">;

  struct WS : terminal<some(s)> {};

  struct SEGMENT : terminal<some(pchar)> {};
  struct URI_TEXT : terminal<some(query_char)> {};
  struct SegmentValue : rule<option(gap::none + ref<SEGMENT>)> {};
  struct UriValue : rule<option(gap::none + ref<URI_TEXT>)> {};

  struct DoiUri
      : rule<ikw<"doi"> + gap::none + lit<":"> +
             assign<&ast::DoiUri::prefix>(ref<SegmentValue>) +
             some(gap::none + lit<"/"> +
                  append<&ast::DoiUri::suffix>(ref<SegmentValue>)) +
             option(gap::none + lit<"?"> +
                    assign<&ast::DoiUri::query>(ref<UriValue>)) +
             option(gap::none + lit<"#"> +
                    assign<&ast::DoiUri::fragment>(ref<UriValue>))> {
  };

  using entry = DoiUri;
  using ignored = WS;
};

}  // namespace doiurl
