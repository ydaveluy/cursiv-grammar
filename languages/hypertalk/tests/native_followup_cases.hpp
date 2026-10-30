#pragma once
#include <cstdio>
#include <string>
#include "cursiv/test/ValidationHelper.hpp"
#include "hypertalk/core/CoreServices.hpp"
#include "hypertalk/core/ast.hpp"

namespace hypertalk_native_followup {
inline bool run(const hypertalk::HypertalkCoreServices& services) {
  const std::string source =
      "on Native\n"
      "  -- a pipe | in a comment is still a comment\n"
      "  wait 7\n"
      "  wait for 8 ticks\n"
      "  send \"ping\"\n"
      "  read from file \"Data\" at 2 until return\n"
      "  push card 3\n"
      "  put true\n"
      "  put false\n"
      "  put date()\n"
      "  put time()\n"
      "  convert it to long date\n"
      "  convert it to English date\n"
      "  ask password clear \"Secret\" with \"default\"\n"
      "  answer file \"Choose\" of type \"TEXT\"\n"
      "  answer program \"Choose\" of type \"APPL\"\n"
      "  unlock screen with dissolve\n"
      "  lock messages\n"
      "  unlock error dialogs\n"
      "  do field 1 as AppleScript\n"
      "  dial \"555\" with modem \"AT\"\n"
      "  doMenu \"Open\", \"File\" without dialog with commandKey\n"
      "  pop card into savedCard\n"
      "  reset printing\n"
      "  show field 1 at 10,20\n"
      "  show all cards\n"
      "  go next\n"
      "end native\n";
  const auto result = services.parser->parse(source);
  if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() || !result.value) {
    std::fprintf(stderr, "hypertalk native: clean native script expected\n"); return false;
  }
  bool defaultTicks=false, explicitTicks=false, send=false, read=false, push=false;
  bool trueValue=false, falseValue=false, date=false, time=false, longDate=false, englishDate=false;
  bool password=false, fileAnswer=false, programAnswer=false, visual=false, messages=false, errors=false;
  bool doLanguage=false, modem=false, menu=false, pop=false, printing=false, showAt=false, allCards=false, go=false;
  for (const auto* node : result.nodes) {
    if (const auto* wait = cursiv::ast_cast<hypertalk::ast::WaitCmd>(node)) {
      const auto* amount = cursiv::ast_cast<hypertalk::ast::NumberLit>(wait->amount);
      defaultTicks |= amount && amount->value == 7 && !wait->unit;
      explicitTicks |= amount && amount->value == 8 && wait->unit == hypertalk::ast::TimeUnitKind::Ticks;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::SendCmd>(node))
      send |= command->message.size() == 1 && !command->target;
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::ReadFileCmd>(node))
      read |= command->file && command->atExpr && command->untilExpr && !command->forExpr;
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::PushCmd>(node)) {
      const auto* card = cursiv::ast_cast<hypertalk::ast::CardPartRef>(command->value);
      push |= card && card->expr;
    }
    if (const auto* value = cursiv::ast_cast<hypertalk::ast::NamedLit>(node)) {
      trueValue |= value->kind == hypertalk::ast::NamedLitKind::True;
      falseValue |= value->kind == hypertalk::ast::NamedLitKind::False;
    }
    if (const auto* call = cursiv::ast_cast<hypertalk::ast::FuncCall>(node)) {
      date |= call->name == "date" && call->args.empty();
      time |= call->name == "time" && call->args.empty();
    }
    if (const auto* format = cursiv::ast_cast<hypertalk::ast::ConversionFormat>(node)) {
      longDate |= format->length == hypertalk::ast::Length::Long;
      englishDate |= format->length == hypertalk::ast::Length::English;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::AskCmd>(node))
      password |= command->password && command->clear && command->defaultAnswer;
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::AnswerCmd>(node)) {
      fileAnswer |= command->file && command->type;
      programAnswer |= command->program && command->type;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::ScreenLockCmd>(node)) {
      visual |= !command->lock && command->target == hypertalk::ast::LockTarget::Screen && command->visualEffect;
      messages |= command->lock && command->target == hypertalk::ast::LockTarget::Messages;
      errors |= !command->lock && command->target == hypertalk::ast::LockTarget::ErrorDialogs;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::SimpleExprCmd>(node)) {
      doLanguage |= command->verb == hypertalk::ast::CmdVerb::Do && command->language;
      modem |= command->verb == hypertalk::ast::CmdVerb::Dial && command->withModem && command->modemCommands;
      menu |= command->verb == hypertalk::ast::CmdVerb::Domenu && command->menuName &&
              command->withoutDialog && command->withKeys.size() == 1;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::PopCardCmd>(node))
      pop |= command->prep == hypertalk::ast::Preposition::Into && command->target;
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::ResetCmd>(node)) printing |= command->printing;
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::ShowHideCmd>(node)) {
      showAt |= command->show && command->target && command->at.size() == 2;
      allCards |= command->show && command->allCards;
    }
    if (const auto* command = cursiv::ast_cast<hypertalk::ast::GoCmd>(node)) {
      const auto* destination = cursiv::ast_cast<hypertalk::ast::CardPartRef>(command->destination);
      go |= destination && destination->position == hypertalk::ast::Position::Next;
    }
  }
  const bool ok = defaultTicks && explicitTicks && send && read && push && trueValue && falseValue &&
                  date && time && longDate && englishDate && password && fileAnswer && programAnswer &&
                  visual && messages && errors && doLanguage && modem && menu && pop && printing &&
                  showAt && allCards && go;
  if (!ok) std::fprintf(stderr, "hypertalk native: command/constant/format fields were lost\n");
  for (const char* invalid : {"lock screen with dissolve\n", "hide all cards\n"})
    if (cursiv::test::validationHelper(services, std::string(invalid)).diagnostics.size() != 1)
      return false;
  return ok;
}
} // namespace hypertalk_native_followup
