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
using Undoubled = cursiv::text_conv<undouble>;

struct Tsv : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};

  // TAB is excluded so a longest-match lexer does not run an unquoted field
  // through the separator into the next one.
  struct TEXT : terminal<!lit<"\""> + some(cr<R"(^\t\n\r)">)> {};
  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">,
                 Undoubled> {};

  // A data rule keeps an empty field as one scalar value, including fields
  // before, between and after separators.
  struct FieldRule : rule<option(ref<TEXT> | ref<STRING>)> {};

  struct RowRule
      : rule<some(append<&ast::Row::fields>(ref<FieldRule>), lit<"\t">)> {};

  struct DocRule
      : rule<create<ast::Document> +
             option(!eof + assign<&ast::Document::header>(ref<RowRule>) +
                    many(ref<EOL> + !eof +
                         append<&ast::Document::rows>(ref<RowRule>)) +
                    option(ref<EOL>))> {};

  using entry = DocRule;
};

} // namespace tsv
