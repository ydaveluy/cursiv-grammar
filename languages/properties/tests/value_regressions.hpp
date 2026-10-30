#pragma once
#include "cursiv/test/ValidationHelper.hpp"
#include "properties/core/ast.hpp"
#include <cstdio>
#include <map>
#include <string>
#include <string_view>
namespace value_regressions {
struct Case {
  const char *name;
  std::string_view source;
  std::map<std::string, std::string> values;
  std::size_t count;
  bool invalid;
};
inline int run(const auto &language) {
  int failures = 0;
  const Case cases[] = {
      {"empty", "", {}, 0, false},
      {"bare", "bare\n", {{"bare", ""}}, 1, false},
      {"empty_equal", "key=\n", {{"key", ""}}, 1, false},
      {"empty_colon", "key:\n", {{"key", ""}}, 1, false},
      {"empty_whitespace", "key \n", {{"key", ""}}, 1, false},
      {"empty_key_equal", "=value\n", {{"", "value"}}, 1, false},
      {"empty_key_colon", ":value\n", {{"", "value"}}, 1, false},
      {"both_empty", "=\n", {{"", ""}}, 1, false},
      {"space_empty_key", "  = value\n", {{"", "value"}}, 1, false},
      {"whitespace_separator", "key value\n", {{"key", "value"}}, 1, false},
      {"mixed_separator",
       "key \t : \f value  \n",
       {{"key", "value  "}},
       1,
       false},
      {"escaped_space_key", "a\\ b=v\n", {{"a b", "v"}}, 1, false},
      {"escaped_separator_key", "a\\:b\\=c=v\n", {{"a:b=c", "v"}}, 1, false},
      {"ordinary",
       "alpha=one\nbeta=two\n",
       {{"alpha", "one"}, {"beta", "two"}},
       2,
       false},
      {"lf", "k=v\nq=r\n", {{"k", "v"}, {"q", "r"}}, 2, false},
      {"crlf", "k=v\r\nq=r\r\n", {{"k", "v"}, {"q", "r"}}, 2, false},
      {"cr", "k=v\rq=r\r", {{"k", "v"}, {"q", "r"}}, 2, false},
      {"trailing_slash_key", "key\\", {{"key", ""}}, 1, false},
      {"trailing_slash_value", "key=value\\", {{"key", "value"}}, 1, false},
      {"trailing_double_slash",
       "key=value\\\\",
       {{"key", "value\\"}},
       1,
       false},
      {"continued_lf", "key=one\\\n  two\n", {{"key", "onetwo"}}, 1, false},
      {"continued_crlf",
       "key=one\\\r\n  two\r\n",
       {{"key", "onetwo"}},
       1,
       false},
      {"continued_cr", "key=one\\\r  two\r", {{"key", "onetwo"}}, 1, false},
      {"continued_empty",
       "key=one\\\n\ntwo=value\n",
       {{"key", "one"}, {"two", "value"}},
       2,
       false},
      {"continued_hash", "key=one\\\n#two\n", {{"key", "one#two"}}, 1, false},
      {"continued_start_hash", "\\\n#comment\n", {}, 0, false},
      {"comment_trailing_slash",
       "#comment\\\nkey=value\n",
       {{"key", "value"}},
       1,
       false},
      {"unicode", "clé=élève € 😀\n", {{"clé", "élève € 😀"}}, 1, false},
      {"unicode_escaped",
       "key=\\u0061\\u20ac\\uD83D\\uDE00\n",
       {{"key", "a€😀"}},
       1,
       false},
      // Java Reader strings retain isolated UTF-16 code units. Their byte
      // representation here is WTF-8, rather than a replacement character.
      {"unpaired_high", "key=\\uD800\n", {{"key", "\xED\xA0\x80"}}, 1, false},
      {"unpaired_low", "key=\\uDC00\n", {{"key", "\xED\xB0\x80"}}, 1, false},
      {"escaped_unknown", "key=\\q\n", {{"key", "q"}}, 1, false},
      {"unicode_malformed", "key=\\uGGGG\n", {}, 1, true},
      {"unicode_short", "key=\\u12\n", {}, 1, true},
      {"duplicate", "key=old\nkey=new\n", {{"key", "new"}}, 2, false},
      {"escaped_duplicate",
       "keya=old\nkey\\u0061=new\n",
       {{"keya", "new"}},
       2,
       false},
      {"continued blank line", "\\\n\n", {}, 0, false},
      {"continued blank then entry",
       "\\\n\nkey=value\n",
       {{"key", "value"}},
       1,
       false},
      {"continued EOF", "\\\n", {{"", ""}}, 1, false},
      {"continued blank preserves empty key",
       "=one\n\\\n\nkey=value\n",
       {{"", "one"}, {"key", "value"}},
       2,
       false},
      {"blank lines", "\n \t\f\n\r\n\r", {}, 0, false},
      {"space hash comment", " \t# comment\n", {}, 0, false},
      {"space bang comment", "\t ! comment\n", {}, 0, false},
      {"escaped hash key", "\\#key=v\n", {{"#key", "v"}}, 1, false},
      {"escaped bang key", " \t\\!key=v\n", {{"!key", "v"}}, 1, false},
      {"invalid key escape", "bad\\uGGGG=value\n", {}, 1, true},
      {"OpenJDK eof-slash.properties", "\\", {{"", ""}}, 1, false},
      {"OpenJDK eof-slash_lf.properties", "\\\n", {{"", ""}}, 1, false},
      {"OpenJDK eof-slash_crlf.properties", "\\\r\n", {}, 0, false},
      {"OpenJDK eof-slash_cr.properties", "\\\r", {{"", ""}}, 1, false},
      {"OpenJDK eof-slash_lf_space.properties", "\\\n ", {}, 0, false},
      {"OpenJDK eof-slash_crlf_space.properties", "\\\r\n ", {}, 0, false},
      {"OpenJDK eof-space_slash_lf_space.properties", " \\\n ", {}, 0, false},
      {"OpenJDK eof-twice_direct_eof.properties",
       "\\\n\\\n",
       {{"", ""}},
       1,
       false},
      {"OpenJDK eof-twice_space_eof.properties", "\\\n\\\n ", {}, 0, false},
      {"OpenJDK eof-continued_blank.properties", "\\\n\n", {}, 0, false},
      {"OpenJDK eof-continued_blank_property.properties",
       "=one\n\\\n\nkey=value\n",
       {{"", "one"}, {"key", "value"}},
       2,
       false},
      {"OpenJDK eof-continued_slash_eof.properties",
       "\\\n \\",
       {{"", ""}},
       1,
       false},
      {"OpenJDK eof-only_space.properties", " \t\f", {}, 0, false},
  };
  for (const auto &c : cases) {
    const auto r = language.language.parser->parse(c.source);
    const auto *doc = r.template value_as<properties::ast::Document>();
    bool good =
        r.accepted && !r.cst().recovered() && r.diagnostics.empty() && doc;
    std::map<std::string, std::string> values;
    std::size_t entries = 0;
    if (doc)
      for (const auto &row : doc->rows) {
        std::visit(
            [&](const auto *node) {
              good = good && node && node->parent == doc;
            },
            row);
        if (const auto *entry = std::get_if<properties::ast::Entry *>(&row);
            entry && *entry) {
          ++entries;
          values[std::string((*entry)->key)] =
              std::string((*entry)->value.value_or(""));
        }
      }
    good = good && entries == c.count && (c.invalid || values == c.values);
    const auto validation = cursiv::test::validationHelper(
        language.language, std::string(c.source));
    std::size_t errors = 0, warnings = 0;
    for (const auto &diagnostic : validation.diagnostics) {
      errors += diagnostic.severity == cursiv::DiagnosticSeverity::Error;
      warnings += diagnostic.severity == cursiv::DiagnosticSeverity::Warning;
      if (diagnostic.severity == cursiv::DiagnosticSeverity::Error)
        good = good && diagnostic.range.begin == c.source.find("\\u") &&
               diagnostic.range.end > diagnostic.range.begin;
    }
    good = good && errors == (c.invalid ? 1u : 0u) &&
           warnings == (std::string_view(c.name).find("duplicate") !=
                                std::string_view::npos
                            ? 1u
                            : 0u);
    if (!good) {
      std::fprintf(stderr, "properties exact values/parents/runtime: %s\n",
                   c.name);
      ++failures;
    }
  }
  return failures;
}
} // namespace value_regressions
