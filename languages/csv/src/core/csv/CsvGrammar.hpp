// The csv language: its grammar. No trivia: every byte is either field text,
// a comma, or a line ending -- nothing is skipped between tokens.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "csv/ast.hpp"
#include "cursiv/grammar/GrammarSpec.hpp"

namespace csv {

inline constexpr auto undouble = [](std::string_view t) -> std::string {
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

struct Csv : cursiv::GrammarSpec {
  struct EOL : terminal<lit<"\r\n"> | cr<R"(\r\n)">> {};
  struct TEXT : terminal<!lit<"\""> + some(cr<R"(^,\n\r)">)> {};

  struct STRING
      : terminal<as<undouble>(lit<"\""> + many(lit<"\"\""> | cr<R"(^")">) +
                              lit<"\"">)> {};

  struct Field : rule<assign<&ast::Field::text>(ref<TEXT> | ref<STRING>) |
                          create<ast::Field>> {};

  struct Row
      : rule<some(append<&ast::Row::fields>(ref<Field>), lit<",">)> {};

  struct Doc : rule<create<ast::Doc> +
                        many(append<&ast::Doc::rows>(ref<Row>) + ref<EOL>) +
                        option(!eof + append<&ast::Doc::rows>(ref<Row>))> {
  };

  using entry = Doc;
};

} // namespace csv
