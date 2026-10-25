#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rfc3080::ast {

enum class Kind { Msg, Rpy, Ans, Err, Nul };

struct Frame : cursiv::Extends<cursiv::AstNode> {
  Kind kind;
  string_view channel;
  string_view msgno;
  boolean more;
  string_view seqno;
  string_view size;
  string_view ansno;
  string_view payload;
};

}  // namespace rfc3080::ast
