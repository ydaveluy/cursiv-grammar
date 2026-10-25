// A TCP header, including its optional bytes.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tcpheader/core/ast.hpp"

namespace tcpheader {

struct Tcpheader : cursiv::GrammarSpec {
  struct Byte : terminal<byte> {};

  struct Word : rule<ref<Byte> + ref<Byte>> {};
  struct Dword : rule<ref<Byte> + ref<Byte> + ref<Byte> + ref<Byte>> {};
  struct Options : rule<some(ref<Byte>)> {};

  struct SegmentHeaderRule
      : rule<assign<&ast::SegmentHeader::sourcePort>(ref<Word>) +
             assign<&ast::SegmentHeader::destinationPort>(ref<Word>) +
             assign<&ast::SegmentHeader::sequenceNumber>(ref<Dword>) +
             assign<&ast::SegmentHeader::ackNumber>(ref<Dword>) +
             assign<&ast::SegmentHeader::flags>(ref<Word>) +
             assign<&ast::SegmentHeader::windowSize>(ref<Word>) +
             assign<&ast::SegmentHeader::checksum>(ref<Word>) +
             assign<&ast::SegmentHeader::urgent>(ref<Word>) +
             option(assign<&ast::SegmentHeader::options>(ref<Options>))> {};

  using entry = SegmentHeaderRule;
};

}  // namespace tcpheader
