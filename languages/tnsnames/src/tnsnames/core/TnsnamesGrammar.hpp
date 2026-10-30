// The tnsnames language: its grammar. Keywords are case-insensitive and
// identifiers carry '.', '-': word_boundary is widened to match, or a
// keyword right against those bytes would misread as a shorter word.
#pragma once

#include <charconv>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tnsnames/core/ast.hpp"

namespace tnsnames {

inline constexpr auto strip_quotes = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

inline constexpr auto strip_trailing_newline = [](std::string_view t) {
  const auto begin = t.find_first_not_of(" \t\r\n");
  const auto end = t.find_last_not_of(" \t\r\n");
  return begin == t.npos ? std::string_view{} : t.substr(begin, end - begin + 1);
};

inline constexpr auto duration_seconds = [](std::string_view text)
    -> cursiv::ConversionResult<double> {
  if (text.empty())
    return cursiv::ConversionResult<double>::err("duration requires a value");
  if (text.front() == '"') text = strip_quotes(text);
  double seconds = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), seconds);
  if (parsed.ec != std::errc{})
    return cursiv::ConversionResult<double>::err("duration outside the supported numeric range");
  const std::string_view unit(parsed.ptr, text.data() + text.size());
  if (!unit.empty() && (unit.front() == 'm' || unit.front() == 'M')) {
    if (unit.size() == 2) seconds /= 1000;
    else seconds *= 60;
  }
  return cursiv::ConversionResult<double>::ok(seconds);
};

struct Tnsnames : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"A-Za-z0-9_$.-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<"#"> + many(cr<R"(^\r\n)">) + option(eol)> {};

  struct ID : terminal<(cr<"A-Za-z0-9_$"> | uc::L) +
                       many(cr<"A-Za-z0-9_$.-"> | uc::L)> {};
  struct INT : terminal<some(d)> {};
  struct QUOTED_INT
      : terminal<as<strip_quotes>(lit<"\""> + some(d) + lit<"\"">)> {};
  struct IntegerValue : rule<ref<INT> | ref<QUOTED_INT>> {};
  static constexpr auto DurationText = some(d) + option(lit<"."> + some(d)) +
      option((cr<"mM"> + cr<"sS">) |
             (cr<"sS"> + cr<"eE"> + cr<"cC">) |
             (cr<"mM"> + cr<"iI"> + cr<"nN">));
  struct DURATION : terminal<DurationText> {};
  struct QUOTED_DURATION
      : terminal<lit<"\""> + DurationText + lit<"\"">> {};
  struct DurationValue
      : rule<as<duration_seconds>(ref<DURATION> | ref<QUOTED_DURATION>)> {};

  struct DQ_STRING
      : terminal<as<strip_quotes>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {
  };
  struct ISQ_STRING
      : terminal<as<strip_quotes>(lit<"'"> + many(cr<"^'">) + lit<"'">)> {};
  struct IUQ_STRING : terminal<as<strip_trailing_newline>(
                          cr<R"(^"'=\r\n# \t)"> + many(cr<R"(^"'=\r\n#)">))> {};

  // Descriptor values retain punctuation (including IPv6 colons); only
  // double quotes delimit a quoted descriptor value in the native reader.
  struct VALUE : terminal<some(cr<R"(^ \t\r\n()="#)">)> {};
  struct ScalarValue : rule<option(ref<DQ_STRING> | ref<VALUE>)> {};

  // `ID ('.' ID)*` joined: written tight it is already one ID, written
  // spaced this rule reassembles the same text (examples/ini's DottedKey).
  struct DottedName : rule<some(ref<ID>, lit<".">)> {};


  static constexpr auto ASendBufParam =
      lit<"("> + ikw<"SEND_BUF_SIZE"> + lit<"="> +
      append<&ast::Protocol::sendBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto ARecvBufParam =
      lit<"("> + ikw<"RECV_BUF_SIZE"> + lit<"="> +
      append<&ast::Protocol::recvBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto AParams = many(ASendBufParam | ARecvBufParam);

  static constexpr auto TcpHostParam =
      lit<"("> + ikw<"HOST"> + lit<"="> +
      append<&ast::TcpProtocol::hosts>(ref<ScalarValue>) + lit<")">;
  static constexpr auto TcpPortParam =
      lit<"("> + ikw<"PORT"> + lit<"="> +
      append<&ast::TcpProtocol::ports>(ref<IntegerValue>) + lit<")">;
  static constexpr auto TcpProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> +
      (ikw<"TCP"> | flag<&ast::TcpProtocol::secure>(ikw<"TCPS">)) + lit<")">;
  struct TcpProtocol
      : rule<create<ast::TcpProtocol> +
             AParams + some((TcpHostParam | TcpPortParam | TcpProtocolMarker) + AParams)> {};

  static constexpr auto IpcKeyParam =
      lit<"("> + ikw<"KEY"> + lit<"="> +
      append<&ast::IpcProtocol::keys>(ref<ScalarValue>) + lit<")">;
  static constexpr auto IpcProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"IPC"> + lit<")">;
  struct IpcProtocol
      : rule<create<ast::IpcProtocol> + AParams + some((IpcKeyParam | IpcProtocolMarker) + AParams)> {
  };

  static constexpr auto SpxServiceParam =
      lit<"("> + ikw<"SERVICE"> + lit<"="> +
      append<&ast::SpxProtocol::services>(ref<ScalarValue>) + lit<")">;
  static constexpr auto SpxProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"SPX"> + lit<")">;
  struct SpxProtocol
      : rule<create<ast::SpxProtocol> + AParams + some((SpxServiceParam | SpxProtocolMarker) + AParams)> {
  };

  static constexpr auto NmpServerParam =
      lit<"("> + ikw<"SERVER"> + lit<"="> +
      append<&ast::NmpProtocol::servers>(ref<ScalarValue>) + lit<")">;
  static constexpr auto NmpPipeParam =
      lit<"("> + ikw<"PIPE"> + lit<"="> +
      append<&ast::NmpProtocol::pipes>(ref<ScalarValue>) + lit<")">;
  static constexpr auto NmpProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"NMP"> + lit<")">;
  struct NmpProtocol
      : rule<create<ast::NmpProtocol> +
             AParams + some((NmpServerParam | NmpPipeParam | NmpProtocolMarker) + AParams)> {};

  static constexpr auto BeqProgramParam =
      lit<"("> + ikw<"PROGRAM"> + lit<"="> +
      append<&ast::BeqProtocol::programs>(ref<ScalarValue>) + lit<")">;
  static constexpr auto BeqArgv0Param =
      lit<"("> + ikw<"ARGV0"> + lit<"="> +
      append<&ast::BeqProtocol::argv0>(ref<ScalarValue>) + lit<")">;
  static constexpr auto BeqProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"BEQ"> + lit<")">;
  static constexpr auto BeqAddressMarker =
      lit<"("> + ikw<"ADDRESS"> + lit<"="> + BeqProtocolMarker + lit<")">;
  static constexpr auto BeqLocalParam =
      lit<"("> + ikw<"LOCAL"> + lit<"="> +
      (flag<&ast::BeqProtocol::argsLocal>(ikw<"YES">) | ikw<"NO">) +
      lit<")">;
  static constexpr auto BeqArgsParam =
      lit<"("> + ikw<"ARGS"> + lit<"="> + lit<"'"> + lit<"("> +
      ikw<"DESCRIPTION"> + lit<"="> +
      some(BeqLocalParam |
           flag<&ast::BeqProtocol::argsHasAddress>(BeqAddressMarker)) +
      lit<")"> + lit<"'"> + lit<")">;
  struct BeqProtocol
      : rule<create<ast::BeqProtocol> +
             AParams + some((BeqProgramParam | BeqArgv0Param | BeqArgsParam |
                  BeqProtocolMarker) + AParams)> {};

  static constexpr auto ProtocolInfo =
      ref<TcpProtocol> | ref<IpcProtocol> | ref<SpxProtocol> |
      ref<NmpProtocol> | ref<BeqProtocol>;

  struct Address
      : rule<lit<"("> + ikw<"ADDRESS"> + lit<"="> +
             assign<&ast::Address::protocol>(ProtocolInfo) + lit<")">> {};

  static constexpr auto AlFailoverParam =
      lit<"("> + ikw<"FAILOVER"> + lit<"="> +
      (flag<&ast::AddressList::failover>(ikw<"YES"> | ikw<"ON"> |
                                              ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto AlLoadBalanceParam =
      lit<"("> + ikw<"LOAD_BALANCE"> + lit<"="> +
      (flag<&ast::AddressList::loadBalance>(ikw<"YES"> | ikw<"ON"> |
                                                 ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto AlSourceRouteParam =
      lit<"("> + ikw<"SOURCE_ROUTE"> + lit<"="> +
      (flag<&ast::AddressList::sourceRoute>(ikw<"YES"> | ikw<"ON">) |
       ikw<"NO"> | ikw<"OFF">) +
      lit<")">;
  static constexpr auto AlParams =
      many(AlFailoverParam | AlLoadBalanceParam | AlSourceRouteParam);

  struct AddressList
      : rule<lit<"("> + ikw<"ADDRESS_LIST"> + lit<"="> + AlParams +
             some(append<&ast::AddressList::addresses>(ref<Address>)) +
             AlParams + lit<")">> {};

  static constexpr auto FoTypeParam =
      lit<"("> + ikw<"TYPE"> + lit<"="> +
      append<&ast::FailoverMode::types>(as<ast::FailoverType::session>(ikw<"SESSION">) | as<ast::FailoverType::select_>(ikw<"SELECT">) |
                                        as<ast::FailoverType::none>(ikw<"NONE">)) +
      lit<")">;
  static constexpr auto FoBackupParam =
      lit<"("> + ikw<"BACKUP"> + lit<"="> +
      append<&ast::FailoverMode::backups>(ref<ScalarValue>) + lit<")">;
  static constexpr auto FoMethodParam =
      lit<"("> + ikw<"METHOD"> + lit<"="> +
      append<&ast::FailoverMode::methods>(as<ast::FailoverMethod::basic>(ikw<"BASIC">) | as<ast::FailoverMethod::preconnect>(ikw<"PRECONNECT">)) +
      lit<")">;
  static constexpr auto FoRetriesParam =
      lit<"("> + ikw<"RETRIES"> + lit<"="> +
      append<&ast::FailoverMode::retries>(ref<IntegerValue>) + lit<")">;
  static constexpr auto FoDelayParam =
      lit<"("> + ikw<"DELAY"> + lit<"="> +
      append<&ast::FailoverMode::delays>(ref<IntegerValue>) + lit<")">;
  struct FailoverMode
      : rule<some(FoTypeParam | FoBackupParam | FoMethodParam |
                  FoRetriesParam | FoDelayParam)> {};

  struct RdbDatabaseValue
      : rule<option(lit<"["> + lit<"."> + ref<ID> + lit<"]">) +
             some(ref<ID>, lit<".">)> {};

  static constexpr auto CdServiceNameParam =
      lit<"("> + ikw<"SERVICE_NAME"> + lit<"="> +
      append<&ast::ConnectData::serviceNames>(ref<ScalarValue>) + lit<")">;
  static constexpr auto CdSidParam =
      lit<"("> + ikw<"SID"> + lit<"="> +
      append<&ast::ConnectData::sids>(ref<ScalarValue>) + lit<")">;
  static constexpr auto CdInstanceNameParam =
      lit<"("> + ikw<"INSTANCE_NAME"> + lit<"="> +
      append<&ast::ConnectData::instanceNames>(ref<ScalarValue>) + lit<")">;
  static constexpr auto CdFailoverModeParam =
      lit<"("> + ikw<"FAILOVER_MODE"> + lit<"="> +
      append<&ast::ConnectData::failoverModes>(ref<FailoverMode>) +
      lit<")">;
  static constexpr auto CdGlobalNameParam =
      lit<"("> + ikw<"GLOBAL_NAME"> + lit<"="> +
      append<&ast::ConnectData::globalNames>(ref<ScalarValue>) + lit<")">;
  static constexpr auto CdHsParam =
      lit<"("> + ikw<"HS"> + lit<"="> +
      flag<&ast::ConnectData::hs>(ikw<"OK">) + lit<")">;
  static constexpr auto CdRdbDatabaseParam =
      lit<"("> + ikw<"RDB_DATABASE"> + lit<"="> +
      append<&ast::ConnectData::rdbDatabases>(ref<RdbDatabaseValue>) + lit<")">;
  static constexpr auto CdServerParam =
      lit<"("> + ikw<"SERVER"> + lit<"="> +
      append<&ast::ConnectData::servers>(as<ast::ServerType::dedicated>(ikw<"DEDICATED">) | as<ast::ServerType::shared_>(ikw<"SHARED">) |
                                         as<ast::ServerType::pooled>(ikw<"POOLED">)) +
      lit<")">;
  static constexpr auto CdUrParam =
      lit<"("> + ikw<"UR"> + lit<"="> + flag<&ast::ConnectData::ur>(ikw<"A">) +
      lit<")">;
  struct ConnectData
      : rule<lit<"("> + ikw<"CONNECT_DATA"> + lit<"="> +
             many(CdServiceNameParam | CdSidParam | CdInstanceNameParam |
                  CdFailoverModeParam | CdGlobalNameParam | CdHsParam |
                  CdRdbDatabaseParam | CdServerParam | CdUrParam) +
             lit<")">> {};

  static constexpr auto DEnableParam =
      flag<&ast::Description::enableBroken>(
          lit<"("> + ikw<"ENABLE"> + lit<"="> + ikw<"BROKEN"> + lit<")">);
  static constexpr auto DFailoverParam =
      lit<"("> + ikw<"FAILOVER"> + lit<"="> +
      (flag<&ast::Description::failover>(ikw<"YES"> | ikw<"ON"> |
                                              ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto DLoadBalanceParam =
      lit<"("> + ikw<"LOAD_BALANCE"> + lit<"="> +
      (flag<&ast::Description::loadBalance>(ikw<"YES"> | ikw<"ON"> |
                                                 ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto DSourceRouteParam =
      lit<"("> + ikw<"SOURCE_ROUTE"> + lit<"="> +
      (flag<&ast::Description::sourceRoute>(ikw<"YES"> | ikw<"ON">) |
       ikw<"NO"> | ikw<"OFF">) +
      lit<")">;
  static constexpr auto DSduParam =
      lit<"("> + ikw<"SDU"> + lit<"="> +
      append<&ast::Description::sdus>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DRecvBufParam =
      lit<"("> + ikw<"RECV_BUF_SIZE"> + lit<"="> +
      append<&ast::Description::recvBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DSendBufParam =
      lit<"("> + ikw<"SEND_BUF_SIZE"> + lit<"="> +
      append<&ast::Description::sendBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DServiceTypeParam =
      lit<"("> + ikw<"TYPE_OF_SERVICE"> + lit<"="> +
      append<&ast::Description::serviceTypes>(ref<ScalarValue>) + lit<")">;
  static constexpr auto DSecurityParam =
      lit<"("> + ikw<"SECURITY"> + lit<"="> + lit<"("> + ikw<"SSL_SERVER_CERT_DN"> +
      lit<"="> + append<&ast::Description::sslServerCertDns>(ref<DQ_STRING>) +
      lit<")"> + lit<")">;
  static constexpr auto DConnTimeoutParam =
      lit<"("> + ikw<"CONNECT_TIMEOUT"> + lit<"="> +
      append<&ast::Description::connectTimeouts>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DRetryCountParam =
      lit<"("> + ikw<"RETRY_COUNT"> + lit<"="> +
      append<&ast::Description::retryCounts>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DRetryDelayParam =
      lit<"("> + ikw<"RETRY_DELAY"> + lit<"="> +
      append<&ast::Description::retryDelays>(ref<IntegerValue>) + lit<")">;
  static constexpr auto DTctParam =
      lit<"("> + ikw<"TRANSPORT_CONNECT_TIMEOUT"> + lit<"="> +
      append<&ast::Description::transportConnectTimeouts>(ref<DurationValue>) + lit<")">;
  static constexpr auto DParams =
      many(DEnableParam | DFailoverParam | DLoadBalanceParam | DSduParam |
           DRecvBufParam | DSendBufParam | DSourceRouteParam |
           DServiceTypeParam | DSecurityParam | DConnTimeoutParam |
           DRetryCountParam | DRetryDelayParam | DTctParam);

  static constexpr auto DescriptionAddresses =
      assign<&ast::Description::addressList>(ref<AddressList>) |
      some(append<&ast::Description::addresses>(ref<Address>));

  // CONNECT_DATA may precede or follow the addresses, and listener
  // descriptions may carry the same description parameters without it.
  struct Description
      : rule<lit<"("> + ikw<"DESCRIPTION"> + lit<"="> + DParams +
             ((assign<&ast::Description::connectData>(ref<ConnectData>) +
               DParams + DescriptionAddresses) |
              (DescriptionAddresses + DParams +
               option(assign<&ast::Description::connectData>(ref<ConnectData>)))) +
             DParams + lit<")">> {};

  static constexpr auto DlFailoverParam =
      lit<"("> + ikw<"FAILOVER"> + lit<"="> +
      (flag<&ast::DescriptionList::failover>(ikw<"YES"> | ikw<"ON"> |
                                                  ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto DlLoadBalanceParam =
      lit<"("> + ikw<"LOAD_BALANCE"> + lit<"="> +
      (flag<&ast::DescriptionList::loadBalance>(ikw<"YES"> | ikw<"ON"> |
                                                     ikw<"TRUE">) |
       ikw<"NO"> | ikw<"OFF"> | ikw<"FALSE">) +
      lit<")">;
  static constexpr auto DlSourceRouteParam =
      lit<"("> + ikw<"SOURCE_ROUTE"> + lit<"="> +
      (flag<&ast::DescriptionList::sourceRoute>(ikw<"YES"> | ikw<"ON">) |
       ikw<"NO"> | ikw<"OFF">) +
      lit<")">;
  static constexpr auto DlParams =
      many(DlFailoverParam | DlLoadBalanceParam | DlSourceRouteParam);

  struct DescriptionList
      : rule<lit<"("> + ikw<"DESCRIPTION_LIST"> + lit<"="> + DlParams +
             some(append<&ast::DescriptionList::descriptions>(
                 ref<Description>)) +
             DlParams + lit<")">> {};

  struct Ifile
      : rule<ikw<"IFILE"> + lit<"="> +
             assign<&ast::Ifile::path>(ref<DQ_STRING> | ref<ISQ_STRING> |
                                       ref<IUQ_STRING>)> {};

  struct TnsEntry
      : rule<some(append<&ast::Entry::aliases>(ref<DottedName>), lit<",">) +
             lit<"="> +
             (assign<&ast::Entry::descriptionList>(ref<DescriptionList>) |
              assign<&ast::Entry::description>(ref<Description>))> {};

  struct LsnrEntry
      : rule<append<&ast::Entry::aliases>(ref<DottedName>) + lit<"="> +
             (assign<&ast::Entry::description>(ref<Description>) |
              assign<&ast::Entry::addressList>(ref<AddressList>) |
              some(append<&ast::Entry::addresses>(ref<Address>)))> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::items>(ref<TnsEntry>) |
                  append<&ast::Document::items>(ref<Ifile>) |
                  append<&ast::Document::items>(ref<LsnrEntry>))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = list<COMMENT>;
};

}  // namespace tnsnames
