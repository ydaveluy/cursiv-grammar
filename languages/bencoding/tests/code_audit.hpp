#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"

inline int check_native_audit(const auto& services) {
  struct Case { const char* text; const char* code; unsigned count; };
  const Case cases[] = {
    {"", "bencoding.document-value", 1},
    {"01:a", "bencoding.string-length", 1},
    {"00:", "bencoding.string-length", 1},
    {"i01e", "bencoding.integer-format", 1},
    {"i-01e", "bencoding.integer-format", 1},
    {"i-0e", "bencoding.integer-format", 1},
  };
  int failures=0;
  for (const auto& c : cases) {
    const auto parsed=services.parser->parse(c.text);
    const auto validated=cursiv::test::validationHelper(services,c.text);
    bool good=parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty() &&
              validated.diagnostics.size()==c.count;
    for (const auto& d : validated.diagnostics) good &= d.code==c.code;
    if (!good) {std::fprintf(stderr,"bencoding: runtime fixture %s\n",c.code);++failures;}
  }
  const auto length=cursiv::test::validationHelper(services,"01:a");
  if (length.diagnostics.size()!=1 || length.diagnostics[0].range.begin!=0 ||
      length.diagnostics[0].range.end!=2) ++failures;
  std::puts("bencoding: 6 permissive forms checked against runtime diagnostics");
  return failures;
}
