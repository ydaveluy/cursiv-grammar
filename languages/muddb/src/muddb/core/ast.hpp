#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace muddb::ast {

struct KeyExpr : cursiv::Extends<cursiv::AstNode> {};
struct KeyReference : cursiv::Extends<KeyExpr> {
  string_view value;
};
struct LegacyKey : cursiv::Extends<KeyExpr> {
  string_view text;
};
struct KeyNot : cursiv::Extends<KeyExpr> {
  KeyExpr *operand = nullptr;
};
struct KeyBinary : cursiv::Extends<KeyExpr> {
  string_view op;
  KeyExpr *left = nullptr;
  KeyExpr *right = nullptr;
};
struct ExtraLine : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Room : cursiv::Extends<cursiv::AstNode> {
  string_view roomno;
  optional<string_view> name;
  optional<string_view> description;
  optional<string_view> location;
  optional<string_view> contents;
  optional<string_view> exits;
  optional<string_view> next_room;
  KeyExpr *key = nullptr;
  optional<string_view> fail_message;
  optional<string_view> succ_message;
  optional<string_view> ofail;
  optional<string_view> osuccess;
  optional<string_view> owner;
  optional<string_view> pennies;
  optional<string_view> flags;
  optional<string_view> password;
  vector<ExtraLine *> extras;
};

struct Db : cursiv::Extends<cursiv::AstNode> {
  vector<Room *> rooms;
};

} // namespace muddb::ast
