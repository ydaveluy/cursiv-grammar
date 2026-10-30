#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fasta/core/ast.hpp"

namespace fasta {

struct Fasta : cursiv::GrammarSpec {
  struct EOL : terminal<eol> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct SEQUENCELINE
      : terminal<cr<R"(^>;\r\n)"> + many(cr<R"(^\r\n)">)> {};

  struct DescriptionLine
      : rule<lit<">"> + option(assign<&ast::DescriptionLine::text>(ref<TEXT>)) +
             line_end> {};

  struct SequenceLine
      : rule<assign<&ast::SequenceLine::text>(ref<SEQUENCELINE>) + line_end> {};

  struct CommentLine
      : rule<lit<";"> +
             option(assign<&ast::CommentLine::text>(ref<TEXT>)) +
             line_end> {};

  struct BlankLine : rule<create<ast::BlankLine> + ref<EOL>> {};
  static constexpr auto BodyLine =
      ref<SequenceLine> | ref<CommentLine> | ref<BlankLine>;

  struct Record
      : rule<assign<&ast::Record::description>(ref<DescriptionLine>) +
             many(append<&ast::Record::body>(BodyLine))> {};

  struct Document
      : rule<many(append<&ast::Document::preamble>(BodyLine)) +
             many(append<&ast::Document::records>(ref<Record>))> {};

  using entry = Document;
};

} // namespace fasta
