// tnsnames end to end: every entry shape (service, listener, IFILE), every
// protocol, every CONNECT_DATA/failover-mode/parameter field, case-insensitive
// keywords, and the upstream corpus.

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/syntax-tree/AstMetaData.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/syntax-tree/Cst.hpp"
#include "cursiv/core/syntax-tree/ParseResult.hpp"
#include "cursiv/test/Check.hpp"
#include "tnsnames/core/ast.hpp"
#include "tnsnames/generated/Tnsnames/AstReflection.hpp"
#include "tnsnames/generated/Tnsnames/Module.hpp"

namespace {

namespace ast = tnsnames::ast;

const cursiv::Parser& engine() {
  static const std::unique_ptr<cursiv::Parser> p =
      tnsnames::generated::Tnsnames::create_parser();
  return *p;
}

const cursiv::AstReflection& reflection() {
  static const std::unique_ptr<cursiv::AstReflection> r =
      tnsnames::generated::Tnsnames::create_ast_reflection();
  return *r;
}

bool accepts(std::string_view text) {
  const cursiv::ParseResult r = engine().parse(text);
  return r.accepted && !r.cst().recovered();
}

template <class T>
T* as(cursiv::AstNode* n) {
  return (n != nullptr && reflection().isInstance(*n, &cursiv::ast_type_tag<T>))
             ? static_cast<T*>(n)
             : nullptr;
}

template <class T>
bool only(const cursiv::vector<T>& list, std::type_identity_t<T> value) {
  return list.size() == 1 && list[0] == value;
}

cursiv::ParseResult build(std::string_view text) {
  cursiv::ParseResult r = engine().parse(text);
  CHECK(r.accepted);
  CHECK(!r.cst().recovered());
  CHECK(r.diagnostics.empty());
  return r;
}

ast::Entry* entry_at(const cursiv::ParseResult& r, std::size_t i) {
  ast::Document* const doc = r.value_as<ast::Document>();
  if (doc == nullptr || i >= doc->items.size()) return nullptr;
  const std::variant<ast::Entry*, ast::Ifile*>& item = doc->items[i];
  return std::holds_alternative<ast::Entry*>(item) ? std::get<ast::Entry*>(item)
                                                    : nullptr;
}

ast::Ifile* ifile_at(const cursiv::ParseResult& r, std::size_t i) {
  ast::Document* const doc = r.value_as<ast::Document>();
  if (doc == nullptr || i >= doc->items.size()) return nullptr;
  const std::variant<ast::Entry*, ast::Ifile*>& item = doc->items[i];
  return std::holds_alternative<ast::Ifile*>(item) ? std::get<ast::Ifile*>(item)
                                                    : nullptr;
}

std::string load(const char* dir, const char* name) {
  const std::string path = std::string(dir) + "/" + name;
  return cursiv::test::read_file(path);
}

void accepted_shapes() {
  // A single address, no wrapper at all -- the bare form of a listener entry.
  CHECK(accepts("l1 = (ADDRESS = (PROTOCOL = IPC)(KEY = k))"));
  // An ADDRESS_LIST with no DESCRIPTION around it.
  CHECK(accepts(
      "l2 = (ADDRESS_LIST = (ADDRESS = (PROTOCOL = TCP)(HOST = h)(PORT = 1)))"));
  // A DESCRIPTION with no CONNECT_DATA: a listener wrapped in DESCRIPTION.
  CHECK(accepts(
      "l3 = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = h)(PORT = 1)))"));
  // A DESCRIPTION with CONNECT_DATA: a service.
  CHECK(accepts(
      "s1 = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = h)(PORT = 1)) "
      "(CONNECT_DATA = (SID = x)))"));
  // A DESCRIPTION_LIST of one description.
  CHECK(accepts(
      "s2 = (DESCRIPTION_LIST = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)"
      "(HOST = h)(PORT = 1)) (CONNECT_DATA = (SID = x))))"));
  // Several aliases, comma separated, only legal ahead of a service body.
  CHECK(accepts(
      "a,b.world,c-2 = (DESCRIPTION = (ADDRESS = (PROTOCOL = TCP)(HOST = h)"
      "(PORT = 1)) (CONNECT_DATA = (SID = x)))"));
  // Several entries and an ifile in one document, any order.
  CHECK(accepts(
      "IFILE = \"other.ora\"\n"
      "l1 = (ADDRESS = (PROTOCOL = IPC)(KEY = k))\n"
      "IFILE = 'x'\n"));
  // An empty document (comments only) still parses: the entry rule is
  // exempt from "must consume something".
  CHECK(accepts(""));
  CHECK(accepts("# just a comment\n"));
}

void keyword_case_insensitive() {
  CHECK(accepts(
      "s = (description = (address = (protocol = tcp)(host = h)(port = 1)) "
      "(connect_data = (sid = x)))"));
  CHECK(accepts(
      "s = (DeScRiPtIoN = (AdDrEsS = (PrOtOcOl = TcP)(HoSt = h)(PoRt = 1)) "
      "(CoNnEcT_DaTa = (SiD = x)))"));
  // A keyword ending in a word byte carries a boundary: SERVERX is a plain
  // name, not SERVER plus junk -- so it lexes as an alias, and this document
  // is a listener entry (bare address, no wrapper).
  CHECK(accepts("SERVERX = (ADDRESS = (PROTOCOL = IPC)(KEY = k))"));
}

void refused_texts() {
  CHECK(!accepts("="));
  CHECK(!accepts("a ="));
  CHECK(!accepts("a = (DESCRIPTION = )"));
  CHECK(!accepts("a = (ADDRESS = (PROTOCOL = TCP)(HOST = h)(PORT = 1)"));  // no ')'
  CHECK(!accepts("a = (ADDRESS = (PROTOCOL = XYZ))"));  // not a protocol
  CHECK(!accepts("IFILE ="));
  CHECK(!accepts("IFILE = \"unterminated"));
  // A description list with zero descriptions: at least one is mandatory.
  CHECK(!accepts("a = (DESCRIPTION_LIST = )"));
}

void protocols_and_addresses() {
  const std::string src = load(CURSIV_TEST_DIR, "protocols.ora");
  const cursiv::ParseResult r = build(src);

  ast::Entry* const ipc = entry_at(r, 0);
  CHECK(ipc != nullptr && ipc->addresses.size() == 1);
  ast::IpcProtocol* const ipcProto =
      ipc != nullptr && !ipc->addresses.empty()
          ? as<ast::IpcProtocol>(ipc->addresses[0]->protocol)
          : nullptr;
  CHECK(ipcProto != nullptr && only(ipcProto->keys, "LISTENER"));

  ast::Entry* const spx = entry_at(r, 1);
  ast::SpxProtocol* const spxProto =
      spx != nullptr && !spx->addresses.empty()
          ? as<ast::SpxProtocol>(spx->addresses[0]->protocol)
          : nullptr;
  CHECK(spxProto != nullptr && only(spxProto->services, "spx_svc"));

  ast::Entry* const nmp = entry_at(r, 2);
  ast::NmpProtocol* const nmpProto =
      nmp != nullptr && !nmp->addresses.empty()
          ? as<ast::NmpProtocol>(nmp->addresses[0]->protocol)
          : nullptr;
  CHECK(nmpProto != nullptr && only(nmpProto->servers, "srv1") &&
        only(nmpProto->pipes, "pipe1"));

  ast::Entry* const beq = entry_at(r, 3);
  CHECK(beq != nullptr && beq->description != nullptr);
  ast::BeqProtocol* const beqProto =
      beq != nullptr && beq->description != nullptr &&
              !beq->description->addresses.empty()
          ? as<ast::BeqProtocol>(beq->description->addresses[0]->protocol)
          : nullptr;
  CHECK(beqProto != nullptr && only(beqProto->programs, "oracle") &&
        only(beqProto->argv0, "orcl") && beqProto->argsLocal &&
        beqProto->argsHasAddress);
  CHECK(beq != nullptr && beq->description != nullptr &&
        beq->description->connectData != nullptr &&
        only(beq->description->connectData->sids, "orcl"));

  ast::Entry* const tcpIp = entry_at(r, 4);
  ast::TcpProtocol* const tcpIpProto =
      tcpIp != nullptr && tcpIp->description != nullptr &&
              !tcpIp->description->addresses.empty()
          ? as<ast::TcpProtocol>(tcpIp->description->addresses[0]->protocol)
          : nullptr;
  CHECK(tcpIpProto != nullptr && only(tcpIpProto->hosts, "192.168.1.10"));
  CHECK(tcpIp != nullptr && tcpIp->description != nullptr &&
        tcpIp->description->connectData != nullptr &&
        only(tcpIp->description->connectData->serviceNames,
             "orcl.example.com"));

  ast::Entry* const mixed = entry_at(r, 5);
  ast::TcpProtocol* const mixedProto =
      mixed != nullptr && mixed->description != nullptr &&
              !mixed->description->addresses.empty()
          ? as<ast::TcpProtocol>(mixed->description->addresses[0]->protocol)
          : nullptr;
  CHECK(mixedProto != nullptr && only(mixedProto->hosts, "dbhost") &&
        only(mixedProto->ports, 1521));
}

void connect_data_fields() {
  const std::string src = load(CURSIV_TEST_DIR, "connect_data.ora");
  const cursiv::ParseResult r = build(src);

  ast::Entry* const full = entry_at(r, 0);
  ast::ConnectData* const cd =
      full != nullptr ? full->description->connectData : nullptr;
  CHECK(cd != nullptr);
  if (cd != nullptr) {
    CHECK(only(cd->serviceNames, "orcl.example.com"));
    CHECK(only(cd->instanceNames, "orcl1"));
    CHECK(cd->failoverModes.size() == 1);
    if (cd->failoverModes.size() == 1) {
      const ast::FailoverMode* const fm = cd->failoverModes[0];
      CHECK(only(fm->types, ast::FailoverType::select_));
      CHECK(only(fm->methods, ast::FailoverMethod::preconnect));
      CHECK(only(fm->backups, "backup.example.com"));
      CHECK(only(fm->retries, 5));
      CHECK(only(fm->delays, 3));
    }
    CHECK(only(cd->globalNames, "orcl_global.example.com"));
    CHECK(cd->hs);
    CHECK(only(cd->servers, ast::ServerType::dedicated));
    CHECK(cd->ur);
  }

  ast::Entry* const sidRdb = entry_at(r, 1);
  ast::ConnectData* const cd2 =
      sidRdb != nullptr ? sidRdb->description->connectData : nullptr;
  CHECK(cd2 != nullptr && only(cd2->sids, "orcl"));
  CHECK(cd2 != nullptr && only(cd2->rdbDatabases, "[.mf]mf_personal.rdb"));

  ast::Entry* const rdbBare = entry_at(r, 2);
  ast::ConnectData* const cd3 =
      rdbBare != nullptr ? rdbBare->description->connectData : nullptr;
  CHECK(cd3 != nullptr && only(cd3->rdbDatabases, "mf_personal.rdb"));
}

void params_fields() {
  const std::string src = load(CURSIV_TEST_DIR, "params.ora");
  const cursiv::ParseResult r = build(src);

  ast::Entry* const full = entry_at(r, 0);
  ast::Description* const d = full != nullptr ? full->description : nullptr;
  CHECK(d != nullptr);
  if (d != nullptr) {
    CHECK(d->enableBroken);
    CHECK(d->failover);      // ON
    CHECK(d->loadBalance);   // TRUE
    CHECK(d->sourceRoute);   // YES
    CHECK(only(d->sdus, 8192));
    CHECK(only(d->recvBufs, 11784));
    CHECK(only(d->sendBufs, 11784));
    CHECK(only(d->serviceTypes, "customtype"));
    CHECK(only(d->sslServerCertDns, "cn=test,dc=example"));
    CHECK(only(d->connectTimeouts, 10));
    CHECK(only(d->retryCounts, 3));
    CHECK(only(d->transportConnectTimeouts, 5));
    CHECK(d->addresses.size() == 1);
    if (!d->addresses.empty()) {
      CHECK(only(d->addresses[0]->protocol->sendBufs, 2048));
      CHECK(only(d->addresses[0]->protocol->recvBufs, 2048));
    }
  }

  ast::Entry* const dl = entry_at(r, 1);
  ast::DescriptionList* const list =
      dl != nullptr ? dl->descriptionList : nullptr;
  CHECK(list != nullptr);
  if (list != nullptr) {
    CHECK(!list->failover);     // off
    CHECK(!list->loadBalance);  // no
    CHECK(list->descriptions.size() == 1);
    if (!list->descriptions.empty()) {
      ast::Description* const inner = list->descriptions[0];
      CHECK(inner->addressList != nullptr);
      CHECK(inner->addressList != nullptr && inner->addressList->sourceRoute);
      CHECK(inner->addressList != nullptr &&
            inner->addressList->addresses.size() == 2);
    }
  }

  // Comments around and inside an entry are trivia, not content.
  ast::Entry* const commented = entry_at(r, 2);
  CHECK(commented != nullptr && commented->addresses.size() == 1);
}

void repeated_parameters() {
  const cursiv::ParseResult r = build(
      "a = (DESCRIPTION = (SDU = 1)"
      "(ADDRESS = (PROTOCOL = TCP)(HOST = h1)(PORT = 1)(HOST = h2))"
      "(SDU = 2)"
      "(CONNECT_DATA = (FAILOVER_MODE = (TYPE = SELECT))(SID = x)"
      "(FAILOVER_MODE = (TYPE = NONE)(TYPE = SESSION))))");
  ast::Entry* const e = entry_at(r, 0);
  ast::Description* const d = e != nullptr ? e->description : nullptr;
  CHECK(d != nullptr);
  if (d == nullptr) return;
  CHECK(d->sdus.size() == 2 && d->sdus[0] == 1 && d->sdus[1] == 2);
  ast::TcpProtocol* const tcp =
      d->addresses.size() == 1 ? as<ast::TcpProtocol>(d->addresses[0]->protocol)
                               : nullptr;
  CHECK(tcp != nullptr && tcp->hosts.size() == 2 && tcp->hosts[0] == "h1" &&
        tcp->hosts[1] == "h2");
  ast::ConnectData* const cd = d->connectData;
  CHECK(cd != nullptr && cd->failoverModes.size() == 2);
  if (cd == nullptr || cd->failoverModes.size() != 2) return;
  CHECK(only(cd->failoverModes[0]->types, ast::FailoverType::select_));
  const auto& types = cd->failoverModes[1]->types;
  CHECK(types.size() == 2 && types[0] == ast::FailoverType::none &&
        types[1] == ast::FailoverType::session);
  CHECK(cd->failoverModes[0]->parent == cd &&
        cd->failoverModes[1]->parent == cd);
}

void ifile_directive() {
  const std::string src = load(CURSIV_TEST_DIR, "ifile.ora");
  const cursiv::ParseResult r = build(src);
  ast::Ifile* const dq = ifile_at(r, 0);
  ast::Ifile* const sq = ifile_at(r, 1);
  ast::Ifile* const uq = ifile_at(r, 2);
  CHECK(dq != nullptr && dq->path == "included.ora");
  CHECK(sq != nullptr && sq->path == "included2.ora");
  CHECK(uq != nullptr && uq->path == "included3.ora");
}

void corpus() {
  const std::string src = load(CURSIV_EXAMPLE_DIR, "tnsnames.test.ora");
  CHECK(!src.empty());
  const cursiv::ParseResult r = build(src);

  ast::Document* const doc = r.value_as<ast::Document>();
  CHECK(doc != nullptr && doc->items.size() == 4);
  if (doc == nullptr || doc->items.size() != 4) return;

  // LSNR_FRED: a service-shaped DESCRIPTION with no CONNECT_DATA -- a
  // listener entry, single alias, single address.
  ast::Entry* const fred = entry_at(r, 0);
  CHECK(fred != nullptr && fred->aliases.size() == 1 &&
        fred->aliases[0] == "LSNR_FRED");
  CHECK(fred != nullptr && fred->description != nullptr);
  CHECK(fred != nullptr && fred->description != nullptr &&
        fred->description->connectData == nullptr);
  CHECK(fred != nullptr && fred->description != nullptr &&
        fred->description->addresses.size() == 1);

  // LSNR_WILMA: a bare address, no wrapper at all.
  ast::Entry* const wilma = entry_at(r, 1);
  CHECK(wilma != nullptr && wilma->description == nullptr &&
        wilma->addressList == nullptr && wilma->addresses.size() == 1);
  ast::IpcProtocol* const wilmaProto =
      wilma != nullptr && !wilma->addresses.empty()
          ? as<ast::IpcProtocol>(wilma->addresses[0]->protocol)
          : nullptr;
  CHECK(wilmaProto != nullptr && only(wilmaProto->keys, "LISTENER"));

  // lsnr_barney: DESCRIPTION wrapping an ADDRESS_LIST of two addresses.
  ast::Entry* const barney = entry_at(r, 2);
  CHECK(barney != nullptr && barney->description != nullptr);
  CHECK(barney != nullptr && barney->description != nullptr &&
        barney->description->addressList != nullptr);
  CHECK(barney != nullptr && barney->description != nullptr &&
        barney->description->addressList != nullptr &&
        barney->description->addressList->addresses.size() == 2);

  // The last entry: three comma-separated aliases, a description list
  // holding one description with the full spread of service parameters,
  // five addresses across four protocols, and a nested BEQ ARGS description.
  ast::Entry* const svc = entry_at(r, 3);
  CHECK(svc != nullptr && svc->aliases.size() == 3);
  if (svc != nullptr && svc->aliases.size() == 3) {
    CHECK(svc->aliases[0] == "alias_1");
    CHECK(svc->aliases[1] == "alias_2.world");
    CHECK(svc->aliases[2] == "alias3.dunbar-it.co.uk");
  }
  ast::DescriptionList* const list =
      svc != nullptr ? svc->descriptionList : nullptr;
  CHECK(list != nullptr && list->descriptions.size() == 1);
  ast::Description* const desc =
      list != nullptr && !list->descriptions.empty() ? list->descriptions[0]
                                                      : nullptr;
  CHECK(desc != nullptr);
  if (desc != nullptr) {
    CHECK(desc->loadBalance);
    CHECK(only(desc->connectTimeouts, 10));
    CHECK(only(desc->sdus, 12345));
    CHECK(only(desc->retryCounts, 3));
    CHECK(only(desc->transportConnectTimeouts, 5));
    CHECK(only(desc->sslServerCertDns, "cn=finance,cn=oracle,dc=uk,dc=co"));
    CHECK(desc->addressList != nullptr);
    CHECK(desc->addressList != nullptr && desc->addressList->addresses.size() == 5);
    CHECK(desc->connectData != nullptr);
    CHECK(desc->connectData != nullptr &&
          only(desc->connectData->servers, ast::ServerType::dedicated));
    CHECK(desc->connectData != nullptr &&
          only(desc->connectData->serviceNames, "momoa61a"));
    if (desc->addressList != nullptr && desc->addressList->addresses.size() == 5) {
      const auto& addrs = desc->addressList->addresses;
      CHECK(as<ast::TcpProtocol>(addrs[0]->protocol) != nullptr);
      CHECK(as<ast::TcpProtocol>(addrs[1]->protocol) != nullptr);
      CHECK(as<ast::SpxProtocol>(addrs[2]->protocol) != nullptr);
      CHECK(as<ast::NmpProtocol>(addrs[3]->protocol) != nullptr);
      ast::BeqProtocol* const beq = as<ast::BeqProtocol>(addrs[4]->protocol);
      CHECK(beq != nullptr && only(beq->programs, "oracle_exe") &&
            only(beq->argv0, "sid_identifier") && beq->argsLocal &&
            beq->argsHasAddress);
    }
  }

  // Parents are wired through the members that own each node.
  CHECK(desc != nullptr && list != nullptr && desc->parent == list);
  CHECK(list != nullptr && svc != nullptr && list->parent == svc);
  CHECK(doc->parent == nullptr);
}


void native_descriptor_values() {
  for (const char *params : {"(PROTOCOL=TCP)(HOST=db)(SEND_BUF_SIZE=1024)(PORT=1521)(RECV_BUF_SIZE=2048)",
                            "(SEND_BUF_SIZE=1024)(HOST=db)(RECV_BUF_SIZE=2048)(PROTOCOL=TCP)(PORT=1521)"}) {
    const std::string source = std::string("a=(DESCRIPTION=(ADDRESS=") + params +
        ")(CONNECT_DATA=(SERVICE_NAME=svc)))";
    const auto r = build(source);
    const auto *e = entry_at(r, 0);
    const auto *address = e && e->description && e->description->addresses.size() == 1
        ? e->description->addresses[0] : nullptr;
    const auto *protocol = address ? as<ast::TcpProtocol>(address->protocol) : nullptr;
    CHECK(protocol && only(protocol->hosts, "db") && only(protocol->ports, 1521) &&
          only(protocol->sendBufs, 1024) && only(protocol->recvBufs, 2048) &&
          protocol->parent == address);
  }

  // python-oracledb 26.0.1: quoted values, IPv6, aliases, TCPS and duration units.
  const std::string source =
      "_a,$b,àlias=(DESCRIPTION=(RETRY_DELAY=2)"
      "(CONNECT_DATA=(SERVICE_NAME=\"svc$pdb\")(SID=TCP))"
      "(ADDRESS=(HOST=2001:db8::1)(PORT=\"1522\")(PROTOCOL=tCpS))"
      "(TRANSPORT_CONNECT_TIMEOUT=300ms))";
  const auto result = build(source);
  const auto* entry = entry_at(result, 0);
  CHECK(entry != nullptr && entry->aliases.size() == 3);
  CHECK(entry != nullptr && entry->aliases[0] == "_a" && entry->aliases[1] == "$b" &&
        entry->aliases[2] == "àlias");
  const auto* description = entry ? entry->description : nullptr;
  CHECK(description != nullptr && only(description->retryDelays, 2));
  CHECK(description != nullptr && only(description->transportConnectTimeouts, 0.3));
  const auto* cd = description ? description->connectData : nullptr;
  CHECK(cd != nullptr && only(cd->serviceNames, "svc$pdb") && only(cd->sids, "TCP"));
  const auto* tcp = description && description->addresses.size() == 1
      ? as<ast::TcpProtocol>(description->addresses[0]->protocol) : nullptr;
  CHECK(tcp != nullptr && tcp->secure && only(tcp->hosts, "2001:db8::1") &&
        only(tcp->ports, 1522));
  CHECK(tcp != nullptr && tcp->parent == description->addresses[0]);
  CHECK(cd != nullptr && cd->parent == description && description->parent == entry);
  if (tcp) {
    const auto span = result.cst()[tcp->cstNodeId];
    CHECK(source.substr(span.begin, span.end - span.begin) ==
          "(HOST=2001:db8::1)(PORT=\"1522\")(PROTOCOL=tCpS)");
  }
  for (const auto& [written, seconds] :
       {std::pair{"2sec", 2.0}, {"1min", 60.0}, {"1.5MiN", 90.0},
        {"\"250ms\"", 0.25}}) {
    const auto text = std::string("a=(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=\"db host\")"
                                  "(PORT=1521))(TRANSPORT_CONNECT_TIMEOUT=") + written + "))";
    const auto parsed = build(text);
    const auto* e = entry_at(parsed, 0);
    CHECK(e && e->description && !e->description->connectData);
    CHECK(e && e->description && only(e->description->transportConnectTimeouts, seconds));
    const auto* protocol = e && e->description && e->description->addresses.size() == 1
        ? as<ast::TcpProtocol>(e->description->addresses[0]->protocol) : nullptr;
    CHECK(protocol && !protocol->secure && only(protocol->hosts, "db host"));
  }
  const auto empty = build("a=(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=/tmp/db)"
                           "(PORT=1521))(CONNECT_DATA=))");
  CHECK(entry_at(empty, 0) && entry_at(empty, 0)->description->connectData);
  const auto blank = build("a=(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=)(PORT=1521)))");
  const auto* blankEntry = entry_at(blank, 0);
  const auto* blankProtocol = blankEntry && blankEntry->description
      ? as<ast::TcpProtocol>(blankEntry->description->addresses[0]->protocol) : nullptr;
  CHECK(blankProtocol && only(blankProtocol->hosts, ""));
  const auto missingDuration = engine().parse(
      "a=(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=db))(TRANSPORT_CONNECT_TIMEOUT=))");
  CHECK(!missingDuration.diagnostics.empty());
  const auto overflow = engine().parse(
      std::string("a=(DESCRIPTION=(ADDRESS=(PROTOCOL=TCP)(HOST=db))") +
      "(TRANSPORT_CONNECT_TIMEOUT=" + std::string(400, '9') + "ms))");
  CHECK(overflow.accepted && !overflow.cst().recovered() && !overflow.diagnostics.empty());
  CHECK(overflow.diagnostics.size() == 1 &&
        overflow.diagnostics[0].kind == cursiv::DiagnosticKind::conversion_error &&
        overflow.diagnostics[0].begin < overflow.diagnostics[0].end);
  const auto include = build("IFILE= child.ora  ");
  CHECK(ifile_at(include, 0) && ifile_at(include, 0)->path == "child.ora");
}

}  // namespace

int main() {
  native_descriptor_values();
  accepted_shapes();
  keyword_case_insensitive();
  refused_texts();
  protocols_and_addresses();
  connect_data_fields();
  params_fields();
  repeated_parameters();
  ifile_directive();
  corpus();
  return cursiv::test::report("tnsnames_grammar_test");
}
