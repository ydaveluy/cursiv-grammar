#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace stacktrace::ast {

struct AtLine : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> indentation;
  optional<string_view> origin;
  string_view className;
  optional<string_view> method;
  string_view file;
  optional<int64> line;
};
struct EllipsisLine : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> indentation;
  int64 more;
};
struct MessageLine : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> thread;
  string_view className;
  optional<string_view> message;
};
struct Section : cursiv::Extends<cursiv::AstNode> {
  optional<string_view> indentation;
  string_view caption;
  MessageLine* message;
  optional<string_view> circularReference;
  vector<variant<AtLine*, EllipsisLine*>> frames;
};
struct StackTrace : cursiv::Extends<cursiv::AstNode> {
  vector<MessageLine*> messages;
  vector<variant<AtLine*, EllipsisLine*>> frames;
  vector<Section*> sections;
};

}  // namespace stacktrace::ast
