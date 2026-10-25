// The pf language: its AST. pf.conf is a flat sequence of directives --
// options, filter rules, tables, anchors, queues -- so File.lines is the
// only real hierarchy; everything else is one node per directive.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pf::ast {

struct Var : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;  // unquoted
};

struct Include : cursiv::Extends<cursiv::AstNode> {
  string_view file;  // unquoted
};

struct LoadAnchor : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view file;  // unquoted
};

struct AnchorClose : cursiv::Extends<cursiv::AstNode> {};

// `timeout` reads a name (`tcp.first`, `interval`...) and its NUMBER; shared
// by `set timeout` and a state option.
struct Timeout : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  std::int64_t value = 0;
};

struct LimitItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  std::int64_t value = 0;
};

struct StateOpt : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Timeout* timeout = nullptr;  // kind == "timeout"
  optional<std::int64_t> num1;
  optional<std::int64_t> num2;
  string_view text;  // source-track's rule/global, overload's table name
  bool flush = false;
  bool global = false;
};

// One `set` statement. `kind` names which clause fired; a bare `set` with
// nothing after is legal too (kind empty).
struct Option : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  vector<Timeout*> timeouts;
  string_view word;  // none/basic/profile, default/normal/..., drop/return,
                      // if-bound/floating, emerg/../urgent, yes/no
  bool noDf = false;
  vector<LimitItem*> limits;
  vector<StateOpt*> stateOpts;  // state-defaults
  string_view file;             // fingerprints filename, unquoted
  struct IfSpec* ifspec = nullptr;  // skip on
};

struct IfaceRef : cursiv::Extends<cursiv::AstNode> {
  bool negated = false;
  string_view name;
};

// A single interface, or a `{ ... }` list of them -- both an `on` clause's
// target and an antispoof rule's target take this shape.
struct IfSpec : cursiv::Extends<cursiv::AstNode> {
  vector<IfaceRef*> ifaces;
};

struct Host : cursiv::Extends<cursiv::AstNode> {
  bool negated = false;
  bool iface = false;  // address written `(name)`: route through it
  bool table = false;  // address written `<name>`: a table reference
  string_view address;
  optional<std::int64_t> maskBits;
  optional<std::int64_t> weight;
};

struct RedirHost : cursiv::Extends<cursiv::AstNode> {
  bool iface = false;
  string_view address;
  optional<std::int64_t> maskBits;
};

struct PortItem : cursiv::Extends<cursiv::AstNode> {
  string_view op;      // "", "=", "!=", "<", "<=", ">", ">=", "<>", "><", ":"
  string_view value1;
  string_view value2;  // set only for a two-operand (range) comparison
};

// The shape `port`, `user` and `group` all share: one comparison, or a
// `{ ... }` list of them.
struct OpSpec : cursiv::Extends<cursiv::AstNode> {
  vector<PortItem*> items;
};

struct OsSpec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // unquoted
};

struct FromSpec : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // any/no-route/urpf-failed/self/route, empty for a host
  vector<Host*> hosts;
  string_view routeName;  // kind == "route"
  OpSpec* port = nullptr;
  OsSpec* os = nullptr;
};

struct ToSpec : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // any/no-route/self/route, empty for a host
  vector<Host*> hosts;
  string_view routeName;
  OpSpec* port = nullptr;
};

struct Hosts : cursiv::Extends<cursiv::AstNode> {
  bool all = false;
  FromSpec* from = nullptr;
  ToSpec* to = nullptr;
};

struct IcmpTypeCode : cursiv::Extends<cursiv::AstNode> {
  string_view type;
  string_view code;
};

struct ScrubOpt : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  optional<std::int64_t> num;
};

struct PortSpec : cursiv::Extends<cursiv::AstNode> {
  string_view value1;  // NUMBER or name
  string_view value2;  // '*', NUMBER or name
};

// The long tail of filter options: ~25 keyword-led clauses. One class,
// `kind` names the clause, and the fields below are shared across clauses
// that need them rather than one struct per keyword.
struct FilterOpt : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  bool viaSet = false;  // `set delay`/`set prio`/`set queue`/`set tos`: the
                        // literal `set` distinguishes these from the bare
                        // `prio`/`tos` clauses, which share the same kind
  OpSpec* opspec = nullptr;  // user, group
  string_view flags1;
  string_view flags2;  // flags a/b
  vector<IcmpTypeCode*> icmpCodes;
  string_view tos;
  vector<StateOpt*> stateOpts;  // .../state (...), scrub's own tcp state opts
  vector<ScrubOpt*> scrubOpts;
  string_view text1;  // label/tag/tagged text, af-to's af, route's verb,
                       // pooltype, source-hash's key
  string_view text2;
  bool negated = false;  // tagged '!', received-on '!'
  optional<std::int64_t> num1;
  optional<std::int64_t> num2;
  vector<RedirHost*> targets;   // redirect/route target(s)
  vector<RedirHost*> targets2;  // af-to's `to`
  Host* target = nullptr;       // divert-to's host
  PortSpec* portspec = nullptr;
  bool staticPort = false;
  bool stickyAddress = false;
  vector<string_view> words;  // set queue/set prio's 1-2 names or numbers
};

struct Protospec : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> protos;
};

struct LogOpt : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // all, matches, user, to
  string_view iface;  // kind == "to"
};

struct Rule : cursiv::Extends<cursiv::AstNode> {
  string_view action;  // pass, match, block, scrub
  string_view blockReturn;  // drop/return/return-rst/return-icmp/return-icmp6
  optional<std::int64_t> returnTtl;
  string_view returnIcmpCode1;
  string_view returnIcmpCode2;
  string_view direction;  // in, out
  bool log = false;
  vector<LogOpt*> logOpts;
  bool quick = false;
  IfSpec* onIf = nullptr;
  optional<std::int64_t> onRdomain;
  string_view af;  // inet, inet6
  Protospec* proto = nullptr;
  Hosts* hosts = nullptr;
  vector<FilterOpt*> filterOpts;
};

struct AntispoofRule : cursiv::Extends<cursiv::AstNode> {
  bool log = false;
  bool quick = false;
  IfSpec* ifspec = nullptr;
  string_view af;
  string_view label;
};

struct TableAddr : cursiv::Extends<cursiv::AstNode> {
  bool negated = false;
  string_view text;  // hostname, self, ipv4 or ipv6 spelling
  IfSpec* ifspec = nullptr;  // the `{ ... }` interface-list form
  optional<std::int64_t> maskBits;
};

struct TableOpt : cursiv::Extends<cursiv::AstNode> {
  bool persist = false;
  bool constFlag = false;
  bool counters = false;
  string_view file;          // the `file` clause, unquoted
  vector<TableAddr*> addrs;  // the `{ ... }` clause
};

struct TableRule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TableOpt*> opts;
};

struct Bandwidth : cursiv::Extends<cursiv::AstNode> {
  double value = 0;
  string_view unit;  // K, M, G
  optional<double> burstValue;
  string_view burstUnit;
  optional<std::int64_t> burstMs;
};

struct QueueOpt : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // bandwidth, min, max, parent, default, flows, quantum, qlimit
  Bandwidth* bw = nullptr;
  string_view text;  // parent's name
  optional<std::int64_t> num;
};

struct QueueRule : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view iface;
  vector<QueueOpt*> opts;
};

struct AnchorRule : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // unquoted when written as a QUOTED_STRING
  string_view direction;
  IfSpec* onIf = nullptr;
  string_view af;
  Protospec* proto = nullptr;
  Hosts* hosts = nullptr;
  vector<FilterOpt*> filterOpts;
  bool opensBlock = false;
};

using Line = std::variant<Option*, Rule*, AntispoofRule*, QueueRule*,
                          AnchorRule*, AnchorClose*, LoadAnchor*, TableRule*,
                          Include*, Var*>;

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Line> lines;
};

}  // namespace pf::ast
