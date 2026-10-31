// The pf language: its grammar -- OpenBSD's packet-filter configuration
// syntax. A ruleset is a flat sequence of directives; the only nesting is
// an anchor's braces, and those are two independent directives (open,
// close), not a recursive block.
#pragma once

#include <cstdint>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pf/core/ast.hpp"

namespace pf {

inline constexpr auto unquote = [](std::string_view text,
                                   std::pmr::memory_resource* memory) {
  if (text.empty() || (text.front() != '\'' && text.front() != '"')) return text;
  const auto body = text.substr(1, text.size() - 2);
  if (body.find_first_of("\\\n") == body.npos) return body;
  const auto room = cursiv::text_room(memory, body.size());
  std::size_t size = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] == '\n') continue;
    if (body[i] == '\\' && i + 1 < body.size()) {
      if (body[i + 1] == '\n') { ++i; continue; }
      if (body[i + 1] == text.front()) ++i;
    }
    room[size++] = body[i];
  }
  return std::string_view(room.data(), size);
};

struct Pf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct CONTINUATION : terminal<lit<"\\">> {};

  struct NUMBER : terminal<some(d)> {};
  struct HEX : terminal<lit<"0"> + cr<"Xx"> + some(cr<"0-9a-fA-F">)> {};
  struct TOSNUM : terminal<(lit<"0x"> + some(cr<"0-9a-fA-F">)) | some(d)> {};
  struct BANDWIDTH_NUMBER : terminal<some(d) + option(lit<"."> + many(d)) +
      option(cr<"eE"> + option(cr<"+-">) + some(d))> {};

  struct IPV4
      : terminal<some(d) + lit<"."> + some(d) + lit<"."> + some(d) +
                 lit<"."> + some(d)> {};
  // An ordinary IPv6 literal needs multi-digit groups and `::` compression
  // (`2607:f700:1234:2::15`, example11): hex runs of 1-4 digits joined by
  // colons, at least one colon required to stay clear of NUMBER and HEX.
  struct IPV6
      : terminal<(some(cr<"0-9a-fA-F">) + lit<":"> + many(cr<"0-9a-fA-F:">)) |
                 (lit<"::"> + many(cr<"0-9a-fA-F:.">))> {};

  static constexpr auto NameCont = cr<"a-zA-Z0-9_.-">;
  static constexpr auto word_boundary = NameCont;
  struct BWUNIT : terminal<((cr<"KMG"> +
                            option(lit<"Bit"> | lit<"bit"> | cr<"Bb">)) |
                           lit<"b">) + !NameCont> {};
  static constexpr auto quoted =
      (lit<"\""> + many((lit<"\\"> + lit<"\"">) | cr<R"(^")">) + lit<"\"">) |
      (lit<"'"> + many((lit<"\\"> + lit<"'">) | cr<"^'">) + lit<"'">);
  struct STRING : terminal<as<unquote>(
      (option(lit<"$">) + cr<"a-zA-Z_"> + many(NameCont)) | quoted)> {};
  struct QUOTED_STRING : terminal<as<unquote>(quoted)> {};

  // --- ifspec: one interface, or a `{ ... }` list; an antispoof target
  // and an onspec both take this shape. ---
  struct IfaceRef
      : rule<option(flag<&ast::IfaceRef::negated>(lit<"!">)) +
             assign<&ast::IfaceRef::name>(ref<STRING>)> {};

  static constexpr auto ifaceItem = ref<IfaceRef>;

  struct IfSpec
      : rule<append<&ast::IfSpec::ifaces>(ifaceItem) |
             (lit<"{"> + append<&ast::IfSpec::ifaces>(ifaceItem) +
              many(option(lit<",">) + append<&ast::IfSpec::ifaces>(ifaceItem)) +
              lit<"}">)> {};

  // --- timeout / limit_item: a name plus a number, shared by `set` and a
  // state option. ---
  struct Timeout
      : rule<assign<&ast::Timeout::name>(
                 kw<"tcp.first"> | kw<"tcp.opening"> | kw<"tcp.established"> |
                 kw<"tcp.closing"> | kw<"tcp.finwait"> | kw<"tcp.closed"> |
                 kw<"udp.first"> | kw<"udp.single"> | kw<"udp.multiple"> |
                 kw<"icmp.first"> | kw<"icmp.error"> | kw<"other.first"> |
                 kw<"other.single"> | kw<"other.multiple"> | kw<"frag"> |
                 kw<"interval"> | kw<"src.track"> | kw<"adaptive.start"> |
                 kw<"adaptive.end">) +
             assign<&ast::Timeout::value>(ref<NUMBER>)> {};

  struct LimitItem
      : rule<assign<&ast::LimitItem::name>(kw<"states"> | kw<"frags"> |
                                           kw<"src-nodes"> | kw<"tables"> |
                                           kw<"table-entries">) +
             assign<&ast::LimitItem::value>(ref<NUMBER>)> {};

  // --- state_opt / state_opts ---
  struct StateOpt
      : rule<
            (assign<&ast::StateOpt::kind>(kw<"max">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>)) |
            assign<&ast::StateOpt::kind>(kw<"no-sync">) |
            (create<ast::StateOpt> +
             assign<&ast::StateOpt::timeout>(ref<Timeout>)) |
            assign<&ast::StateOpt::kind>(kw<"sloppy">) |
            assign<&ast::StateOpt::kind>(kw<"pflow">) |
            (assign<&ast::StateOpt::kind>(kw<"source-track">) +
             option(assign<&ast::StateOpt::text>(kw<"rule"> | kw<"global">))) |
            (assign<&ast::StateOpt::kind>(kw<"max-src-nodes">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>)) |
            (assign<&ast::StateOpt::kind>(kw<"max-src-states">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>)) |
            (assign<&ast::StateOpt::kind>(kw<"max-src-conn-rate">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>) + lit<"/"> +
             assign<&ast::StateOpt::num2>(ref<NUMBER>)) |
            (assign<&ast::StateOpt::kind>(kw<"max-src-conn">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>)) |
            (assign<&ast::StateOpt::kind>(kw<"overload">) + lit<"<"> +
             assign<&ast::StateOpt::text>(ref<STRING>) + lit<">"> +
             option(flag<&ast::StateOpt::flush>(kw<"flush">) +
                    option(flag<&ast::StateOpt::global>(kw<"global">)))) |
            assign<&ast::StateOpt::kind>(kw<"if-bound">) |
            assign<&ast::StateOpt::kind>(kw<"floating">)> {};

  static constexpr auto stateOptsList =
      append<&ast::FilterOpt::stateOpts>(ref<StateOpt>) +
      many(option(lit<",">) +
           append<&ast::FilterOpt::stateOpts>(ref<StateOpt>));

  // --- an unbounded run of scrub options, like state options and filter
  // options, not capped at two (Deviation). ---
  struct ScrubOpt
      : rule<assign<&ast::ScrubOpt::kind>(kw<"no-df">) |
             (assign<&ast::ScrubOpt::kind>(kw<"min-ttl">) +
              assign<&ast::ScrubOpt::num>(ref<NUMBER>)) |
             (assign<&ast::ScrubOpt::kind>(kw<"max-mss">) +
              assign<&ast::ScrubOpt::num>(ref<NUMBER>)) |
             (assign<&ast::ScrubOpt::kind>(kw<"reassemble">) + kw<"tcp">) |
             assign<&ast::ScrubOpt::kind>(kw<"random-id">)> {};

  static constexpr auto scrubOptsList =
      append<&ast::FilterOpt::scrubOpts>(ref<ScrubOpt>) +
      many(option(lit<",">) +
           append<&ast::FilterOpt::scrubOpts>(ref<ScrubOpt>));

  // --- host / redirhost / address ---
  struct Host
      : rule<option(flag<&ast::Host::negated>(lit<"!">)) +
             ((flag<&ast::Host::iface>(lit<"(">) +
               assign<&ast::Host::address>(ref<STRING>) + lit<")">) |
              assign<&ast::Host::address>(
                  ref<STRING> | ref<IPV4> | ref<IPV6>) |
              (flag<&ast::Host::table>(lit<"<">) +
               assign<&ast::Host::address>(ref<STRING>) + lit<">">)) +
             option(lit<"/"> + assign<&ast::Host::maskBits>(ref<NUMBER>)) +
             option(kw<"weight"> +
                    assign<&ast::Host::weight>(ref<NUMBER>))> {};

  struct RedirHost
      : rule<((flag<&ast::RedirHost::iface>(lit<"(">) +
               assign<&ast::RedirHost::address>(ref<STRING>) + lit<")">) |
              assign<&ast::RedirHost::address>(
                  ref<STRING> | ref<IPV4> | ref<IPV6>)) +
             option(lit<"/"> +
                    assign<&ast::RedirHost::maskBits>(ref<NUMBER>))> {};

  struct RouteHost
      : rule<((flag<&ast::RedirHost::iface>(lit<"(">) +
               assign<&ast::RedirHost::routeInterface>(ref<STRING>) +
               option(assign<&ast::RedirHost::address>(ref<STRING> | ref<IPV4> | ref<IPV6>) +
                      option(lit<"/"> + assign<&ast::RedirHost::maskBits>(ref<NUMBER>))) + lit<")">) |
              (assign<&ast::RedirHost::address>(ref<STRING> | ref<IPV4> | ref<IPV6>) +
               option(lit<"/"> + assign<&ast::RedirHost::maskBits>(ref<NUMBER>)))) +
             option(kw<"weight"> + assign<&ast::RedirHost::weight>(ref<NUMBER>))> {};
  static constexpr auto routeHostList =
      append<&ast::FilterOpt::targets>(ref<RouteHost>) |
      (lit<"{"> + append<&ast::FilterOpt::targets>(ref<RouteHost>) +
       many(option(lit<",">) + append<&ast::FilterOpt::targets>(ref<RouteHost>)) + lit<"}">);

  static constexpr auto redirHostList =
      append<&ast::FilterOpt::targets>(ref<RedirHost>) |
      (lit<"{"> + append<&ast::FilterOpt::targets>(ref<RedirHost>) +
       many(option(lit<",">) +
            append<&ast::FilterOpt::targets>(ref<RedirHost>)) +
       lit<"}">);

  static constexpr auto redirHostList2 =
      append<&ast::FilterOpt::targets2>(ref<RedirHost>) |
      (lit<"{"> + append<&ast::FilterOpt::targets2>(ref<RedirHost>) +
       many(option(lit<",">) +
            append<&ast::FilterOpt::targets2>(ref<RedirHost>)) +
       lit<"}">);

  // --- unary_op / binary_op / op_list -- port, user, group all read this
  // shape after their own leading keyword. ---
  struct UnaryOp
      : rule<option(assign<&ast::PortItem::op>(
                 lit<"!="> | lit<"<="> | lit<">="> | lit<"="> | lit<"<"> |
                 lit<">">)) +
             assign<&ast::PortItem::value1>(ref<STRING> | ref<NUMBER>)> {};

  struct BinaryOp
      : rule<assign<&ast::PortItem::value1>(ref<NUMBER>) +
             assign<&ast::PortItem::op>(lit<"<>"> | lit<"><"> | lit<":">) +
             assign<&ast::PortItem::value2>(ref<NUMBER>)> {};

  static constexpr auto opListItem = ref<BinaryOp> | ref<UnaryOp>;

  struct OpSpec
      : rule<append<&ast::OpSpec::items>(opListItem) |
             (lit<"{"> + append<&ast::OpSpec::items>(opListItem) +
              many(option(lit<",">) + append<&ast::OpSpec::items>(opListItem)) +
              lit<"}">)> {};

  // --- os / os_list ---
  struct OsSpec
      : rule<kw<"os"> +
             (append<&ast::OsSpec::names>(ref<STRING>) |
              (lit<"{"> + append<&ast::OsSpec::names>(ref<STRING>) +
               many(option(lit<",">) + append<&ast::OsSpec::names>(ref<STRING>)) +
               lit<"}">))> {};

  // --- portspec: `port` NUMBER-or-name (`:` `*`-or-NUMBER-or-name)? ---
  struct PortSpec
      : rule<kw<"port"> +
             assign<&ast::PortSpec::value1>(ref<STRING> | ref<NUMBER>) +
             option(lit<":"> + assign<&ast::PortSpec::value2>(
                                   lit<"*"> | ref<STRING> | ref<NUMBER>))> {
  };

  // --- flags ---
  struct FLAGCHARS : terminal<some(cr<"FSRPAUEW">)> {};

  // --- icmp_type_code / icmp_list ---
  struct IcmpTypeCode
      : rule<assign<&ast::IcmpTypeCode::type>(ref<STRING> | ref<NUMBER>) +
             option(kw<"code"> +
                    assign<&ast::IcmpTypeCode::code>(ref<STRING> | ref<NUMBER>))> {
  };

  static constexpr auto icmpList =
      append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCode>) |
      (lit<"{"> + append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCode>) +
       many(option(lit<",">) + append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCode>)) +
       lit<"}">);

  // --- pooltype ---
  static constexpr auto pooltypeClause =
      (assign<&ast::FilterOpt::text1>(kw<"bitmask">)) |
      (assign<&ast::FilterOpt::text1>(kw<"least-states">)) |
      (assign<&ast::FilterOpt::text1>(kw<"random">)) |
      (assign<&ast::FilterOpt::text1>(kw<"round-robin">)) |
      (assign<&ast::FilterOpt::text1>(kw<"source-hash">) +
       option(assign<&ast::FilterOpt::text2>(ref<HEX> | ref<QUOTED_STRING>)));

  static constexpr auto stickyAddressOpt =
      option(flag<&ast::FilterOpt::stickyAddress>(kw<"sticky-address">));

  // --- the long tail of filter options: ~25 keyword-led clauses. One
  // class, `kind` names the clause. `divert-packet`/`divert-to` read a
  // single `portspec` clause instead of the word `port` twice in a row
  // (Deviation). ---
  struct FilterOpt
      : rule<
            (assign<&ast::FilterOpt::kind>(kw<"user">) +
             assign<&ast::FilterOpt::opspec>(ref<OpSpec>)) |
            (assign<&ast::FilterOpt::kind>(kw<"group">) +
             assign<&ast::FilterOpt::opspec>(ref<OpSpec>)) |
            (assign<&ast::FilterOpt::kind>(kw<"flags">) +
             (flag<&ast::FilterOpt::flagsAny>(kw<"any">) |
              (option(assign<&ast::FilterOpt::flags1>(ref<FLAGCHARS>)) +
               flag<&ast::FilterOpt::hasFlagMask>(lit<"/">) +
               assign<&ast::FilterOpt::flags2>(ref<FLAGCHARS>)) |
              assign<&ast::FilterOpt::flags1>(ref<FLAGCHARS>))) |
            (assign<&ast::FilterOpt::kind>(kw<"icmp-type">) + icmpList) |
            (assign<&ast::FilterOpt::kind>(kw<"icmp6-type">) + icmpList) |
            (assign<&ast::FilterOpt::kind>(kw<"tos">) +
             assign<&ast::FilterOpt::tos>(
                 kw<"lowdelay"> | kw<"throughput"> | kw<"reliability"> |
                 ref<TOSNUM>)) |
            (assign<&ast::FilterOpt::kind>(kw<"no"> | kw<"keep"> |
                                          kw<"modulate"> | kw<"synproxy">) +
             kw<"state"> +
             option(lit<"("> + stateOptsList + lit<")">)) |
            (assign<&ast::FilterOpt::kind>(kw<"scrub">) + lit<"("> +
             scrubOptsList + lit<")">) |
            assign<&ast::FilterOpt::kind>(kw<"fragment">) |
            assign<&ast::FilterOpt::kind>(kw<"allow-opts">) |
            assign<&ast::FilterOpt::kind>(kw<"once">) |
            (assign<&ast::FilterOpt::kind>(kw<"divert-packet">) +
             assign<&ast::FilterOpt::portspec>(ref<PortSpec>)) |
            assign<&ast::FilterOpt::kind>(kw<"divert-reply">) |
            (assign<&ast::FilterOpt::kind>(kw<"divert-to">) +
             assign<&ast::FilterOpt::target>(ref<Host>) +
             assign<&ast::FilterOpt::portspec>(ref<PortSpec>)) |
            (assign<&ast::FilterOpt::kind>(kw<"label">) +
             assign<&ast::FilterOpt::text1>(ref<QUOTED_STRING>)) |
            (assign<&ast::FilterOpt::kind>(kw<"tag">) +
             assign<&ast::FilterOpt::text1>(ref<STRING>)) |
            (option(flag<&ast::FilterOpt::negated>(lit<"!">)) +
             assign<&ast::FilterOpt::kind>(kw<"tagged">) +
             assign<&ast::FilterOpt::text1>(ref<STRING>)) |
            (assign<&ast::FilterOpt::kind>(kw<"max-pkt-rate">) +
             assign<&ast::FilterOpt::num1>(ref<NUMBER>) + lit<"/"> +
             assign<&ast::FilterOpt::num2>(ref<NUMBER>)) |
            (flag<&ast::FilterOpt::viaSet>(kw<"set">) +
             assign<&ast::FilterOpt::kind>(kw<"delay">) +
             assign<&ast::FilterOpt::num1>(ref<NUMBER>)) |
            (flag<&ast::FilterOpt::viaSet>(kw<"set">) +
             assign<&ast::FilterOpt::kind>(kw<"prio">) +
             (assign<&ast::FilterOpt::num1>(ref<NUMBER>) |
              (lit<"("> + assign<&ast::FilterOpt::num1>(ref<NUMBER>) +
               option(option(lit<",">) +
                      assign<&ast::FilterOpt::num2>(ref<NUMBER>)) +
               lit<")">))) |
            (flag<&ast::FilterOpt::viaSet>(kw<"set">) +
             assign<&ast::FilterOpt::kind>(kw<"queue">) +
             (append<&ast::FilterOpt::words>(ref<STRING>) |
              (lit<"("> + append<&ast::FilterOpt::words>(ref<STRING>) +
               option(option(lit<",">) +
                      append<&ast::FilterOpt::words>(ref<STRING>)) +
               lit<")">))) |
            (assign<&ast::FilterOpt::kind>(kw<"rtable">) +
             assign<&ast::FilterOpt::num1>(ref<NUMBER>)) |
            (assign<&ast::FilterOpt::kind>(kw<"probability">) +
             assign<&ast::FilterOpt::num1>(ref<NUMBER>) + lit<"%">) |
            (assign<&ast::FilterOpt::kind>(kw<"prio">) +
             assign<&ast::FilterOpt::num1>(ref<NUMBER>)) |
            (assign<&ast::FilterOpt::kind>(kw<"af-to">) +
             assign<&ast::FilterOpt::text1>(kw<"inet"> | kw<"inet6">) +
             kw<"from"> + redirHostList +
             option(kw<"to"> + redirHostList2)) |
            (assign<&ast::FilterOpt::kind>(kw<"binat-to">) + redirHostList +
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpec>)) + option(pooltypeClause) +
             stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"rdr-to">) + redirHostList +
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpec>)) + option(pooltypeClause) +
             stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"nat-to">) + redirHostList +
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpec>)) + option(pooltypeClause) +
             stickyAddressOpt +
             option(flag<&ast::FilterOpt::staticPort>(
                 kw<"static-port">))) |
            (assign<&ast::FilterOpt::kind>(kw<"route-to">) + routeHostList +
             option(pooltypeClause) + stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"reply-to">) + routeHostList +
             option(pooltypeClause) + stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"dup-to">) + routeHostList +
             option(pooltypeClause) + stickyAddressOpt) |
            (flag<&ast::FilterOpt::viaSet>(kw<"set">) +
             assign<&ast::FilterOpt::kind>(kw<"tos">) +
             assign<&ast::FilterOpt::tos>(
                 kw<"lowdelay"> | kw<"throughput"> | kw<"reliability"> |
                 ref<TOSNUM>)) |
            (option(flag<&ast::FilterOpt::negated>(lit<"!">)) +
             assign<&ast::FilterOpt::kind>(kw<"received-on">) +
             assign<&ast::FilterOpt::text1>(ref<STRING>))> {};

  static constexpr auto filterOptsList =
      append<&ast::Rule::filterOpts>(ref<FilterOpt>) +
      many(option(lit<",">) + append<&ast::Rule::filterOpts>(ref<FilterOpt>));

  static constexpr auto anchorFilterOptsList =
      append<&ast::AnchorRule::filterOpts>(ref<FilterOpt>) +
      many(option(lit<",">) +
           append<&ast::AnchorRule::filterOpts>(ref<FilterOpt>));

  // --- protospec / proto_list ---
  struct Protospec
      : rule<kw<"proto"> +
             (append<&ast::Protospec::protos>(ref<STRING> | ref<NUMBER>) |
              (lit<"{"> +
               append<&ast::Protospec::protos>(ref<STRING> | ref<NUMBER>) +
               many(option(lit<",">) + append<&ast::Protospec::protos>(
                                            ref<STRING> | ref<NUMBER>)) +
               lit<"}">))> {};

  // --- af ---
  static constexpr auto afWord = kw<"inet"> | kw<"inet6">;

  // --- hosts: fromspec? tospec?, or `all` ---
  static constexpr auto fromspecOs =
      option(assign<&ast::FromSpec::os>(ref<OsSpec>));

  struct FromSpec
      : rule<kw<"from"> +
             ((assign<&ast::FromSpec::kind>(kw<"any"> | kw<"no-route"> |
                                            kw<"urpf-failed"> | kw<"self">)) |
              append<&ast::FromSpec::hosts>(ref<Host>) |
              (lit<"{"> + append<&ast::FromSpec::hosts>(ref<Host>) +
               many(option(lit<",">) +
                    append<&ast::FromSpec::hosts>(ref<Host>)) +
               lit<"}">) |
              (assign<&ast::FromSpec::kind>(kw<"route">) +
               assign<&ast::FromSpec::routeName>(
                   ref<STRING> | ref<QUOTED_STRING>))) +
             option(kw<"port"> +
                    assign<&ast::FromSpec::port>(ref<OpSpec>)) +
             fromspecOs> {};

  struct ToSpec
      : rule<kw<"to"> +
             ((assign<&ast::ToSpec::kind>(kw<"any"> | kw<"no-route"> |
                                          kw<"self">)) |
              append<&ast::ToSpec::hosts>(ref<Host>) |
              (lit<"{"> + append<&ast::ToSpec::hosts>(ref<Host>) +
               many(option(lit<",">) +
                    append<&ast::ToSpec::hosts>(ref<Host>)) +
               lit<"}">) |
              (assign<&ast::ToSpec::kind>(kw<"route">) +
               assign<&ast::ToSpec::routeName>(
                   ref<STRING> | ref<QUOTED_STRING>))) +
             option(kw<"port"> +
                    assign<&ast::ToSpec::port>(ref<OpSpec>))> {};

  struct Hosts
      : rule<flag<&ast::Hosts::all>(kw<"all">) |
             (option(assign<&ast::Hosts::from>(ref<FromSpec>)) +
              option(assign<&ast::Hosts::to>(ref<ToSpec>)))> {};

  // --- action / return ---
  static constexpr auto returnClause =
      assign<&ast::Rule::blockReturn>(kw<"drop">) |
      assign<&ast::Rule::blockReturn>(kw<"return">) |
      (assign<&ast::Rule::blockReturn>(kw<"return-rst">) +
       option(lit<"("> + kw<"ttl"> +
              assign<&ast::Rule::returnTtl>(ref<NUMBER>) + lit<")">)) |
      (assign<&ast::Rule::blockReturn>(kw<"return-icmp">) +
       option(lit<"("> +
              assign<&ast::Rule::returnIcmpCode1>(ref<STRING> | ref<NUMBER>) +
              option(option(lit<",">) +
                     assign<&ast::Rule::returnIcmpCode2>(ref<STRING> |
                                                         ref<NUMBER>)) +
              lit<")">)) |
      (assign<&ast::Rule::blockReturn>(kw<"return-icmp6">) +
       option(lit<"("> +
              assign<&ast::Rule::returnIcmpCode1>(ref<STRING> | ref<NUMBER>) +
              lit<")">));

  static constexpr auto actionExpr =
      assign<&ast::Rule::action>(kw<"pass">) |
      assign<&ast::Rule::action>(kw<"match">) |
      (assign<&ast::Rule::action>(kw<"block">) + option(returnClause)) |
      assign<&ast::Rule::action>(kw<"scrub">);

  // --- logging / logopts: no trailing NUMBER after the list -- real
  // pf.conf logging options (`log(all)`) carry none (Deviation). ---
  struct LogOpt
      : rule<assign<&ast::LogOpt::kind>(kw<"all"> | kw<"matches"> |
                                        kw<"user">) |
             (assign<&ast::LogOpt::kind>(kw<"to">) +
              assign<&ast::LogOpt::iface>(ref<STRING>))> {};

  static constexpr auto loggingClause =
      flag<&ast::Rule::log>(kw<"log">) +
      option(lit<"("> +
             append<&ast::Rule::logOpts>(ref<LogOpt>) +
             many(option(lit<",">) +
                  append<&ast::Rule::logOpts>(ref<LogOpt>)) +
             lit<")">);

  // --- onspec ---
  static constexpr auto onspecClause =
      kw<"on"> +
      (assign<&ast::Rule::onIf>(ref<IfSpec>) |
       (kw<"rdomain"> + assign<&ast::Rule::onRdomain>(ref<NUMBER>)));

  // --- pf_rule ---
  struct Rule
      : rule<actionExpr +
             option(assign<&ast::Rule::direction>(kw<"in"> | kw<"out">)) +
             many(loggingClause |
                  flag<&ast::Rule::quick>(kw<"quick">)) +
             option(onspecClause) +
             option(assign<&ast::Rule::af>(afWord)) +
             option(assign<&ast::Rule::proto>(ref<Protospec>)) +
             assign<&ast::Rule::hosts>(ref<Hosts>) +
             option(filterOptsList)> {};

  // --- antispoof_rule ---
  struct AntispoofRule
      : rule<kw<"antispoof"> +
             option(flag<&ast::AntispoofRule::log>(kw<"log">)) +
             option(flag<&ast::AntispoofRule::quick>(kw<"quick">)) +
             kw<"for"> +
             assign<&ast::AntispoofRule::ifspec>(ref<IfSpec>) +
             option(assign<&ast::AntispoofRule::af>(afWord)) +
             option(kw<"label"> +
                    assign<&ast::AntispoofRule::label>(ref<STRING>))> {};

  // --- table_rule ---
  // `tableaddr`'s bare `ifspec` reads as a single name identically to
  // `hostname` (both a plain STRING), so only the `{ ... }` list form -- the
  // one shape a bare name cannot also spell -- is read as an ifspec here;
  // a single interface still reads as the generic name below it.
  struct IfSpecBraced
      : rule<lit<"{"> + append<&ast::IfSpec::ifaces>(ifaceItem) +
             many(option(lit<",">) + append<&ast::IfSpec::ifaces>(ifaceItem)) +
             lit<"}">> {};

  struct TableAddr
      : rule<option(flag<&ast::TableAddr::negated>(lit<"!">)) +
             (assign<&ast::TableAddr::ifspec>(ref<IfSpecBraced>) |
              assign<&ast::TableAddr::text>(kw<"self">) |
              assign<&ast::TableAddr::text>(ref<IPV4>) |
              assign<&ast::TableAddr::text>(ref<IPV6>) |
              assign<&ast::TableAddr::text>(ref<STRING>)) +
             option(lit<"/"> +
                    assign<&ast::TableAddr::maskBits>(ref<NUMBER>))> {};

  static constexpr auto tableAddrsList =
      append<&ast::TableOpt::addrs>(ref<TableAddr>) +
      many(option(lit<",">) + append<&ast::TableOpt::addrs>(ref<TableAddr>));

  struct TableOpt
      : rule<flag<&ast::TableOpt::persist>(kw<"persist">) |
             flag<&ast::TableOpt::constFlag>(kw<"const">) |
             flag<&ast::TableOpt::counters>(kw<"counters">) |
             (kw<"file"> + assign<&ast::TableOpt::file>(ref<QUOTED_STRING>)) |
             (lit<"{"> + option(tableAddrsList) + lit<"}">)> {};

  static constexpr auto tableOptsList =
      append<&ast::TableRule::opts>(ref<TableOpt>) +
      many(append<&ast::TableRule::opts>(ref<TableOpt>));

  struct TableRule
      : rule<kw<"table"> + lit<"<"> +
             assign<&ast::TableRule::name>(ref<STRING>) + lit<">"> +
             option(tableOptsList)> {};

  // --- queue_rule ---
  template <auto Value, auto Unit>
  static constexpr auto bandwidthSpec =
      (assign<Value>(ref<BANDWIDTH_NUMBER>) + gap::none +
       assign<Unit>(ref<BWUNIT>)) | assign<Value>(ref<NUMBER>);

  struct Bandwidth
      : rule<bandwidthSpec<&ast::Bandwidth::value, &ast::Bandwidth::unit> +
             option(kw<"burst"> +
                    bandwidthSpec<&ast::Bandwidth::burstValue,
                                  &ast::Bandwidth::burstUnit> +
                    kw<"for"> +
                    assign<&ast::Bandwidth::burstMs>(ref<NUMBER>) +
                    gap::none + kw<"ms">)> {};

  struct QueueOpt
      : rule<(assign<&ast::QueueOpt::kind>(kw<"bandwidth">) +
              assign<&ast::QueueOpt::bw>(ref<Bandwidth>)) |
             (assign<&ast::QueueOpt::kind>(kw<"min">) +
              assign<&ast::QueueOpt::bw>(ref<Bandwidth>)) |
             (assign<&ast::QueueOpt::kind>(kw<"max">) +
              assign<&ast::QueueOpt::bw>(ref<Bandwidth>)) |
             (assign<&ast::QueueOpt::kind>(kw<"parent">) +
              assign<&ast::QueueOpt::text>(ref<STRING>)) |
             assign<&ast::QueueOpt::kind>(kw<"default">) |
             (assign<&ast::QueueOpt::kind>(kw<"flows">) +
              assign<&ast::QueueOpt::num>(ref<NUMBER>)) |
             (assign<&ast::QueueOpt::kind>(kw<"quantum">) +
              assign<&ast::QueueOpt::num>(ref<NUMBER>)) |
             (assign<&ast::QueueOpt::kind>(kw<"qlimit">) +
              assign<&ast::QueueOpt::num>(ref<NUMBER>))> {};

  static constexpr auto queueOptsList =
      append<&ast::QueueRule::opts>(ref<QueueOpt>) +
      many(option(lit<",">) + append<&ast::QueueRule::opts>(ref<QueueOpt>));

  struct QueueRule
      : rule<kw<"queue"> + assign<&ast::QueueRule::name>(ref<STRING>) +
             option(kw<"on"> +
                    assign<&ast::QueueRule::iface>(ref<STRING>)) +
             queueOptsList> {};

  // --- anchor_rule / anchor_close / load_anchor ---
  struct AnchorRule
      : rule<kw<"anchor"> +
             option(assign<&ast::AnchorRule::name>(
                 ref<STRING> | ref<QUOTED_STRING>)) +
             option(assign<&ast::AnchorRule::direction>(kw<"in"> | kw<"out">)) +
             option(flag<&ast::AnchorRule::quick>(kw<"quick">)) +
             option(kw<"on"> +
                    assign<&ast::AnchorRule::onIf>(ref<IfSpec>)) +
             option(assign<&ast::AnchorRule::af>(afWord)) +
             option(assign<&ast::AnchorRule::proto>(ref<Protospec>)) +
             assign<&ast::AnchorRule::hosts>(ref<Hosts>) +
             option(anchorFilterOptsList) +
             option(flag<&ast::AnchorRule::opensBlock>(lit<"{">))> {};

  struct AnchorClose : rule<create<ast::AnchorClose> + lit<"}">> {};

  struct LoadAnchor
      : rule<kw<"load"> + kw<"anchor"> +
             assign<&ast::LoadAnchor::name>(ref<STRING>) + kw<"from"> +
             assign<&ast::LoadAnchor::file>(ref<QUOTED_STRING>)> {};

  // --- include / var ---
  struct Include
      : rule<kw<"include"> +
             assign<&ast::Include::file>(ref<QUOTED_STRING>)> {};

  struct Var
      : rule<assign<&ast::Var::name>(ref<STRING>) + lit<"="> +
             assign<&ast::Var::value>(ref<QUOTED_STRING>)> {};

  // --- option: the `set` statement. Real pf.conf takes exactly one clause
  // per `set`, which is what every corpus file writes; a combination like
  // `optimization ... limit ...` in one statement is not produced
  // (Deviation).
  struct Option
      : rule<kw<"set"> +
             option(
                 (assign<&ast::Option::kind>(kw<"timeout">) +
                  (append<&ast::Option::timeouts>(ref<Timeout>) |
                   (lit<"{"> +
                    append<&ast::Option::timeouts>(ref<Timeout>) +
                    many(option(lit<",">) +
                         append<&ast::Option::timeouts>(ref<Timeout>)) +
                    lit<"}">))) |
                 (assign<&ast::Option::kind>(kw<"ruleset-optimization">) +
                  option(assign<&ast::Option::word>(
                      kw<"none"> | kw<"basic"> | kw<"profile">))) |
                 (assign<&ast::Option::kind>(kw<"optimization">) +
                  option(assign<&ast::Option::word>(
                      kw<"default"> | kw<"normal"> | kw<"high-latency"> |
                      kw<"satellite"> | kw<"aggressive"> |
                      kw<"conservative">))) |
                 (assign<&ast::Option::kind>(kw<"limit">) +
                  (append<&ast::Option::limits>(ref<LimitItem>) |
                   (lit<"{"> +
                    append<&ast::Option::limits>(ref<LimitItem>) +
                    many(option(lit<",">) +
                         append<&ast::Option::limits>(ref<LimitItem>)) +
                    lit<"}">))) |
                 (assign<&ast::Option::kind>(kw<"loginterface">) +
                  assign<&ast::Option::word>(ref<STRING> | kw<"none">)) |
                 (assign<&ast::Option::kind>(kw<"block-policy">) +
                  assign<&ast::Option::word>(kw<"drop"> | kw<"return">)) |
                 (assign<&ast::Option::kind>(kw<"state-policy">) +
                  assign<&ast::Option::word>(kw<"if-bound"> |
                                             kw<"floating">)) |
                 (assign<&ast::Option::kind>(kw<"state-defaults">) +
                  append<&ast::Option::stateOpts>(ref<StateOpt>) +
                  many(option(lit<",">) +
                       append<&ast::Option::stateOpts>(ref<StateOpt>))) |
                 (assign<&ast::Option::kind>(kw<"fingerprints">) +
                  assign<&ast::Option::file>(ref<QUOTED_STRING>)) |
                 (assign<&ast::Option::kind>(kw<"skip">) + kw<"on"> +
                  assign<&ast::Option::ifspec>(ref<IfSpec>)) |
                 (assign<&ast::Option::kind>(kw<"debug">) +
                  assign<&ast::Option::word>(
                      kw<"emerg"> | kw<"alert"> | kw<"crit"> | kw<"err"> |
                      kw<"warning"> | kw<"notice"> | kw<"info"> |
                      kw<"debug"> | kw<"urgent">)) |
                 (assign<&ast::Option::kind>(kw<"reassemble">) +
                  assign<&ast::Option::word>(kw<"yes"> | kw<"no">) +
                  option(flag<&ast::Option::noDf>(kw<"no-df">))))> {};

  // --- line / file_ ---
  static constexpr auto lineItem =
      ref<Option> | ref<Rule> | ref<AntispoofRule> |
      ref<QueueRule> | ref<AnchorRule> | ref<AnchorClose> |
      ref<LoadAnchor> | ref<TableRule> | ref<Include> |
      ref<Var>;

  struct File
      : rule<create<ast::File> + many(append<&ast::File::lines>(lineItem))> {
  };

  using entry = File;
  using ignored = list<WS, COMMENT, CONTINUATION>;
};

}  // namespace pf
