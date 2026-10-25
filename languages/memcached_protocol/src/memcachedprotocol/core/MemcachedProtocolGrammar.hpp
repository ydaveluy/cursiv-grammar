// The memcachedprotocol language: its grammar.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "memcachedprotocol/core/ast.hpp"

namespace memcachedprotocol {

struct MemcachedProtocol : cursiv::GrammarSpec {
  struct WS : terminal<some(lit<" ">)> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct KEY : terminal<some(cr<R"(^ \r\n\x00)">)> {};

  struct INTEGER : terminal<option(cr<"+-">) + some(d)> {};

  struct SET : terminal<kw<"set">, value<ast::StorageOp::set>> {};
  struct ADD : terminal<kw<"add">, value<ast::StorageOp::add>> {};
  struct REPLACE : terminal<kw<"replace">, value<ast::StorageOp::replace>> {};
  struct APPEND : terminal<kw<"append">, value<ast::StorageOp::append>> {};
  struct PREPEND : terminal<kw<"prepend">, value<ast::StorageOp::prepend>> {};
  struct CAS : terminal<kw<"cas">, value<ast::StorageOp::cas>> {};

  static constexpr auto storageName =
      ref<SET> | ref<ADD> | ref<REPLACE> | ref<APPEND> | ref<PREPEND>;

  static constexpr auto noreply =
      option(flag<&ast::StorageCommand::noreply>(kw<"noreply">));

  struct StorageCommand
      : rule<((assign<&ast::StorageCommand::op>(storageName) +
               assign<&ast::StorageCommand::key>(ref<KEY>) +
               assign<&ast::StorageCommand::flags>(ref<INTEGER>) +
               assign<&ast::StorageCommand::exptime>(ref<INTEGER>) +
               assign<&ast::StorageCommand::bytes>(ref<INTEGER>)) |
              (assign<&ast::StorageCommand::op>(ref<CAS>) +
               assign<&ast::StorageCommand::key>(ref<KEY>) +
               assign<&ast::StorageCommand::flags>(ref<INTEGER>) +
               assign<&ast::StorageCommand::exptime>(ref<INTEGER>) +
               assign<&ast::StorageCommand::bytes>(ref<INTEGER>) +
               assign<&ast::StorageCommand::casUnique>(ref<INTEGER>))) +
             noreply> {};

  struct RetrievalCommand
      : rule<((assign<&ast::RetrievalCommand::operation>(
                   kw<"get"> | kw<"gets"> | kw<"bget">)) |
              (assign<&ast::RetrievalCommand::operation>(kw<"gat"> | kw<"gats">) +
               assign<&ast::RetrievalCommand::exptime>(ref<INTEGER>))) +
             some(append<&ast::RetrievalCommand::keys>(ref<KEY>))> {};

  struct DeleteCommand
      : rule<kw<"delete"> + assign<&ast::DeleteCommand::key>(ref<KEY>) +
             option(assign<&ast::DeleteCommand::time>(ref<INTEGER>)) +
             option(flag<&ast::DeleteCommand::noreply>(kw<"noreply">))> {};

  struct IncrementCommand
      : rule<kw<"incr"> + assign<&ast::IncrementCommand::key>(ref<KEY>) +
             assign<&ast::IncrementCommand::amount>(ref<INTEGER>) +
             option(
                 flag<&ast::IncrementCommand::noreply>(kw<"noreply">))> {};

  struct DecrementCommand
      : rule<kw<"decr"> + assign<&ast::DecrementCommand::key>(ref<KEY>) +
             assign<&ast::DecrementCommand::amount>(ref<INTEGER>) +
             option(
                 flag<&ast::DecrementCommand::noreply>(kw<"noreply">))> {};

  struct TouchCommand
      : rule<kw<"touch"> + assign<&ast::TouchCommand::key>(ref<KEY>) +
             assign<&ast::TouchCommand::exptime>(ref<INTEGER>) +
             option(flag<&ast::TouchCommand::noreply>(kw<"noreply">))> {};

  struct MetaCommand
      : rule<((assign<&ast::MetaCommand::operation>(
                   kw<"mg"> | kw<"md"> | kw<"ma"> | kw<"me">) +
               assign<&ast::MetaCommand::key>(ref<KEY>)) |
              (assign<&ast::MetaCommand::operation>(kw<"ms">) +
               assign<&ast::MetaCommand::key>(ref<KEY>) +
               assign<&ast::MetaCommand::bytes>(ref<INTEGER>)) |
              assign<&ast::MetaCommand::operation>(kw<"mn">)) +
             many(append<&ast::MetaCommand::flags>(ref<KEY>))> {};

  struct StatisticsCommand
      : rule<kw<"stats"> + option(
                 assign<&ast::StatisticsCommand::option>(ref<KEY>) +
                 many(append<&ast::StatisticsCommand::arguments>(ref<KEY>)))> {};

  struct FlushCommand
      : rule<kw<"flush_all"> +
             option(assign<&ast::FlushCommand::delay>(ref<INTEGER>)) +
             option(flag<&ast::FlushCommand::noreply>(kw<"noreply">))> {};

  struct VersionCommand : rule<create<ast::VersionCommand>(kw<"version">)> {};

  struct VerbosityCommand
      : rule<kw<"verbosity"> +
             assign<&ast::VerbosityCommand::level>(ref<INTEGER>) +
             option(flag<&ast::VerbosityCommand::noreply>(kw<"noreply">))> {};

  struct QuitCommand : rule<create<ast::QuitCommand>(kw<"quit">)> {};

  struct Command : rule<ref<TouchCommand> | ref<MetaCommand> | ref<StorageCommand>      //
                        | ref<RetrievalCommand>  //
                        | ref<DeleteCommand>     //
                        | ref<IncrementCommand>  //
                        | ref<DecrementCommand>  //
                        | ref<StatisticsCommand> //
                        | ref<FlushCommand>      //
                        | ref<VersionCommand>    //
                        | ref<VerbosityCommand>  //
                        | ref<QuitCommand>> {};

  struct CommandLine
      : rule<many(ref<EOL>) +
             many(append<&ast::CommandLine::commands>(ref<Command>) +
                  (some(ref<EOL>) | eof))> {};

  using entry = CommandLine;
  using ignore = WS;
};

} // namespace memcachedprotocol
