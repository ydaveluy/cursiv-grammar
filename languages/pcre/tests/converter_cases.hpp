#pragma once

#include <cstdio>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>

#include "pcre/core/PcreGrammar.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace pcre_converter_cases {

inline bool run(const cursiv::Parser& parser) {
  bool ok = true;
  const auto check = [&](bool value, const char* reason) {
    if (!value) { std::fprintf(stderr, "pcre converter: %s\n", reason); ok = false; }
  };
  const auto error = [](const auto& result) {
    return !result && result.error.kind == cursiv::DiagnosticKind::conversion_error &&
           !result.error.message.empty();
  };
  for (std::string_view text : {"", "(", "(?", "(?#", "(?#body", "a(?#)", "(?!a)",
                                "(?#body]", "(?#a)b)", "(?#a)(?#b)"})
    check(error(pcre::comment_text(text)), "malformed comment fragment returns an error");
  for (const auto [text, expected] : {
           std::pair<std::string_view, std::string_view>{"(?#)", ""},
           {"(?#note)", "note"}, {"(?#line\nnext)", "line\nnext"},
           {"(?#\\text)", "\\text"}}) {
    const auto result = pcre::comment_text(text);
    check(result && *result.value == expected, "valid comment text is retained");
  }
  // Malformed text is rejected before touching the supplied arena.
  for (std::string_view text : {"", "'", "{", "a", "aa", "[]", "()", "<>",
                                "'a\"", "{a'", "'''", "{}}", "'a'b'", "{a}b}"})
    check(error(pcre::callout_text(text, std::pmr::null_memory_resource())),
          "malformed callout fragment returns an error without allocation");
  std::pmr::monotonic_buffer_resource memory;
  for (char open : std::string_view("'\"`^%#${")) {
    const char close = open == '{' ? '}' : open;
    const std::string empty = std::string(1, open) + close;
    const auto noText = pcre::callout_text(empty, std::pmr::null_memory_resource());
    check(noText && noText.value->empty(), "empty callout text needs no allocation");
    const std::string source = std::string(1, open) + "a" + close + close + "b" + close;
    const std::string expected = std::string("a") + close + "b";
    const auto result = pcre::callout_text(source, &memory);
    check(result && *result.value == expected, "all native callout delimiters un-double closing delimiters");
  }
  for (std::string_view text : {"", "\\", "1", "/1", "\\0", "\\01", "\\-1", "\\+1",
                                "\\1x", "\\1 ", "\\9223372036854775808", "\\18446744073709551615"})
    check(error(pcre::backref_number(text)), "malformed or overflowing backreference returns an error");
  check(error(pcre::backref_number(std::string_view("\\1\0x", 4))),
        "embedded NUL is not a decimal digit");
  for (const auto [text, expected] : {
           std::pair<std::string_view, std::int64_t>{"\\1", 1}, {"\\7", 7}, {"\\8", 8},
           {"\\9", 9}, {"\\89", 89}, {"\\65535", 65535},
           {"\\9223372036854775807", INT64_MAX}}) {
    const auto result = pcre::backref_number(text);
    check(result && *result.value == expected, "positive decimal conversion and int64 boundary are retained");
  }
  for (const std::string_view source : {"(?#", "(?#body", "(?C'", "(?C'body)",
                                       "(?C{body)", "(?C'a'b')", "(?C{a}b})",
                                       "\\9999999999999999999999999999999999999999999999"}) {
    const auto result = parser.parse(source);
    const bool rejected = !result.accepted || result.cst().recovered() || !result.diagnostics.empty();
    check(rejected, "incomplete comment/callout or overflowing reference never parses clean");
    if (!rejected) std::fprintf(stderr, "  source=%.*s\n", int(source.size()), source.data());
  }
  return ok;
}

} // namespace pcre_converter_cases
