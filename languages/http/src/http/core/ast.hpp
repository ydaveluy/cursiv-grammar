// The http language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace http::ast {

struct RequestLine : cursiv::Extends<cursiv::AstNode> {
  string_view method;
  string_view target;
  string_view version;
};

struct HeaderField : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct StatusLine : cursiv::Extends<cursiv::AstNode> {
  string_view version;
  string_view status;
  string_view reason;
};

struct Message : cursiv::Extends<cursiv::AstNode> {
  RequestLine *request_line;
  StatusLine *status_line;
  vector<HeaderField *> headers;
  string_view body;
  boolean headersTerminated;
};

} // namespace http::ast
