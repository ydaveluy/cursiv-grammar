#pragma once
#include <cstdio>
#include <string>
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

inline bool uri_component_cases(const auto& language) {
  struct Case { const char* source; const char* code; };
  static constexpr Case cases[] = {
    {"http://host/a:@!$&'()*+,;=-._~/%00%ff", ""},
    {"../a:b/c", ""},
    {"scheme:a:b", ""},
    {"http://host/?q=:/?@!$'()*+,;=&other=%26%3d%00", ""},
    {"http://host/?&&a=&=v&&", ""},
    {"http://host/#:/?@!$&'()*+,;=%00", ""},
    {"http://host/%C3%A9?q=%CE%B1#%F0%9F%98%80", ""},
    {"?a=b=c/d?e", ""},
    {"#", ""},
    {"http://host/a b", "url.path"},
    {"http://host/a\\b", "url.path"},
    {"http://host/a[b]", "url.path"},
    {"http://host/a^b", "url.path"},
    {"http://host/a%", "url.path"},
    {"http://host/%A", "url.path"},
    {"http://host/%GG", "url.path"},
    {"http://host/a\tb", "url.path"},
    {"http://host/a\177b", "url.path"},
    {"http://host/é", "url.path"},
    {"http://host/?a b=v", "url.query"},
    {"http://host/?a=v w", "url.query"},
    {"http://host/?a=bad%", "url.query"},
    {"http://host/?bad%Q1=v", "url.query"},
    {"http://host/?a=[v]", "url.query"},
    {"http://host/?a=é", "url.query"},
    {"http://host/#a b", "url.fragment"},
    {"http://host/#bad%", "url.fragment"},
    {"http://host/#%Q0", "url.fragment"},
    {"http://host/#a[b]", "url.fragment"},
    {"http://host/#a\\b", "url.fragment"},
    {"http://host/#é", "url.fragment"},
  };
  for (const auto& c : cases) {
    const std::string source(c.source);
    const auto parsed = language.language.parser->parse(source);
    auto checked = cursiv::test::validationHelper(language.language, source);
    bool ok = parsed.accepted && !parsed.cst().recovered() &&
              parsed.diagnostics.empty() && parsed.cst().text() == source;
    if (*c.code == 0) ok &= checked.diagnostics.empty();
    else ok &= checked.diagnostics.size() == 1 &&
               checked.diagnostics[0].code && checked.diagnostics[0].code->isString() &&
               checked.diagnostics[0].code->string() == c.code &&
               checked.diagnostics[0].severity == cursiv::DiagnosticSeverity::Error &&
               checked.diagnostics[0].range.begin < checked.diagnostics[0].range.end &&
               checked.diagnostics[0].range.end <= source.size();
    checked.dispose();
    if (!ok) {
      std::fprintf(stderr, "URL URI component check: %s (%s)\n", c.source, c.code);
      return false;
    }
  }
  return true;
}
