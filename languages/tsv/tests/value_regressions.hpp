#pragma once
#include "cursiv/test/ValidationHelper.hpp"
#include "tsv/core/ast.hpp"
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
      {"quote_in_unquoted", "a\"b\tc\n", {{"a\"b", "c"}}},
      {"space_then_quote", " a\"b\t\"c\"\n", {{" a\"b", "c"}}},
      {"embedded_nul", std::string_view("a\0b\tc\n", 6), {{std::string("a\0b", 3), "c"}}},
      {"empty", "", {}},
      {"one_field", "alpha", {{"alpha"}}},
      {"one_field_lf", "alpha\n", {{"alpha"}}},
      {"one_field_crlf", "alpha\r\n", {{"alpha"}}},
      {"one_field_cr", "alpha\r", {{"alpha"}}},
      {"ordinary", "a\tb\nc\td\n", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_no_final", "a\tb\nc\td", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_crlf", "a\tb\r\nc\td\r\n", {{"a", "b"}, {"c", "d"}}},
      {"ordinary_cr", "a\tb\rc\td\r", {{"a", "b"}, {"c", "d"}}},
      {"one_empty_lf", "\n", {{""}}},
      {"two_empty_lf", "\n\n", {{""}, {""}}},
      {"one_empty_crlf", "\r\n", {{""}}},
      {"two_empty_crlf", "\r\n\r\n", {{""}, {""}}},
      {"one_empty_cr", "\r", {{""}}},
      {"two_empty_cr", "\r\r", {{""}, {""}}},
      {"empty_middle", "a\t\tb\n", {{"a", "", "b"}}},
      {"empty_first", "\ta\n", {{"", "a"}}},
      {"empty_last", "a\t\n", {{"a", ""}}},
      {"empty_both", "\t\n", {{"", ""}}},
      {"all_empty", "\t\t\n", {{"", "", ""}}},
      {"header_empty", "a\t\tb\nx\ty\tz\n", {{"a", "", "b"}, {"x", "y", "z"}}},
      {"data_empty", "a\tb\tc\nx\t\tz\n", {{"a", "b", "c"}, {"x", "", "z"}}},
      {"blank_between", "a\tb\n\nc\td\n", {{"a", "b"}, {""}, {"c", "d"}}},
      {"blank_tail", "a\tb\n\n", {{"a", "b"}, {""}}},
      {"quoted_empty", "\"\"\t\"\"\n", {{"", ""}}},
      {"quoted_delimiter", "\"a\tb\"\tc\n", {{"a\tb", "c"}}},
      {"doubled_quotes", "\"a\"\"b\"\tc\n", {{"a\"b", "c"}}},
      {"quoted_lf", "\"a\nb\"\tc\n", {{"a\nb", "c"}}},
      {"quoted_crlf", "\"a\r\nb\"\tc\n", {{"a\r\nb", "c"}}},
      {"quoted_cr", "\"a\rb\"\tc\n", {{"a\rb", "c"}}},
      {"whitespace", " a \t b \n", {{" a ", " b "}}},
      {"unicode", "élève\t€\t😀\n", {{"élève", "€", "😀"}}},
      {"last_quoted_no_final", "a\t\"b\"", {{"a", "b"}}},
      {"heterogeneous",
       "a\tb\nx\nx\ty\tz\n",
       {{"a", "b"}, {"x"}, {"x", "y", "z"}}},
      {"mixed_eol", "a\tb\n\rc\td\r\n", {{"a", "b"}, {""}, {"c", "d"}}},
  };
  for (const auto &c : cases) {
    const auto r = language.language.parser->parse(c.source);
    const auto *doc = r.template value_as<tsv::ast::Document>();
    bool good =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty() && doc;
    if (good)
      good = c.rows.empty()
                 ? !doc->header && doc->rows.empty()
                 : doc->header && doc->rows.size() + 1 == c.rows.size();
    if (good)
      for (std::size_t i = 0; i < c.rows.size(); ++i) {
        const auto *row = i == 0 ? doc->header : doc->rows[i - 1];
        good = good && row && row->parent == doc &&
               row->fields.size() == c.rows[i].size();
        if (good)
          for (std::size_t k = 0; k < c.rows[i].size(); ++k)
            good = good && row->fields[k] == c.rows[i][k];
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
      std::fprintf(stderr, "tsv exact rows/values/parents/validation: %s\n",
                   c.name);
      ++failures;
    }
  }
  return failures;
}
} // namespace value_regressions
