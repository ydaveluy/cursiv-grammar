// The csv language: its grammar. No trivia: every byte is either field text,
// a comma, or a line ending -- nothing is skipped between tokens.
#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>

#include "csv/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace csv {

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

struct Csv : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct TEXT : terminal<!lit<"\""> + some(cr<R"(^,\n\r)">)> {};

  struct STRING
      : terminal<lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) + lit<"\"">,
                 Undoubled> {};

  struct FieldRule : rule<assign<&ast::Field::text>(ref<TEXT> | ref<STRING>) |
                          create<ast::Field>> {};

  struct RowRule
      : rule<some(append<&ast::Row::fields>(ref<FieldRule>), lit<",">)> {};

  struct DocRule : rule<create<ast::Doc> +
                        many(append<&ast::Doc::rows>(ref<RowRule>) + ref<EOL>) +
                        option(!eof + append<&ast::Doc::rows>(ref<RowRule>))> {
  };

  using entry = DocRule;
};

} // namespace csv
