#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "qif/core/ast.hpp"

namespace qif {

struct Qif : cursiv::GrammarSpec {
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct TEXT : terminal<some(cr<R"(^\r\n)">)> {};
  struct CODE : terminal<cr<R"(^!^\r\n)">> {};

  struct HeaderRule
      : rule<lit<"!"> + option(assign<&ast::Header::text>(ref<TEXT>)) +
             (ref<EOL> | !any)> {};

  struct FieldRule
      : rule<assign<&ast::Field::code>(ref<CODE>) +
             option(assign<&ast::Field::text>(ref<TEXT>)) + (ref<EOL> | !any)> {
  };

  struct RecordRule
      : rule<create<ast::Record> +
             ((some(append<&ast::Record::fields>(ref<FieldRule>)) +
               option(flag<&ast::Record::terminated>(lit<"^">))) |
              flag<&ast::Record::terminated>(lit<"^">))> {};

  struct DocumentRule : rule<create<ast::Document> +
                             many(append<&ast::Document::entries>(
                                      ref<HeaderRule> | ref<RecordRule>) |
                                  ref<EOL>)> {};

  using entry = DocumentRule;
};

} // namespace qif
