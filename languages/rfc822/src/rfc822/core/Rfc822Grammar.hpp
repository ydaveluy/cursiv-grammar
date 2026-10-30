// The rfc822 language: its grammar.
#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "rfc822/core/ast.hpp"

namespace rfc822 {

inline constexpr auto unquote = [](std::string_view t,
                                   std::pmr::memory_resource* mem) {
  const auto body = t.substr(1, t.size() - 2);
  if (body.find_first_of("\\\r") == std::string_view::npos) return body;
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\r' && i + 1 < body.size() && body[i + 1] == '\n') {
      ++i;  // Unfold unquoted CRLF, retaining its following whitespace.
      continue;
    }
    if (body[i] == '\\' && i + 1 < body.size()) {
      ++i;
      if (body[i] == '\r' && i + 2 < body.size() && body[i + 1] == '\n' &&
          (body[i + 2] == ' ' || body[i + 2] == '\t')) {
        room[n++] = '\r';
        room[n++] = '\n';
        i += 2;  // Quoted CRLF is data; discard its first folding whitespace.
        continue;
      }
    }
    room[n++] = body[i];
  }
  return std::string_view(room.data(), n);
};

struct Rfc822 : cursiv::GrammarSpec {
  // atext: any byte but the twelve specials that carry their own meaning
  // (they lex as literals below, not as atext), controls and '\r'/'\n'.
  static constexpr auto atext = cr<R"(^()<>@,;:."[]\\\x00-\x20\x7f)">;
  struct ATOM : terminal<some(atext)> {};

  static constexpr auto fold = lit<"\r\n"> + &cr<" \t">;
  static constexpr auto pair = lit<"\\"> + (fold | any);
  struct QUOTED : terminal<as<unquote>(
                      lit<"\""> + many(pair | fold | cr<R"(^"\\\r\n)">) + lit<"\"">)> {
  };
  struct DOMLIT : terminal<as<unquote>(
                      lit<"["> + many(pair | fold | cr<R"(^[]\\\r\n)">) + lit<"]">)> {
  };

  static constexpr auto word = ref<ATOM> | ref<QUOTED>;

  struct DomainRef : rule<assign<&ast::DomainRef::name>(ref<ATOM>)> {};
  struct DomainLiteral
      : rule<assign<&ast::DomainLiteral::text>(ref<DOMLIT>)> {};
  struct Subdomain : rule<ref<DomainRef> | ref<DomainLiteral>> {};

  struct AddrSpec
      : rule<some(append<&ast::AddrSpec::localPart>(word), lit<".">) +
             lit<"@"> +
             some(append<&ast::AddrSpec::domain>(ref<Subdomain>),
                  lit<".">)> {};

  struct Route
      : rule<lit<"@"> + some(append<&ast::Route::domain>(ref<Subdomain>),
                             lit<".">)> {};

  struct RouteAddr
      : rule<lit<"<"> +
             option(many(lit<",">) +
                    some(append<&ast::RouteAddr::routes>(ref<Route>),
                         some(lit<",">)) + many(lit<",">) +
                    lit<":">) +
             assign<&ast::RouteAddr::addr>(ref<AddrSpec>) + lit<">">> {};

  struct BareMailbox
      : rule<assign<&ast::BareMailbox::addr>(ref<AddrSpec>)> {};

  struct NamedMailbox
      : rule<many(append<&ast::NamedMailbox::phrase>(word)) +
             assign<&ast::NamedMailbox::routeAddr>(ref<RouteAddr>)> {};

  struct Mailbox : rule<ref<BareMailbox> | ref<NamedMailbox>> {};

  struct Group
      : rule<some(append<&ast::Group::phrase>(word)) + lit<":"> +
             // RFC 822 #lists permit null elements, including at the ends.
             many(append<&ast::Group::mailboxes>(ref<Mailbox>) | lit<",">) +
             lit<";">> {};

  struct Address : rule<ref<Mailbox> | ref<Group>> {};

  struct WS : terminal<some(s)> {};
  struct COMMENT
      : terminal<lit<"("> +
                 many(pair | ref<COMMENT> | cr<R"(^()\\)">) +
                 lit<")">> {};
  using ignored = WS;
  using hidden = list<COMMENT>;
  using entry = Address;
};

}  // namespace rfc822
