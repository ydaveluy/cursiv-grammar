// The tnsnames language: its AST. A DESCRIPTION shared by a service entry
// and a listener entry is one class -- connectData is null for the latter.
// A parameter may be written again: its list keeps every value in order, the
// last one redefining the others.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tnsnames::ast {

enum class ServerType : std::uint8_t { dedicated, shared_, pooled };
enum class FailoverType : std::uint8_t { session, select_, none };
enum class FailoverMethod : std::uint8_t { basic, preconnect };

struct Protocol : cursiv::Extends<cursiv::AstNode> {
  vector<uint64> sendBufs;
  vector<uint64> recvBufs;
};

struct TcpProtocol : cursiv::Extends<Protocol> {
  boolean secure;  // TCPS; false for TCP.
  vector<string_view> hosts;
  vector<uint64> ports;
};

struct IpcProtocol : cursiv::Extends<Protocol> {
  vector<string_view> keys;
};

struct SpxProtocol : cursiv::Extends<Protocol> {
  vector<string_view> services;
};

struct NmpProtocol : cursiv::Extends<Protocol> {
  vector<string_view> servers;
  vector<string_view> pipes;
};

// ARGS' single-quoted nested description only ever carries these two
// markers, so they are flattened rather than mirrored as a nested tree.
struct BeqProtocol : cursiv::Extends<Protocol> {
  vector<string_view> programs;
  vector<string_view> argv0;
  boolean argsLocal;
  boolean argsHasAddress;
};

struct Address : cursiv::Extends<cursiv::AstNode> {
  Protocol* protocol;
};

struct AddressList : cursiv::Extends<cursiv::AstNode> {
  boolean failover;
  boolean loadBalance;
  boolean sourceRoute;
  vector<Address*> addresses;
};

struct FailoverMode : cursiv::Extends<cursiv::AstNode> {
  vector<FailoverType> types;
  vector<string_view> backups;
  vector<FailoverMethod> methods;
  vector<uint64> retries;
  vector<uint64> delays;
};

struct ConnectData : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> serviceNames;
  vector<string_view> sids;
  vector<string_view> instanceNames;
  vector<FailoverMode*> failoverModes;
  vector<string_view> globalNames;
  boolean hs;  // (HS=OK): presence is the fact, OK is the only legal value
  vector<string_view> rdbDatabases;
  vector<ServerType> servers;
  boolean ur;  // (UR=A): presence is the fact, A is the only legal value
};

// Built by two different rules: one requires connectData (a service), the
// other never touches it (a listener wrapped in its own DESCRIPTION).
struct Description : cursiv::Extends<cursiv::AstNode> {
  boolean enableBroken;
  boolean failover;
  boolean loadBalance;
  boolean sourceRoute;
  vector<uint64> sdus;
  vector<uint64> recvBufs;
  vector<uint64> sendBufs;
  vector<string_view> serviceTypes;
  vector<string_view> sslServerCertDns;
  vector<uint64> connectTimeouts;
  vector<uint64> retryCounts;
  vector<uint64> retryDelays;
  vector<double> transportConnectTimeouts;  // Seconds, including ms/sec/min input.
  AddressList* addressList;
  vector<Address*> addresses;
  ConnectData* connectData;
};

struct DescriptionList : cursiv::Extends<cursiv::AstNode> {
  boolean failover;
  boolean loadBalance;
  boolean sourceRoute;
  vector<Description*> descriptions;
};

struct Ifile : cursiv::Extends<cursiv::AstNode> {
  string_view path;
};

// One name=value entry: a service (descriptionList or description), or a
// listener (description, addressList or a bare run of addresses).
struct Entry : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> aliases;
  DescriptionList* descriptionList;
  Description* description;
  AddressList* addressList;
  vector<Address*> addresses;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Entry*, Ifile*>> items;
};

}  // namespace tnsnames::ast
