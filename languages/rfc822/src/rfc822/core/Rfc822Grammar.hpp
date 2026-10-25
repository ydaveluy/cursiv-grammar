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
  if (body.find('\\') == std::string_view::npos) return body;
  auto room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\\' && i + 1 < body.size()) ++i;
    room[n++] = body[i];
  }
  return std::string_view(room.data(), n);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Rfc822 : cursiv::GrammarSpec {
  // atext: any byte but the twelve specials that carry their own meaning
  // (they lex as literals below, not as atext), controls and '\r'/'\n'.
  static constexpr auto atext = cr<R"(^()<>@,;:."[]\\\x00-\x20\x7f)">;
  struct ATOM : terminal<some(atext)> {};

  static constexpr auto pair = lit<"\\"> + any;
  struct QUOTED
      : terminal<lit<"\""> + many(pair | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 Unquoted> {};
  struct DOMLIT
      : terminal<lit<"["> + many(pair | cr<R"(^[]\\\r\n)">) + lit<"]">,
                 Unquoted> {};

  static constexpr auto word = ref<ATOM> | ref<QUOTED>;

  struct DomainRefRule : rule<assign<&ast::DomainRef::name>(ref<ATOM>)> {};
  struct DomainLiteralRule
      : rule<assign<&ast::DomainLiteral::text>(ref<DOMLIT>)> {};
  struct SubdomainRule : rule<ref<DomainRefRule> | ref<DomainLiteralRule>> {};

  struct AddrSpecRule
      : rule<some(append<&ast::AddrSpec::localPart>(word), lit<".">) +
             lit<"@"> +
             some(append<&ast::AddrSpec::domain>(ref<SubdomainRule>),
                  lit<".">)> {};

  struct RouteRule
      : rule<lit<"@"> + some(append<&ast::Route::domain>(ref<SubdomainRule>),
                             lit<".">)> {};

  struct RouteAddrRule
      : rule<lit<"<"> +
             option(some(append<&ast::RouteAddr::routes>(ref<RouteRule>),
                         lit<",">) +
                    lit<":">) +
             assign<&ast::RouteAddr::addr>(ref<AddrSpecRule>) + lit<">">> {};

  struct BareMailboxRule
      : rule<assign<&ast::BareMailbox::addr>(ref<AddrSpecRule>)> {};

  struct NamedMailboxRule
      : rule<many(append<&ast::NamedMailbox::phrase>(word)) +
             assign<&ast::NamedMailbox::routeAddr>(ref<RouteAddrRule>)> {};

  struct MailboxRule : rule<ref<BareMailboxRule> | ref<NamedMailboxRule>> {};

  struct GroupRule
      : rule<some(append<&ast::Group::phrase>(word)) + lit<":"> +
             many(append<&ast::Group::mailboxes>(ref<MailboxRule>) +
                  option(lit<",">)) +
             lit<";">> {};

  struct AddressRule : rule<ref<MailboxRule> | ref<GroupRule>> {};

  struct WS : terminal<some(s)> {};
  struct COMMENT
      : terminal<lit<"("> +
                 many(pair | ref<COMMENT> | cr<R"(^()\\)">) +
                 lit<")">> {};
  using ignore = WS;
  using hidden = list<COMMENT>;
  using entry = AddressRule;
};

}  // namespace rfc822
