// The clf language: its AST. One entry per log line -- Common and Combined
// Log Format share every field up to bytes; Combined adds referer/useragent.
#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace clf::ast {

struct Line : cursiv::Extends<cursiv::AstNode> {
  string_view host;        // hostname or address
  string_view logname;     // identd name, "-" when absent
  string_view username;    // authenticated user, "-" when absent
  string_view date;        // "07/Mar/2004"
  string_view time;        // "16:05:49"
  string_view tz;          // "-0800"
  string request;     // between the quotes, e.g. "GET / HTTP/1.1"
  string statuscode;
  string bytes;       // "-" when the body byte count is unavailable
  optional<string> referer;
  optional<string> useragent;
};

struct Log : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;  // blank lines contribute none
};

}  // namespace clf::ast
