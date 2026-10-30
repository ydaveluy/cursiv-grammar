#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rcs/core/ast.hpp"

namespace rcs {

inline constexpr auto unat = [](std::string_view t,
                                std::pmr::memory_resource *mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('@') == std::string_view::npos)
    return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '@')
      ++i;
  }
  return std::string_view(room.data(), n);
};

struct Rcs : cursiv::GrammarSpec {
  static constexpr auto snippetChar = cr<"^;: \t\n\r\f\v">;
  static constexpr auto word_boundary = snippetChar;
  struct WS : terminal<some(cr<" \t\f\r\n\v">)> {};
  struct IDENT : terminal<some(snippetChar)> {};
  struct OptionalIdent : rule<option(ref<IDENT>)> {};
  struct REVISION : terminal<some(d | lit<".">)> {};
  struct STRING
      : terminal<as<unat>(lit<"@"> + many(lit<"@@"> | cr<"^@">) + lit<"@">)> {};
  struct PHRASE_NAME
      : terminal<!cr<"0-9.@;: \t\n\r\f\v"> + some(cr<"^;:@ \t\n\r\f\v">)> {};
  struct WORD : terminal<some(cr<"^;:@ \t\n\r\f\v">)> {};
  struct COLON : terminal<lit<":">> {};

  struct Symbol : rule<create<ast::Symbol>(
                          assign<&ast::Symbol::name>(ref<IDENT>) + lit<":"> +
                          assign<&ast::Symbol::revision>(ref<REVISION>))> {};

  struct NewPhrase : rule<create<ast::NewPhrase>(
                             !kw<"desc"> + !kw<"text"> +
                             append<&ast::NewPhrase::words>(ref<PHRASE_NAME>) +
                             many(append<&ast::NewPhrase::words>(
                                 ref<STRING> | ref<WORD> | ref<COLON>)) +
                             lit<";">)> {};

  struct Admin
      : rule<create<ast::Admin>(
            lit<"head"> + option(assign<&ast::Admin::head>(ref<REVISION>)) +
            lit<";"> +
            option(lit<"branch"> +
                   option(assign<&ast::Admin::branch>(ref<REVISION>)) +
                   lit<";">) +
            lit<"access"> + many(append<&ast::Admin::access>(ref<IDENT>)) +
            lit<";"> + lit<"symbols"> +
            many(append<&ast::Admin::symbols>(ref<Symbol>)) + lit<";"> +
            lit<"locks"> + many(append<&ast::Admin::locks>(ref<Symbol>)) + lit<";"> +
            option(flag<&ast::Admin::strict>(lit<"strict"> + lit<";">)) +
            option(lit<"integrity"> +
                   option(assign<&ast::Admin::integrity>(ref<STRING>)) +
                   lit<";">) +
            option(lit<"comment"> +
                   option(assign<&ast::Admin::comment>(ref<STRING>)) +
                   lit<";">) +
            option(lit<"expand"> +
                   option(assign<&ast::Admin::expand>(ref<STRING>)) +
                   lit<";">) +
            many(append<&ast::Admin::newphrases>(ref<NewPhrase>)))> {};

  struct Delta
      : rule<create<ast::Delta>(
            assign<&ast::Delta::revision>(ref<REVISION>) + lit<"date"> +
            assign<&ast::Delta::date>(ref<REVISION>) + lit<";"> +
            lit<"author"> + option(assign<&ast::Delta::author>(ref<IDENT>)) +
            lit<";"> + lit<"state"> +
            option(assign<&ast::Delta::state>(ref<IDENT>)) + lit<";"> +
            lit<"branches"> +
            many(append<&ast::Delta::branches>(ref<REVISION>)) + lit<";"> +
            lit<"next"> + option(assign<&ast::Delta::next>(ref<REVISION>)) +
            lit<";"> +
            option(lit<"commitid"> +
                   assign<&ast::Delta::commitid>(ref<OptionalIdent>) +
                   lit<";">) +
            many(append<&ast::Delta::newphrases>(ref<NewPhrase>)))> {};

  struct DeltaText
      : rule<create<ast::DeltaText>(
            assign<&ast::DeltaText::revision>(ref<REVISION>) + lit<"log"> +
            assign<&ast::DeltaText::log>(ref<STRING>) +
            many(append<&ast::DeltaText::newphrases>(ref<NewPhrase>)) +
            option(flag<&ast::DeltaText::hasText>(
                lit<"text"> + assign<&ast::DeltaText::text>(ref<STRING>))))> {};

  struct File
      : rule<create<ast::File>(
            assign<&ast::File::admin>(ref<Admin>) +
            many(append<&ast::File::deltas>(ref<Delta>)) + lit<"desc"> +
            assign<&ast::File::desc>(ref<STRING>) +
            many(append<&ast::File::deltatexts>(ref<DeltaText>)))> {};

  using entry = File;
  using ignored = WS;
};

} // namespace rcs
