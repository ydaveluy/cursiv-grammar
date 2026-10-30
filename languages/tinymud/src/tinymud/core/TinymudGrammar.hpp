// The tinymud language: its grammar. A command line typed at a MUD prompt --
// an @-command or a plain verb, one per line.
#pragma once

#include <cstdint>
#include <charconv>
#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinymud/core/ast.hpp"

namespace tinymud {

inline constexpr auto normalized = [](std::string_view text, std::pmr::memory_resource* memory) {
  const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f'; };
  while (!text.empty() && space(text[0])) text.remove_prefix(1);
  while (!text.empty() && space(text.back())) text.remove_suffix(1);
  bool changed = false;
  for (std::size_t i = 0; i < text.size(); ++i)
    changed |= space(text[i]) && (text[i] != ' ' || (i && space(text[i - 1])));
  if (!changed) return text;
  const auto room = cursiv::text_room(memory, text.size());
  std::size_t size = 0;
  for (const char c : text) {
    if (space(c)) { if (size && room[size - 1] != ' ') room[size++] = ' '; }
    else room[size++] = c;
  }
  return std::string_view(room.data(), size);
};
inline constexpr auto dbref = [](std::string_view text) -> cursiv::ConversionResult<std::int64_t> {
  if (text.starts_with('#')) text.remove_prefix(1);
  if (text.empty()) return cursiv::ConversionResult<std::int64_t>::err("Expected an object reference number");
  std::int64_t value = 0;
  const auto r = std::from_chars(text.data(), text.data() + text.size(), value);
  if (r.ec != std::errc{} || r.ptr != text.data() + text.size())
    return cursiv::ConversionResult<std::int64_t>::err("Object reference is outside signed 64-bit range");
  return cursiv::ConversionResult<std::int64_t>::ok(value);
};


struct Tinymud : cursiv::GrammarSpec {
  struct WS : terminal<cr<R"( \t)">> {};

  // First arguments, free remainders and exit aliases have distinct delimiters.
  static constexpr auto ArgGap = gap::has(cr<R"( \t)">);
  struct STRING : terminal<as<normalized>(cr<R"(^= \t\v\f\r\n)"> + many(cr<R"(^=\r\n)">))> {};
  struct TAIL : terminal<as<normalized>(cr<R"(^ \t\v\f\r\n)"> + many(cr<R"(^\r\n)">))> {};
  struct ALIAS : terminal<as<normalized>(cr<R"(^;= \t\v\f\r\n)"> + many(cr<R"(^;=\r\n)">))> {};
  struct FLAG_NAME : terminal<some(cr<"a-zA-Z_">)> {};
  struct NUMBER : terminal<as<std::int64_t>(some(d))> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct DBREF : terminal<as<dbref>(option(lit<"#">) + option(lit<"-">) + some(d))> {};
  struct TargetName : rule<!ref<DBREF> + ref<STRING>> {};
  struct Command;
  struct Action;
  struct Instruction;

  struct BootCommand
      : rule<ikw<"@boot"> + ArgGap + assign<&ast::BootCommand::player>(ref<STRING>)> {};

  struct ChownCommand
      : rule<ikw<"@chown"> + ArgGap + assign<&ast::ChownCommand::object>(ref<STRING>) +
             lit<"="> + assign<&ast::ChownCommand::player>(ref<STRING>)> {};

  struct CreateCommand
      : rule<ikw<"@create"> + ArgGap + assign<&ast::CreateCommand::name>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::CreateCommand::cost>(ref<NUMBER>))> {};

  struct DescribeCommand
      : rule<(ikw<"@describe"> | ikw<"@desc">) + ArgGap +
             assign<&ast::DescribeCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    option(assign<&ast::DescribeCommand::description>(ref<TAIL>)))> {};

  struct DigCommand
      : rule<ikw<"@dig"> + ArgGap + assign<&ast::DigCommand::name>(ref<STRING>)> {};

  struct DumpCommand
      : rule<create<ast::DumpCommand> + ikw<"@dump">> {};

  struct FailCommand
      : rule<kw<"@fail"> + ArgGap + assign<&ast::FailCommand::name>(ref<STRING>) +
             option(lit<"="> +
                    option(assign<&ast::FailCommand::message>(ref<TAIL>)))> {};

  struct FindCommand
      : rule<kw<"@find"> +
             option(ArgGap + assign<&ast::FindCommand::name>(ref<STRING>))> {};

  struct ForceCommand
      : rule<kw<"@force"> + ArgGap + assign<&ast::ForceCommand::player>(ref<STRING>) +
             lit<"="> +
             assign<&ast::ForceCommand::command>(ref<Instruction>)> {};

  struct LinkCommand
      : rule<ikw<"@link"> + ArgGap + assign<&ast::LinkCommand::object>(ref<STRING>) +
             lit<"="> +
             assign<&ast::LinkCommand::target>(ref<DBREF> | ref<TargetName>)> {};

  struct LockCommand
      : rule<ikw<"@lock"> + ArgGap + assign<&ast::LockCommand::object>(ref<STRING>) +
             lit<"="> + assign<&ast::LockCommand::key>(ref<TAIL>)> {};

  struct NameCommand : rule<ikw<"@name"> + ArgGap + assign<&ast::NameCommand::object>(ref<STRING>) +
      lit<"="> + assign<&ast::NameCommand::name>(ref<TAIL>)> {};

  struct NewPasswordCommand
      : rule<kw<"@newpassword"> + ArgGap +
             assign<&ast::NewPasswordCommand::player>(ref<STRING>) +
             option(lit<"="> + option(assign<&ast::NewPasswordCommand::password>(ref<TAIL>)))> {};

  struct OfailCommand
      : rule<ikw<"@ofail"> + ArgGap + assign<&ast::OfailCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    option(assign<&ast::OfailCommand::message>(ref<TAIL>)))> {};

  struct OpenCommand
      : rule<ikw<"@open"> + ArgGap +
             some(append<&ast::OpenCommand::directions>(ref<ALIAS>),
                  lit<";">) +
             option(lit<"="> +
                    (assign<&ast::OpenCommand::number>(ref<DBREF>) | assign<&ast::OpenCommand::destination>(ref<TargetName>)))> {};

  struct OsuccessCommand
      : rule<(ikw<"@osuccess"> | ikw<"@osucc">) + ArgGap +
             assign<&ast::OsuccessCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    option(assign<&ast::OsuccessCommand::message>(ref<TAIL>)))> {};

  struct PasswordCommand
      : rule<ikw<"@password"> + ArgGap +
             assign<&ast::PasswordCommand::oldPassword>(ref<STRING>) +
             lit<"="> +
             assign<&ast::PasswordCommand::newPassword>(ref<TAIL>)> {};

  struct PcreateCommand
      : rule<ikw<"@pcreate"> + ArgGap +
             assign<&ast::PcreateCommand::name>(ref<STRING>) + option(lit<"="> + assign<&ast::PcreateCommand::password>(ref<TAIL>))> {};

  struct SetCommand
      : rule<ikw<"@set"> + ArgGap + assign<&ast::SetCommand::object>(ref<STRING>) +
             lit<"="> +
             option(flag<&ast::SetCommand::negate>(some(lit<"!">))) +
             (assign<&ast::SetCommand::flagName>(ref<FLAG_NAME>) | assign<&ast::SetCommand::flag>(ref<NUMBER>))> {};

  struct ShutdownCommand
      : rule<create<ast::ShutdownCommand> + kw<"@shutdown">> {};

  struct StatsCommand
      : rule<ikw<"@stats"> +
             option(ArgGap + assign<&ast::StatsCommand::player>(ref<STRING>))> {};

  struct SuccessCommand
      : rule<(ikw<"@success"> | ikw<"@succ">) + ArgGap +
             assign<&ast::SuccessCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    option(assign<&ast::SuccessCommand::message>(ref<TAIL>)))> {};

  struct TeleportCommand
      : rule<ikw<"@teleport"> + ArgGap +
             option(assign<&ast::TeleportCommand::object>(ref<STRING>) +
                    lit<"=">) +
             assign<&ast::TeleportCommand::room>(ref<STRING>)> {};

  struct ToadCommand
      : rule<ikw<"@toad"> + ArgGap + assign<&ast::ToadCommand::player>(ref<STRING>) + option(lit<"="> + assign<&ast::ToadCommand::name>(ref<TAIL>))> {};

  struct UnlinkCommand
      : rule<ikw<"@unlink"> + ArgGap +
             assign<&ast::UnlinkCommand::direction>(ref<STRING>)> {};

  struct UnlockCommand
      : rule<ikw<"@unlock"> + ArgGap +
             assign<&ast::UnlockCommand::object>(ref<STRING>)> {};

  struct WallCommand
      : rule<kw<"@wall"> + ArgGap + assign<&ast::WallCommand::message>(ref<TAIL>)> {
  };

  struct Command
      : rule<ref<BootCommand> | ref<ChownCommand> |
             ref<CreateCommand> | ref<DescribeCommand> |
             ref<DigCommand> | ref<DumpCommand> |
             ref<FailCommand> | ref<FindCommand> |
             ref<ForceCommand> | ref<LinkCommand> |
             ref<LockCommand> | ref<NameCommand> |
             ref<NewPasswordCommand> | ref<OfailCommand> |
             ref<OpenCommand> | ref<OsuccessCommand> |
             ref<PasswordCommand> | ref<PcreateCommand> |
             ref<SetCommand> | ref<ShutdownCommand> |
             ref<StatsCommand> | ref<SuccessCommand> |
             ref<TeleportCommand> | ref<ToadCommand> |
             ref<UnlinkCommand> | ref<UnlockCommand> |
             ref<WallCommand>> {};

  struct DropAction
      : rule<(ikw<"drop"> | ikw<"throw">) + ArgGap +
             assign<&ast::DropAction::object>(ref<STRING>)> {};

  struct ExamineAction
      : rule<ikw<"examine"> + ArgGap +
             assign<&ast::ExamineAction::object>(ref<STRING>)> {};

  struct GetAction
      : rule<(ikw<"get"> | ikw<"take">) + ArgGap +
             assign<&ast::GetAction::object>(ref<STRING>)> {};

  struct GiveAction
      : rule<ikw<"give"> + ArgGap + assign<&ast::GiveAction::player>(ref<STRING>) +
             lit<"="> + assign<&ast::GiveAction::pennies>(ref<NUMBER>)> {};

  struct GotoAction
      : rule<(ikw<"go"> | ikw<"goto"> | ikw<"move">) + ArgGap +
             assign<&ast::GotoAction::direction>(ref<STRING>)> {};

  struct GripeAction
      : rule<ikw<"gripe"> + ArgGap +
             assign<&ast::GripeAction::message>(ref<TAIL>)> {};

  struct HelpAction : rule<create<ast::HelpAction> + ikw<"help">> {};

  struct InventoryAction
      : rule<create<ast::InventoryAction> +
             (ikw<"inventory"> | ikw<"inv">)> {};

  struct KillAction
      : rule<ikw<"kill"> + ArgGap + assign<&ast::KillAction::player>(ref<STRING>) +
             option(flag<&ast::KillAction::hasCost>(lit<"=">) + assign<&ast::KillAction::cost>(ref<NUMBER>))> {};

  struct LookAction
      : rule<(ikw<"look"> | ikw<"read">) +
             option(ArgGap + assign<&ast::LookAction::object>(ref<STRING>))> {};

  struct NewsAction : rule<create<ast::NewsAction> + ikw<"news">> {};

  struct PageAction
      : rule<ikw<"page"> + ArgGap + assign<&ast::PageAction::player>(ref<STRING>) +
             option(lit<"="> + option(assign<&ast::PageAction::message>(ref<TAIL>)))> {};

  struct QuitAction : rule<create<ast::QuitAction> + ikw<"quit">> {};

  struct RobAction
      : rule<ikw<"rob"> + ArgGap + assign<&ast::RobAction::player>(ref<STRING>)> {};

  struct SayAction
      : rule<ikw<"say"> + ArgGap + assign<&ast::SayAction::message>(ref<TAIL>)> {};

  struct ScoreAction : rule<create<ast::ScoreAction> + ikw<"score">> {};

  struct WhisperAction
      : rule<ikw<"whisper"> + ArgGap +
             assign<&ast::WhisperAction::player>(ref<STRING>) + lit<"="> +
             assign<&ast::WhisperAction::message>(ref<TAIL>)> {};

  struct WhoAction
      : rule<ikw<"who"> +
             option(ArgGap + assign<&ast::WhoAction::player>(ref<STRING>))> {};

  struct Action
      : rule<ref<DropAction> | ref<ExamineAction> |
             ref<GetAction> | ref<GiveAction> | ref<GotoAction> |
             ref<GripeAction> | ref<HelpAction> |
             ref<InventoryAction> | ref<KillAction> |
             ref<LookAction> | ref<NewsAction> | ref<PageAction> |
             ref<QuitAction> | ref<RobAction> | ref<SayAction> |
             ref<ScoreAction> | ref<WhisperAction> |
             ref<WhoAction>> {};

  struct Instruction : rule<ref<Command> | ref<Action>> {};
  struct Line : rule<ref<Instruction> + ref<EOL>> {};

  struct Session
      : rule<create<ast::Session> +
             many(ref<EOL> | append<&ast::Session::lines>(ref<Line>))> {};

  using entry = Session;
  using ignored = WS;
};

}  // namespace tinymud
