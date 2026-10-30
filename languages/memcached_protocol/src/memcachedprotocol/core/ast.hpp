// The memcachedprotocol language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace memcachedprotocol::ast {

struct Command : cursiv::Extends<cursiv::AstNode> {};

enum class StorageOp { set, add, replace, append, prepend, cas };

struct StorageCommand : cursiv::Extends<Command> {
  StorageOp op;
  string_view key;
  string_view flags;
  string_view exptime;
  string_view bytes;
  optional<string_view> casUnique;  // only op == cas
  boolean noreply;
};

struct RetrievalCommand : cursiv::Extends<Command> {
  string_view operation;
  optional<string_view> exptime;
  vector<string_view> keys;
};

struct DeleteCommand : cursiv::Extends<Command> {
  string_view key;
  optional<string_view> time;
  boolean noreply;
};

struct IncrementCommand : cursiv::Extends<Command> {
  string_view key;
  string_view amount;
  boolean noreply;
};

struct DecrementCommand : cursiv::Extends<Command> {
  string_view key;
  string_view amount;
  boolean noreply;
};

struct TouchCommand : cursiv::Extends<Command> {
  string_view key;
  string_view exptime;
  boolean noreply;
};

struct MetaCommand : cursiv::Extends<Command> {
  string_view operation;
  optional<string_view> key;
  optional<string_view> bytes;
  vector<string_view> flags;
};

struct StatisticsCommand : cursiv::Extends<Command> {
  optional<string_view> option;
  vector<string_view> arguments;
};

struct FlushCommand : cursiv::Extends<Command> {
  optional<string_view> delay;
  boolean noreply;
};

struct VersionCommand : cursiv::Extends<Command> {
  vector<string_view> arguments;
};

struct VerbosityCommand : cursiv::Extends<Command> {
  string_view level;
  boolean noreply;
};

struct QuitCommand : cursiv::Extends<Command> {
  vector<string_view> arguments;
};
struct AdministrationCommand : cursiv::Extends<Command> {
  string_view operation;
  vector<string_view> arguments;
};

struct CommandLine : cursiv::Extends<cursiv::AstNode> {
  vector<Command*> commands;
};

}  // namespace memcachedprotocol::ast
