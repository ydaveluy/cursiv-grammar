#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "qif/core/ast.hpp"

namespace qif {

struct Qif : cursiv::GrammarSpec {
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct CODE : terminal<cr<R"(^!^\r\n)">> {};
  struct RECORD_END : terminal<lit<"^"> + (ref<EOL> | eof)> {};

  struct Header
      : rule<lit<"!"> + option(assign<&ast::Header::text>(ref<TEXT>)) +
             (ref<EOL> | eof)> {};

  struct Field
      : rule<assign<&ast::Field::code>(ref<CODE>) +
             option(assign<&ast::Field::text>(ref<TEXT>)) + (ref<EOL> | eof)> {
  };

  struct Record
      : rule<create<ast::Record> +
             ((some(append<&ast::Record::fields>(ref<Field>)) +
               option(flag<&ast::Record::terminated>(ref<RECORD_END>))) |
              flag<&ast::Record::terminated>(ref<RECORD_END>))> {};

  struct Document : rule<create<ast::Document> +
                             many(append<&ast::Document::entries>(
                                      ref<Header> | ref<Record>) |
                                  ref<EOL>)> {};

  using entry = Document;
};

} // namespace qif
