#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "gff3/core/ast.hpp"

namespace gff3 {

struct Gff3 : cursiv::GrammarSpec {
  struct EOL : terminal<eol> {};
  struct TEXT : terminal<some(cr<R"(^\t\r\n)">)> {};
  struct SEQID : terminal<!cr<"#>"> + some(cr<R"(^\t\r\n)">)> {};
  struct Field : rule<option(ref<TEXT>)> {};
  struct LineEnd : rule<ref<EOL> | eof> {};
  struct FastaOpen : terminal<lit<"##FASTA"> + !cr<R"(^\r\n)">> {};
  struct COMMENT : terminal<!ref<FastaOpen> + lit<"#"> + many(cr<R"(^\r\n)">)> {
  };
  struct KEY : terminal<some(cr<R"(^=;\t\r\n)">)> {};
  struct VALUE : terminal<some(cr<R"(^,;\t\r\n)">)> {};
  struct AttributeValue : rule<option(ref<VALUE>)> {};
  struct Attribute
      : rule<create<ast::Attribute>(
            assign<&ast::Attribute::key>(ref<KEY>) +
            option(flag<&ast::Attribute::hasEquals>(lit<"=">) +
                   some(append<&ast::Attribute::values>(ref<AttributeValue>),
                        lit<",">)))> {};
  struct CommentLine
      : rule<create<ast::CommentLine>(
            assign<&ast::CommentLine::text>(ref<COMMENT>) + ref<LineEnd>)> {};

  static constexpr auto attributes =
      flag<&ast::Feature::attributesMissing>(lit<".">) |
      option(some(append<&ast::Feature::attributes>(ref<Attribute>),
                  lit<";">) +
             option(lit<";">));
  struct DataLine
      : rule<create<ast::Feature>(
            assign<&ast::Feature::seqid>(ref<SEQID>) + lit<"\t"> +
            assign<&ast::Feature::source>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::type>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::start>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::end>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::score>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::strand>(ref<Field>) + lit<"\t"> +
            assign<&ast::Feature::phase>(ref<Field>) +
            option(flag<&ast::Feature::hasAttributeColumn>(lit<"\t">) +
                   attributes +
                   many(lit<"\t"> + append<&ast::Feature::extra>(ref<Field>))) +
            ref<LineEnd>)> {};

  struct NAME : terminal<some(cr<R"(^ \t\r\n)">)> {};
  struct SPACE : terminal<some(cr<" \t">)> {};
  struct REST : terminal<some(cr<R"(^\r\n)">)> {};
  struct BASES : terminal<!lit<">"> + some(cr<R"(^\r\n)">)> {};
  struct Sequence
      : rule<create<ast::Sequence>(
            lit<">"> + many(ref<SPACE>) +
            assign<&ast::Sequence::name>(ref<NAME>) +
            option(ref<SPACE> +
                   option(assign<&ast::Sequence::description>(ref<REST>))) +
            ref<LineEnd> +
            many(ref<EOL> |
                 (append<&ast::Sequence::lines>(ref<BASES>) + ref<LineEnd>)))> {
  };
  struct FASTA_COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  struct Fasta
      : rule<create<ast::Fasta>(
            (flag<&ast::Fasta::marked>(ref<FastaOpen>) + ref<LineEnd> +
             many(ref<EOL> |
                  (append<&ast::Fasta::leadingComments>(ref<FASTA_COMMENT>) +
                   ref<LineEnd>)) +
             many(ref<EOL> |
                  append<&ast::Fasta::sequences>(ref<Sequence>))) |
            some(append<&ast::Fasta::sequences>(ref<Sequence>)))> {};
  struct Doc : rule<create<ast::Document>(
                       many(ref<EOL> | append<&ast::Document::lines>(
                                           ref<DataLine> | ref<CommentLine>)) +
                       option(assign<&ast::Document::fasta>(ref<Fasta>)))> {
  };

  using entry = Doc;
};

} // namespace gff3
