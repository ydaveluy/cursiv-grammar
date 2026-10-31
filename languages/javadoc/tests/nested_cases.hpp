#pragma once
#include <string_view>
namespace javadoc_nested {
struct Case { std::string_view name, source, names, edges; unsigned braces; };
inline constexpr Case cases[] = {
  {"summary-code", "/** {@summary One {@code two}} */", "summary,code", "summary>code", 0},
  {"summary-link-code", "/** {@summary See {@link String label {@code x}}}. */", "summary,link,code", "link>code,summary>link", 0},
  {"return-code", "/** {@return a {@code value}} */", "return,code", "return>code", 0},
  {"link-label-code", "/** {@link String label {@code x}} */", "link,code", "link>code", 0},
  {"linkplain-label-literal", "/** {@linkplain String label {@literal <x>}} */", "linkplain,literal", "linkplain>literal", 0},
  {"index-description-code", "/** {@index word description {@code x}} */", "index,code", "index>code", 0},
  {"index-quoted-term", "/** {@index \"{@code search}\" description {@literal x}} */", "index,literal", "index>literal", 0},
  {"code-raw", "/** {@code {@summary raw}} */", "code", "", 0},
  {"literal-raw", "/** {@literal {@code raw}} */", "literal", "", 0},
  {"custom-raw", "/** {@custom {@code raw}} */", "custom", "", 0},
  {"snippet-raw", "/** {@snippet :\n {@code raw}\n } */", "snippet", "", 0},
  {"summary-plain-braces", "/** {@summary before {inside {@code x}} after} */", "summary,code", "summary>code", 1},
  {"top-level-escaped-open", "/** {@@code x} */", "", "", 0},
  {"summary-html-attribute", "/** {@summary <span title=\"{@code x}\">y</span>} */", "summary,code", "summary>code", 0},
  {"wrapped-summary-multiline", "/**\r\n * {@summary first\r\n * {@code second} {@docRoot}}\r\n */", "summary,code,docRoot", "summary>code,summary>docRoot", 0},
  {"block-return-summary", "/**\n * @return {@summary one {@code two}}\n */", "summary,code", "return>summary,summary>code", 0},
  {"bare-summary", "{@summary One {@code two}}", "summary,code", "summary>code", 0},
  {"bare-link", "{@link String label {@code x}}", "link,code", "link>code", 0},
  {"bare-literal", "{@literal {@code raw}}", "literal", "", 0},
  {"bare-block-return", "@return {@summary one {@code two}}", "summary,code", "return>summary,summary>code", 0},
};
inline constexpr std::string_view invalid[] = {
  "/** {@summary {@@code x}} */",
  "/** {@link {@code x}} */",
};
}  // namespace javadoc_nested
