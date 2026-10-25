// The fields and option bytes of a TCP header.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tcpheader::ast {

struct SegmentHeader : cursiv::Extends<cursiv::AstNode> {
  string_view sourcePort;
  string_view destinationPort;
  string_view sequenceNumber;
  string_view ackNumber;
  string_view flags;
  string_view windowSize;
  string_view checksum;
  string_view urgent;
  string_view options;
};

}  // namespace tcpheader::ast
