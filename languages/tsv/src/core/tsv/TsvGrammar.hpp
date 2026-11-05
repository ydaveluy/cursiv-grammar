// The tsv language: its grammar.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "cursiv/grammar/GrammarSpec.hpp"
#include "tsv/ast.hpp"

namespace tsv {

inline constexpr auto undouble = [](std::string_view t) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('"') == std::string_view::npos)
    return std::string(body);
  std::string out;
  out.reserve(body.size());
  for (std::size_t i = 0; i < body.size(); ++i) {
    out += body[i];
    if (body[i] == '"')
      ++i;
  }
  return out;
};

struct Tsv : cursiv::GrammarSpec {
  struct EOL : terminal<eol> {};

  // TAB is excluded so a longest-match lexer does not run an unquoted field
  // through the separator into the next one.
  struct TEXT : terminal<!lit<"\""> + some(cr<R"(^\t\n\r)">)> {};
  struct STRING
      : terminal<as<undouble>(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) +
                              lit<"\"">)> {};

  // A data rule keeps an empty field as one scalar value, including fields
  // before, between and after separators.
  struct Field : rule<option(ref<TEXT> | ref<STRING>)> {};

  struct Row
      : rule<some(append<&ast::Row::fields>(ref<Field>), lit<"\t">)> {};

  struct Doc
      : rule<create<ast::Document> +
             option(!eof + assign<&ast::Document::header>(ref<Row>) +
                    many(ref<EOL> + !eof +
                         append<&ast::Document::rows>(ref<Row>)) +
                    option(ref<EOL>))> {};

  using entry = Doc;
};

} // namespace tsv
