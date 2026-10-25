#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fasta/core/ast.hpp"

namespace fasta {

struct Fasta : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct SEQUENCELINE
      : terminal<cr<R"(^>;\r\n)"> + many(cr<R"(^\r\n)">)> {};

  struct DescriptionLineRule
      : rule<lit<">"> + option(assign<&ast::DescriptionLine::text>(ref<TEXT>)) +
             line_end> {};

  struct SequenceLineRule
      : rule<assign<&ast::SequenceLine::text>(ref<SEQUENCELINE>) + line_end> {};

  struct CommentLineRule
      : rule<lit<";"> +
             option(assign<&ast::CommentLine::text>(ref<TEXT>)) +
             line_end> {};

  struct BlankLineRule : rule<create<ast::BlankLine> + ref<EOL>> {};
  static constexpr auto BodyLine =
      ref<SequenceLineRule> | ref<CommentLineRule> | ref<BlankLineRule>;

  struct RecordRule
      : rule<assign<&ast::Record::description>(ref<DescriptionLineRule>) +
             many(append<&ast::Record::body>(BodyLine))> {};

  struct DocumentRule
      : rule<many(append<&ast::Document::preamble>(BodyLine)) +
             many(append<&ast::Document::records>(ref<RecordRule>))> {};

  using entry = DocumentRule;
};

} // namespace fasta
