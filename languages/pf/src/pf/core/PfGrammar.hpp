// The pf language: its grammar -- OpenBSD's packet-filter configuration
// syntax. A ruleset is a flat sequence of directives; the only nesting is
// an anchor's braces, and those are two independent directives (open,
// close), not a recursive block.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pf/core/ast.hpp"

namespace pf {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Pf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">)> {};
  struct CONTINUATION : terminal<lit<"\\">> {};

  struct NUMBER : terminal<some(d)> {};
  struct HEX : terminal<lit<"0"> + cr<"Xx"> + some(cr<"0-9a-fA-F">)> {};
  struct BWUNIT : terminal<cr<"KMG">> {};
  struct TOSNUM : terminal<option(lit<"0x">) + some(d)> {};

  struct IPV4
      : terminal<some(d) + lit<"."> + some(d) + lit<"."> + some(d) +
                 lit<"."> + some(d)> {};
  // An ordinary IPv6 literal needs multi-digit groups and `::` compression
  // (`2607:f700:1234:2::15`, example11): hex runs of 1-4 digits joined by
  // colons, at least one colon required to stay clear of NUMBER and HEX.
  struct IPV6
      : terminal<(some(cr<"0-9a-fA-F">) + lit<":"> + many(cr<"0-9a-fA-F:">)) |
                 (lit<"::"> + many(cr<"0-9a-fA-F:.">))> {};

  struct STRING : terminal<option(lit<"$">) + cr<"a-zA-Z"> + many(w)> {};
  struct QUOTED_STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  // --- ifspec: one interface, or a `{ ... }` list; an antispoof target
  // and an onspec both take this shape. ---
  struct IfaceRefRule
      : rule<option(flag<&ast::IfaceRef::negated>(lit<"!">)) +
             assign<&ast::IfaceRef::name>(ref<STRING>)> {};

  static constexpr auto ifaceItem = ref<IfaceRefRule>;

  struct IfSpecRule
      : rule<append<&ast::IfSpec::ifaces>(ifaceItem) |
             (lit<"{"> + append<&ast::IfSpec::ifaces>(ifaceItem) +
              many(option(lit<",">) + append<&ast::IfSpec::ifaces>(ifaceItem)) +
              lit<"}">)> {};

  // --- timeout / limit_item: a name plus a number, shared by `set` and a
  // state option. ---
  struct TimeoutRule
      : rule<assign<&ast::Timeout::name>(
                 kw<"tcp.first"> | kw<"tcp.opening"> | kw<"tcp.established"> |
                 kw<"tcp.closing"> | kw<"tcp.finwait"> | kw<"tcp.closed"> |
                 kw<"udp.first"> | kw<"udp.single"> | kw<"udp.multiple"> |
                 kw<"icmp.first"> | kw<"icmp.error"> | kw<"other.first"> |
                 kw<"other.single"> | kw<"other.multiple"> | kw<"frag"> |
                 kw<"interval"> | kw<"src.track"> | kw<"adaptive.start"> |
                 kw<"adaptive.end">) +
             assign<&ast::Timeout::value>(ref<NUMBER>)> {};

  struct LimitItemRule
      : rule<assign<&ast::LimitItem::name>(kw<"states"> | kw<"frags"> |
                                           kw<"src-nodes"> | kw<"tables"> |
                                           kw<"table-entries">) +
             assign<&ast::LimitItem::value>(ref<NUMBER>)> {};

  // --- state_opt / state_opts ---
  struct StateOptRule
      : rule<
            (assign<&ast::StateOpt::kind>(kw<"max">) +
             assign<&ast::StateOpt::num1>(ref<NUMBER>)) |
            assign<&ast::StateOpt::kind>(kw<"no-sync">) |
            (create<ast::StateOpt> +
             assign<&ast::StateOpt::timeout>(ref<TimeoutRule>)) |
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
      append<&ast::FilterOpt::stateOpts>(ref<StateOptRule>) +
      many(option(lit<",">) +
           append<&ast::FilterOpt::stateOpts>(ref<StateOptRule>));

  // --- an unbounded run of scrub options, like state options and filter
  // options, not capped at two (Deviation). ---
  struct ScrubOptRule
      : rule<assign<&ast::ScrubOpt::kind>(kw<"no-df">) |
             (assign<&ast::ScrubOpt::kind>(kw<"min-ttl">) +
              assign<&ast::ScrubOpt::num>(ref<NUMBER>)) |
             (assign<&ast::ScrubOpt::kind>(kw<"max-mss">) +
              assign<&ast::ScrubOpt::num>(ref<NUMBER>)) |
             (assign<&ast::ScrubOpt::kind>(kw<"reassemble">) + kw<"tcp">) |
             assign<&ast::ScrubOpt::kind>(kw<"random-id">)> {};

  static constexpr auto scrubOptsList =
      append<&ast::FilterOpt::scrubOpts>(ref<ScrubOptRule>) +
      many(option(lit<",">) +
           append<&ast::FilterOpt::scrubOpts>(ref<ScrubOptRule>));

  // --- host / redirhost / address ---
  struct HostRule
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

  struct RedirHostRule
      : rule<((flag<&ast::RedirHost::iface>(lit<"(">) +
               assign<&ast::RedirHost::address>(ref<STRING>) + lit<")">) |
              assign<&ast::RedirHost::address>(
                  ref<STRING> | ref<IPV4> | ref<IPV6>)) +
             option(lit<"/"> +
                    assign<&ast::RedirHost::maskBits>(ref<NUMBER>))> {};

  static constexpr auto redirHostList =
      append<&ast::FilterOpt::targets>(ref<RedirHostRule>) |
      (lit<"{"> + append<&ast::FilterOpt::targets>(ref<RedirHostRule>) +
       many(option(lit<",">) +
            append<&ast::FilterOpt::targets>(ref<RedirHostRule>)) +
       lit<"}">);

  static constexpr auto redirHostList2 =
      append<&ast::FilterOpt::targets2>(ref<RedirHostRule>) |
      (lit<"{"> + append<&ast::FilterOpt::targets2>(ref<RedirHostRule>) +
       many(option(lit<",">) +
            append<&ast::FilterOpt::targets2>(ref<RedirHostRule>)) +
       lit<"}">);

  // --- unary_op / binary_op / op_list -- port, user, group all read this
  // shape after their own leading keyword. ---
  struct UnaryOpRule
      : rule<option(assign<&ast::PortItem::op>(
                 lit<"!="> | lit<"<="> | lit<">="> | lit<"="> | lit<"<"> |
                 lit<">">)) +
             assign<&ast::PortItem::value1>(ref<STRING> | ref<NUMBER>)> {};

  struct BinaryOpRule
      : rule<assign<&ast::PortItem::value1>(ref<NUMBER>) +
             assign<&ast::PortItem::op>(lit<"<>"> | lit<"><"> | lit<":">) +
             assign<&ast::PortItem::value2>(ref<NUMBER>)> {};

  static constexpr auto opListItem = ref<BinaryOpRule> | ref<UnaryOpRule>;

  struct OpSpecRule
      : rule<append<&ast::OpSpec::items>(opListItem) |
             (lit<"{"> + append<&ast::OpSpec::items>(opListItem) +
              many(option(lit<",">) + append<&ast::OpSpec::items>(opListItem)) +
              lit<"}">)> {};

  // --- os / os_list ---
  struct OsSpecRule
      : rule<kw<"os"> +
             (append<&ast::OsSpec::names>(ref<QUOTED_STRING>) |
              (lit<"{"> + append<&ast::OsSpec::names>(ref<QUOTED_STRING>) +
               many(lit<","> + append<&ast::OsSpec::names>(ref<QUOTED_STRING>)) +
               lit<"}">))> {};

  // --- portspec: `port` NUMBER-or-name (`:` `*`-or-NUMBER-or-name)? ---
  struct PortSpecRule
      : rule<kw<"port"> +
             assign<&ast::PortSpec::value1>(ref<STRING> | ref<NUMBER>) +
             option(lit<":"> + assign<&ast::PortSpec::value2>(
                                   lit<"*"> | ref<STRING> | ref<NUMBER>))> {
  };

  // --- flags ---
  struct FLAGCHARS : terminal<some(cr<"FSRPAUEW">)> {};

  // --- icmp_type_code / icmp_list ---
  struct IcmpTypeCodeRule
      : rule<assign<&ast::IcmpTypeCode::type>(ref<STRING> | ref<NUMBER>) +
             option(kw<"code"> +
                    assign<&ast::IcmpTypeCode::code>(ref<STRING> | ref<NUMBER>))> {
  };

  static constexpr auto icmpList =
      append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCodeRule>) |
      (lit<"{"> + append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCodeRule>) +
       many(lit<","> + append<&ast::FilterOpt::icmpCodes>(ref<IcmpTypeCodeRule>)) +
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
  struct FilterOptRule
      : rule<
            (assign<&ast::FilterOpt::kind>(kw<"user">) +
             assign<&ast::FilterOpt::opspec>(ref<OpSpecRule>)) |
            (assign<&ast::FilterOpt::kind>(kw<"group">) +
             assign<&ast::FilterOpt::opspec>(ref<OpSpecRule>)) |
            (assign<&ast::FilterOpt::kind>(kw<"flags">) +
             assign<&ast::FilterOpt::flags1>(ref<FLAGCHARS>) +
             option(lit<"/"> +
                    assign<&ast::FilterOpt::flags2>(ref<FLAGCHARS>))) |
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
             scrubOptsList + option(lit<")">)) |
            assign<&ast::FilterOpt::kind>(kw<"fragment">) |
            assign<&ast::FilterOpt::kind>(kw<"allow-opts">) |
            assign<&ast::FilterOpt::kind>(kw<"once">) |
            (assign<&ast::FilterOpt::kind>(kw<"divert-packet">) +
             assign<&ast::FilterOpt::portspec>(ref<PortSpecRule>)) |
            assign<&ast::FilterOpt::kind>(kw<"divert-reply">) |
            (assign<&ast::FilterOpt::kind>(kw<"divert-to">) +
             assign<&ast::FilterOpt::target>(ref<HostRule>) +
             assign<&ast::FilterOpt::portspec>(ref<PortSpecRule>)) |
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
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpecRule>)) + option(pooltypeClause) +
             stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"rdr-to">) + redirHostList +
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpecRule>)) + option(pooltypeClause) +
             stickyAddressOpt) |
            (assign<&ast::FilterOpt::kind>(kw<"nat-to">) + redirHostList +
             option(assign<&ast::FilterOpt::portspec>(ref<PortSpecRule>)) + option(pooltypeClause) +
             stickyAddressOpt +
             option(flag<&ast::FilterOpt::staticPort>(
                 kw<"static-port">))) |
            (assign<&ast::FilterOpt::kind>(kw<"route-to">) + redirHostList) |
            (assign<&ast::FilterOpt::kind>(kw<"reply-to">) + redirHostList) |
            (assign<&ast::FilterOpt::kind>(kw<"dup-to">) + redirHostList) |
            (flag<&ast::FilterOpt::viaSet>(kw<"set">) +
             assign<&ast::FilterOpt::kind>(kw<"tos">) +
             assign<&ast::FilterOpt::tos>(
                 kw<"lowdelay"> | kw<"throughput"> | kw<"reliability"> |
                 ref<TOSNUM>)) |
            (option(flag<&ast::FilterOpt::negated>(lit<"!">)) +
             assign<&ast::FilterOpt::kind>(kw<"received-on">) +
             assign<&ast::FilterOpt::text1>(ref<STRING>))> {};

  static constexpr auto filterOptsList =
      append<&ast::Rule::filterOpts>(ref<FilterOptRule>) +
      many(option(lit<",">) + append<&ast::Rule::filterOpts>(ref<FilterOptRule>));

  static constexpr auto anchorFilterOptsList =
      append<&ast::AnchorRule::filterOpts>(ref<FilterOptRule>) +
      many(option(lit<",">) +
           append<&ast::AnchorRule::filterOpts>(ref<FilterOptRule>));

  // --- protospec / proto_list ---
  struct ProtospecRule
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
      option(assign<&ast::FromSpec::os>(ref<OsSpecRule>));

  struct FromSpecRule
      : rule<kw<"from"> +
             ((assign<&ast::FromSpec::kind>(kw<"any"> | kw<"no-route"> |
                                            kw<"urpf-failed"> | kw<"self">)) |
              append<&ast::FromSpec::hosts>(ref<HostRule>) |
              (lit<"{"> + append<&ast::FromSpec::hosts>(ref<HostRule>) +
               many(option(lit<",">) +
                    append<&ast::FromSpec::hosts>(ref<HostRule>)) +
               lit<"}">) |
              (assign<&ast::FromSpec::kind>(kw<"route">) +
               assign<&ast::FromSpec::routeName>(
                   ref<STRING> | ref<QUOTED_STRING>))) +
             option(kw<"port"> +
                    assign<&ast::FromSpec::port>(ref<OpSpecRule>)) +
             fromspecOs> {};

  struct ToSpecRule
      : rule<kw<"to"> +
             ((assign<&ast::ToSpec::kind>(kw<"any"> | kw<"no-route"> |
                                          kw<"self">)) |
              append<&ast::ToSpec::hosts>(ref<HostRule>) |
              (lit<"{"> + append<&ast::ToSpec::hosts>(ref<HostRule>) +
               many(option(lit<",">) +
                    append<&ast::ToSpec::hosts>(ref<HostRule>)) +
               lit<"}">) |
              (assign<&ast::ToSpec::kind>(kw<"route">) +
               assign<&ast::ToSpec::routeName>(
                   ref<STRING> | ref<QUOTED_STRING>))) +
             option(kw<"port"> +
                    assign<&ast::ToSpec::port>(ref<OpSpecRule>))> {};

  struct HostsRule
      : rule<flag<&ast::Hosts::all>(kw<"all">) |
             (option(assign<&ast::Hosts::from>(ref<FromSpecRule>)) +
              option(assign<&ast::Hosts::to>(ref<ToSpecRule>)))> {};

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
  struct LogOptRule
      : rule<assign<&ast::LogOpt::kind>(kw<"all"> | kw<"matches"> |
                                        kw<"user">) |
             (assign<&ast::LogOpt::kind>(kw<"to">) +
              assign<&ast::LogOpt::iface>(ref<STRING>))> {};

  static constexpr auto loggingClause =
      flag<&ast::Rule::log>(kw<"log">) +
      option(lit<"("> +
             append<&ast::Rule::logOpts>(ref<LogOptRule>) +
             many(option(lit<",">) +
                  append<&ast::Rule::logOpts>(ref<LogOptRule>)) +
             lit<")">);

  // --- onspec ---
  static constexpr auto onspecClause =
      kw<"on"> +
      (assign<&ast::Rule::onIf>(ref<IfSpecRule>) |
       (kw<"rdomain"> + assign<&ast::Rule::onRdomain>(ref<NUMBER>)));

  // --- pf_rule ---
  struct RuleRule
      : rule<actionExpr +
             option(assign<&ast::Rule::direction>(kw<"in"> | kw<"out">)) +
             many(loggingClause |
                  flag<&ast::Rule::quick>(kw<"quick">)) +
             option(onspecClause) +
             option(assign<&ast::Rule::af>(afWord)) +
             option(assign<&ast::Rule::proto>(ref<ProtospecRule>)) +
             assign<&ast::Rule::hosts>(ref<HostsRule>) +
             option(filterOptsList)> {};

  // --- antispoof_rule ---
  struct AntispoofRuleRule
      : rule<kw<"antispoof"> +
             option(flag<&ast::AntispoofRule::log>(kw<"log">)) +
             option(flag<&ast::AntispoofRule::quick>(kw<"quick">)) +
             kw<"for"> +
             assign<&ast::AntispoofRule::ifspec>(ref<IfSpecRule>) +
             option(assign<&ast::AntispoofRule::af>(afWord)) +
             option(kw<"label"> +
                    assign<&ast::AntispoofRule::label>(ref<STRING>))> {};

  // --- table_rule ---
  // `tableaddr`'s bare `ifspec` reads as a single name identically to
  // `hostname` (both a plain STRING), so only the `{ ... }` list form -- the
  // one shape a bare name cannot also spell -- is read as an ifspec here;
  // a single interface still reads as the generic name below it.
  struct IfSpecBracedRule
      : rule<lit<"{"> + append<&ast::IfSpec::ifaces>(ifaceItem) +
             many(option(lit<",">) + append<&ast::IfSpec::ifaces>(ifaceItem)) +
             lit<"}">> {};

  struct TableAddrRule
      : rule<option(flag<&ast::TableAddr::negated>(lit<"!">)) +
             (assign<&ast::TableAddr::ifspec>(ref<IfSpecBracedRule>) |
              assign<&ast::TableAddr::text>(kw<"self">) |
              assign<&ast::TableAddr::text>(ref<IPV4>) |
              assign<&ast::TableAddr::text>(ref<IPV6>) |
              assign<&ast::TableAddr::text>(ref<STRING>)) +
             option(lit<"/"> +
                    assign<&ast::TableAddr::maskBits>(ref<NUMBER>))> {};

  static constexpr auto tableAddrsList =
      append<&ast::TableOpt::addrs>(ref<TableAddrRule>) +
      many(option(lit<",">) + append<&ast::TableOpt::addrs>(ref<TableAddrRule>));

  struct TableOptRule
      : rule<flag<&ast::TableOpt::persist>(kw<"persist">) |
             flag<&ast::TableOpt::constFlag>(kw<"const">) |
             flag<&ast::TableOpt::counters>(kw<"counters">) |
             (kw<"file"> + assign<&ast::TableOpt::file>(ref<QUOTED_STRING>)) |
             (lit<"{"> + option(tableAddrsList) + lit<"}">)> {};

  static constexpr auto tableOptsList =
      append<&ast::TableRule::opts>(ref<TableOptRule>) +
      many(append<&ast::TableRule::opts>(ref<TableOptRule>));

  struct TableRuleRule
      : rule<kw<"table"> + lit<"<"> +
             assign<&ast::TableRule::name>(ref<STRING>) + lit<">"> +
             option(tableOptsList)> {};

  // --- queue_rule ---
  struct BandwidthRule
      : rule<assign<&ast::Bandwidth::value>(ref<NUMBER>) +
             option(assign<&ast::Bandwidth::unit>(ref<BWUNIT>)) +
             option(kw<"burst"> +
                    assign<&ast::Bandwidth::burstValue>(ref<NUMBER>) +
                    option(assign<&ast::Bandwidth::burstUnit>(ref<BWUNIT>)) +
                    kw<"for"> +
                    assign<&ast::Bandwidth::burstMs>(ref<NUMBER>) +
                    kw<"ms">)> {};

  struct QueueOptRule
      : rule<(assign<&ast::QueueOpt::kind>(kw<"bandwidth">) +
              assign<&ast::QueueOpt::bw>(ref<BandwidthRule>)) |
             (assign<&ast::QueueOpt::kind>(kw<"min">) +
              assign<&ast::QueueOpt::bw>(ref<BandwidthRule>)) |
             (assign<&ast::QueueOpt::kind>(kw<"max">) +
              assign<&ast::QueueOpt::bw>(ref<BandwidthRule>)) |
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
      append<&ast::QueueRule::opts>(ref<QueueOptRule>) +
      many(append<&ast::QueueRule::opts>(ref<QueueOptRule>));

  struct QueueRuleRule
      : rule<kw<"queue"> + assign<&ast::QueueRule::name>(ref<STRING>) +
             option(kw<"on"> +
                    assign<&ast::QueueRule::iface>(ref<STRING>)) +
             queueOptsList> {};

  // --- anchor_rule / anchor_close / load_anchor ---
  struct AnchorRuleRule
      : rule<kw<"anchor"> +
             option(assign<&ast::AnchorRule::name>(
                 ref<STRING> | ref<QUOTED_STRING>)) +
             option(assign<&ast::AnchorRule::direction>(kw<"in"> | kw<"out">)) +
             option(kw<"on"> +
                    assign<&ast::AnchorRule::onIf>(ref<IfSpecRule>)) +
             option(assign<&ast::AnchorRule::af>(afWord)) +
             option(assign<&ast::AnchorRule::proto>(ref<ProtospecRule>)) +
             assign<&ast::AnchorRule::hosts>(ref<HostsRule>) +
             option(anchorFilterOptsList) +
             option(flag<&ast::AnchorRule::opensBlock>(lit<"{">))> {};

  struct AnchorCloseRule : rule<create<ast::AnchorClose> + lit<"}">> {};

  struct LoadAnchorRule
      : rule<kw<"load"> + kw<"anchor"> +
             assign<&ast::LoadAnchor::name>(ref<STRING>) + kw<"from"> +
             assign<&ast::LoadAnchor::file>(ref<QUOTED_STRING>)> {};

  // --- include / var ---
  struct IncludeRule
      : rule<kw<"include"> +
             assign<&ast::Include::file>(ref<QUOTED_STRING>)> {};

  struct VarRule
      : rule<assign<&ast::Var::name>(ref<STRING>) + lit<"="> +
             assign<&ast::Var::value>(ref<QUOTED_STRING>)> {};

  // --- option: the `set` statement. Real pf.conf takes exactly one clause
  // per `set`, which is what every corpus file writes; a combination like
  // `optimization ... limit ...` in one statement is not produced
  // (Deviation).
  struct OptionRule
      : rule<kw<"set"> +
             option(
                 (assign<&ast::Option::kind>(kw<"timeout">) +
                  (append<&ast::Option::timeouts>(ref<TimeoutRule>) |
                   (lit<"{"> +
                    append<&ast::Option::timeouts>(ref<TimeoutRule>) +
                    many(lit<","> +
                         append<&ast::Option::timeouts>(ref<TimeoutRule>)) +
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
                  (append<&ast::Option::limits>(ref<LimitItemRule>) |
                   (lit<"{"> +
                    append<&ast::Option::limits>(ref<LimitItemRule>) +
                    many(lit<","> +
                         append<&ast::Option::limits>(ref<LimitItemRule>)) +
                    lit<"}">))) |
                 (assign<&ast::Option::kind>(kw<"loginterface">) +
                  assign<&ast::Option::word>(ref<STRING> | kw<"none">)) |
                 (assign<&ast::Option::kind>(kw<"block-policy">) +
                  assign<&ast::Option::word>(kw<"drop"> | kw<"return">)) |
                 (assign<&ast::Option::kind>(kw<"state-policy">) +
                  assign<&ast::Option::word>(kw<"if-bound"> |
                                             kw<"floating">)) |
                 (assign<&ast::Option::kind>(kw<"state-defaults">) +
                  append<&ast::Option::stateOpts>(ref<StateOptRule>) +
                  many(option(lit<",">) +
                       append<&ast::Option::stateOpts>(ref<StateOptRule>))) |
                 (assign<&ast::Option::kind>(kw<"fingerprints">) +
                  assign<&ast::Option::file>(ref<QUOTED_STRING>)) |
                 (assign<&ast::Option::kind>(kw<"skip">) + kw<"on"> +
                  assign<&ast::Option::ifspec>(ref<IfSpecRule>)) |
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
      ref<OptionRule> | ref<RuleRule> | ref<AntispoofRuleRule> |
      ref<QueueRuleRule> | ref<AnchorRuleRule> | ref<AnchorCloseRule> |
      ref<LoadAnchorRule> | ref<TableRuleRule> | ref<IncludeRule> |
      ref<VarRule>;

  struct FileRule
      : rule<create<ast::File> + many(append<&ast::File::lines>(lineItem))> {
  };

  using entry = FileRule;
  using ignore = list<WS, COMMENT, CONTINUATION>;
};

}  // namespace pf
