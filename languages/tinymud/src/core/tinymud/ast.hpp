// The tinymud language: its AST. One node class per @-command and per
// action verb; every field is a view of the document, never owned text.
#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace tinymud::ast {

struct Line : cursiv::Extends<cursiv::AstNode> {};
struct Command : cursiv::Extends<Line> {};
struct Action : cursiv::Extends<Line> {};

struct BootCommand : cursiv::Extends<Command> {
  string player;
};

struct ChownCommand : cursiv::Extends<Command> {
  string object;
  string player;
};

struct CreateCommand : cursiv::Extends<Command> {
  string name;
  optional<int64> cost;
};

struct DescribeCommand : cursiv::Extends<Command> {
  string object;
  string description;  // empty when omitted
};

struct DigCommand : cursiv::Extends<Command> {
  string name;
};

struct DumpCommand : cursiv::Extends<Command> {};

struct FailCommand : cursiv::Extends<Command> {
  string name;
  string message;  // empty when omitted
};

struct FindCommand : cursiv::Extends<Command> {
  string name;  // empty when omitted
};

struct ForceCommand : cursiv::Extends<Command> {
  string player;
  Line* command;
};

// number, dir_ and room collapse to one target: a room number, or a name
// (a direction and a room name are the same STRING shape).
struct LinkCommand : cursiv::Extends<Command> {
  string object;
  variant<int64, string> target;
};

struct LockCommand : cursiv::Extends<Command> {
  string object;
  string key;
};

struct NameCommand : cursiv::Extends<Command> {
  string object;
  string name; // Complete RHS; player-password split needs the object's type.
  string_view password;  // legacy slot; complete RHS retained in name
};

struct NewPasswordCommand : cursiv::Extends<Command> {
  string player;
  string password;  // empty when omitted
};

struct OfailCommand : cursiv::Extends<Command> {
  string object;
  string message;  // empty when omitted
};

struct OpenCommand : cursiv::Extends<Command> {
  vector<string> directions;
  optional<int64> number;
  string destination;
};

struct OsuccessCommand : cursiv::Extends<Command> {
  string object;
  string message;  // empty when omitted
};

struct PasswordCommand : cursiv::Extends<Command> {
  string oldPassword;
  string newPassword;
};

struct PcreateCommand : cursiv::Extends<Command> {
  string name;
  string password;
};

struct SetCommand : cursiv::Extends<Command> {
  string object;
  boolean negate;
  int64 flag; // legacy numeric syntax; native flags are names
  string_view flagName;
};

struct ShutdownCommand : cursiv::Extends<Command> {};

struct StatsCommand : cursiv::Extends<Command> {
  string player;
};

struct SuccessCommand : cursiv::Extends<Command> {
  string object;
  string message;  // empty when omitted
};

struct TeleportCommand : cursiv::Extends<Command> {
  string object;  // empty when omitted
  string room;
};

struct ToadCommand : cursiv::Extends<Command> {
  string player;
  string name;
};

struct UnlinkCommand : cursiv::Extends<Command> {
  string direction;
};

struct UnlockCommand : cursiv::Extends<Command> {
  string object;
};

struct WallCommand : cursiv::Extends<Command> {
  string message;
};

struct DropAction : cursiv::Extends<Action> {
  string object;
};

struct ExamineAction : cursiv::Extends<Action> {
  string object;
};

struct GetAction : cursiv::Extends<Action> {
  string object;
};

struct GiveAction : cursiv::Extends<Action> {
  string player;
  int64 pennies;
};

struct GotoAction : cursiv::Extends<Action> {
  string direction;
};

struct GripeAction : cursiv::Extends<Action> {
  string message;
};

struct HelpAction : cursiv::Extends<Action> {};

struct InventoryAction : cursiv::Extends<Action> {};

struct KillAction : cursiv::Extends<Action> {
  string player;
  int64 cost;
  boolean hasCost;
};

struct LookAction : cursiv::Extends<Action> {
  string object;
};

struct NewsAction : cursiv::Extends<Action> {};

struct PageAction : cursiv::Extends<Action> {
  string player;
  string message;
};

struct QuitAction : cursiv::Extends<Action> {};

struct RobAction : cursiv::Extends<Action> {
  string player;
};

struct SayAction : cursiv::Extends<Action> {
  string message;
};

struct ScoreAction : cursiv::Extends<Action> {};

struct WhisperAction : cursiv::Extends<Action> {
  string player;
  string message;
};

struct WhoAction : cursiv::Extends<Action> {
  string player;  // empty when omitted
};

struct Session : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace tinymud::ast
