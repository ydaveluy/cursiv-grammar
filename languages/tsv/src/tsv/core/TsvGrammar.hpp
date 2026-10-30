// The tsv language: its grammar.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tsv/core/ast.hpp"

namespace tsv {

inline constexpr auto undouble = [](std::string_view t,
                                    std::pmr::memory_resource *mem) {
  const std::string_view body = t.substr(1, t.size() - 2);
  if (body.find('"') == std::string_view::npos)
    return body;
  const std::span<char> room = cursiv::text_room(mem, body.size());
  std::size_t n = 0;
  for (std::size_t i = 0; i < body.size(); ++i) {
    room[n++] = body[i];
    if (body[i] == '"')
      ++i;
  }
  return std::string_view(room.data(), n);
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
