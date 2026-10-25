#pragma once
#include "csv/core/ast.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
namespace value_regressions {
struct Case {
  const char *name;
  std::string_view source;
  std::vector<std::vector<std::string>> rows;
};
inline int run(const auto &language) {
  int failures = 0;
  const Case cases[] = {
      {"quote_in_unquoted", "a\"b,c\n", {{"a\"b", "c"}}},
      {"space_then_quote", " a\"b,\"c\"\n", {{" a\"b", "c"}}},
      {"embedded_nul", std::string_view("a\0b,c\n", 6), {{std::string("a\0b", 3), "c"}}},
      {"empty", "", {}},
      {"one_field", "alpha", {{"alpha"}}},
      {"one_field_lf", "alpha\n", {{"alpha"}}},
      {"one_field_crlf", "alpha\r\n", {{"alpha"}}},
      {"one_field_cr", "alpha\r", {{"alpha"}}},
      {"ordinary", "a,b\nc,d\n", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_no_final", "a,b\nc,d", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_crlf", "a,b\r\nc,d\r\n", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_cr", "a,b\rc,d\r", {{"a", "b"}, {"c", "d"}}},
      {"one_empty_lf", "\n", {{""}}},
      {"two_empty_lf", "\n\n", {{""}, {""}}},
      {"one_empty_crlf", "\r\n", {{""}}},
      {"two_empty_crlf", "\r\n\r\n", {{""}, {""}}},
      {"one_empty_cr", "\r", {{""}}},
      {"two_empty_cr", "\r\r", {{""}, {""}}},
      {"empty_middle", "a,,b\n", {{"a", "", "b"}}},
      {"empty_first", ",a\n", {{"", "a"}}},
      {"empty_last", "a,\n", {{"a", ""}}},
      {"empty_both", ",\n", {{"", ""}}},
      {"all_empty", ",,\n", {{"", "", ""}}},
      {"header_empty", "a,,b\nx,y,z\n", {{"a", "", "b"}, {"x", "y", "z"}}},
      {"data_empty", "a,b,c\nx,,z\n", {{"a", "b", "c"}, {"x", "", "z"}}},
      {"blank_between", "a,b\n\nc,d\n", {{"a", "b"}, {""}, {"c", "d"}}},
      {"blank_tail", "a,b\n\n", {{"a", "b"}, {""}}},
      {"quoted_empty", "\"\",\"\"\n", {{"", ""}}},
      {"quoted_delimiter", "\"a,b\",c\n", {{"a,b", "c"}}},
      {"doubled_quotes", "\"a\"\"b\",c\n", {{"a\"b", "c"}}},
      {"quoted_lf", "\"a\nb\",c\n", {{"a\nb", "c"}}},
      {"quoted_crlf", "\"a\r\nb\",c\n", {{"a\r\nb", "c"}}},
      {"quoted_cr", "\"a\rb\",c\n", {{"a\rb", "c"}}},
      {"whitespace", " a , b \n", {{" a ", " b "}}},
      {"unicode", "élève,€,😀\n", {{"élève", "€", "😀"}}},
      {"last_quoted_no_final", "a,\"b\"", {{"a", "b"}}},
      {"heterogeneous",
       "a,b\nx\nx,y,z\n",
       {{"a", "b"}, {"x"}, {"x", "y", "z"}}},
      {"mixed_eol", "a,b\n\rc,d\r\n", {{"a", "b"}, {""}, {"c", "d"}}},
  };
  for (const auto &c : cases) {
    const auto r = language.language.parser->parse(c.source);
    const auto *doc = r.template value_as<csv::ast::Doc>();
    bool good =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty() && doc;
    if (good)
      good = doc->rows.size() == c.rows.size();
    if (good)
      for (std::size_t i = 0; i < c.rows.size(); ++i) {
        const auto *row = doc->rows[i];
        good = good && row && row->parent == doc &&
               row->fields.size() == c.rows[i].size();
        if (good)
          for (std::size_t k = 0; k < c.rows[i].size(); ++k)
            good = good && row->fields[k] && row->fields[k]->parent == row &&
                   row->fields[k]->text == c.rows[i][k];
      }
    std::size_t warnings = 0;
    for (std::size_t i = 1; i < c.rows.size(); ++i)
      warnings += c.rows[i].size() != c.rows[0].size();
    const auto validation = cursiv::test::validationHelper(
        language.language, std::string(c.source));
    good = good && validation.diagnostics.size() == warnings;
    for (const auto &diagnostic : validation.diagnostics)
      good = good && diagnostic.severity == cursiv::DiagnosticSeverity::Warning;
    if (!good) {
      std::fprintf(stderr, "csv exact rows/values/parents/validation: %s\n",
                   c.name);
      ++failures;
    }
  }
  return failures;
}
} // namespace value_regressions
