// The tinymud language: its grammar. A command line typed at a MUD prompt --
// an @-command or a plain verb, one per line.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tinymud/core/ast.hpp"

namespace tinymud {

struct Tinymud : cursiv::GrammarSpec {
  struct WS : terminal<cr<R"( \t)">> {};

  // The continuation class includes a space: a name, description or message
  // is everything up to the next '=', ';' or line end, spaces included.
  struct STRING : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_. %,'">)> {};
  struct NUMBER : terminal<some(d), std::int64_t> {};
  struct EOL : terminal<option(lit<"\r">) + lit<"\n">> {};

  struct CommandRule;

  struct BootCommandRule
      : rule<kw<"@boot"> + assign<&ast::BootCommand::player>(ref<STRING>)> {};

  struct ChownCommandRule
      : rule<kw<"@chown"> + assign<&ast::ChownCommand::object>(ref<STRING>) +
             lit<"="> + assign<&ast::ChownCommand::player>(ref<STRING>)> {};

  struct CreateCommandRule
      : rule<kw<"@create"> + assign<&ast::CreateCommand::name>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::CreateCommand::cost>(ref<NUMBER>))> {};

  struct DescribeCommandRule
      : rule<(kw<"@describe"> | kw<"@desc">) +
             assign<&ast::DescribeCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::DescribeCommand::description>(
                        ref<STRING>))> {};

  struct DigCommandRule
      : rule<kw<"@dig"> + assign<&ast::DigCommand::name>(ref<STRING>)> {};

  struct DumpCommandRule
      : rule<create<ast::DumpCommand> + kw<"@dump">> {};

  struct FailCommandRule
      : rule<kw<"@fail"> + assign<&ast::FailCommand::name>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::FailCommand::message>(ref<STRING>))> {};

  struct FindCommandRule
      : rule<kw<"@find"> +
             option(assign<&ast::FindCommand::name>(ref<STRING>))> {};

  struct ForceCommandRule
      : rule<kw<"@force"> + assign<&ast::ForceCommand::player>(ref<STRING>) +
             lit<"="> +
             assign<&ast::ForceCommand::command>(ref<CommandRule>)> {};

  struct LinkCommandRule
      : rule<kw<"@link"> + assign<&ast::LinkCommand::object>(ref<STRING>) +
             lit<"="> +
             assign<&ast::LinkCommand::target>(ref<NUMBER> |
                                               ref<STRING>)> {};

  struct LockCommandRule
      : rule<kw<"@lock"> + assign<&ast::LockCommand::object>(ref<STRING>) +
             lit<"="> + assign<&ast::LockCommand::key>(ref<STRING>)> {};

  struct NameCommandRule
      : rule<kw<"@name"> + assign<&ast::NameCommand::object>(ref<STRING>) +
             lit<"="> + assign<&ast::NameCommand::name>(ref<STRING>) +
             option(assign<&ast::NameCommand::password>(ref<STRING>))> {};

  struct NewPasswordCommandRule
      : rule<kw<"@newpassword"> +
             assign<&ast::NewPasswordCommand::player>(ref<STRING>) +
             option(lit<"="> + assign<&ast::NewPasswordCommand::password>(
                                    ref<STRING>))> {};

  struct OfailCommandRule
      : rule<kw<"@ofail"> + assign<&ast::OfailCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::OfailCommand::message>(ref<STRING>))> {};

  struct OpenCommandRule
      : rule<kw<"@open"> +
             some(append<&ast::OpenCommand::directions>(ref<STRING>),
                  lit<";">) +
             option(lit<"="> +
                    assign<&ast::OpenCommand::number>(ref<NUMBER>))> {};

  struct OsuccessCommandRule
      : rule<(kw<"@osuccess"> | kw<"@osucc">) +
             assign<&ast::OsuccessCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::OsuccessCommand::message>(ref<STRING>))> {};

  struct PasswordCommandRule
      : rule<kw<"@password"> +
             assign<&ast::PasswordCommand::oldPassword>(ref<STRING>) +
             lit<"="> +
             assign<&ast::PasswordCommand::newPassword>(ref<STRING>)> {};

  struct PcreateCommandRule
      : rule<kw<"@pcreate"> +
             assign<&ast::PcreateCommand::name>(ref<STRING>)> {};

  struct SetCommandRule
      : rule<kw<"@set"> + assign<&ast::SetCommand::object>(ref<STRING>) +
             lit<"="> +
             option(flag<&ast::SetCommand::negate>(lit<"!">)) +
             assign<&ast::SetCommand::flag>(ref<NUMBER>)> {};

  struct ShutdownCommandRule
      : rule<create<ast::ShutdownCommand> + kw<"@shutdown">> {};

  struct StatsCommandRule
      : rule<kw<"@stats"> +
             assign<&ast::StatsCommand::player>(ref<STRING>)> {};

  struct SuccessCommandRule
      : rule<(kw<"@success"> | kw<"@succ">) +
             assign<&ast::SuccessCommand::object>(ref<STRING>) +
             option(lit<"="> +
                    assign<&ast::SuccessCommand::message>(ref<STRING>))> {};

  struct TeleportCommandRule
      : rule<kw<"@teleport"> +
             option(assign<&ast::TeleportCommand::object>(ref<STRING>) +
                    lit<"=">) +
             assign<&ast::TeleportCommand::room>(ref<STRING>)> {};

  struct ToadCommandRule
      : rule<kw<"@toad"> + assign<&ast::ToadCommand::player>(ref<STRING>)> {};

  struct UnlinkCommandRule
      : rule<kw<"@unlink"> +
             assign<&ast::UnlinkCommand::direction>(ref<STRING>)> {};

  struct UnlockCommandRule
      : rule<kw<"@unlock"> +
             assign<&ast::UnlockCommand::object>(ref<STRING>)> {};

  struct WallCommandRule
      : rule<kw<"@wall"> + assign<&ast::WallCommand::message>(ref<STRING>)> {
  };

  struct CommandRule
      : rule<ref<BootCommandRule> | ref<ChownCommandRule> |
             ref<CreateCommandRule> | ref<DescribeCommandRule> |
             ref<DigCommandRule> | ref<DumpCommandRule> |
             ref<FailCommandRule> | ref<FindCommandRule> |
             ref<ForceCommandRule> | ref<LinkCommandRule> |
             ref<LockCommandRule> | ref<NameCommandRule> |
             ref<NewPasswordCommandRule> | ref<OfailCommandRule> |
             ref<OpenCommandRule> | ref<OsuccessCommandRule> |
             ref<PasswordCommandRule> | ref<PcreateCommandRule> |
             ref<SetCommandRule> | ref<ShutdownCommandRule> |
             ref<StatsCommandRule> | ref<SuccessCommandRule> |
             ref<TeleportCommandRule> | ref<ToadCommandRule> |
             ref<UnlinkCommandRule> | ref<UnlockCommandRule> |
             ref<WallCommandRule>> {};

  struct DropActionRule
      : rule<(kw<"drop"> | kw<"throw">) +
             assign<&ast::DropAction::object>(ref<STRING>)> {};

  struct ExamineActionRule
      : rule<kw<"examine"> +
             assign<&ast::ExamineAction::object>(ref<STRING>)> {};

  struct GetActionRule
      : rule<(kw<"get"> | kw<"take">) +
             assign<&ast::GetAction::object>(ref<STRING>)> {};

  struct GiveActionRule
      : rule<kw<"give"> + assign<&ast::GiveAction::player>(ref<STRING>) +
             lit<"="> + assign<&ast::GiveAction::pennies>(ref<NUMBER>)> {};

  struct GotoActionRule
      : rule<(kw<"go"> | kw<"goto"> | kw<"move">) +
             assign<&ast::GotoAction::direction>(ref<STRING>)> {};

  struct GripeActionRule
      : rule<kw<"gripe"> +
             assign<&ast::GripeAction::message>(ref<STRING>)> {};

  struct HelpActionRule : rule<create<ast::HelpAction> + kw<"help">> {};

  struct InventoryActionRule
      : rule<create<ast::InventoryAction> +
             (kw<"inventory"> | kw<"inv">)> {};

  struct KillActionRule
      : rule<kw<"kill"> + assign<&ast::KillAction::player>(ref<STRING>) +
             lit<"="> + assign<&ast::KillAction::cost>(ref<NUMBER>)> {};

  struct LookActionRule
      : rule<(kw<"look"> | kw<"read">) +
             assign<&ast::LookAction::object>(ref<STRING>)> {};

  struct NewsActionRule : rule<create<ast::NewsAction> + kw<"news">> {};

  struct PageActionRule
      : rule<kw<"page"> + assign<&ast::PageAction::player>(ref<STRING>) +
             lit<"="> + assign<&ast::PageAction::message>(ref<STRING>)> {};

  struct QuitActionRule : rule<create<ast::QuitAction> + kw<"quit">> {};

  struct RobActionRule
      : rule<kw<"rob"> + assign<&ast::RobAction::player>(ref<STRING>)> {};

  struct SayActionRule
      : rule<kw<"say"> + assign<&ast::SayAction::message>(ref<STRING>)> {};

  struct ScoreActionRule : rule<create<ast::ScoreAction> + kw<"score">> {};

  struct WhisperActionRule
      : rule<kw<"whisper"> +
             assign<&ast::WhisperAction::player>(ref<STRING>) + lit<"="> +
             assign<&ast::WhisperAction::message>(ref<STRING>)> {};

  struct WhoActionRule
      : rule<kw<"who"> +
             option(assign<&ast::WhoAction::player>(ref<STRING>))> {};

  struct ActionRule
      : rule<ref<DropActionRule> | ref<ExamineActionRule> |
             ref<GetActionRule> | ref<GiveActionRule> | ref<GotoActionRule> |
             ref<GripeActionRule> | ref<HelpActionRule> |
             ref<InventoryActionRule> | ref<KillActionRule> |
             ref<LookActionRule> | ref<NewsActionRule> | ref<PageActionRule> |
             ref<QuitActionRule> | ref<RobActionRule> | ref<SayActionRule> |
             ref<ScoreActionRule> | ref<WhisperActionRule> |
             ref<WhoActionRule>> {};

  struct LineRule
      : rule<(ref<CommandRule> | ref<ActionRule>) + ref<EOL>> {};

  struct SessionRule
      : rule<create<ast::Session> +
             some(append<&ast::Session::lines>(ref<LineRule>)) +
             many(ref<EOL>)> {};

  using entry = SessionRule;
  using ignore = WS;
};

}  // namespace tinymud
