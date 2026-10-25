// The tinymud language: its AST. One node class per @-command and per
// action verb; every field is a view of the document, never owned text.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tinymud::ast {

struct Line : cursiv::Extends<cursiv::AstNode> {};
struct Command : cursiv::Extends<Line> {};
struct Action : cursiv::Extends<Line> {};

struct BootCommand : cursiv::Extends<Command> {
  string_view player;
};

struct ChownCommand : cursiv::Extends<Command> {
  string_view object;
  string_view player;
};

struct CreateCommand : cursiv::Extends<Command> {
  string_view name;
  optional<int64> cost;
};

struct DescribeCommand : cursiv::Extends<Command> {
  string_view object;
  string_view description;  // empty when omitted
};

struct DigCommand : cursiv::Extends<Command> {
  string_view name;
};

struct DumpCommand : cursiv::Extends<Command> {};

struct FailCommand : cursiv::Extends<Command> {
  string_view name;
  string_view message;  // empty when omitted
};

struct FindCommand : cursiv::Extends<Command> {
  string_view name;  // empty when omitted
};

struct ForceCommand : cursiv::Extends<Command> {
  string_view player;
  Command* command;
};

// number, dir_ and room collapse to one target: a room number, or a name
// (a direction and a room name are the same STRING shape).
struct LinkCommand : cursiv::Extends<Command> {
  string_view object;
  variant<int64, string_view> target;
};

struct LockCommand : cursiv::Extends<Command> {
  string_view object;
  string_view key;
};

struct NameCommand : cursiv::Extends<Command> {
  string_view object;
  string_view name;
  string_view password;  // empty when omitted
};

struct NewPasswordCommand : cursiv::Extends<Command> {
  string_view player;
  string_view password;  // empty when omitted
};

struct OfailCommand : cursiv::Extends<Command> {
  string_view object;
  string_view message;  // empty when omitted
};

struct OpenCommand : cursiv::Extends<Command> {
  vector<string_view> directions;
  optional<int64> number;
};

struct OsuccessCommand : cursiv::Extends<Command> {
  string_view object;
  string_view message;  // empty when omitted
};

struct PasswordCommand : cursiv::Extends<Command> {
  string_view oldPassword;
  string_view newPassword;
};

struct PcreateCommand : cursiv::Extends<Command> {
  string_view name;
};

struct SetCommand : cursiv::Extends<Command> {
  string_view object;
  boolean negate;
  int64 flag;
};

struct ShutdownCommand : cursiv::Extends<Command> {};

struct StatsCommand : cursiv::Extends<Command> {
  string_view player;
};

struct SuccessCommand : cursiv::Extends<Command> {
  string_view object;
  string_view message;  // empty when omitted
};

struct TeleportCommand : cursiv::Extends<Command> {
  string_view object;  // empty when omitted
  string_view room;
};

struct ToadCommand : cursiv::Extends<Command> {
  string_view player;
};

struct UnlinkCommand : cursiv::Extends<Command> {
  string_view direction;
};

struct UnlockCommand : cursiv::Extends<Command> {
  string_view object;
};

struct WallCommand : cursiv::Extends<Command> {
  string_view message;
};

struct DropAction : cursiv::Extends<Action> {
  string_view object;
};

struct ExamineAction : cursiv::Extends<Action> {
  string_view object;
};

struct GetAction : cursiv::Extends<Action> {
  string_view object;
};

struct GiveAction : cursiv::Extends<Action> {
  string_view player;
  int64 pennies;
};

struct GotoAction : cursiv::Extends<Action> {
  string_view direction;
};

struct GripeAction : cursiv::Extends<Action> {
  string_view message;
};

struct HelpAction : cursiv::Extends<Action> {};

struct InventoryAction : cursiv::Extends<Action> {};

struct KillAction : cursiv::Extends<Action> {
  string_view player;
  int64 cost;
};

struct LookAction : cursiv::Extends<Action> {
  string_view object;
};

struct NewsAction : cursiv::Extends<Action> {};

struct PageAction : cursiv::Extends<Action> {
  string_view player;
  string_view message;
};

struct QuitAction : cursiv::Extends<Action> {};

struct RobAction : cursiv::Extends<Action> {
  string_view player;
};

struct SayAction : cursiv::Extends<Action> {
  string_view message;
};

struct ScoreAction : cursiv::Extends<Action> {};

struct WhisperAction : cursiv::Extends<Action> {
  string_view player;
  string_view message;
};

struct WhoAction : cursiv::Extends<Action> {
  string_view player;  // empty when omitted
};

struct Session : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace tinymud::ast
