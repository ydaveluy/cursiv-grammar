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
using Unquoted = cursiv::text_conv<strip_quotes>;

inline constexpr auto strip_trailing_newline = [](std::string_view t) {
  const auto begin = t.find_first_not_of(" \t\r\n");
  const auto end = t.find_last_not_of(" \t\r\n");
  return begin == t.npos ? std::string_view{} : t.substr(begin, end - begin + 1);
};
using Unnewlined = cursiv::text_conv<strip_trailing_newline>;

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
  struct QUOTED_INT : terminal<lit<"\""> + some(d) + lit<"\"">, Unquoted> {};
  struct IntegerValue : rule<ref<INT> | ref<QUOTED_INT>> {};
  static constexpr auto DurationText = some(d) + option(lit<"."> + some(d)) +
      option((cr<"mM"> + cr<"sS">) |
             (cr<"sS"> + cr<"eE"> + cr<"cC">) |
             (cr<"mM"> + cr<"iI"> + cr<"nN">));
  struct DURATION : terminal<DurationText> {};
  struct QUOTED_DURATION
      : terminal<lit<"\""> + DurationText + lit<"\"">> {};
  struct DurationValue
      : rule<ref<DURATION> | ref<QUOTED_DURATION>, text_conv<duration_seconds>> {};

  struct DQ_STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};
  struct ISQ_STRING : terminal<lit<"'"> + many(cr<"^'">) + lit<"'">, Unquoted> {};
  struct IUQ_STRING
      : terminal<cr<R"(^"'=\r\n# \t)"> +
                 many(cr<R"(^"'=\r\n#)">), Unnewlined> {};

  // Descriptor values retain punctuation (including IPv6 colons); only
  // double quotes delimit a quoted descriptor value in the native reader.
  struct VALUE : terminal<some(cr<R"(^ \t\r\n()="#)">)> {};
  struct ScalarValue : rule<option(ref<DQ_STRING> | ref<VALUE>)> {};

  // `ID ('.' ID)*` joined: written tight it is already one ID, written
  // spaced this rule reassembles the same text (examples/ini's DottedKey).
  struct DottedName : rule<some(ref<ID>, lit<".">)> {};

  struct SessionKw : terminal<ikw<"SESSION">, value<ast::FailoverType::session>> {};
  struct SelectKw : terminal<ikw<"SELECT">, value<ast::FailoverType::select_>> {};
  struct NoneKw : terminal<ikw<"NONE">, value<ast::FailoverType::none>> {};
  struct BasicKw : terminal<ikw<"BASIC">, value<ast::FailoverMethod::basic>> {};
  struct PreconnectKw
      : terminal<ikw<"PRECONNECT">, value<ast::FailoverMethod::preconnect>> {};
  struct DedicatedKw
      : terminal<ikw<"DEDICATED">, value<ast::ServerType::dedicated>> {};
  struct SharedKw : terminal<ikw<"SHARED">, value<ast::ServerType::shared_>> {};
  struct PooledKw : terminal<ikw<"POOLED">, value<ast::ServerType::pooled>> {};

  static constexpr auto TcpHostParam =
      lit<"("> + ikw<"HOST"> + lit<"="> +
      append<&ast::TcpProtocol::hosts>(ref<ScalarValue>) + lit<")">;
  static constexpr auto TcpPortParam =
      lit<"("> + ikw<"PORT"> + lit<"="> +
      append<&ast::TcpProtocol::ports>(ref<IntegerValue>) + lit<")">;
  static constexpr auto TcpProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> +
      (ikw<"TCP"> | flag<&ast::TcpProtocol::secure>(ikw<"TCPS">)) + lit<")">;
  struct TcpProtocolRule
      : rule<create<ast::TcpProtocol> +
             some(TcpHostParam | TcpPortParam | TcpProtocolMarker)> {};

  static constexpr auto IpcKeyParam =
      lit<"("> + ikw<"KEY"> + lit<"="> +
      append<&ast::IpcProtocol::keys>(ref<ScalarValue>) + lit<")">;
  static constexpr auto IpcProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"IPC"> + lit<")">;
  struct IpcProtocolRule
      : rule<create<ast::IpcProtocol> + some(IpcKeyParam | IpcProtocolMarker)> {
  };

  static constexpr auto SpxServiceParam =
      lit<"("> + ikw<"SERVICE"> + lit<"="> +
      append<&ast::SpxProtocol::services>(ref<ScalarValue>) + lit<")">;
  static constexpr auto SpxProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"SPX"> + lit<")">;
  struct SpxProtocolRule
      : rule<create<ast::SpxProtocol> + some(SpxServiceParam | SpxProtocolMarker)> {
  };

  static constexpr auto NmpServerParam =
      lit<"("> + ikw<"SERVER"> + lit<"="> +
      append<&ast::NmpProtocol::servers>(ref<ScalarValue>) + lit<")">;
  static constexpr auto NmpPipeParam =
      lit<"("> + ikw<"PIPE"> + lit<"="> +
      append<&ast::NmpProtocol::pipes>(ref<ScalarValue>) + lit<")">;
  static constexpr auto NmpProtocolMarker =
      lit<"("> + ikw<"PROTOCOL"> + lit<"="> + ikw<"NMP"> + lit<")">;
  struct NmpProtocolRule
      : rule<create<ast::NmpProtocol> +
             some(NmpServerParam | NmpPipeParam | NmpProtocolMarker)> {};

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
  struct BeqProtocolRule
      : rule<create<ast::BeqProtocol> +
             some(BeqProgramParam | BeqArgv0Param | BeqArgsParam |
                  BeqProtocolMarker)> {};

  static constexpr auto ProtocolInfo =
      ref<TcpProtocolRule> | ref<IpcProtocolRule> | ref<SpxProtocolRule> |
      ref<NmpProtocolRule> | ref<BeqProtocolRule>;

  static constexpr auto ASendBufParam =
      lit<"("> + ikw<"SEND_BUF_SIZE"> + lit<"="> +
      append<&ast::Address::sendBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto ARecvBufParam =
      lit<"("> + ikw<"RECV_BUF_SIZE"> + lit<"="> +
      append<&ast::Address::recvBufs>(ref<IntegerValue>) + lit<")">;
  static constexpr auto AParams = many(ASendBufParam | ARecvBufParam);

  struct AddressRule
      : rule<lit<"("> + ikw<"ADDRESS"> + lit<"="> +
             assign<&ast::Address::protocol>(ProtocolInfo) + AParams +
             lit<")">> {};

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

  struct AddressListRule
      : rule<lit<"("> + ikw<"ADDRESS_LIST"> + lit<"="> + AlParams +
             some(append<&ast::AddressList::addresses>(ref<AddressRule>)) +
             AlParams + lit<")">> {};

  static constexpr auto FoTypeParam =
      lit<"("> + ikw<"TYPE"> + lit<"="> +
      append<&ast::FailoverMode::types>(ref<SessionKw> | ref<SelectKw> |
                                        ref<NoneKw>) +
      lit<")">;
  static constexpr auto FoBackupParam =
      lit<"("> + ikw<"BACKUP"> + lit<"="> +
      append<&ast::FailoverMode::backups>(ref<ScalarValue>) + lit<")">;
  static constexpr auto FoMethodParam =
      lit<"("> + ikw<"METHOD"> + lit<"="> +
      append<&ast::FailoverMode::methods>(ref<BasicKw> | ref<PreconnectKw>) +
      lit<")">;
  static constexpr auto FoRetriesParam =
      lit<"("> + ikw<"RETRIES"> + lit<"="> +
      append<&ast::FailoverMode::retries>(ref<IntegerValue>) + lit<")">;
  static constexpr auto FoDelayParam =
      lit<"("> + ikw<"DELAY"> + lit<"="> +
      append<&ast::FailoverMode::delays>(ref<IntegerValue>) + lit<")">;
  struct FailoverModeRule
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
      append<&ast::ConnectData::failoverModes>(ref<FailoverModeRule>) +
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
      append<&ast::ConnectData::servers>(ref<DedicatedKw> | ref<SharedKw> |
                                         ref<PooledKw>) +
      lit<")">;
  static constexpr auto CdUrParam =
      lit<"("> + ikw<"UR"> + lit<"="> + flag<&ast::ConnectData::ur>(ikw<"A">) +
      lit<")">;
  struct ConnectDataRule
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
      assign<&ast::Description::addressList>(ref<AddressListRule>) |
      some(append<&ast::Description::addresses>(ref<AddressRule>));

  // CONNECT_DATA may precede or follow the addresses, and listener
  // descriptions may carry the same description parameters without it.
  struct DescriptionRule
      : rule<lit<"("> + ikw<"DESCRIPTION"> + lit<"="> + DParams +
             ((assign<&ast::Description::connectData>(ref<ConnectDataRule>) +
               DParams + DescriptionAddresses) |
              (DescriptionAddresses + DParams +
               option(assign<&ast::Description::connectData>(ref<ConnectDataRule>)))) +
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

  struct DescriptionListRule
      : rule<lit<"("> + ikw<"DESCRIPTION_LIST"> + lit<"="> + DlParams +
             some(append<&ast::DescriptionList::descriptions>(
                 ref<DescriptionRule>)) +
             DlParams + lit<")">> {};

  struct IfileRule
      : rule<ikw<"IFILE"> + lit<"="> +
             assign<&ast::Ifile::path>(ref<DQ_STRING> | ref<ISQ_STRING> |
                                       ref<IUQ_STRING>)> {};

  struct TnsEntryRule
      : rule<some(append<&ast::Entry::aliases>(ref<DottedName>), lit<",">) +
             lit<"="> +
             (assign<&ast::Entry::descriptionList>(ref<DescriptionListRule>) |
              assign<&ast::Entry::description>(ref<DescriptionRule>))> {};

  struct LsnrEntryRule
      : rule<append<&ast::Entry::aliases>(ref<DottedName>) + lit<"="> +
             (assign<&ast::Entry::description>(ref<DescriptionRule>) |
              assign<&ast::Entry::addressList>(ref<AddressListRule>) |
              some(append<&ast::Entry::addresses>(ref<AddressRule>)))> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::items>(ref<TnsEntryRule>) |
                  append<&ast::Document::items>(ref<IfileRule>) |
                  append<&ast::Document::items>(ref<LsnrEntryRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = list<COMMENT>;
};

}  // namespace tnsnames
